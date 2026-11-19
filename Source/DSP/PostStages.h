// =============================================================================
//  BURNIX by PIRADEX — estagios a taxa base (puro C++): cor, estereo, ganho automatico, medidores
// =============================================================================
#pragma once
#include "Biquad.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace bx
{
// ---- COLOR + AIR: tilt (pivot 1 kHz), BODY (prateleira grave 120 Hz) e AIR (prateleira aguda 12 kHz). So no ramo wet.
class ToneStage
{
public:
    void prepare (double s) { sr = s; set (0, 0, 0, true); for (auto& c : f) for (auto& b : c) b.reset(); }
    void set (double tiltDb, double body01, double air01, bool force = false)
    {
        if (! force && tiltDb == t && body01 == bo && air01 == ai) return;
        t = tiltDb; bo = body01; ai = air01;
        active = std::fabs (t) > 1e-6 || bo > 1e-6 || ai > 1e-6;
        const BiquadCoeffs k[4] = { design::lowShelf (1000.0, 0.5, -t * 0.5, sr), design::highShelf (1000.0, 0.5, t * 0.5, sr),
                                    design::lowShelf (120.0, 0.7, 4.0 * bo, sr), design::highShelf (12000.0, 0.7, 6.0 * ai, sr) };
        for (auto& c : f) for (int i = 0; i < 4; ++i) c[(size_t) i].set (k[i]);
    }
    inline double process (int ch, double x) { if (! active) return x; for (auto& b : f[(size_t) ch]) x = b.process (x); return x; }
private:
    double sr = 48000, t = 1e9, bo = 1e9, ai = 1e9; bool active = false;
    std::array<std::array<Biquad, 4>, 2> f;
};

// ---- largura e mono-bass (em M/S). Mono-bass: S passa por um passa-alto LR4 e M pelo passa-tudo equivalente
//      (mesma fase acima do corte; abaixo do corte o lado e removido, sem vazamento de graves para S).
class StereoStage
{
public:
    void prepare (double s) { sr = s; set (1.0, false, 120.0, true); reset(); }
    void reset() { for (auto& b : hp) b.reset(); ap.reset(); }
    void set (double width, bool mono, double hz, bool force = false)
    {
        if (! force && width == w && mono == mb && hz == f) return;
        const bool rs = mono != mb; w = width; mb = mono; f = hz;
        for (auto& b : hp) b.set (design::highpass (f, 0.70710678, sr));
        ap.set (design::allpass (f, 0.70710678, sr));
        if (rs) reset();
    }
    bool identity() const { return ! mb && std::fabs (w - 1.0) < 1e-9; }
    inline void process (double& l, double& r)
    {
        if (identity()) return;
        constexpr double k = 0.70710678118654752;
        double m = (l + r) * k, s = (l - r) * k * w;
        if (mb) { m = ap.process (m); s = hp[1].process (hp[0].process (s)); }
        l = (m + s) * k; r = (m - s) * k;
    }
private:
    double sr = 48000, w = -1, f = -1; bool mb = true;
    std::array<Biquad, 2> hp; Biquad ap;
};

// ---- ganho automatico (opt-in): iguala o RMS de saida ao do sinal seco alinhado. Limite +-18 dB, lento (sem bombear).
class LoudnessMatcher
{
public:
    void prepare (double s) { a = envCoeff (400.0, s); g = envCoeff (600.0, s); reset(); }
    void reset() { pr = po = 1e-8; gain = 1.0; }
    inline double next (double ref2, double out2, bool on)
    {
        pr += a * (ref2 - pr); po += a * (out2 - po);
        double target = 1.0;
        if (on && pr > 1e-9 && po > 1e-9) target = std::clamp (std::sqrt (pr / po), dbToGain (-18.0), dbToGain (18.0));
        gain += g * (target - gain);
        return gain;
    }
    double currentDb() const { return gainToDb (gain); }
private:
    double a = 0.001, g = 0.001, pr = 1e-8, po = 1e-8, gain = 1.0;
};

// ---- true peak (estimativa): interpolacao 4x com FIR de sinc janelado (Kaiser-ish via Blackman), 4 fases x 16 taps
class TruePeak
{
public:
    TruePeak()
    {
        constexpr int N = kTaps * 4;
        for (int p = 0; p < 4; ++p)
            for (int t = 0; t < kTaps; ++t)
            {
                const double n = t * 4 + p - (N - 1) / 2.0;
                const double x = n / 4.0, sinc = std::fabs (x) < 1e-12 ? 1.0 : std::sin (kPi * x) / (kPi * x);
                const double win = 0.42 - 0.5 * std::cos (2 * kPi * (t * 4 + p) / (N - 1)) + 0.08 * std::cos (4 * kPi * (t * 4 + p) / (N - 1));
                h[(size_t) p][(size_t) t] = sinc * win;
            }
        reset();
    }
    void reset() { for (auto& b : buf) b.fill (0); pos[0] = pos[1] = 0; }
    inline double push (int ch, double x)    // devolve o maior |valor| interpolado desta amostra
    {
        auto& b = buf[(size_t) ch]; b[(size_t) pos[ch]] = x; double m = 0;
        for (int p = 0; p < 4; ++p)
        {
            double y = 0; int idx = pos[ch];
            for (int t = 0; t < kTaps; ++t) { y += h[(size_t) p][(size_t) t] * b[(size_t) idx]; idx = (idx + kTaps - 1) % kTaps; }
            m = std::max (m, std::fabs (y));
        }
        pos[ch] = (pos[ch] + 1) % kTaps;
        return m;
    }
private:
    static constexpr int kTaps = 16;
    std::array<std::array<double, kTaps>, 4> h {};
    std::array<std::array<double, kTaps>, 2> buf {};
    int pos[2] { 0, 0 };
};
} // namespace bx
