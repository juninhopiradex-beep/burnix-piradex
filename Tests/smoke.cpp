// =============================================================================
//  BURNIX by PIRADEX — teste de integracao (JUCE, sem anfitriao). Processa audio real pelo AudioProcessor e
//  imprime MEDICOES REAIS: bypass null, delta null a drive 0, latencia reportada vs medida, aliasing 1x/4x/8x/16x,
//  THD por voicing, CPU por qualidade, sample rates 44.1–192 kHz, blocos de 1 a 4096, automacao, estado/presets,
//  ausencia de alocacoes na thread de audio e capturas do editor (PERFORM / SCULPT / ENGINEER / login).
// =============================================================================
#include "Plugin/PluginProcessor.h"
#include "GUI/PluginEditor.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>

static thread_local bool g_track = false; static thread_local long g_allocs = 0;
void* operator new (std::size_t n) { if (g_track) ++g_allocs; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (std::size_t n) { if (g_track) ++g_allocs; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

static int fails = 0, passes = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf ("FALHOU  "); } else { ++passes; std::printf ("ok      "); } std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)
#define INFO(...) do { std::printf ("INFO    "); std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)

static void setP (BurnixProcessor& p, const juce::String& id, float v) { auto* q = p.apvts.getParameter (id); jassert (q != nullptr); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
static std::unique_ptr<BurnixProcessor> make (double fs, int bs)
{
    auto p = std::make_unique<BurnixProcessor>(); p->setRateAndBufferSizeDetails (fs, bs); p->prepareToPlay (fs, bs); return p;
}
// processa x em L e xr (ou x) em R, blocos de bs; devolve L e R
static void run (BurnixProcessor& p, const std::vector<float>& x, int bs, std::vector<float>& outL, std::vector<float>* outR = nullptr, const std::vector<float>* xr = nullptr)
{
    outL.assign (x.size(), 0); if (outR) outR->assign (x.size(), 0); juce::MidiBuffer midi;
    juce::AudioBuffer<float> b (2, bs);
    for (size_t off = 0; off < x.size(); off += (size_t) bs)
    {
        const int n = (int) std::min<size_t> ((size_t) bs, x.size() - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { b.setSample (0, i, x[off + (size_t) i]); b.setSample (1, i, xr ? (*xr)[off + (size_t) i] : x[off + (size_t) i]); }
        p.processBlock (b, midi);
        for (int i = 0; i < n; ++i) { outL[off + (size_t) i] = b.getSample (0, i); if (outR) (*outR)[off + (size_t) i] = b.getSample (1, i); }
    }
}
static std::vector<float> sine (double f, double fs, size_t n, float a) { std::vector<float> v (n); for (size_t i = 0; i < n; ++i) v[i] = a * (float) std::sin (2 * M_PI * f * (double) i / fs); return v; }
static std::vector<float> noise (size_t n, float a, int seed = 1) { juce::Random r (seed); std::vector<float> v (n); for (auto& x : v) x = a * (2 * r.nextFloat() - 1); return v; }
static bool finite (const std::vector<float>& v) { for (float x : v) if (! std::isfinite (x)) return false; return true; }
static double amp (const std::vector<float>& y, size_t from, size_t n, double hz, double fs)
{ double c = 0, s = 0; for (size_t i = 0; i < n; ++i) { const double ph = 2 * M_PI * hz * (double) i / fs; c += y[from + i] * std::cos (ph); s += y[from + i] * std::sin (ph); } return 2 * std::sqrt (c * c + s * s) / (double) n; }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    std::printf ("BURNIX by PIRADEX — teste de integracao (medicoes reais)\n\n");

    // 1) BYPASS: saida = entrada atrasada exatamente pela latencia reportada (todas as qualidades)
    for (int q = 0; q < 4; ++q)
        for (int ph = 0; ph < 2; ++ph)
        {
            if (q == 0 && ph == 1) continue;
            auto p = make (48000, 512); setP (*p, "quality", (float) q); setP (*p, "osPhase", (float) ph); setP (*p, "bypass", 1);
            p->prepareToPlay (48000, 512);
            const auto x = noise (48000, 0.5f); std::vector<float> y; run (*p, x, 512, y);
            const int L = p->latency(); double worst = 0;
            for (size_t i = 24000; i < x.size(); ++i) worst = std::max (worst, (double) std::abs (y[i] - x[i - (size_t) L]));
            CHECK (worst == 0.0, "bypass %-9s %-9s: latencia %d amostras, diferenca maxima %.3g (null exato)", bxp::qualityChoices()[q].toRawUTF8(), ph ? "LINEAR" : "MIN-PHASE", L, worst);
        }

    // 2) DELTA a drive 0 (6 bandas, M+S): o BURNIX nao acrescenta nada -> silencio
    for (int q = 0; q < 4; ++q)
    {
        auto p = make (48000, 256); setP (*p, "quality", (float) q); setP (*p, "numBands", 6); setP (*p, "delta", 1);
        for (int b = 0; b < 6; ++b) { setP (*p, bxp::bandId (b, "driveM"), 0); setP (*p, bxp::bandId (b, "driveS"), 0); setP (*p, bxp::bandId (b, "route"), 3); }
        p->prepareToPlay (48000, 256);
        const auto x = noise (48000, 0.5f, 7); std::vector<float> y; run (*p, x, 256, y);
        double pk = 0; for (size_t i = 12000; i < y.size(); ++i) pk = std::max (pk, (double) std::abs (y[i]));
        CHECK (pk < 1e-5, "delta a drive 0 (%s): pico %.1f dBFS", bxp::qualityChoices()[q].toRawUTF8(), juce::Decibels::gainToDecibels (pk, -200.0));
    }

    // 3) LATENCIA reportada = medida (impulso, drive 0, mix 100): pico do caminho processado
    for (int q = 1; q < 4; ++q)
        for (int ph = 0; ph < 2; ++ph)
        {
            auto p = make (48000, 512); setP (*p, "quality", (float) q); setP (*p, "osPhase", (float) ph); setP (*p, "numBands", 1); setP (*p, "b1_driveM", 0);
            p->prepareToPlay (48000, 512);
            std::vector<float> x (8192, 0.f); x[2000] = 1.f; std::vector<float> y; run (*p, x, 512, y);
            int at = 0; float mx = 0; for (size_t i = 0; i < y.size(); ++i) if (std::abs (y[i]) > mx) { mx = std::abs (y[i]); at = (int) i - 2000; }
            if (ph == 1) CHECK (std::abs (at - p->latency()) <= 1, "latencia %s LINEAR: reportada %d, pico do impulso a %d", bxp::qualityChoices()[q].toRawUTF8(), p->latency(), at);
            else INFO ("latencia %s MIN-PHASE: reportada %d (atraso de grupo a baixa frequencia), pico do impulso a %d (fase minima)", bxp::qualityChoices()[q].toRawUTF8(), p->latency(), at);
        }

    // 4) ALIASING: seno 15 kHz -6 dBFS, FIRESTORM drive 9, 1 banda, 48 kHz. Todos os harmonicos > 24 kHz dobram:
    //    a energia fora do fundamental (20 Hz..20 kHz) e aliasing. Comparacao 1x / 4x / 8x / 16x.
    std::printf ("\n");
    double aliasDb[4] {};
    for (int q = 0; q < 4; ++q)
    {
        auto p = make (48000, 512); setP (*p, "quality", (float) q); setP (*p, "numBands", 1); setP (*p, "b1_voice", 6); setP (*p, "b1_driveM", 9);
        p->prepareToPlay (48000, 512);
        const auto x = sine (15000, 48000, 48000 * 2, 0.5f); std::vector<float> y; run (*p, x, 512, y);
        const size_t from = 48000, n = 48000; const double a1 = amp (y, from, n, 15000, 48000);
        double tot = 0; for (size_t i = 0; i < n; ++i) tot += (double) y[from + i] * y[from + i]; tot /= n;
        const double rest = std::max (1e-30, tot - a1 * a1 / 2);
        aliasDb[q] = 10 * std::log10 (rest / (a1 * a1 / 2));
        INFO ("aliasing %-9s: energia fora do fundamental = %.1f dB relativo ao fundamental", bxp::qualityChoices()[q].toRawUTF8(), aliasDb[q]);
    }
    CHECK (aliasDb[1] < aliasDb[0] - 10, "4x reduz o aliasing pelo menos 10 dB face a 1x (%.1f dB)", aliasDb[0] - aliasDb[1]);
    CHECK (aliasDb[3] <= aliasDb[1] + 0.5, "16x nao piora face a 4x (%.1f vs %.1f dB)", aliasDb[3], aliasDb[1]);

    // 5) THD por voicing (medicao interna, 192 kHz, seno 1 kHz -6 dBFS, 1 banda, drive 5)
    std::printf ("\n");
    for (int v = 0; v < 6; ++v)
    {
        bx::EngineParams e; e.numBands = 1; e.band[0].driveM = 5; e.band[0].voice = v;
        const auto r = BurnixProcessor::measureHarmonics (e);
        INFO ("THD %-13s drive 5: %6.2f %%   H2 %6.1f dB   H3 %6.1f dB   H5 %6.1f dB", bx::voicingName (v), r.thd * 100, r.db[2], r.db[3], r.db[5]);
        CHECK (r.valid && std::isfinite (r.thd), "medicao de harmonicos valida (%s)", bx::voicingName (v));
    }

    // 6) sample rates e tamanhos de bloco: saida finita, latencia coerente
    std::printf ("\n");
    for (double fs : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        for (int bs : { 1, 32, 480, 4096 })
        {
            auto p = make (fs, bs); setP (*p, "numBands", 6); setP (*p, "heat", 8);
            const auto x = noise ((size_t) (fs * 0.25), 0.7f, 3); std::vector<float> y, yr; run (*p, x, bs, y, &yr);
            CHECK (finite (y) && finite (yr), "fs %.1f kHz bloco %d: saida finita (latencia %d)", fs / 1000, bs, p->latency());
        }

    // 7) automacao rapida de todos os parametros (cada bloco valores aleatorios) + troca de qualidade em andamento
    {
        auto p = make (48000, 64); juce::Random r (11); juce::MidiBuffer m; juce::AudioBuffer<float> b (2, 64); bool ok = true; float mx = 0;
        for (int blk = 0; blk < 3000; ++blk)
        {
            for (auto* prm : p->getParameters()) if (r.nextFloat() < 0.05f) prm->setValueNotifyingHost (r.nextFloat());
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 64; ++i) b.setSample (c, i, 0.8f * (2 * r.nextFloat() - 1));
            p->processBlock (b, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 64; ++i) { ok = ok && std::isfinite (b.getSample (c, i)); mx = std::max (mx, std::abs (b.getSample (c, i))); }
        }
        CHECK (ok, "automacao aleatoria de todos os parametros (3000 blocos, input/output ate +24 dB): saida finita, pico %.1f dBFS", juce::Decibels::gainToDecibels (mx));
    }

    // 8) nenhuma alocacao de memoria na thread de audio
    {
        auto p = make (48000, 512); setP (*p, "numBands", 6); setP (*p, "quality", 3);
        p->prepareToPlay (48000, 512);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; const auto x = noise (512, 0.5f);
        for (int i = 0; i < 20; ++i) { for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, x.data(), 512); p->processBlock (b, m); }
        g_allocs = 0; g_track = true;
        for (int i = 0; i < 200; ++i) { for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, x.data(), 512); p->processBlock (b, m); }
        g_track = false;
        CHECK (g_allocs == 0, "alocacoes durante processBlock (200 blocos, ULTRA 16x, 6 bandas): %ld", g_allocs);
    }

    // 9) CPU por qualidade (6 bandas, 48 kHz, bloco 512, morph ativo) — percentagem do tempo real numa thread
    std::printf ("\n");
    for (int q = 0; q < 4; ++q)
    {
        auto p = make (48000, 512); setP (*p, "numBands", 6); setP (*p, "quality", (float) q); setP (*p, "morphX", 0.6f); setP (*p, "morphY", 0.6f);
        p->prepareToPlay (48000, 512);
        const auto x = noise (48000 * 4, 0.5f); std::vector<float> y;
        const auto t0 = std::chrono::steady_clock::now(); run (*p, x, 512, y);
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        INFO ("CPU %-9s 6 bandas: %.1f %% do tempo real (4 s de audio em %.3f s)", bxp::qualityChoices()[q].toRawUTF8(), 100 * s / 4.0, s);
    }

    // 10) estado e presets
    std::printf ("\n");
    {
        auto p = make (48000, 512); p->applyPreset (3); setP (*p, "b2_driveM", 6.6f);
        juce::MemoryBlock mb; p->getStateInformation (mb);
        auto p2 = make (48000, 512); p2->setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = true; for (int i = 0; i < p->getParameters().size(); ++i) same = same && std::abs (p->getParameters()[i]->getValue() - p2->getParameters()[i]->getValue()) < 1e-6f;
        CHECK (same, "estado guardado e reposto (preset %s + alteracao)", p->currentPresetName().toRawUTF8());
        bool allOk = true;
        for (int i = 0; i < p->getNumPrograms(); ++i)
        {
            p->applyPreset (i); p->prepareToPlay (48000, 512);
            const auto x = sine (220, 48000, 24000, 0.5f); std::vector<float> y; run (*p, x, 512, y);
            double pk = 0; for (auto v : y) pk = std::max (pk, (double) std::abs (v));
            INFO ("preset %-22s pico de saida %.1f dBFS (entrada -6 dBFS), latencia %d", p->getProgramName (i).toRawUTF8(), juce::Decibels::gainToDecibels (pk, -200.0), p->latency());
            allOk = allOk && finite (y) && pk < 8.0;
        }
        CHECK (allOk, "todos os %d presets de fabrica processam audio finito", p->getNumPrograms());
    }

    // 11) editor: capturas das tres vistas e do login
    {
        auto p = make (48000, 512); p->applyPreset (3);
        const auto x = noise (48000, 0.3f); std::vector<float> y; run (*p, x, 512, y);
        std::unique_ptr<BurnixEditor> ed (dynamic_cast<BurnixEditor*> (p->createEditor()));
        const char* names[] { "editor_perform.png", "editor_sculpt.png", "editor_engineer.png" };
        for (int v = 0; v < 3; ++v)
        {
            ed->setView (v); if (v == 1) ed->setBand (1);
            for (int k = 0; k < 6; ++k) { run (*p, x, 512, y); ed->pump(); }
            if (v == 2) { for (int k = 0; k < 60 && ! ed->orbit().report.valid; ++k) { juce::MessageManager::getInstance()->runDispatchLoopUntil (50); } }
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (names[v]); f.deleteFile();
            juce::FileOutputStream os (f); juce::PNGImageFormat png; const bool ok = png.writeImageToStream (img, os);
            CHECK (ok && img.getWidth() == 1200, "captura %s (%dx%d)", names[v], img.getWidth(), img.getHeight());
        }
        ed->setScale (0.75f); CHECK (ed->getWidth() == 900, "escala 75 %%: %dx%d", ed->getWidth(), ed->getHeight());
        ed->setScale (2.0f);  CHECK (ed->getWidth() == 2400, "escala 200 %%: %dx%d", ed->getWidth(), ed->getHeight());
        // esquemas de cores e idioma (capturas para aprovar)
        ed->setView (0);
        for (int sk = 0; sk < 5; ++sk)
        {
            ed->testSkin (sk); for (int k = 0; k < 3; ++k) { run (*p, x, 512, y); ed->pump(); }
            auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile ("skin_" + juce::String (sk) + ".png"); f.deleteFile();
            juce::FileOutputStream os (f); juce::PNGImageFormat png; CHECK (png.writeImageToStream (im, os), "captura do esquema de cores %d", sk);
        }
        ed->testSkin (0); ed->testLang (1); ed->setView (1); for (int k = 0; k < 3; ++k) { run (*p, x, 512, y); ed->pump(); }
        { auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f); juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile ("lang_en_sculpt.png"); f.deleteFile();
          juce::FileOutputStream os (f); juce::PNGImageFormat png; CHECK (png.writeImageToStream (im, os), "captura em ingles (SCULPT)"); }
        ed->testLang (0);
        // copiar / colar regulacoes entre instancias
        { auto p2 = make (48000, 512); CHECK (p2->loadStateFromText (p->stateAsText()) && std::abs (p2->apvts.getRawParameterValue ("b2_driveM")->load() - p->apvts.getRawParameterValue ("b2_driveM")->load()) < 1e-6f, "copiar/colar regulacoes entre instancias"); }
        ed->setScale (1.0f); ed->setView (2); ed->showLoginForTest();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile ("editor_login.png"); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        INFO ("captura editor_login.png");
    }

    std::printf ("\nResultado: %d verificacoes ok, %d falhadas\n", passes, fails);
    return fails == 0 ? 0 : 1;
}
