// =============================================================================
//  BURNIX by PIRADEX — NonlinearTubeStage e HarmonicEngine (puro C++, sem JUCE)
//
//  Seis voicings ORIGINAIS (nao sao modelos de valvulas reais nem copias de plugins comerciais):
//    EMBER         tanh com bias interno pequeno       -> 3.o dominante + 2.o ligeiro, pouca aspereza
//    TRIODE BLOOM  tanh assimetrico (negativo mais macio) + bias  -> 2.o harmonico proeminente
//    PENTODE EDGE  curva u/(1+|u|^2.5)^0.4, joelho mais duro e simetrico -> impares, mais "mordida"
//    IRON CORE     tanh com memoria de 1.a ordem (u + h(u - m)), m segue u a 250 Hz -> densidade low-mid
//                  (aproximacao suave; NAO e um modelo de histerese de transformador)
//    VELVET        atan (joelho muito suave) + passa-baixo progressivo com o drive -> pouco THD aparente
//    FIRESTORM     clipper com joelho quadratico (C1) e limiares assimetricos (+1 / -0.8) -> agressivo
//
//  Normalizacao:  y = (f(s*x + b) - f(b)) / (f(s + b) - f(b))   -> sem DC estatica e x=1 -> y=1.
//  O drive (0..10) mapeia s = 0.35 * 10^(drive*2.7/20). Abaixo de drive 1 o resultado e misturado com o
//  sinal limpo (drive 0 = identidade exata). A componente adicionada (y - x) passa por um bloqueador de DC
//  a 5 Hz: o sinal limpo nunca e filtrado.
//  Dinamica opcional: DENSITY (sag: o drive baixa e a assimetria sobe com a envolvente) e TRANSIENT
//  PRESERVE (menos saturacao durante ataques: envolvente rapida vs lenta).
//  Segundo estagio (STAGE BLEND): PENTODE com ganho INTERSTAGE, em serie depois (ou antes, TOPOLOGY) do voicing.
// =============================================================================
#pragma once
#include "Biquad.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace bx
{
enum Voicing { Ember = 0, TriodeBloom, PentodeEdge, IronCore, Velvet, Firestorm, kNumVoicings };
inline const char* voicingName (int v)
{
    static const char* n[] = { "EMBER", "TRIODE BLOOM", "PENTODE EDGE", "IRON CORE", "VELVET", "FIRESTORM" };
    return n[std::clamp (v, 0, kNumVoicings - 1)];
}

// ---- curvas estaticas -------------------------------------------------------------------------------
namespace curve
{
    inline double ember (double u)   { return std::tanh (u); }
    inline double triode (double u)  { return u >= 0 ? std::tanh (u) : std::tanh (0.6 * u) / 0.6; }
    inline double pentode (double u) { const double a = std::fabs (u); return u * std::exp (-0.4 * std::log1p (a * a * std::sqrt (a))); }   // u / (1 + |u|^2.5)^0.4
    inline double velvet (double u)  { return (2.0 / kPi) * std::atan (0.5 * u * kPi * 0.5) * 2.0; }   // joelho largo: metade da curvatura
    inline double knee (double v, double t)       // v >= 0: linear ate t-w, joelho quadratico, teto t
    {
        const double w = 0.3 * t;
        if (v <= t - w) return v;
        if (v >= t + w) return t;
        const double d = v - (t - w); return v - d * d / (4.0 * w);
    }
    inline double fire (double u)    { const double v = 1.6 * u; return v >= 0 ? knee (v, 1.0) : -knee (-v, 0.8); }

    inline double eval (int v, double u)
    {
        switch (v)
        {
            case Ember:       return ember (u);
            case TriodeBloom: return triode (u);
            case PentodeEdge: return pentode (u);
            case IronCore:    return ember (u);       // a memoria e aplicada antes, no estagio
            case Velvet:      return velvet (u);
            case Firestorm:   return fire (u);
            default:          return u;
        }
    }
    inline double internalBias (int v)
    {
        switch (v) { case Ember: return 0.05; case TriodeBloom: return 0.18; case IronCore: return 0.06; case Velvet: return 0.04; default: return 0.0; }
    }
}

inline double driveToScale (double drive) { return 0.35 * std::pow (10.0, std::clamp (drive, 0.0, 10.0) * 2.7 / 20.0); }

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
    std::array<double, kNumVoicings> weights { 0, 1, 0, 0, 0, 0 };   // soma 1
};

// um "lane" = uma banda x um canal (L, R, M ou S)
class TubeStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        cIron = 1.0 - std::exp (-2.0 * kPi * 250.0 / sr);
        aSagA = envCoeff (3.0, sr); aSagR = envCoeff (80.0, sr);
        aFastA = envCoeff (0.5, sr); aFastR = envCoeff (15.0, sr);
        aSlowA = envCoeff (15.0, sr); aSlowR = envCoeff (150.0, sr);
        const double R = std::exp (-2.0 * kPi * 5.0 / sr); dcR = R;
        lastVelvetHz = -1; reset();
    }
    void reset() { sag = fast = slow = 0; ironM = 0; ironMB = 0; dcX = dcY = 0; velvetLP.reset(); for (auto& c : cache) c = {}; }

    inline double process (double x, const TubeParams& p)
    {
        const double ax = std::fabs (x);
        sag  += (ax > sag  ? aSagA  : aSagR)  * (ax - sag);
        fast += (ax > fast ? aFastA : aFastR) * (ax - fast);
        slow += (ax > slow ? aSlowA : aSlowR) * (ax - slow);

        const double a = std::min (1.0, p.drive);                 // mistura com o limpo abaixo de drive 1
        if (a <= 0.0) { dcX = dcY = 0; return x; }
        const double e = std::min (sag, 1.5);
        const double s = driveToScale (p.drive) * (1.0 - 0.45 * p.density * e / 1.5);
        const double b = 0.5 * p.bias + 0.25 * p.density * std::min (sag, 1.0);
        const double tr = slow > 1e-5 ? std::clamp ((fast - 1.1 * slow) / slow, 0.0, 1.0) : 0.0;
        const double aEff = a * (1.0 - p.transient * tr);

        double y;
        if (p.stageBlend <= 0.0)       y = voiced (x, s, b, p);
        else if (! p.swapOrder)      { const double v = voiced (x, s, b, p); y = v + p.stageBlend * (stageB (v, p) - v); }
        else                         { const double v = stageB (x, p) ; const double w = x + p.stageBlend * (v - x); y = voiced (w, s, b, p); }

        // so a componente adicionada passa pelo bloqueador de DC (5 Hz)
        const double d = y - x;
        const double dc = d - dcX + dcR * dcY; dcX = d; dcY = dc;
        return x + aEff * dc;
    }

private:
    // (f(s*x+b) - f(b)) / (f(s+b) - f(b)), com u0 = s*x. f(b) e o denominador ficam em cache enquanto s e b nao mudam
    // (o caso normal com DENSITY a 0): uma so avaliacao da curva por amostra.
    struct NormCache { double s = -1, b = 1e9, bb = 0, f0 = 0, inv = 1; };
    std::array<NormCache, kNumVoicings + 1> cache;
    inline double norm (int v, double u0, double s, double b, int slot)
    {
        auto& c = cache[(size_t) slot];
        if (s != c.s || b != c.b)
        {
            c.s = s; c.b = b; c.bb = b + curve::internalBias (v); c.f0 = curve::eval (v, c.bb);
            const double den = curve::eval (v, s + c.bb) - c.f0; c.inv = 1.0 / (std::fabs (den) > 1e-9 ? den : 1e-9);
        }
        return (curve::eval (v, u0 + c.bb) - c.f0) * c.inv;
    }
    inline double voiced (double x, double s, double b, const TubeParams& p)
    {
        double y = 0;
        for (int v = 0; v < kNumVoicings; ++v)
        {
            const double w = p.weights[(size_t) v];
            if (w < 1e-4) continue;
            double u0 = s * x;
            if (v == IronCore) { ironM += cIron * (u0 - ironM); u0 = u0 + 0.35 * (u0 - ironM); }
            double c = norm (v, u0, s, b, v);
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
        return norm (PentodeEdge, s * x, s, 0.0, kNumVoicings);
    }

    double sr = 48000, cIron = 0, aSagA = 0, aSagR = 0, aFastA = 0, aFastR = 0, aSlowA = 0, aSlowR = 0, dcR = 0.999;
    double sag = 0, fast = 0, slow = 0, ironM = 0, ironMB = 0, dcX = 0, dcY = 0, lastVelvetHz = -1;
    Biquad velvetLP;
};
} // namespace bx
