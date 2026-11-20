// =============================================================================
//  BURNIX by PIRADEX — NonlinearTubeStage e HarmonicEngine (puro C++, sem JUCE)
//
//  Seis voicings ORIGINAIS (nao sao modelos de valvulas reais nem copias de plugins comerciais):
//    EMBER         tanh com bias interno pequeno       -> 3.o dominante + 2.o ligeiro, pouca aspereza
//    TRIODE BLOOM  tanh assimetrico (negativo mais macio) + bias  -> 2.o harmonico proeminente
//    PENTODE EDGE  curva u/(1+|u|^2.5)^0.4, joelho mais duro e simetrico -> impares, mais "mordida"
//    IRON CORE     motor 1.3: transformador simplificado -> satura mais nos GRAVES (pre-enfase de graves antes da
//                  curva e de-enfase depois, resposta linear plana) com histerese leve (operador "play").
//                  motor 1.2 (sessoes antigas): memoria de 1.a ordem u + h(u - m), m a 250 Hz.
//    VELVET        atan (joelho muito suave) + passa-baixo progressivo com o drive -> pouco THD aparente
//    FIRESTORM     clipper com joelho quadratico (C1) e limiares assimetricos (+1 / -0.8) -> agressivo.
//                  motor 1.3: joelho duas vezes mais largo (comeca a saturar mais cedo, sem degrau).
//    DRAW (2.0)    HARMONIC DRAW: soma de polinomios de Chebyshev T1..T7, com os niveis H2..H7 desenhados
//                  pelo utilizador. Num seno a fundo de escala, cada harmonico sai exatamente ao nivel desenhado;
//                  a niveis mais baixos caem naturalmente (A^k). Sem ADAA (coeficientes variaveis): usar HQ 4x ou mais.
//
//  Normalizacao:  y = (f(s*x + b) - f(b)) / (f(s + b) - f(b))   -> sem DC estatica e x=1 -> y=1.
//  O drive (0..10) mapeia s = 0.35 * 10^(drive*2.7/20). Abaixo de drive 1 o resultado e misturado com o
//  sinal limpo (drive 0 = identidade exata). A componente adicionada (y - x) passa por um bloqueador de DC
//  a 5 Hz: o sinal limpo nunca e filtrado.
//
//  Motor 1.3 (sessoes novas; as sessoes da 1.2 continuam com o motor antigo, som identico):
//    - GANHO COMPENSADO: cada curva e dividida pelo seu ganho medio a -12 dBFS, por isso o drive muda o caracter
//      e nao o volume (opcional, ligado por omissao).
//    - ADAA (antialiasing por antiderivada, 1.a ordem) em todas as curvas: (F(v1) - F(v0)) / (v1 - v0).
//      Formas fechadas para tanh, atan e o joelho; tabela com interpolacao de Hermite para o PENTODE.
//    - DRIFT analogico: bias que respira com o nivel e pequenas diferencas fixas entre canais (semente por instancia).
//  Dinamica opcional: DENSITY (sag: o drive baixa e a assimetria sobe com a envolvente) e TRANSIENT
//  PRESERVE (menos saturacao durante ataques: envolvente rapida vs lenta).
//  Segundo estagio (STAGE BLEND): PENTODE com ganho INTERSTAGE, em serie depois (ou antes, TOPOLOGY) do voicing.
// =============================================================================
#pragma once
#include "Biquad.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace bx
{
enum Voicing { Ember = 0, TriodeBloom, PentodeEdge, IronCore, Velvet, Firestorm, Draw, kNumVoicings };
constexpr int kClassicVoicings = 6;     // os seis voicings da 1.x (DRAW e da 2.0)
inline const char* voicingName (int v)
{
    static const char* n[] = { "EMBER", "TRIODE BLOOM", "PENTODE EDGE", "IRON CORE", "VELVET", "FIRESTORM", "DRAW" };
    return n[std::clamp (v, 0, kNumVoicings - 1)];
}

// ---- curvas estaticas e as suas antiderivadas --------------------------------------------------------
namespace curve
{
    inline double ember (double u)   { return std::tanh (u); }
    inline double triode (double u)  { return u >= 0 ? std::tanh (u) : std::tanh (0.6 * u) / 0.6; }
    inline double pentode (double u) { const double a = std::fabs (u); return u * std::exp (-0.4 * std::log1p (a * a * std::sqrt (a))); }   // u / (1 + |u|^2.5)^0.4
    constexpr double kVel = kPi * 0.25;
    inline double velvet (double u)  { return std::atan (kVel * u) / kVel; }   // = (4/pi) atan(pi u / 4): joelho largo
    inline double knee (double v, double t, double w)       // v >= 0: linear ate t-w, joelho quadratico, teto t
    {
        if (v <= t - w) return v;
        if (v >= t + w) return t;
        const double d = v - (t - w); return v - d * d / (4.0 * w);
    }
    inline double fire (double u, bool wide)
    {
        const double v = 1.6 * u, wf = wide ? 0.6 : 0.3;
        return v >= 0 ? knee (v, 1.0, wf) : -knee (-v, 0.8, 0.8 * wf);
    }

    // antiderivadas (F(0) = 0)
    inline double logcosh (double u) { const double a = std::fabs (u); return a + std::log1p (std::exp (-2.0 * a)) - 0.69314718055994531; }
    inline double Fember (double u)  { return logcosh (u); }
    inline double Ftriode (double u) { return u >= 0 ? logcosh (u) : logcosh (0.6 * u) / 0.36; }
    inline double Fvelvet (double u) { const double z = kVel * u; return (u * std::atan (z) - std::log1p (z * z) / (2.0 * kVel)) / kVel; }
    inline double Kn (double v, double t, double w)          // integral de knee de 0 a v (v >= 0)
    {
        if (v <= t - w) return 0.5 * v * v;
        if (v >= t + w) return 0.5 * (t + w) * (t + w) - 2.0 * w * w / 3.0 + t * (v - (t + w));
        const double d = v - (t - w); return 0.5 * v * v - d * d * d / (12.0 * w);
    }
    inline double Ffire (double u, bool wide)
    {
        const double v = 1.6 * u, wf = wide ? 0.6 : 0.3;
        return (v >= 0 ? Kn (v, 1.0, wf) : Kn (-v, 0.8, 0.8 * wf)) / 1.6;
    }
    // Antiderivadas pares (de curvas impares) em tabela: F em [0, 32], passo 1/128, integrada com Simpson; leitura com
    // Hermite cubico usando f exata nos nos. E mais barato do que exp/log por amostra e o erro fica abaixo de 1e-10.
    // PENTODE nao tem forma fechada; tanh e atan tem, mas a tabela poupa CPU (Mac antigos).
    struct EvenTable
    {
        static constexpr double kMax = 32.0, kInv = 128.0, kStep = 1.0 / 128.0;
        std::vector<double> F, f;
        double (*fn) (double) = nullptr, (*tail) (double) = nullptr;
        EvenTable (double (*fun) (double), double (*tailF) (double)) : fn (fun), tail (tailF)
        {
            const int n = (int) (kMax * kInv) + 1; F.assign ((size_t) n, 0.0); f.assign ((size_t) n, 0.0);
            for (int i = 0; i < n; ++i) f[(size_t) i] = fn (i * kStep);
            for (int i = 1; i < n; ++i)
            {
                const double x0 = (i - 1) * kStep, x1 = i * kStep;
                F[(size_t) i] = F[(size_t) i - 1] + kStep / 6.0 * (fn (x0) + 4.0 * fn (0.5 * (x0 + x1)) + fn (x1));
            }
        }
        inline double eval (double u) const
        {
            const double a = std::fabs (u);
            if (a >= kMax) return tail (a);
            const double x = a * kInv; const int i = std::min ((int) x, (int) F.size() - 2); const double t = x - i;
            const double p0 = F[(size_t) i], p1 = F[(size_t) i + 1], m0 = f[(size_t) i] * kStep, m1 = f[(size_t) i + 1] * kStep;
            const double t2 = t * t, t3 = t2 * t;
            return (2 * t3 - 3 * t2 + 1) * p0 + (t3 - 2 * t2 + t) * m0 + (-2 * t3 + 3 * t2) * p1 + (t3 - t2) * m1;
        }
    };
    inline double tailTanh (double a) { return a - 0.69314718055994531 + std::log1p (std::exp (-2.0 * a)); }
    inline double tailVelvet (double a) { return Fvelvet (a); }
    inline double tailPentode (double a);
    inline const EvenTable& tanhTable()    { static const EvenTable t (ember, tailTanh); return t; }
    inline const EvenTable& velvetTable()  { static const EvenTable t (velvet, tailVelvet); return t; }
    inline const EvenTable& pentodeTable() { static const EvenTable t (pentode, tailPentode); return t; }
    inline double tailPentode (double a) { const auto& t = pentodeTable(); return t.F.back() + (a - EvenTable::kMax) * pentode (0.5 * (a + EvenTable::kMax)); }
    inline double FemberT (double u)   { return tanhTable().eval (u); }
    inline double FtriodeT (double u)  { return u >= 0 ? tanhTable().eval (u) : tanhTable().eval (0.6 * u) / 0.36; }
    inline double FvelvetT (double u)  { return velvetTable().eval (u); }
    inline double Fpentode (double u)  { return pentodeTable().eval (u); }

    // HARMONIC DRAW: a[1] = 1 (fundamental), a[2..7] = amplitudes desenhadas (a[0] por usar)
    constexpr double kDrawMax = 7.8 / 0.95; // drive maximo: um seno a 0 dBFS entra a amplitude 0.95 (zona exata de Chebyshev)
    inline double draw (double u, const double* a)
    {
        double t = u / kDrawMax;            // Chebyshev exato ate |t| = 0.95; acima, joelho suave ate 1 (nunca passa de 1)
        if (std::fabs (t) > 0.95) t = (t < 0 ? -1.0 : 1.0) * (0.95 + 0.05 * std::tanh ((std::fabs (t) - 0.95) / 0.05));
        double t0 = 1.0, t1 = t, y = t;                 // T0, T1
        for (int k = 2; k <= 7; ++k) { const double tk = 2.0 * t * t1 - t0; y += a[k] * tk; t0 = t1; t1 = tk; }
        return y;
    }

    inline double eval (int v, double u, bool legacy)
    {
        switch (v)
        {
            case Ember:       return ember (u);
            case TriodeBloom: return triode (u);
            case PentodeEdge: return pentode (u);
            case IronCore:    return ember (u);       // a memoria e aplicada antes, no estagio
            case Velvet:      return velvet (u);
            case Firestorm:   return fire (u, ! legacy);
            default:          return u;
        }
    }
    inline double antideriv (int v, double u, bool legacy)
    {
        switch (v)
        {
            case Ember: case IronCore: return FemberT (u);
            case TriodeBloom:          return FtriodeT (u);
            case PentodeEdge:          return Fpentode (u);
            case Velvet:               return FvelvetT (u);
            case Firestorm:            return Ffire (u, ! legacy);
            default:                   return 0.5 * u * u;
        }
    }
    inline double internalBias (int v)
    {
        switch (v) { case Ember: return 0.05; case TriodeBloom: return 0.18; case IronCore: return 0.06; case Velvet: return 0.04; default: return 0.0; }
    }
}

inline double driveToScale (double drive) { return 0.35 * std::pow (10.0, std::clamp (drive, 0.0, 10.0) * 2.7 / 20.0); }
// curva PROGRESSIVA do botao (motor 1.3): o meio do botao ja satura de forma audivel; 0 e 10 iguais aos da 1.2
inline double progressiveDrive (double knob) { const double k = std::clamp (knob, 0.0, 10.0); return 10.0 * std::pow (k / 10.0, 0.7); }

// pesos do morph: quatro pontos cardeais (cima, direita, baixo, esquerda) no quadrado unitario
inline std::array<double, 4> morphWeights (double x, double y)
{
    const double px[4] = { 0.5, 1.0, 0.5, 0.0 }, py[4] = { 0.0, 0.5, 1.0, 0.5 };
    std::array<double, 4> w {}; double sum = 0; int best = 0; double bestD = 1e9;
    for (int i = 0; i < 4; ++i)
    {
        const double d = std::hypot (x - px[i], y - py[i]);
        if (d < bestD) { bestD = d; best = i; }
        const double t = std::max (0.0, 1.0 - d / 0.70710678);
        w[(size_t) i] = t * t; sum += w[(size_t) i];
    }
    if (sum < 1e-9) { w = {}; w[(size_t) best] = 1.0; return w; }
    for (auto& v : w) v /= sum;
    return w;
}

struct TubeParams
{
    double drive = 3.0;                 // 0..10
    double bias = 0.0;                  // -1..1
    double density = 0.0;               // 0..1
    double transient = 0.0;             // 0..1
    double stageBlend = 0.0;            // 0..1 (quantidade do 2.o estagio)
    double interstageDb = 0.0;          // -12..12
    bool   swapOrder = false;           // false: voicing -> pentode ; true: pentode -> voicing
    bool   legacy = false;              // true: motor da 1.2 (sem ADAA, IRON e FIRESTORM antigos, sem compensacao)
    bool   levelComp = true;            // ganho compensado (so no motor 1.3)
    double drift = 0.0;                 // 0..1 drift analogico (so no motor 1.3)
    double attackDb = 0.0;              // 2.0: +-12 dB de drive extra (ou a menos) durante os ataques desta banda
    const double* drawA = nullptr;      // 2.0: coeficientes do DRAW (8 valores, a[1] = 1)
    int drawVersion = 0;                // muda quando o desenho muda (invalida a cache)
    std::array<double, kNumVoicings> weights { 0, 1, 0, 0, 0, 0, 0 };   // soma 1
};

// um "lane" = uma banda x um canal (L, R, M ou S)
class TubeStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        cIron = 1.0 - std::exp (-2.0 * kPi * 250.0 / sr);
        ironEmph.set (design::onePoleLowpass (kIronHz, sr));
        ironDeemph.set (design::onePoleLowpass (kIronHz * (1.0 + kIronK), sr));
        aSagA = envCoeff (3.0, sr); aSagR = envCoeff (80.0, sr);
        aFastA = envCoeff (0.5, sr); aFastR = envCoeff (15.0, sr);
        aSlowA = envCoeff (15.0, sr); aSlowR = envCoeff (150.0, sr);
        const double R = std::exp (-2.0 * kPi * 5.0 / sr); dcR = R;
        lastVelvetHz = -1; reset();
    }
    void reset()
    {
        sag = fast = slow = 0; ironM = 0; ironH = 0; dcX = dcY = 0; xPrev = 0;
        velvetLP.reset(); ironEmph.reset(); ironDeemph.reset();
        for (auto& c : cache) c = {};
        for (auto& a : adaa) a = {};
    }
    // tolerancia analogica fixa desta lane (r1, r2 em -1..1, vindos de uma semente por instancia)
    void setTolerance (double r1, double r2) { tolS = r1; tolB = r2; }

    inline double process (double x, const TubeParams& p)
    {
        const double ax = std::fabs (x);
        sag  += (ax > sag  ? aSagA  : aSagR)  * (ax - sag);
        fast += (ax > fast ? aFastA : aFastR) * (ax - fast);
        slow += (ax > slow ? aSlowA : aSlowR) * (ax - slow);

        const double a = std::min (1.0, p.drive);                 // mistura com o limpo abaixo de drive 1
        if (a <= 0.0) { dcX = dcY = 0; xPrev = x; for (auto& st : adaa) st.primed = false; return x; }
        const double e = std::min (sag, 1.5);
        if (p.drive != lastDrive) { lastDrive = p.drive; lastScale = driveToScale (p.drive); }
        const double tr = slow > 1e-5 ? std::clamp ((fast - 1.1 * slow) / slow, 0.0, 1.0) : 0.0;
        double s = lastScale * (1.0 - 0.45 * p.density * e / 1.5);
        if (p.attackDb != 0.0 && tr > 0.0) s *= dbToGain (p.attackDb * tr);       // 2.0: ataque mais (ou menos) quente
        double b = 0.5 * p.bias + 0.25 * p.density * std::min (sag, 1.0);
        if (! p.legacy && p.drift > 0.0)
        {
            s *= 1.0 + 0.06 * p.drift * tolS;
            b += p.drift * (0.04 * tolB + 0.12 * std::min (slow, 1.0));   // o bias "respira" com o nivel
        }
        const double aEff = a * (1.0 - p.transient * tr);

        double y;
        if (p.stageBlend <= 0.0)     { y = voiced (x, s, b, p); adaa[(size_t) kNumVoicings].primed = false; }
        else if (! p.swapOrder)      { const double v = voiced (x, s, b, p); y = v + p.stageBlend * (stageB (v, p) - v); }
        else                         { const double v = stageB (x, p) ; const double w = x + p.stageBlend * (v - x); y = voiced (w, s, b, p); }

        // so a componente adicionada passa pelo bloqueador de DC (5 Hz). Com ADAA a curva atrasa meia amostra:
        // a referencia linear e a media das duas ultimas amostras, para o fundamental nao "vazar" para o adicionado.
        const double ref = p.legacy ? x : 0.5 * (x + xPrev); xPrev = x;
        const double d = y - ref;
        const double dc = d - dcX + dcR * dcY; dcX = d; dcY = dc;
        return x + aEff * dc;
    }

private:
    static constexpr double kIronHz = 120.0, kIronK = 1.0, kRef = 0.25;   // IRON: +6 dB nos graves; ganho medido a -12 dBFS

    // (f(s*x+b) - f(b)) / (f(s+b) - f(b)), com u0 = s*x. f(b), o denominador e a compensacao ficam em cache enquanto
    // s e b nao mudam (o caso normal com DENSITY e DRIFT a 0).
    struct NormCache { double s = -1, b = 1e9, bb = 0, f0 = 0, inv = 1, comp = 1; bool leg = false; int dv = -1; };
    struct AdaaState { double v = 0, F = 0; bool primed = false; };
    std::array<NormCache, kNumVoicings + 1> cache;
    std::array<AdaaState, kNumVoicings + 1> adaa;

    inline double f (int v, double u, const TubeParams& p) const { return v == Draw ? curve::draw (u, p.drawA != nullptr ? p.drawA : kDrawFlat) : curve::eval (v, u, p.legacy); }
    inline void update (NormCache& c, int v, double s, double b, const TubeParams& p)
    {
        const int dv = v == Draw ? p.drawVersion : 0;
        if (s == c.s && b == c.b && p.legacy == c.leg && dv == c.dv) return;
        c.s = s; c.b = b; c.leg = p.legacy; c.dv = dv; c.bb = b + curve::internalBias (v); c.f0 = f (v, c.bb, p);
        const double den = f (v, s + c.bb, p) - c.f0; c.inv = 1.0 / (std::fabs (den) > 1e-9 ? den : 1e-9);
        const double g = (f (v, s * kRef + c.bb, p) - f (v, -s * kRef + c.bb, p)) * c.inv / (2.0 * kRef);
        c.comp = g > 1e-6 ? 1.0 / g : 1.0;
    }
    static constexpr double kDrawFlat[8] { 0, 1, 0, 0, 0, 0, 0, 0 };
    inline double shape (int v, double arg, bool legacy, int slot)
    {
        if (legacy) return curve::eval (v, arg, true);
        auto& st = adaa[(size_t) slot];
        const double F1 = curve::antideriv (v, arg, false);
        if (! st.primed) { st.primed = true; st.v = arg; st.F = F1; return curve::eval (v, arg, false); }
        const double dv = arg - st.v;
        const double y = std::fabs (dv) > 1e-6 ? (F1 - st.F) / dv : curve::eval (v, 0.5 * (arg + st.v), false);
        st.v = arg; st.F = F1;
        return y;
    }
    inline double norm (int v, double u0, double s, double b, int slot, const TubeParams& p)
    {
        auto& c = cache[(size_t) slot];
        update (c, v, s, b, p);
        const double y = ((v == Draw ? f (v, u0 + c.bb, p) : shape (v, u0 + c.bb, p.legacy, slot)) - c.f0) * c.inv;
        return (! p.legacy && p.levelComp) ? y * c.comp : y;
    }
    inline double voiced (double x, double s, double b, const TubeParams& p)
    {
        double y = 0;
        for (int v = 0; v < kNumVoicings; ++v)
        {
            const double w = p.weights[(size_t) v];
            if (w < 1e-4) { adaa[(size_t) v].primed = false; continue; }   // ao voltar, o ADAA recomeca sem salto
            double u0 = s * x, c;
            if (v == IronCore && p.legacy) { ironM += cIron * (u0 - ironM); u0 = u0 + 0.35 * (u0 - ironM); c = norm (v, u0, s, b, v, p); }
            else if (v == IronCore)
            {
                // transformador: os graves entram mais quentes (+6 dB abaixo de ~120 Hz), histerese "play" e de-enfase
                const double ue = u0 + kIronK * ironEmph.process (u0);
                const double dl = 0.02 * s; ironH = std::clamp (ironH, ue - dl, ue + dl);
                c = norm (v, 0.5 * (ue + ironH), s, b, v, p);
                c = c - (kIronK / (1.0 + kIronK)) * ironDeemph.process (c);
            }
            else c = norm (v, u0, s, b, v, p);
            if (v == Velvet)
            {
                const double hz = 20000.0 / (1.0 + 0.18 * p.drive);
                if (std::fabs (hz - lastVelvetHz) > 1.0) { velvetLP.set (design::onePoleLowpass (std::min (hz, 0.45 * sr), sr)); lastVelvetHz = hz; }
                c = velvetLP.process (c);
            }
            y += w * c;
        }
        return y;
    }
    inline double stageB (double x, const TubeParams& p)
    {
        const double s = 0.8 * dbToGain (p.interstageDb);
        return norm (PentodeEdge, s * x, s, 0.0, kNumVoicings, p);
    }

    double sr = 48000, cIron = 0, aSagA = 0, aSagR = 0, aFastA = 0, aFastR = 0, aSlowA = 0, aSlowR = 0, dcR = 0.999;
    double sag = 0, fast = 0, slow = 0, ironM = 0, ironH = 0, dcX = 0, dcY = 0, lastVelvetHz = -1, xPrev = 0;
    double tolS = 0, tolB = 0, lastDrive = -1, lastScale = 1;
    Biquad velvetLP, ironEmph, ironDeemph;
};
} // namespace bx
