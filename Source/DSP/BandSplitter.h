// =============================================================================
//  BURNIX by PIRADEX — BandSplitter: divisor de 1 a 6 bandas com crossovers Linkwitz-Riley
//
//  Topologia em cascata: em cada crossover k, LP_k(resto) = banda k e HP_k(resto) = novo resto.
//  Para que a soma das bandas seja um passa-tudo (modulo plano), cada banda k passa ainda por um
//  passa-tudo equivalente a cada crossover acima dela (LR4: AP2 Q=0.7071; LR8: AP2 Q=0.5412 x AP2 Q=1.3066).
//  Identidades usadas (validas tambem apos a transformada bilinear, porque LP/HP/AP partilham o
//  mesmo prototipo e a mesma pre-distorcao):  LR_LP + LR_HP = AP  (ordem par: LP e HP em fase).
//  Puro C++ (sem JUCE), testado em Tests/test_dsp.cpp (reconstrucao +-0.01 dB).
// =============================================================================
#pragma once
#include "Biquad.h"
#include <algorithm>
#include <array>

namespace bx
{
constexpr int kMaxBands = 6;
constexpr int kMaxX = kMaxBands - 1;

enum class Slope { LR24 = 0, LR48 = 1 };

class BandSplitter
{
public:
    void prepare (double sampleRate) { sr = sampleRate; dirty = true; reset(); }
    void reset()
    {
        for (auto& ch : st) { for (auto& x : ch.lp) for (auto& b : x) b.reset(); for (auto& x : ch.hp) for (auto& b : x) b.reset();
                              for (auto& bb : ch.ap) for (auto& x : bb) for (auto& b : x) b.reset(); }
    }

    // frequencias ordenadas e afastadas pelo menos 1/3 de oitava; so muda coeficientes se algo mudou
    void setup (int numBands, const double* xo, Slope s)
    {
        numBands = std::clamp (numBands, 1, kMaxBands);
        std::array<double, kMaxX> f {};
        double lo = 20.0;
        for (int k = 0; k < numBands - 1; ++k)
        {
            f[(size_t) k] = std::clamp (xo[k], lo * 1.26, 0.45 * sr / 1.26);
            lo = f[(size_t) k];
        }
        bool same = ! dirty && numBands == nb && s == slope;
        for (int k = 0; same && k < numBands - 1; ++k) same = std::abs (f[(size_t) k] - freq[(size_t) k]) < 1e-6;
        if (same) return;
        const bool structural = dirty || numBands != nb || s != slope;
        nb = numBands; slope = s; freq = f; dirty = false;
        for (int k = 0; k < nb - 1; ++k)
        {
            const double fc = freq[(size_t) k];
            if (slope == Slope::LR24)
            {
                lp[(size_t) k] = { design::lowpass (fc, 0.70710678, sr), design::lowpass (fc, 0.70710678, sr), {}, {} };
                hp[(size_t) k] = { design::highpass (fc, 0.70710678, sr), design::highpass (fc, 0.70710678, sr), {}, {} };
                ap[(size_t) k] = { design::allpass (fc, 0.70710678, sr), {} };
            }
            else
            {
                const double q1 = 0.54119610, q2 = 1.30656296;     // Butterworth de 4.a ordem
                lp[(size_t) k] = { design::lowpass (fc, q1, sr), design::lowpass (fc, q2, sr), design::lowpass (fc, q1, sr), design::lowpass (fc, q2, sr) };
                hp[(size_t) k] = { design::highpass (fc, q1, sr), design::highpass (fc, q2, sr), design::highpass (fc, q1, sr), design::highpass (fc, q2, sr) };
                ap[(size_t) k] = { design::allpass (fc, q1, sr), design::allpass (fc, q2, sr) };
            }
        }
        for (auto& ch : st)
        {
            for (int k = 0; k < kMaxX; ++k)
            {
                for (int i = 0; i < 4; ++i) { ch.lp[(size_t) k][(size_t) i].set (lp[(size_t) k][(size_t) i]); ch.hp[(size_t) k][(size_t) i].set (hp[(size_t) k][(size_t) i]); }
                for (int b = 0; b < kMaxBands; ++b) for (int i = 0; i < 2; ++i) ch.ap[(size_t) b][(size_t) k][(size_t) i].set (ap[(size_t) k][(size_t) i]);
            }
        }
        if (structural) reset();
    }

    int numBands() const { return nb; }
    double crossover (int k) const { return freq[(size_t) k]; }

    // divide uma amostra do canal ch em nb bandas (out[0..nb-1])
    inline void process (int ch, double x, double* out)
    {
        auto& s = st[(size_t) ch];
        const int stages = slope == Slope::LR24 ? 2 : 4, apStages = slope == Slope::LR24 ? 1 : 2;
        double rest = x;
        for (int k = 0; k < nb - 1; ++k)
        {
            double l = rest, h = rest;
            for (int i = 0; i < stages; ++i) { l = s.lp[(size_t) k][(size_t) i].process (l); h = s.hp[(size_t) k][(size_t) i].process (h); }
            out[k] = l; rest = h;
        }
        out[nb - 1] = rest;
        // compensacao de fase: banda b passa pelos passa-tudo dos crossovers acima dela
        for (int b = 0; b < nb - 1; ++b)
            for (int k = b + 1; k < nb - 1; ++k)
                for (int i = 0; i < apStages; ++i) out[b] = s.ap[(size_t) b][(size_t) k][(size_t) i].process (out[b]);
    }

private:
    struct ChanState
    {
        std::array<std::array<Biquad, 4>, kMaxX> lp, hp;
        std::array<std::array<std::array<Biquad, 2>, kMaxX>, kMaxBands> ap;
    };
    double sr = 48000.0; int nb = 1; Slope slope = Slope::LR24; bool dirty = true;
    std::array<double, kMaxX> freq {};
    std::array<std::array<BiquadCoeffs, 4>, kMaxX> lp {}, hp {};
    std::array<std::array<BiquadCoeffs, 2>, kMaxX> ap {};
    std::array<ChanState, 2> st;
};
} // namespace bx
