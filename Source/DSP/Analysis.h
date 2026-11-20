// =============================================================================
//  BURNIX by PIRADEX — analise de material (puro C++). Usada pelo ASSISTENTE (o que esta a entrar?) e pelo
//  MATCH DE REFERENCIA (que densidade tem a faixa de referencia, banda a banda?).
//
//  Oito bandas (passa-banda de 2.a ordem sobre a soma mono): energia, pico e fator de crista (pico/RMS) por banda,
//  centroide espectral aproximado, correlacao estereo e densidade de ataques (ataques por segundo).
//  O fator de crista e a medida usada para "densidade": a saturacao baixa a crista, por isso uma referencia com
//  menos crista numa banda pede mais drive nessa banda. E uma estimativa honesta, nao uma copia do som.
// =============================================================================
#pragma once
#include "Biquad.h"
#include <array>
#include <cmath>

namespace bx
{
class Analyzer
{
public:
    static constexpr int kBands = 8;
    static constexpr double kEdges[kBands + 1] { 20, 80, 200, 500, 1000, 2500, 6000, 12000, 20000 };
    struct Result
    {
        std::array<double, kBands> energyDb {}, crestDb {};
        double centroidHz = 0, lowShare = 0, correlation = 1, attacksPerSec = 0, rmsDb = -120, seconds = 0;
        bool valid = false;
    };
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int b = 0; b < kBands; ++b)
        {
            const double lo = kEdges[b], hi = std::min (kEdges[b + 1], 0.45 * sr), c = std::sqrt (lo * hi), q = c / std::max (1.0, hi - lo);
            bp[(size_t) b].set (design::bandpass (c, std::max (0.3, q), sr));
        }
        aF = envCoeff (2.0, sr); rF = envCoeff (30.0, sr); aS = envCoeff (40.0, sr); rS = envCoeff (300.0, sr);
        reset();
    }
    void reset()
    {
        for (auto& f : bp) f.reset();
        e2.fill (0); pk.fill (0); n = 0; sLL = sRR = sLR = 0; fast = slow = 0; attacks = 0; armed = true; tot2 = 0;
    }
    inline void push (double l, double r)
    {
        const double m = 0.5 * (l + r);
        for (int b = 0; b < kBands; ++b) { const double y = bp[(size_t) b].process (m); e2[(size_t) b] += y * y; pk[(size_t) b] = std::max (pk[(size_t) b], std::fabs (y)); }
        sLL += l * l; sRR += r * r; sLR += l * r; tot2 += m * m;
        const double a = std::fabs (m);
        fast += (a > fast ? aF : rF) * (a - fast); slow += (a > slow ? aS : rS) * (a - slow);
        if (armed && slow > 1e-4 && fast > 1.8 * slow) { ++attacks; armed = false; }
        else if (! armed && fast < 1.2 * slow) armed = true;
        ++n;
    }
    double seconds() const { return n / sr; }
    Result result() const
    {
        Result r; if (n < 64) return r;
        double sumE = 0, cen = 0;
        for (int b = 0; b < kBands; ++b)
        {
            const double rms = std::sqrt (e2[(size_t) b] / n);
            r.energyDb[(size_t) b] = gainToDb (rms);
            r.crestDb[(size_t) b] = rms > 1e-7 ? gainToDb (pk[(size_t) b] / rms) : 0.0;
            sumE += e2[(size_t) b]; cen += e2[(size_t) b] * std::sqrt (kEdges[b] * kEdges[b + 1]);
        }
        r.centroidHz = sumE > 0 ? cen / sumE : 0;
        r.lowShare = sumE > 0 ? (e2[0] + e2[1]) / sumE : 0;
        r.correlation = (sLL > 0 && sRR > 0) ? sLR / std::sqrt (sLL * sRR) : 1.0;
        r.seconds = seconds(); r.attacksPerSec = attacks / std::max (1e-3, r.seconds);
        r.rmsDb = gainToDb (std::sqrt (tot2 / n)); r.valid = r.rmsDb > -70;
        return r;
    }
private:
    double sr = 48000; std::array<Biquad, kBands> bp;
    std::array<double, kBands> e2 {}, pk {};
    long n = 0; double sLL = 0, sRR = 0, sLR = 0, tot2 = 0, fast = 0, slow = 0, aF = 0, rF = 0, aS = 0, rS = 0; int attacks = 0; bool armed = true;
};

// Classificacao simples (regras explicaveis, nao uma rede neuronal): devolve a categoria de presets mais provavel
// e o motivo em palavras. Categorias = as do navegador de presets.
struct Guess { const char* category; const char* reasonPt; const char* reasonEn; };
inline Guess classify (const Analyzer::Result& r)
{
    const double crestMid = 0.5 * (r.crestDb[3] + r.crestDb[4]);
    if (r.lowShare > 0.55 && r.attacksPerSec > 1.2 && r.crestDb[1] > 11) return { "Kick", "muita energia abaixo de 200 Hz com ataques curtos e regulares", "lots of energy below 200 Hz with short, regular attacks" };
    if (r.lowShare > 0.55)                                               return { "808 & Sub", "a maior parte da energia está abaixo de 200 Hz e é sustentada", "most of the energy sits below 200 Hz and is sustained" };
    if (r.lowShare > 0.30 && r.centroidHz < 700)                          return { "Baixo", "graves fortes e pouco brilho", "strong lows and little top end" };
    if (r.correlation < 0.85 && r.lowShare > 0.12 && crestMid < 14)       return { "Master", "espectro completo, estéreo largo e já denso", "full spectrum, wide stereo and already dense" };
    if (r.attacksPerSec > 3.0 && crestMid > 13)                           return { "Bateria", "muitos ataques por segundo e picos altos", "many attacks per second and high peaks" };
    if (r.correlation > 0.97 && r.centroidHz > 900 && r.centroidHz < 3500) return { "Voz", "fonte quase mono com o centro do espectro na zona da voz", "near-mono source centred in the vocal range" };
    if (r.centroidHz >= 3500)                                             return { "Percussao", "muita energia nos agudos", "lots of high-frequency energy" };
    if (r.correlation < 0.9)                                              return { "Mix Bus", "material estéreo com várias fontes", "stereo material with several sources" };
    return { "Guitarra", "fonte de médios com ataques moderados", "mid-range source with moderate attacks" };
}
} // namespace bx
