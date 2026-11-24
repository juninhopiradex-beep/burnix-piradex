// =============================================================================
//  BURNIX by PIRADEX — medidor de loudness ITU-R BS.1770-4 / EBU R128 (2.1 "Mastering"). Puro C++ (sem JUCE).
//
//  K-weighting (pre-filtro high-shelf + RLB passa-alto, desenhados para qualquer taxa como na libebur128),
//  blocos de 400 ms com sobreposicao de 75 % (passo de 100 ms):
//    MOMENTARY  = bloco de 400 ms        SHORT-TERM = janela de 3 s
//    INTEGRATED = media dos blocos acima de -70 LUFS e acima de (media - 10 LU)          (BS.1770-4)
//    LRA        = P95 - P10 dos valores short-term acima de -70 LUFS e de (media - 20 LU) (EBU Tech 3342)
//  Integrated e LRA usam histogramas de 0,01 LU: memoria fixa, sem alocacoes no audio. Testado em Tests/test_dsp.cpp.
// =============================================================================
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace bx
{
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        // pre-filtro (shelf de +4 dB a ~1.7 kHz) e RLB (passa-alto ~38 Hz): formulas da libebur128
        const double pi = 3.14159265358979323846;
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (pi * f0 / sr), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            pre = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773, K = std::tan (pi * f0 / sr);
            const double a0 = 1.0 + K / Q + K * K;
            rlb = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
        }
        step = std::max (1, (int) std::lround (0.1 * sr));
        reset();
    }

    void reset()
    {
        for (auto& c : z) c = {};
        acc = 0.0; accN = 0; ring.fill (0.0); ringN = 0; ringPos = 0;
        intHist.fill (0.0); intCount.fill (0); stHist.fill (0); stTotal = 0; stSum = 0.0; stCnt = 0; totalSub = 0;
        mom = st = -120.0;
    }

    // uma amostra estereo (ou mono: r = l e mono = true)
    inline void process (double l, double r, bool mono = false)
    {
        const double kl = weigh (0, l), kr = mono ? 0.0 : weigh (1, r);
        acc += kl * kl + kr * kr;
        if (++accN == step) { pushSubBlock (acc / step); acc = 0.0; accN = 0; }
    }

    double momentary() const { return mom; }
    double shortTerm() const { return st; }
    double integrated() const
    {
        // passo 1: porta absoluta (-70); passo 2: porta relativa (media - 10 LU)
        double e = 0.0; long n = 0;
        for (int b = 0; b < kBins; ++b) { e += intHist[(size_t) b]; n += intCount[(size_t) b]; }
        if (n == 0) return -120.0;
        const double rel = toLufs (e / (double) n) - 10.0;
        e = 0.0; n = 0;
        for (int b = binOf (rel); b < kBins; ++b) { if (b < 0) continue; e += intHist[(size_t) b]; n += intCount[(size_t) b]; }
        return n > 0 ? toLufs (e / (double) n) : -120.0;
    }
    double range() const
    {
        if (stCnt == 0) return 0.0;
        const double rel = toLufs (stSum / (double) stCnt) - 20.0;
        long tot = 0; const int b0 = std::max (0, binOf (rel));
        for (int b = b0; b < kBins; ++b) tot += stHist[(size_t) b];
        if (tot < 2) return 0.0;
        auto pct = [&] (double q) { const long want = (long) std::ceil (q * (double) tot); long c = 0; for (int b = b0; b < kBins; ++b) { c += stHist[(size_t) b]; if (c >= want) return lufsOf (b); } return lufsOf (kBins - 1); };
        return std::max (0.0, pct (0.95) - pct (0.10));
    }
    double seconds() const { return 0.1 * (double) totalSub; }

private:
    struct Coeffs { double b0, b1, b2, a1, a2; };
    struct State { double s1 = 0, s2 = 0, t1 = 0, t2 = 0; };
    static constexpr int kBins = 7500;                        // -70 .. +5 LUFS em passos de 0,01
    static int binOf (double lufs) { return (int) std::floor ((lufs + 70.0) * 100.0); }
    static double lufsOf (int b) { return -70.0 + (b + 0.5) / 100.0; }
    static double toLufs (double ms) { return ms > 0 ? -0.691 + 10.0 * std::log10 (ms) : -120.0; }

    inline double weigh (int c, double x)
    {
        auto& s = z[(size_t) c];
        const double y1 = pre.b0 * x + s.s1; s.s1 = pre.b1 * x - pre.a1 * y1 + s.s2; s.s2 = pre.b2 * x - pre.a2 * y1;
        const double y2 = rlb.b0 * y1 + s.t1; s.t1 = rlb.b1 * y1 - rlb.a1 * y2 + s.t2; s.t2 = rlb.b2 * y1 - rlb.a2 * y2;
        return y2;
    }
    void pushSubBlock (double ms)
    {
        ring[(size_t) ringPos] = ms; ringPos = (ringPos + 1) % 30; ringN = std::min (ringN + 1, 30); ++totalSub;
        auto meanLast = [&] (int k) { double e = 0; for (int i = 1; i <= k; ++i) e += ring[(size_t) ((ringPos - i + 30) % 30)]; return e / k; };
        if (ringN >= 4)
        {
            const double e = meanLast (4); mom = toLufs (e);
            if (mom >= -70.0) { const int b = std::min (kBins - 1, binOf (mom)); intHist[(size_t) b] += e; ++intCount[(size_t) b]; }
        }
        if (ringN >= 30)
        {
            const double e = meanLast (30); st = toLufs (e);
            if (st >= -70.0) { const int b = std::min (kBins - 1, binOf (st)); ++stHist[(size_t) b]; stSum += e; ++stCnt; }
        }
    }

    double sr = 48000.0; int step = 4800;
    Coeffs pre {}, rlb {};
    std::array<State, 2> z {};
    double acc = 0.0; int accN = 0;
    std::array<double, 30> ring {}; int ringN = 0, ringPos = 0; long totalSub = 0;
    std::array<double, kBins> intHist {}; std::array<long, kBins> intCount {};
    std::array<long, kBins> stHist {}; long stTotal = 0; double stSum = 0.0; long stCnt = 0;
    double mom = -120.0, st = -120.0;
};
} // namespace bx
