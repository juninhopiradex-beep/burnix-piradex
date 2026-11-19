// =============================================================================
//  BURNIX by PIRADEX — testes do DSP puro (sem JUCE). Imprime medicoes reais e falha (exit 1) se um
//  criterio de aceitacao nao for cumprido.  Compilar: cmake -DBX_BUILD_TESTS=ON ; ./test_dsp
// =============================================================================
#include "DSP/BurnixEngine.h"
#include "DSP/PostStages.h"
#include <cstdio>
#include <vector>

using namespace bx;
static int fails = 0, passes = 0;
#define CHECK(cond, ...) do { if (cond) { ++passes; } else { ++fails; std::printf ("  FALHOU: "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

// amplitude de uma frequencia num sinal (projecao seno/cosseno sobre ciclos inteiros)
static double amp (const std::vector<double>& x, double hz, double sr)
{
    double c = 0, s = 0; const size_t n = x.size();
    for (size_t i = 0; i < n; ++i) { const double ph = 2 * kPi * hz * (double) i / sr; c += x[i] * std::cos (ph); s += x[i] * std::sin (ph); }
    return 2.0 * std::sqrt (c * c + s * s) / (double) n;
}

struct Harm { double h[11] {}; double thd() const { double s = 0; for (int k = 2; k <= 10; ++k) s += h[k] * h[k]; return std::sqrt (s) / std::max (h[1], 1e-12); } };

static Harm measureVoicing (int v, double drive, double level, double sr = 384000.0, double f0 = 1000.0)
{
    TubeStage t; t.prepare (sr); TubeParams p; p.drive = drive; p.weights = {}; p.weights[(size_t) v] = 1;
    const int warm = (int) sr / 2, n = (int) (sr / f0) * 200;            // 200 ciclos inteiros
    std::vector<double> y ((size_t) n);
    for (int i = 0; i < warm + n; ++i)
    {
        const double x = level * std::sin (2 * kPi * f0 * i / sr), o = t.process (x, p);
        if (i >= warm) y[(size_t) (i - warm)] = o;
    }
    Harm h; for (int k = 1; k <= 10; ++k) h.h[k] = amp (y, f0 * k, sr); return h;
}

int main()
{
    std::printf ("BURNIX by PIRADEX — testes DSP (resultados reais)\n\n");

    // 1) reconstrucao dos crossovers: |soma das bandas| plana
    std::printf ("[1] Reconstrucao do BandSplitter (modulo da soma, 20 Hz a 20 kHz)\n");
    for (int slope = 0; slope < 2; ++slope)
        for (int nb : { 2, 3, 6 })
        {
            const double sr = 48000; BandSplitter s; s.prepare (sr); const double xo[5] { 90, 250, 950, 5000, 12000 };
            s.setup (nb, xo, (Slope) slope);
            const int N = 1 << 16; std::vector<double> imp ((size_t) N); double out[kMaxBands];
            for (int i = 0; i < N; ++i) { s.process (0, i == 0 ? 1.0 : 0.0, out); double sum = 0; for (int b = 0; b < nb; ++b) sum += out[b]; imp[(size_t) i] = sum; }
            double worst = 0;
            for (double hz = 20; hz < 20000; hz *= 1.05)
            {
                double c = 0, si = 0; for (int i = 0; i < N; ++i) { c += imp[(size_t) i] * std::cos (2 * kPi * hz * i / sr); si -= imp[(size_t) i] * std::sin (2 * kPi * hz * i / sr); }
                worst = std::max (worst, std::fabs (gainToDb (std::sqrt (c * c + si * si))));
            }
            std::printf ("    %s  %d bandas: desvio maximo %.4f dB\n", slope ? "LR48" : "LR24", nb, worst);
            CHECK (worst < 0.01, "reconstrucao %d bandas slope %d desvio %.4f dB", nb, slope, worst);
        }

    // 2) drive 0 = identidade exata (motor completo: wet == dry)
    std::printf ("\n[2] Drive 0 -> wet identico a dry (motor completo, 6 bandas, M+S)\n");
    {
        BurnixEngine e; e.prepare (192000); EngineParams p; p.numBands = 6;
        for (auto& b : p.band) { b.driveM = b.driveS = 0; b.route = Route::MidSide; }
        p.heat = 5; e.setParams (p);
        double worst = 0;
        for (int i = 0; i < 200000; ++i)
        {
            const double l = 0.7 * std::sin (i * 0.013) + 0.2 * std::sin (i * 0.31), r = 0.5 * std::sin (i * 0.021 + 1);
            double wl, wr, dl, dr; e.process (l, r, wl, wr, dl, dr); worst = std::max ({ worst, std::fabs (wl - dl), std::fabs (wr - dr) });
        }
        std::printf ("    diferenca maxima wet-dry: %.3g\n", worst);
        CHECK (worst < 1e-12, "drive 0 nao e identidade: %.3g", worst);
    }

    // 3) THD por voicing (seno 1 kHz, -6 dBFS, drive 3 e 7) e caracter par/impar
    std::printf ("\n[3] Harmonicos por voicing (1 kHz, -6 dBFS, sem oversampling extra: fs = 384 kHz)\n");
    std::printf ("    %-13s %6s %9s %9s %9s %9s %9s\n", "voicing", "drive", "THD %", "H2 dB", "H3 dB", "H4 dB", "H5 dB");
    Harm byV[kNumVoicings];
    for (int v = 0; v < kNumVoicings; ++v)
        for (double d : { 3.0, 7.0 })
        {
            const auto h = measureVoicing (v, d, 0.5);
            if (d == 7.0) byV[v] = h;
            auto db = [&] (int k) { return gainToDb (h.h[k] / std::max (h.h[1], 1e-12)); };
            std::printf ("    %-13s %6.1f %9.3f %9.1f %9.1f %9.1f %9.1f\n", voicingName (v), d, 100 * h.thd(), db (2), db (3), db (4), db (5));
            CHECK (std::isfinite (h.thd()) && h.thd() < 1.0, "THD invalido %s", voicingName (v));
        }
    CHECK (byV[TriodeBloom].h[2] > byV[PentodeEdge].h[2] * 3, "TRIODE BLOOM devia ter mais 2.o harmonico do que PENTODE EDGE");
    CHECK (byV[PentodeEdge].h[3] > byV[Velvet].h[3], "PENTODE EDGE devia ter mais 3.o harmonico do que VELVET");
    CHECK (byV[Firestorm].thd() > byV[Ember].thd(), "FIRESTORM devia distorcer mais do que EMBER");

    // 4) monotonia do drive (THD cresce com o drive) para todos os voicings
    std::printf ("\n[4] THD cresce com o drive\n");
    for (int v = 0; v < kNumVoicings; ++v)
    {
        double prev = -1; bool mono = true;
        for (double d = 1; d <= 10; d += 1) { const double t = measureVoicing (v, d, 0.5, 192000).thd(); if (t + 1e-6 < prev) mono = false; prev = t; }
        std::printf ("    %-13s %s (THD a drive 10: %.2f %%)\n", voicingName (v), mono ? "ok" : "NAO MONOTONO", 100 * prev);
        CHECK (mono, "THD nao cresce com o drive em %s", voicingName (v));
    }

    // 5) sem DC na saida com bias (bloqueador de DC so na componente adicionada)
    std::printf ("\n[5] DC residual com bias +1 e TRIODE BLOOM drive 8\n");
    {
        TubeStage t; t.prepare (48000); TubeParams p; p.drive = 8; p.bias = 1.0; double sum = 0; const int n = 48000 * 4;
        for (int i = 0; i < n; ++i) { const double y = t.process (0.5 * std::sin (2 * kPi * 100 * i / 48000.0), p); if (i > n / 2) sum += y; }
        const double dc = sum / (n / 2);
        std::printf ("    DC medio: %.2e\n", dc);
        CHECK (std::fabs (dc) < 1e-3, "DC residual %.3g", dc);
    }

    // 6) robustez: NaN/Inf/picos enormes nao propagam (com sanitize) e o estado recupera
    std::printf ("\n[6] Robustez numerica\n");
    {
        BurnixEngine e; e.prepare (48000); EngineParams p; p.numBands = 4; for (auto& b : p.band) b.driveM = 10; e.setParams (p);
        bool finite = true;
        for (int i = 0; i < 48000; ++i)
        {
            double x = std::sin (i * 0.05); if (i == 100) x = 1e30; if (i == 200) x = sanitize (std::nan ("")); if (i == 300) x = sanitize (INFINITY);
            x = sanitize (x);
            double wl, wr, dl, dr; e.process (x, x, wl, wr, dl, dr); finite = finite && std::isfinite (wl) && std::isfinite (wr);
        }
        std::printf ("    saida sempre finita: %s\n", finite ? "sim" : "NAO");
        CHECK (finite, "saida nao finita");
    }

    // 7) M/S ortonormal e mono-bass sem graves no lado
    std::printf ("\n[7] Mono-bass 120 Hz: lado a 40 Hz atenuado\n");
    {
        StereoStage st; st.prepare (48000); st.set (1.0, true, 120.0);
        std::vector<double> side ((size_t) 48000);
        for (int i = 0; i < 96000; ++i) { double l = std::sin (2 * kPi * 40 * i / 48000.0), r = -l; st.process (l, r); if (i >= 48000) side[(size_t) (i - 48000)] = (l - r) * 0.70710678; }
        const double att = gainToDb (amp (side, 40, 48000) / std::sqrt (2.0));
        std::printf ("    nivel do lado a 40 Hz: %.1f dB\n", att);
        CHECK (att < -20, "mono-bass deixa passar lado a 40 Hz (%.1f dB)", att);
    }

    std::printf ("\nResultado: %d verificacoes ok, %d falhadas\n", passes, fails);
    return fails == 0 ? 0 : 1;
}
