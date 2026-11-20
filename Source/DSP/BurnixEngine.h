// =============================================================================
//  BURNIX by PIRADEX — motor de bandas (puro C++, sem JUCE). Corre a taxa sobreamostrada.
//
//  Por amostra estereo (L, R):
//    1) BandSplitter divide em 1..6 bandas (Linkwitz-Riley, soma passa-tudo).
//    2) Por banda, ChannelRouter: L/R (drive MID nos dois canais) | MID | SIDE | M+S (drives independentes).
//       M = (L+R)/sqrt2, S = (L-R)/sqrt2  e o inverso  L = (M+S)/sqrt2, R = (M-S)/sqrt2  (ortonormal: sem perda).
//    3) Em cada "lane" (banda x canal): filtro seletivo F (nenhum/HP/LP/BP/NOTCH/SHELF) -> TubeStage.
//         INSERT    y = x + mix * (Sat(Fx) - Fx)     (substitui so a regiao filtrada pela versao saturada)
//         PARALLEL  y = x + mix * Sat(Fx)            (soma o ramo saturado ao sinal intacto; o nivel sobe)
//    4) wet = soma das bandas processadas; dry = soma das bandas SEM processamento (mesma fase dos crossovers),
//       para que a mistura global e o DELTA nao tenham filtro-pente.
//  Solo: so as bandas em solo entram em wet e dry. Medidor de atividade por banda (MID e SIDE).
// =============================================================================
#pragma once
#include "BandSplitter.h"
#include "TubeStage.h"
#include <atomic>

namespace bx
{
enum class Route { LR = 0, Mid, Side, MidSide };
enum class FilterType { None = 0, HP, LP, BP, Notch, Shelf };
enum class Mode { Insert = 0, Parallel };
constexpr int kMorph = -1;                     // voice = kMorph -> usa o morph global

struct BandParams
{
    bool on = true, solo = false;
    Route route = Route::LR;
    double driveM = 3.0, driveS = 3.0;         // 0..10 (antes do HEAT)
    int voice = kMorph;                        // -1 morph, 0..5 voicing fixo
    FilterType filter = FilterType::None; double fHz = 1000.0, fQ = 0.7;
    Mode mode = Mode::Insert;
    double mix = 1.0;                          // 0..1
    double dyn = 0.0;                          // 2.0: -1..1 dinamica da banda (+ = mais punch, - = mais sustain)
    double attackDb = 0.0;                     // 2.0: -12..12 dB de drive nos ataques
};

struct EngineParams
{
    int numBands = 3; double xo[kMaxX] { 120, 900, 3500, 8000, 14000 }; Slope slope = Slope::LR24;
    std::array<BandParams, kMaxBands> band;
    double heat = 5.0;                         // escala dos drives: drive_efetivo = drive * heat / 5
    std::array<int, 4> corner { TriodeBloom, PentodeEdge, Firestorm, Velvet };   // cima, direita, baixo, esquerda
    double morphX = 0.5, morphY = 0.3;
    double bias = 0.0, density = 0.0, transient = 0.0, stageBlend = 0.0, interstageDb = 0.0; bool swapOrder = false;
    bool legacy = false;                       // motor da 1.2 (sessoes antigas): som identico ao da versao 1.2
    bool levelComp = true;                     // ganho compensado (motor 1.3)
    bool progressive = true;                   // curva do drive: progressiva (1.3) ou classica (1.2)
    double drift = 0.0;                        // 0..1 drift analogico (motor 1.3)
    std::array<double, 8> draw { 0, 1, 0.1, 0.05, 0.01, 0.005, 0.001, 0.0005 };   // 2.0: amplitudes do DRAW (a[1] = 1)
    int drawVersion = 0;
};

// A/B MORPH (2.0): mistura dois conjuntos de parametros. Numeros interpolam (frequencias em escala logaritmica);
// escolhas (bandas, voicings, rotas...) vem do lado mais proximo.
inline EngineParams lerpEngine (const EngineParams& a, const EngineParams& b, double t)
{
    t = std::clamp (t, 0.0, 1.0); EngineParams e = t < 0.5 ? a : b;
    auto L = [t] (double x, double y) { return x + t * (y - x); };
    auto G = [t] (double x, double y) { return std::exp (std::log (std::max (1.0, x)) + t * (std::log (std::max (1.0, y)) - std::log (std::max (1.0, x)))); };
    e.heat = L (a.heat, b.heat); e.morphX = L (a.morphX, b.morphX); e.morphY = L (a.morphY, b.morphY);
    e.bias = L (a.bias, b.bias); e.density = L (a.density, b.density); e.transient = L (a.transient, b.transient);
    e.stageBlend = L (a.stageBlend, b.stageBlend); e.interstageDb = L (a.interstageDb, b.interstageDb); e.drift = L (a.drift, b.drift);
    for (int k = 0; k < kMaxX; ++k) e.xo[k] = G (a.xo[k], b.xo[k]);
    for (int i = 0; i < kMaxBands; ++i)
    {
        auto& d = e.band[(size_t) i]; const auto& x = a.band[(size_t) i]; const auto& y = b.band[(size_t) i];
        d.driveM = L (x.on ? x.driveM : 0.0, y.on ? y.driveM : 0.0); d.driveS = L (x.on ? x.driveS : 0.0, y.on ? y.driveS : 0.0);
        d.mix = L (x.mix, y.mix); d.fHz = G (x.fHz, y.fHz); d.fQ = L (x.fQ, y.fQ); d.dyn = L (x.dyn, y.dyn); d.attackDb = L (x.attackDb, y.attackDb); d.on = x.on || y.on;
    }
    for (size_t k = 0; k < e.draw.size(); ++k) e.draw[k] = L (a.draw[k], b.draw[k]);
    e.drawVersion = 1000000 + (int) std::lround (t * 1000.0) + 7 * a.drawVersion + 13 * b.drawVersion;
    return e;
}

class BurnixEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate; splitter.prepare (sr);
        for (auto& b : lanes) for (auto& l : b) { l.tube.prepare (sr); l.filt.reset(); }
        smooth = envCoeff (20.0, sr);
        dA1 = envCoeff (0.3, sr); dR1 = envCoeff (60.0, sr); dA2 = envCoeff (30.0, sr); dR2 = envCoeff (300.0, sr); dG = envCoeff (0.5, sr);
        for (auto& d : dynSt) d = {};
        for (auto& c : cur) c = {};
        first = true; lastFilter = {}; lastFilterSet = false;
    }
    void reset() { splitter.reset(); for (auto& b : lanes) for (auto& l : b) { l.tube.reset(); l.filt.reset(); } first = true; }

    // tolerancias analogicas fixas por lane, a partir de uma semente por instancia (o mesmo som sempre que a sessao abre)
    void setSeed (unsigned int seed)
    {
        unsigned int z = seed ? seed : 0x9E3779B9u;
        auto rnd = [&z] { z ^= z << 13; z ^= z >> 17; z ^= z << 5; return (double) (z & 0xFFFFFF) / (double) 0x7FFFFF - 1.0; };
        for (auto& b : lanes) for (auto& l : b) { const double r1 = rnd(), r2 = rnd(); l.tube.setTolerance (r1, r2); }
    }

    void setParams (const EngineParams& p)
    {
        P = p;
        splitter.setup (P.numBands, P.xo, P.slope);
        const auto mw = morphWeights (P.morphX, P.morphY);
        for (int b = 0; b < kMaxBands; ++b)
        {
            auto& w = weights[(size_t) b]; w = {};
            const int v = P.band[(size_t) b].voice;
            if (v < 0) for (int i = 0; i < 4; ++i) w[(size_t) std::clamp (P.corner[(size_t) i], 0, kNumVoicings - 1)] += mw[(size_t) i];
            else w[(size_t) std::clamp (v, 0, kNumVoicings - 1)] = 1.0;
            // filtro seletivo: so recalcula se mudou
            const auto& bp = P.band[(size_t) b];
            const FiltKey key { (int) bp.filter, bp.fHz, bp.fQ };
            if (! lastFilterSet || ! (key == lastFilter[(size_t) b]))
            {
                BiquadCoeffs c;
                switch (bp.filter)
                {
                    case FilterType::HP:    c = design::highpass  (bp.fHz, bp.fQ, sr); break;
                    case FilterType::LP:    c = design::lowpass   (bp.fHz, bp.fQ, sr); break;
                    case FilterType::BP:    c = design::bandpass  (bp.fHz, bp.fQ, sr); break;
                    case FilterType::Notch: c = design::notch     (bp.fHz, bp.fQ, sr); break;
                    case FilterType::Shelf: c = design::highShelf (bp.fHz, 0.7, 6.0, sr); break;
                    default: break;
                }
                for (auto& l : lanes[(size_t) b]) l.filt.set (c);
                lastFilter[(size_t) b] = key;
            }
        }
        lastFilterSet = true;
        anySolo = false; for (int b = 0; b < P.numBands; ++b) anySolo = anySolo || (P.band[(size_t) b].solo && P.band[(size_t) b].on);
        heatScale = std::clamp (P.heat, 0.0, 10.0) / 5.0;
        for (int b = 0; b < kMaxBands; ++b) tgt[(size_t) b] = target (b);     // uma vez por bloco (pow fora do ciclo por amostra)
        if (first) { for (int b = 0; b < kMaxBands; ++b) cur[(size_t) b] = target (b); curG = targetG(); first = false; }
    }

    inline void process (double l, double r, double& wl, double& wr, double& dl, double& dr)
    {
        double bl[kMaxBands], br[kMaxBands];
        splitter.process (0, l, bl); splitter.process (1, r, br);
        wl = wr = dl = dr = 0;
        const auto tg = targetG(); curG.bias += smooth * (tg.bias - curG.bias); curG.stage += smooth * (tg.stage - curG.stage);
        TubeParams tp; tp.bias = curG.bias; tp.density = P.density; tp.transient = P.transient; tp.stageBlend = curG.stage;
        tp.interstageDb = P.interstageDb; tp.swapOrder = P.swapOrder;
        tp.legacy = P.legacy; tp.levelComp = P.levelComp; tp.drift = P.legacy ? 0.0 : std::clamp (P.drift, 0.0, 1.0);
        tp.drawA = P.draw.data(); tp.drawVersion = P.drawVersion;
        for (int b = 0; b < P.numBands; ++b)
        {
            const auto& bp = P.band[(size_t) b];
            auto& c = cur[(size_t) b]; const auto& t = tgt[(size_t) b];
            c.dm += smooth * (t.dm - c.dm); c.ds += smooth * (t.ds - c.ds); c.mix += smooth * (t.mix - c.mix);
            const bool include = ! anySolo || (bp.solo && bp.on);
            double xl = bl[b], xr = br[b], yl = xl, yr = xr;
            if (bp.on)
            {
                tp.weights = weights[(size_t) b]; tp.attackDb = std::clamp (bp.attackDb, -12.0, 12.0);
                auto& L = lanes[(size_t) b];
                if (bp.route == Route::LR)
                {
                    tp.drive = c.dm; yl = lane (L[0], xl, tp, bp, c.mix); yr = lane (L[1], xr, tp, bp, c.mix);
                    accum (b, 0, xl, yl); accum (b, 0, xr, yr);
                }
                else
                {
                    constexpr double k = 0.70710678118654752;
                    const double m = (xl + xr) * k, s = (xl - xr) * k;
                    double ym = m, ys = s;
                    if (bp.route != Route::Side) { tp.drive = c.dm; ym = lane (L[0], m, tp, bp, c.mix); }
                    if (bp.route != Route::Mid)  { tp.drive = c.ds; ys = lane (L[1], s, tp, bp, c.mix); }
                    accum (b, 0, m, ym); accum (b, 1, s, ys);
                    yl = (ym + ys) * k; yr = (ym - ys) * k;
                }
            }
            if (bp.on && bp.dyn != 0.0)
            {
                // dinamica da banda (2.0): ganho = (envolvente rapida / lenta)^k, limitado a +-12 dB, ligado em estereo
                auto& d = dynSt[(size_t) b]; const double ax = 0.5 * (std::fabs (xl) + std::fabs (xr));
                d.fast += (ax > d.fast ? dA1 : dR1) * (ax - d.fast); d.slow += (ax > d.slow ? dA2 : dR2) * (ax - d.slow);
                if (d.slow > 1e-6)
                {
                    const double g = std::clamp (std::pow (std::max (1.0, d.fast / d.slow), 1.5 * std::clamp (bp.dyn, -1.0, 1.0)), 0.25, 4.0);   // so mexe nos ataques
                    d.g += dG * (g - d.g);
                }
                yl *= d.g; yr *= d.g;
            }
            if (include) { wl += yl; wr += yr; dl += xl; dr += xr; }
        }
    }

    // medidores de atividade (dB de distorcao adicionada relativa a banda), lidos pela GUI
    struct Activity { std::atomic<float> mid { -120.f }, side { -120.f }; };
    std::array<Activity, kMaxBands> activity;
    void publishActivity()
    {
        for (int b = 0; b < kMaxBands; ++b)
            for (int k = 0; k < 2; ++k)
            {
                auto& a = acc[(size_t) b][(size_t) k];
                const float db = a.sig > 1e-12 ? (float) (10.0 * std::log10 (std::max (a.diff, 1e-14) / a.sig)) : -120.f;
                (k == 0 ? activity[(size_t) b].mid : activity[(size_t) b].side).store (std::max (-120.f, db));
                a = {};
            }
    }
    const EngineParams& params() const { return P; }
    double sampleRate() const { return sr; }

private:
    struct Lane { TubeStage tube; Biquad filt; };
    struct Cur { double dm = 0, ds = 0, mix = 1; };
    struct CurG { double bias = 0, stage = 0; };
    struct FiltKey { int t = 0; double hz = 0, q = 0; bool operator== (const FiltKey& o) const { return t == o.t && hz == o.hz && q == o.q; } };
    struct Acc { double diff = 0, sig = 0; };

    Cur target (int b) const
    {
        const auto& bp = P.band[(size_t) b];
        const double dm = std::clamp (bp.driveM * heatScale, 0.0, 10.0), ds = std::clamp (bp.driveS * heatScale, 0.0, 10.0);
        const bool prog = P.progressive && ! P.legacy;
        return { prog ? progressiveDrive (dm) : dm, prog ? progressiveDrive (ds) : ds, std::clamp (bp.mix, 0.0, 1.0) };
    }
    CurG targetG() const { return { std::clamp (P.bias, -1.0, 1.0), std::clamp (P.stageBlend, 0.0, 1.0) }; }

    inline double lane (Lane& L, double x, const TubeParams& tp, const BandParams& bp, double mix)
    {
        const double fx = bp.filter == FilterType::None ? x : L.filt.process (x);
        const double sat = L.tube.process (fx, tp);
        return bp.mode == Mode::Insert ? x + mix * (sat - fx) : x + mix * sat;
    }
    inline void accum (int b, int k, double x, double y) { auto& a = acc[(size_t) b][(size_t) k]; const double d = y - x; a.diff += d * d; a.sig += x * x; }

    double sr = 48000, smooth = 0.001, heatScale = 1.0;
    EngineParams P;
    BandSplitter splitter;
    std::array<std::array<Lane, 2>, kMaxBands> lanes;
    std::array<std::array<double, kNumVoicings>, kMaxBands> weights {};
    std::array<Cur, kMaxBands> cur {}, tgt {}; CurG curG {};
    std::array<FiltKey, kMaxBands> lastFilter {}; bool lastFilterSet = false;
    std::array<std::array<Acc, 2>, kMaxBands> acc {};
    bool anySolo = false, first = true;
    struct DynState { double fast = 0, slow = 0, g = 1; };
    std::array<DynState, kMaxBands> dynSt {};
    double dA1 = 0.01, dR1 = 0.001, dA2 = 0.001, dR2 = 0.0001, dG = 0.01;
};
} // namespace bx
