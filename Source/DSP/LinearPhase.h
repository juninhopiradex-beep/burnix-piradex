// =============================================================================
//  BURNIX by PIRADEX — crossovers de FASE LINEAR (2.1 "Mastering"). Puro C++ (sem JUCE), testado em Tests/test_dsp.cpp.
//
//  Para N bandas desenha N-1 passa-baixo FIR simetricos (sinc com janela Blackman-Harris), todos com o mesmo
//  comprimento L e o mesmo atraso D = (L-1)/2:
//      banda 0 = LP_0(x)      banda k = LP_k(x) - LP_{k-1}(x)      banda N-1 = x(n-D) - LP_{N-2}(x)
//  A soma e telescopica: dá exatamente x(n-D) (reconstrucao perfeita, qualquer que seja a qualidade dos FIR),
//  e cada banda tem fase linear (sem rotacao de fase nos graves, ao contrario dos Linkwitz-Riley).
//
//  Convolucao em blocos no dominio da frequencia (overlap-save com particoes uniformes): custo por amostra
//  ~ (numero de particoes x crossovers) multiplicacoes complexas + FFTs de 2B pontos. Latencia total B + D,
//  sempre multipla de 16 (para dar um numero inteiro de amostras depois de descer o oversampling 1x/4x/8x/16x).
// =============================================================================
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace bx
{
namespace fft
{
// FFT complexa radix-2 iterativa (in-place). n potencia de 2.
class Plan
{
public:
    void init (int size)
    {
        n = size; int lg = 0; while ((1 << lg) < n) ++lg; logN = lg;
        rev.assign ((size_t) n, 0);
        for (int i = 0; i < n; ++i) { int r = 0; for (int b = 0; b < logN; ++b) if (i & (1 << b)) r |= 1 << (logN - 1 - b); rev[(size_t) i] = r; }
        tw.resize ((size_t) n / 2);
        for (int i = 0; i < n / 2; ++i) tw[(size_t) i] = std::polar (1.0f, (float) (-2.0 * 3.14159265358979323846 * i / n));
    }
    int size() const { return n; }
    void forward (std::complex<float>* a) const { run (a, false); }
    void inverse (std::complex<float>* a) const { run (a, true); const float s = 1.0f / (float) n; for (int i = 0; i < n; ++i) a[i] *= s; }
private:
    void run (std::complex<float>* a, bool inv) const
    {
        for (int i = 0; i < n; ++i) { const int r = rev[(size_t) i]; if (r > i) std::swap (a[i], a[r]); }
        for (int len = 2; len <= n; len <<= 1)
        {
            const int half = len >> 1, step = n / len;
            for (int i = 0; i < n; i += len)
                for (int j = 0; j < half; ++j)
                {
                    auto w = tw[(size_t) (j * step)]; if (inv) w = std::conj (w);
                    const auto u = a[i + j], v = a[i + j + half] * w;
                    a[i + j] = u + v; a[i + j + half] = u - v;
                }
        }
    }
    int n = 0, logN = 0; std::vector<int> rev; std::vector<std::complex<float>> tw;
};
} // namespace fft

class LinearPhaseSplitter
{
public:
    static constexpr int kMaxX = 5;

    // comprimento ~ 60 ms (a taxa do motor), limitado a 16385 taps; atraso D multiplo de 16
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        int d = (int) std::lround (0.030 * sr); d = std::clamp (d, 256, 8192); d = (d + 15) / 16 * 16;
        D = d; L = 2 * D + 1;
        B = 16; while (B * 16 < L) B <<= 1;                 // ~16 particoes
        P = (L + B - 1) / B;
        plan.init (2 * B);
        for (auto& c : ch) c.alloc (B, P, D, kMaxX);
        filt.assign ((size_t) kMaxX, std::vector<std::complex<float>> ((size_t) (P * 2 * B)));
        hTmp.assign ((size_t) L, 0.f); tTmp.assign ((size_t) L, 0.0); bufTmp.assign ((size_t) (2 * B), {});   // sem alocacoes no setup()
        nx = -1; for (auto& f : freq) f = -1.0;
        reset();
    }
    void reset() { for (auto& c : ch) c.clear(); }
    int latency() const { return B + D; }

    // muda os filtros so quando algo mudou (custo: desenho + P FFTs por crossover)
    void setup (int numCrossovers, const double* fc)
    {
        numCrossovers = std::clamp (numCrossovers, 0, kMaxX);
        bool same = numCrossovers == nx;
        for (int k = 0; same && k < numCrossovers; ++k) same = std::abs (fc[k] - freq[(size_t) k]) < 1e-6;
        if (same) return;
        nx = numCrossovers;
        auto& h = hTmp; auto& buf = bufTmp;
        for (int k = 0; k < nx; ++k)
        {
            freq[(size_t) k] = fc[k];
            design (fc[k], h);
            for (int p = 0; p < P; ++p)
            {
                std::fill (buf.begin(), buf.end(), std::complex<float> {});
                for (int i = 0; i < B; ++i) { const int t = p * B + i; if (t < L) buf[(size_t) i] = h[(size_t) t]; }
                plan.forward (buf.data());
                std::copy (buf.begin(), buf.end(), filt[(size_t) k].begin() + (long) (p * 2 * B));
            }
        }
    }

    // uma amostra do canal c -> nx+1 bandas (latencia B + D)
    inline void process (int c, double x, double* out)
    {
        auto& s = ch[(size_t) c];
        s.in[(size_t) (B + s.pos)] = (float) x;                     // segunda metade = bloco atual
        s.dly[(size_t) s.dpos] = x;                                   // x atrasado (para a banda de cima)
        const double xd = s.dly[(size_t) ((s.dpos + 1) % (int) s.dly.size())];   // atraso = tamanho - 1 = B + D
        s.dpos = (s.dpos + 1) % (int) s.dly.size();
        double prev = 0.0;
        for (int k = 0; k < nx; ++k) { const double y = s.out[(size_t) k][(size_t) s.pos]; out[k] = y - prev; prev = y; }
        out[nx] = xd - prev;
        if (++s.pos == B) { block (s); s.pos = 0; }
    }

private:
    struct Chan
    {
        std::vector<float> in;                                        // [bloco anterior | bloco atual]
        std::vector<std::vector<std::complex<float>>> fdl;           // espectros dos ultimos P blocos
        std::vector<std::vector<double>> out;                        // saida dos LP (ja com o atraso de bloco)
        std::vector<double> dly; std::vector<std::complex<float>> X, Y;
        int pos = 0, dpos = 0, head = 0;
        void alloc (int B, int P, int D, int nx)
        {
            in.assign ((size_t) (2 * B), 0.f); fdl.assign ((size_t) P, std::vector<std::complex<float>> ((size_t) (2 * B)));
            out.assign ((size_t) nx, std::vector<double> ((size_t) B, 0.0)); dly.assign ((size_t) (B + D + 1), 0.0);
            X.assign ((size_t) (2 * B), {}); Y.assign ((size_t) (2 * B), {});
        }
        void clear()
        {
            std::fill (in.begin(), in.end(), 0.f); for (auto& f : fdl) std::fill (f.begin(), f.end(), std::complex<float> {});
            for (auto& o : out) std::fill (o.begin(), o.end(), 0.0); std::fill (dly.begin(), dly.end(), 0.0); pos = dpos = head = 0;
        }
    };

    void block (Chan& s)
    {
        for (int i = 0; i < 2 * B; ++i) s.X[(size_t) i] = s.in[(size_t) i];
        plan.forward (s.X.data());
        s.head = (s.head + P - 1) % P;
        std::copy (s.X.begin(), s.X.end(), s.fdl[(size_t) s.head].begin());
        for (int k = 0; k < nx; ++k)
        {
            std::fill (s.Y.begin(), s.Y.end(), std::complex<float> {});
            const auto* H = filt[(size_t) k].data();
            for (int p = 0; p < P; ++p)
            {
                const auto& Xp = s.fdl[(size_t) ((s.head + p) % P)]; const auto* Hp = H + p * 2 * B;
                for (int i = 0; i < 2 * B; ++i) s.Y[(size_t) i] += Xp[(size_t) i] * Hp[i];
            }
            plan.inverse (s.Y.data());
            for (int i = 0; i < B; ++i) s.out[(size_t) k][(size_t) i] = s.Y[(size_t) (B + i)].real();   // overlap-save: 2.a metade
        }
        std::copy (s.in.begin() + B, s.in.end(), s.in.begin());
    }

    void design (double fc, std::vector<float>& h)
    {
        const double pi = 3.14159265358979323846, w = 2.0 * fc / sr;
        double sum = 0.0; auto& t = tTmp;
        for (int n = 0; n < L; ++n)
        {
            const double m = n - D, sinc = m == 0 ? w : std::sin (pi * w * m) / (pi * m);
            const double a = 2.0 * pi * n / (L - 1);
            const double win = 0.35875 - 0.48829 * std::cos (a) + 0.14128 * std::cos (2 * a) - 0.01168 * std::cos (3 * a);   // Blackman-Harris
            t[(size_t) n] = sinc * win; sum += t[(size_t) n];
        }
        for (int n = 0; n < L; ++n) h[(size_t) n] = (float) (t[(size_t) n] / sum);   // ganho DC = 1 exato
    }

    double sr = 48000.0; int L = 1, D = 0, B = 16, P = 1, nx = -1;
    std::array<double, kMaxX> freq {};
    fft::Plan plan; std::array<Chan, 2> ch; std::vector<std::vector<std::complex<float>>> filt;
    std::vector<float> hTmp; std::vector<double> tTmp; std::vector<std::complex<float>> bufTmp;
};
} // namespace bx
