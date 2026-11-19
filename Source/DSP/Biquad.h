// =============================================================================
//  BURNIX by PIRADEX — biquad (forma transposta direta II, double) e desenhos RBJ / Butterworth
//  Puro C++ (sem JUCE): usado no DSP e nos testes.
// =============================================================================
#pragma once
#include <cmath>
#include <complex>

namespace bx
{
constexpr double kPi = 3.14159265358979323846;

struct BiquadCoeffs
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;   // a0 normalizado a 1

    // resposta em frequencia (para testes e para desenhar curvas)
    std::complex<double> response (double hz, double sr) const
    {
        const double w = 2.0 * kPi * hz / sr;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        return (b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2);
    }
};

// ---- desenhos (Robert Bristow-Johnson, "Audio EQ Cookbook") -------------------------------------
namespace design
{
    inline double clampHz (double hz, double sr) { return std::fmin (std::fmax (hz, 5.0), 0.49 * sr); }

    inline BiquadCoeffs lowpass (double hz, double q, double sr)
    {
        const double w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), a = std::sin (w) / (2 * q), a0 = 1 + a;
        return { (1 - c) / 2 / a0, (1 - c) / a0, (1 - c) / 2 / a0, -2 * c / a0, (1 - a) / a0 };
    }
    inline BiquadCoeffs highpass (double hz, double q, double sr)
    {
        const double w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), a = std::sin (w) / (2 * q), a0 = 1 + a;
        return { (1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0, -2 * c / a0, (1 - a) / a0 };
    }
    inline BiquadCoeffs bandpass (double hz, double q, double sr)          // ganho 0 dB no centro
    {
        const double w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), a = std::sin (w) / (2 * q), a0 = 1 + a;
        return { a / a0, 0, -a / a0, -2 * c / a0, (1 - a) / a0 };
    }
    inline BiquadCoeffs notch (double hz, double q, double sr)
    {
        const double w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), a = std::sin (w) / (2 * q), a0 = 1 + a;
        return { 1 / a0, -2 * c / a0, 1 / a0, -2 * c / a0, (1 - a) / a0 };
    }
    inline BiquadCoeffs allpass (double hz, double q, double sr)
    {
        const double w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), a = std::sin (w) / (2 * q), a0 = 1 + a;
        return { (1 - a) / a0, -2 * c / a0, (1 + a) / a0, -2 * c / a0, (1 - a) / a0 };
    }
    inline BiquadCoeffs lowShelf (double hz, double q, double gainDb, double sr)
    {
        const double A = std::pow (10.0, gainDb / 40), w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / (2 * q), k = 2 * std::sqrt (A) * al, a0 = (A + 1) + (A - 1) * c + k;
        return { A * ((A + 1) - (A - 1) * c + k) / a0, 2 * A * ((A - 1) - (A + 1) * c) / a0, A * ((A + 1) - (A - 1) * c - k) / a0,
                 -2 * ((A - 1) + (A + 1) * c) / a0, ((A + 1) + (A - 1) * c - k) / a0 };
    }
    inline BiquadCoeffs highShelf (double hz, double q, double gainDb, double sr)
    {
        const double A = std::pow (10.0, gainDb / 40), w = 2 * kPi * clampHz (hz, sr) / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / (2 * q), k = 2 * std::sqrt (A) * al, a0 = (A + 1) - (A - 1) * c + k;
        return { A * ((A + 1) + (A - 1) * c + k) / a0, -2 * A * ((A - 1) + (A + 1) * c) / a0, A * ((A + 1) + (A - 1) * c - k) / a0,
                 2 * ((A - 1) - (A + 1) * c) / a0, ((A + 1) - (A - 1) * c - k) / a0 };
    }
    inline BiquadCoeffs onePoleLowpass (double hz, double sr)            // 1.a ordem (b2 = a2 = 0), transformada bilinear
    {
        const double k = std::tan (kPi * clampHz (hz, sr) / sr), a0 = 1 + k;
        return { k / a0, k / a0, 0, (k - 1) / a0, 0 };
    }
}

struct Biquad
{
    BiquadCoeffs c; double z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    void set (const BiquadCoeffs& k) { c = k; }
    inline double process (double x)
    {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
};

// ---- utilidades -----------------------------------------------------------------------------------
inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g)  { return 20.0 * std::log10 (std::fmax (g, 1e-12)); }
inline double sanitize (double x)  { return std::isfinite (x) ? std::fmax (-64.0, std::fmin (64.0, x)) : 0.0; }   // protecao numerica (nao e limitador)

// coeficiente de um seguidor de envolvente de 1.a ordem
inline double envCoeff (double ms, double sr) { return ms <= 0 ? 1.0 : 1.0 - std::exp (-1.0 / (0.001 * ms * sr)); }
} // namespace bx
