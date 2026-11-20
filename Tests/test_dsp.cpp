// =============================================================================
//  BURNIX by PIRADEX — testes do DSP puro (sem JUCE). Imprime medicoes reais e falha (exit 1) se um
//  criterio de aceitacao nao for cumprido.  Compilar: cmake -DBX_BUILD_TESTS=ON ; ./test_dsp
// =============================================================================
#include "DSP/BurnixEngine.h"
#include "DSP/PostStages.h"
#include "../Tests/ref/TubeStage_v12.h"
#include <random>
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
    for (int slope = 0; slope < 4; ++slope)
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
            static const char* sn[] { "LR24", "LR48", "6 dB", "LR12" };
            std::printf ("    %s  %d bandas: desvio maximo %.4f dB\n", sn[slope], nb, worst);
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
    Harm byV[kClassicVoicings];
    for (int v = 0; v < kClassicVoicings; ++v)
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
    for (int v = 0; v < kClassicVoicings; ++v)
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

    // ---- 1.3: motor legado identico ao da 1.2, tabelas do ADAA, ganho compensado, curva progressiva
    std::printf ("\n[13a] Motor legado (sessoes da 1.2) contra o TubeStage original da 1.2.0, amostra a amostra\n");
    {
        std::mt19937 g (7); std::uniform_real_distribution<double> U (-1.2, 1.2);
        double worst = 0;
        struct Cfg { double drive, bias, density, transient, stage, inter; bool swap; };
        const Cfg cfgs[] { { 5, 0, 0, 0, 0, 0, false }, { 9, 0.4, 0.6, 0.5, 0, 0, false }, { 3, -0.3, 0, 0, 0.7, 6, false }, { 7, 0, 0.3, 0, 0.5, -4, true }, { 0.5, 0, 0, 0, 0, 0, false } };
        for (int v = 0; v < kClassicVoicings; ++v)
            for (const auto& c : cfgs)
            {
                TubeStage a; bx12::TubeStage r; a.prepare (192000); r.prepare (192000);
                TubeParams pa; bx12::TubeParams pr;
                pa.legacy = true; pa.drive = pr.drive = c.drive; pa.bias = pr.bias = c.bias; pa.density = pr.density = c.density;
                pa.transient = pr.transient = c.transient; pa.stageBlend = pr.stageBlend = c.stage; pa.interstageDb = pr.interstageDb = c.inter;
                pa.swapOrder = pr.swapOrder = c.swap; pa.weights = {}; pr.weights = {}; pa.weights[(size_t) v] = pr.weights[(size_t) v] = 1;
                for (int i = 0; i < 40000; ++i) { const double x = U (g) * (0.5 + 0.5 * std::sin (i * 0.001)); worst = std::max (worst, std::fabs (a.process (x, pa) - r.process (x, pr))); }
            }
        std::printf ("    6 voicings x 5 regulacoes x 40000 amostras: diferenca maxima %.3g\n", worst);
        CHECK (worst < 1e-12, "motor legado igual ao da 1.2 (diferenca %.3g)", worst);
    }
    std::printf ("\n[13b] Antiderivadas em tabela (ADAA) contra a forma fechada\n");
    {
        double w = 0;
        for (double u = -40; u <= 40; u += 0.0137)
        {
            w = std::max (w, std::fabs (curve::FemberT (u) - curve::Fember (u)));
            w = std::max (w, std::fabs (curve::FtriodeT (u) - curve::Ftriode (u)));
            w = std::max (w, std::fabs (curve::FvelvetT (u) - curve::Fvelvet (u)));
        }
        // PENTODE: a derivada numerica da tabela tem de dar a curva
        double wp = 0; for (double u = -30; u <= 30; u += 0.0213) wp = std::max (wp, std::fabs ((curve::Fpentode (u + 1e-4) - curve::Fpentode (u - 1e-4)) / 2e-4 - curve::pentode (u)));
        std::printf ("    erro maximo tanh/triode/velvet %.2g ; derivada do PENTODE %.2g\n", w, wp);
        CHECK (w < 1e-9 && wp < 1e-6, "tabelas do ADAA exatas (%.2g / %.2g)", w, wp);
    }
    std::printf ("\n[13c] Ganho compensado: nivel do fundamental de drive 1 a 10 (seno 1 kHz a -12 dBFS)\n");
    for (int v = 0; v < kClassicVoicings; ++v)
    {
        double lo = 1e9, hi = -1e9; std::printf ("    %-13s", voicingName (v));
        for (double d : { 1.0, 3.0, 5.0, 7.0, 10.0 })
        {
            TubeStage t; t.prepare (192000); TubeParams p; p.drive = d; p.weights = {}; p.weights[(size_t) v] = 1;
            const int warm = 96000, n = 192 * 200; std::vector<double> y ((size_t) n);
            for (int i = 0; i < warm + n; ++i) { const double o = t.process (0.2512 * std::sin (2 * kPi * 1000.0 * i / 192000.0), p); if (i >= warm) y[(size_t) (i - warm)] = o; }
            const double gdb = gainToDb (amp (y, 1000, 192000) / 0.2512); lo = std::min (lo, gdb); hi = std::max (hi, gdb);
            std::printf ("  d%-2.0f %+5.1f", d, gdb);
        }
        std::printf ("  dB\n");
        CHECK (hi - lo < 2.0 && std::fabs (hi) < 2.0 && std::fabs (lo) < 2.0, "%s: drive nao muda o volume mais de 2 dB (%.1f a %.1f)", voicingName (v), lo, hi);
    }
    std::printf ("\n[13d] Curva progressiva: THD no meio do botao (drive 5, -12 dBFS)\n");
    for (int v = 0; v < kClassicVoicings; ++v)
    {
        const auto h = measureVoicing (v, progressiveDrive (5.0), 0.2512), h10 = measureVoicing (v, progressiveDrive (10.0), 0.2512);
        std::printf ("    %-13s THD %5.2f %% no meio, %5.2f %% no maximo\n", voicingName (v), 100 * h.thd(), 100 * h10.thd());
        CHECK (h.thd() > 0.012 && h.thd() < h10.thd(), "%s: o meio do botao ja satura de forma audivel (%.2f %%)", voicingName (v), 100 * h.thd());
    }

    // ---- 2.0: HARMONIC DRAW, dinamica por banda, ataque por banda, A/B morph
    std::printf ("\n[20a] HARMONIC DRAW: um seno a fundo de escala sai com os harmonicos desenhados\n");
    {
        TubeStage t; t.prepare (384000); TubeParams p; p.drive = 10; p.levelComp = false; p.weights = {}; p.weights[Draw] = 1;
        const double a[8] { 0, 1, 0.1, 0.05, 0.0, 0.02, 0.0, 0.0 }; p.drawA = a; p.drawVersion = 1;
        const auto h = [&] { const int warm = 192000, n = 384 * 200; std::vector<double> y ((size_t) n);
            for (int i = 0; i < warm + n; ++i) { const double o = t.process (std::sin (2 * kPi * 1000.0 * i / 384000.0), p); if (i >= warm) y[(size_t) (i - warm)] = o; }
            Harm r; for (int k = 1; k <= 6; ++k) r.h[k] = amp (y, 1000.0 * k, 384000.0); return r; }();
        std::printf ("    desenhado H2 -20 dB, H3 -26 dB, H5 -34 dB ; medido H2 %.1f, H3 %.1f, H4 %.1f, H5 %.1f dB\n",
                     gainToDb (h.h[2] / h.h[1]), gainToDb (h.h[3] / h.h[1]), gainToDb (h.h[4] / h.h[1]), gainToDb (h.h[5] / h.h[1]));
        CHECK (std::fabs (gainToDb (h.h[2] / h.h[1]) + 20) < 2.5 && std::fabs (gainToDb (h.h[3] / h.h[1]) + 26) < 2.5 && std::fabs (gainToDb (h.h[5] / h.h[1]) + 34) < 2.5 && h.h[4] < h.h[5] * 0.1,
               "DRAW: os harmonicos saem ao nivel desenhado (+-2.5 dB) e o H4 desligado fica 20 dB abaixo");
    }
    std::printf ("\n[20b] Dinamica por banda: ataques de um sinal em rajadas\n");
    {
        auto crest = [] (double dyn)      // pico na rajada relativo ao pico na parte sustentada, em dB
        {
            BurnixEngine e; e.prepare (48000); EngineParams P; P.numBands = 1; P.band[0].driveM = 0; P.band[0].dyn = dyn; e.setParams (P);
            double pb = 0, ps = 0;
            for (int i = 0; i < 96000; ++i)
            {
                const int ph = i % 12000; const double env = ph < 600 ? 1.0 : 0.3, x = env * 0.5 * std::sin (2 * kPi * 220.0 * i / 48000.0);
                double wl, wr, dl, dr; e.process (x, x, wl, wr, dl, dr);
                if (i > 48000) { if (ph < 600) pb = std::max (pb, std::fabs (wl)); else if (ph > 6000) ps = std::max (ps, std::fabs (wl)); }
            }
            return gainToDb (pb / ps);
        };
        const double c0 = crest (0.0), cp = crest (0.8), cn = crest (-0.8);
        std::printf ("    ataque acima do sustain: DYN 0 %.1f dB, DYN +80 %% %.1f dB (mais punch), DYN -80 %% %.1f dB (mais sustain)\n", c0, cp, cn);
        CHECK (cp > c0 + 1.0 && cn < c0 - 1.0, "DYN + aumenta e DYN - reduz os ataques");
    }
    std::printf ("\n[20c] Ataque por banda: +12 dB de drive nos ataques\n");
    {
        auto addedOnAttack = [] (double atk)
        {
            TubeStage t; t.prepare (48000); TubeParams p; p.drive = 4; p.attackDb = atk; p.weights = {}; p.weights[TriodeBloom] = 1;
            double s = 0;
            for (int i = 0; i < 48000; ++i) { const double env = (i % 12000) < 300 ? 0.8 : 0.1, x = env * std::sin (2 * kPi * 200.0 * i / 48000.0), y = t.process (x, p); if ((i % 12000) < 300 && i > 12000) s += (y - x) * (y - x); }
            return s;
        };
        const double a0 = addedOnAttack (0), a1 = addedOnAttack (12);
        std::printf ("    energia acrescentada nos ataques: 0 dB -> %.3g ; +12 dB -> %.3g\n", a0, a1);
        CHECK (a1 > a0 * 1.5, "o ataque fica mais saturado com ATTACK +12 dB");
    }
    std::printf ("\n[20d] A/B morph: a meio, os parametros numericos ficam a meio\n");
    {
        EngineParams A, Bp; A.heat = 2; Bp.heat = 8; A.band[0].driveM = 1; Bp.band[0].driveM = 9; A.xo[0] = 100; Bp.xo[0] = 400;
        const auto m = lerpEngine (A, Bp, 0.5);
        std::printf ("    heat %.1f, drive %.1f, crossover %.0f Hz\n", m.heat, m.band[0].driveM, m.xo[0]);
        CHECK (std::fabs (m.heat - 5) < 1e-9 && std::fabs (m.band[0].driveM - 5) < 1e-9 && std::fabs (m.xo[0] - 200) < 1e-6, "A/B morph interpola (crossover em escala logaritmica)");
    }

    std::printf ("\nResultado: %d verificacoes ok, %d falhadas\n", passes, fails);
    return fails == 0 ? 0 : 1;
}
