#include "PluginProcessor.h"
#include "Access.h"
#include "AppPaths.h"
#include "../GUI/PluginEditor.h"

using namespace bxp;

BurnixProcessor::BurnixProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BURNIX", createLayout())
{
    auto P = [this] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); jassert (p != nullptr); return p; };
    pInput = P ("input"); pHeat = P ("heat"); pMx = P ("morphX"); pMy = P ("morphY");
    pCorner[0] = P ("cornerTop"); pCorner[1] = P ("cornerRight"); pCorner[2] = P ("cornerBottom"); pCorner[3] = P ("cornerLeft");
    pDensity = P ("density"); pBias = P ("bias"); pStage = P ("stageBlend"); pTopo = P ("topology"); pInter = P ("interstage"); pTrans = P ("transient");
    pColor = P ("color"); pBody = P ("body"); pAir = P ("air"); pWidth = P ("width"); pMono = P ("monoBass"); pMonoF = P ("monoFreq");
    pMix = P ("mix"); pOut = P ("output"); pAuto = P ("autoGain"); pDelta = P ("delta"); pBypass = P ("bypass");
    pQuality = P ("quality"); pPhase = P ("osPhase"); pBands = P ("numBands"); pSlope = P ("slope");
    for (int k = 0; k < bx::kMaxX; ++k) pXo[k] = P ("xo" + juce::String (k + 1));
    pComp = P ("levelComp"); pCurve = P ("driveCurve"); pDrift = P ("drift"); pClip = P ("clip"); pRender = P ("renderQuality");
    apvts.state.setProperty ("engine", 13, nullptr);
    apvts.state.setProperty ("seed", (int) (juce::Random::getSystemRandom().nextInt() & 0x7fffffff), nullptr);
    AccessManager::get();          // inicializa ja aqui (thread de mensagens): nunca a primeira vez na thread de audio
    if (hasDefault()) { loadUserPreset (bxpaths::userDir().getChildFile ("default.burnix")); presetName = "Default"; undo.clearUndoHistory(); }
    for (int b = 0; b < bx::kMaxBands; ++b)
        bp[(size_t) b] = { P (bandId (b, "on")), P (bandId (b, "solo")), P (bandId (b, "route")), P (bandId (b, "driveM")), P (bandId (b, "driveS")),
                           P (bandId (b, "voice")), P (bandId (b, "filter")), P (bandId (b, "freq")), P (bandId (b, "q")), P (bandId (b, "mode")), P (bandId (b, "mix")) };
}

bool BurnixProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

bx::EngineParams BurnixProcessor::makeEngineParams() const
{
    bx::EngineParams e;
    auto ld = [] (const std::atomic<float>* p) { return (double) p->load(); };
    auto li = [] (const std::atomic<float>* p) { return (int) std::lround (p->load()); };
    e.numBands = juce::jlimit (1, bx::kMaxBands, li (pBands));
    for (int k = 0; k < bx::kMaxX; ++k) e.xo[k] = ld (pXo[k]);
    { const int sl = li (pSlope); e.slope = sl == 1 ? bx::Slope::LR48 : sl == 2 ? bx::Slope::LR6 : sl == 3 ? bx::Slope::LR12 : bx::Slope::LR24; }
    e.legacy = legacyEngine.load(); e.levelComp = pComp->load() > 0.5f; e.progressive = li (pCurve) == 0; e.drift = ld (pDrift) / 100.0;
    e.heat = ld (pHeat); e.morphX = ld (pMx); e.morphY = ld (pMy);
    for (int i = 0; i < 4; ++i) e.corner[(size_t) i] = li (pCorner[i]);
    e.bias = ld (pBias) / 100.0; e.density = ld (pDensity) / 100.0; e.transient = ld (pTrans) / 100.0;
    e.stageBlend = ld (pStage) / 100.0; e.interstageDb = ld (pInter); e.swapOrder = li (pTopo) == 1;
    for (int b = 0; b < bx::kMaxBands; ++b)
    {
        auto& d = e.band[(size_t) b]; const auto& s = bp[(size_t) b];
        d.on = s.on->load() > 0.5f; d.solo = s.solo->load() > 0.5f; d.route = (bx::Route) juce::jlimit (0, 3, li (s.route));
        d.driveM = ld (s.driveM); d.driveS = ld (s.driveS); d.voice = li (s.voice) - 1;          // 0 = MORPH -> -1
        d.filter = (bx::FilterType) juce::jlimit (0, 5, li (s.filter)); d.fHz = ld (s.freq); d.fQ = ld (s.q);
        d.mode = li (s.mode) == 1 ? bx::Mode::Parallel : bx::Mode::Insert; d.mix = ld (s.mix) / 100.0;
    }
    return e;
}

void BurnixProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; maxBlock = juce::jmax (16, samplesPerBlock);
    const int nch = 2;
    for (int q = 1; q < 4; ++q)
        for (int ph = 0; ph < 2; ++ph)
        {
            const auto ft = ph == 0 ? juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR : juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple;
            const int stages = q == 1 ? 2 : (q == 2 ? 3 : 4);
            for (auto* arr : { &osWet, &osDry })
            {
                auto& o = (*arr)[(size_t) q][(size_t) ph];
                o = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) nch, (size_t) stages, ft, true, true);
                o->initProcessing ((size_t) maxBlock);
            }
        }
    const auto seed = (unsigned int) (int) apvts.state.getProperty ("seed", 12345);
    for (int q = 0; q < 4; ++q) { engines[(size_t) q].prepare (sr * qualityFactor (q)); engines[(size_t) q].setSeed (seed); }
    deltaC = 1.0 - std::exp (-1.0 / (0.08 * sr)); deltaW[0] = deltaW[1] = deltaD[0] = deltaD[1] = 0;
    tone.prepare (sr); stereo.prepare (sr); loud.prepare (sr); tp.reset();
    work.setSize (2, maxBlock); wetBuf.setSize (2, maxBlock); dryBuf.setSize (2, maxBlock);
    rawDelay.setSize (2, 16384); rawDelay.clear(); rawPos = 0;
    inGain.reset (sr, 0.02); outGain.reset (sr, 0.02); mixS.reset (sr, 0.02); bypassMix.reset (sr, 0.01);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pInput->load()));
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    mixS.setCurrentAndTargetValue (pMix->load() / 100.f);
    bypassMix.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 1.f : 0.f);
    fade = 1.f; fadeState = 0;
    { const int ph = (int) std::lround (pPhase->load()) == 1 ? 1 : 0; switchConfig (effectiveQuality (juce::jlimit (0, 3, (int) std::lround (pQuality->load())), ph), ph); }
}

// qualidade de exportacao: ao renderizar (offline) pode subir para 8x/16x. So em MIN-PHASE e com oversampling ja ligado,
// porque ai a latencia reportada e a mesma (6 amostras): o bounce fica alinhado igual ao que se ouviu.
int BurnixProcessor::effectiveQuality (int wantQ, int phase) const
{
    if (! isNonRealtime() || phase != 0 || wantQ < 1) return wantQ;
    const int r = (int) std::lround (pRender->load());
    return juce::jmax (wantQ, r == 1 ? 2 : (r == 2 ? 3 : wantQ));
}

void BurnixProcessor::switchConfig (int q, int phase)
{
    activeQ = q; activePhase = phase;
    if (q > 0) { osWet[(size_t) q][(size_t) phase]->reset(); osDry[(size_t) q][(size_t) phase]->reset(); }
    engines[(size_t) q].reset();
    currentLatency = q > 0 ? (int) std::lround (osWet[(size_t) q][(size_t) phase]->getLatencyInSamples()) : 0;
    setLatencySamples (currentLatency);
}

void BurnixProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)  { processT (b); }
void BurnixProcessor::processBlock (juce::AudioBuffer<double>& b, juce::MidiBuffer&) { processT (b); }

template <typename T>
void BurnixProcessor::processT (juce::AudioBuffer<T>& buffer)
{
    juce::ScopedNoDenormals nd;
    const auto t0 = juce::Time::getHighResolutionTicks();
    const int numCh = juce::jmin (2, buffer.getNumChannels()), total = buffer.getNumSamples();
    if (numCh == 0 || total == 0) return;
    float inTmp[2][512], outTmp[2][512];
    for (int pos = 0; pos < total; )
    {
        const int n = juce::jmin (total - pos, maxBlock, 512);
        for (int c = 0; c < numCh; ++c) { auto* s = buffer.getReadPointer (c, pos); for (int i = 0; i < n; ++i) inTmp[c][i] = (float) s[i]; }
        const float* in[2] { inTmp[0], inTmp[numCh > 1 ? 1 : 0] }; float* out[2] { outTmp[0], outTmp[1] };
        processChunk (in, out, numCh, n);
        for (int c = 0; c < numCh; ++c) { auto* d = buffer.getWritePointer (c, pos); for (int i = 0; i < n; ++i) d[i] = (T) outTmp[c][i]; }
        pos += n;
    }
    const double secs = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0), block = total / sr;
    meters.cpu.store ((float) (100.0 * secs / juce::jmax (1e-9, block)));
}

void BurnixProcessor::processChunk (const float* const* in, float* const* out, int numCh, int n)
{
    const bool unlocked = AccessManager::get().isUnlocked();
    // ---- qualidade: trocar com fade curto (sem cliques) e reportar a nova latencia
    const int wantPh = (int) std::lround (pPhase->load()) == 1 ? 1 : 0;
    const int wantQ = effectiveQuality (juce::jlimit (0, 3, (int) std::lround (pQuality->load())), wantPh);
    const bool needSwitch = wantQ != activeQ || (wantQ > 0 && wantPh != activePhase);
    if (needSwitch && isNonRealtime()) { switchConfig (wantQ, wantPh); fadeState = 0; fade = 1.f; }   // a exportar: troca ja, sem fade
    else if (fadeState == 0 && needSwitch) { pendingQ = wantQ; pendingPhase = wantPh; fadeState = 1; }

    if (loudReset.exchange (false)) loud.reset();
    auto& eng = engines[(size_t) activeQ];
    eng.setParams (makeEngineParams());
    tone.set (pColor->load(), pBody->load() / 100.0, pAir->load() / 100.0);
    stereo.set (pWidth->load() / 100.0, pMono->load() > 0.5f, pMonoF->load());
    inGain.setTargetValue (juce::Decibels::decibelsToGain (pInput->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    mixS.setTargetValue (pMix->load() / 100.f);
    bypassMix.setTargetValue ((pBypass->load() > 0.5f || ! unlocked) ? 1.f : 0.f);
    const bool delta = pDelta->load() > 0.5f, autoG = pAuto->load() > 0.5f, clip = pClip->load() > 0.5f, legacy = legacyEngine.load();

    // ---- entrada (protecao numerica: NaN/Inf -> 0, nao e limitador) + medidores de entrada
    float ip = 0; double is = 0;
    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        for (int c = 0; c < 2; ++c)
        {
            const float x = (float) bx::sanitize (in[c][i]);
            ip = juce::jmax (ip, std::abs (x)); is += (double) x * x;
            work.setSample (c, i, x * g);
            rawDelay.setSample (c, (rawPos + i) & 16383, x);
        }
    }
    meters.inPeak.store (juce::Decibels::gainToDecibels (ip, -120.f));
    meters.inRms.store ((float) juce::Decibels::gainToDecibels (std::sqrt (is / (2.0 * n)), -120.0));

    // ---- motor (sobreamostrado). wet e dry saem com a mesma latencia e a mesma fase
    if (activeQ == 0)
    {
        for (int i = 0; i < n; ++i)
        {
            double wl, wr, dl, dr; eng.process (work.getSample (0, i), work.getSample (1, i), wl, wr, dl, dr);
            wetBuf.setSample (0, i, (float) wl); wetBuf.setSample (1, i, (float) wr); dryBuf.setSample (0, i, (float) dl); dryBuf.setSample (1, i, (float) dr);
        }
    }
    else
    {
        auto& ow = *osWet[(size_t) activeQ][(size_t) activePhase]; auto& od = *osDry[(size_t) activeQ][(size_t) activePhase];
        juce::dsp::AudioBlock<float> wb (work); auto sub = wb.getSubBlock (0, (size_t) n);
        auto up = ow.processSamplesUp (sub); auto upd = od.processSamplesUp (sub);
        const int N = (int) up.getNumSamples();
        float* ul = up.getChannelPointer (0); float* ur = up.getChannelPointer (1);
        float* vl = upd.getChannelPointer (0); float* vr = upd.getChannelPointer (1);
        for (int j = 0; j < N; ++j)
        {
            double wl, wr, dl, dr; eng.process (ul[j], ur[j], wl, wr, dl, dr);
            ul[j] = (float) wl; ur[j] = (float) wr; vl[j] = (float) dl; vr[j] = (float) dr;
        }
        juce::dsp::AudioBlock<float> wo (wetBuf), dO (dryBuf);
        auto wos = wo.getSubBlock (0, (size_t) n); auto dos = dO.getSubBlock (0, (size_t) n);
        ow.processSamplesDown (wos); od.processSamplesDown (dos);
    }
    eng.publishActivity();

    // ---- pos: cor, mistura, estereo, ganho automatico, saida, bypass/licenca, medidores
    const float fadeStep = (float) (1.0 / (0.006 * sr));
    float op = 0, tpk = 0; double os2 = 0, hs2 = 0; bool doSwitch = false;
    const int lat = currentLatency;
    for (int i = 0; i < n; ++i)
    {
        double wl = tone.process (0, wetBuf.getSample (0, i)), wr = tone.process (1, wetBuf.getSample (1, i));
        const double dl = dryBuf.getSample (0, i), dr = dryBuf.getSample (1, i);
        const double m = mixS.getNextValue();
        double ol, orr;
        // o que o BURNIX acrescenta: molhado menos o seco com o mesmo ganho (minimos quadrados, ~80 ms).
        // Alimenta o medidor HARM e o DELTA puro (sem a diferenca de volume).
        deltaW[0] += deltaC * (wl * dl - deltaW[0]); deltaD[0] += deltaC * (dl * dl - deltaD[0]);
        deltaW[1] += deltaC * (wr * dr - deltaW[1]); deltaD[1] += deltaC * (dr * dr - deltaD[1]);
        const double gl = deltaD[0] > 1e-12 ? juce::jlimit (0.0, 8.0, deltaW[0] / deltaD[0]) : 1.0;
        const double gr = deltaD[1] > 1e-12 ? juce::jlimit (0.0, 8.0, deltaW[1] / deltaD[1]) : 1.0;
        const double hl = m * (wl - gl * dl), hr = m * (wr - gr * dr); hs2 += hl * hl + hr * hr;
        if (delta)
        {
            if (legacy) { ol = m * (wl - dl); orr = m * (wr - dr); }
            else        { ol = hl; orr = hr; }
            loud.next (0, 0, false);
        }
        else
        {
            ol = dl + m * (wl - dl); orr = dr + m * (wr - dr);
            stereo.process (ol, orr);
            const double g = loud.next (0.5 * (dl * dl + dr * dr), 0.5 * (ol * ol + orr * orr), autoG);
            if (autoG) { ol *= g; orr *= g; }
        }
        const double og = outGain.getNextValue();
        ol = bx::sanitize (ol * og * fade); orr = bx::sanitize (orr * og * fade);
        if (clip) { ol = bx::safetyClip (ol); orr = bx::safetyClip (orr); }

        if (fadeState == 1) { fade -= fadeStep; if (fade <= 0.f) { fade = 0.f; doSwitch = true; } }
        else if (fadeState == 2) { fade += fadeStep; if (fade >= 1.f) { fade = 1.f; fadeState = 0; } }

        const int rp = (rawPos + i - lat) & 16383;
        const double rl = rawDelay.getSample (0, rp), rr = rawDelay.getSample (1, rp);
        const float bmx = bypassMix.getNextValue();
        if (bmx >= 1.f)      { ol = rl; orr = rr; }               // bypass/bloqueado: entrada intacta, alinhada pela latencia
        else if (bmx > 0.f)  { ol += bmx * (rl - ol); orr += bmx * (rr - orr); }

        out[0][i] = (float) ol; if (numCh > 1) out[1][i] = (float) orr;
        op = juce::jmax (op, (float) std::abs (ol), (float) std::abs (orr)); os2 += ol * ol + orr * orr;
        tpk = juce::jmax (tpk, (float) tp.push (0, ol), (float) tp.push (1, orr));
        const int w = specWrite.load (std::memory_order_relaxed);
        specRing[(size_t) (w & 8191)] = (float) (0.5 * (ol + orr)); specWrite.store (w + 1, std::memory_order_release);
    }
    rawPos = (rawPos + n) & 16383;
    meters.outPeak.store (juce::Decibels::gainToDecibels (op, -120.f));
    meters.outRms.store ((float) juce::Decibels::gainToDecibels (std::sqrt (os2 / (2.0 * n)), -120.0));
    meters.outTp.store (juce::Decibels::gainToDecibels (tpk, -120.f));
    meters.harmRms.store (unlocked && pBypass->load() < 0.5f ? (float) juce::Decibels::gainToDecibels (std::sqrt (hs2 / (2.0 * n)), -120.0) : -120.f);
    meters.autoGainDb.store (autoG ? (float) loud.currentDb() : 0.f);
    if (doSwitch) { switchConfig (pendingQ, pendingPhase); fadeState = 2; }
}

bool BurnixProcessor::popSpectrum (float* dest, int n)
{
    n = juce::jmin (n, 4096);
    const int w = specWrite.load (std::memory_order_acquire);
    if (w < n) return false;
    for (int i = 0; i < n; ++i) dest[i] = specRing[(size_t) ((w - n + i) & 8191)];
    return true;
}

BurnixProcessor::HarmReport BurnixProcessor::measureHarmonics (const bx::EngineParams& p, double levelDb)
{
    HarmReport r; const double fs = 192000, f0 = 1000, a = std::pow (10.0, levelDb / 20.0);
    bx::BurnixEngine e; e.prepare (fs); e.setParams (p);
    const int warm = (int) (fs * 0.5), n = (int) (fs / f0) * 250;
    std::vector<double> y ((size_t) n);
    for (int i = 0; i < warm + n; ++i)
    {
        const double x = a * std::sin (2 * bx::kPi * f0 * i / fs); double wl, wr, dl, dr; e.process (x, x, wl, wr, dl, dr);
        if (i >= warm) y[(size_t) (i - warm)] = wl;
    }
    auto amp = [&] (double hz) { double c = 0, s = 0; for (int i = 0; i < n; ++i) { const double ph = 2 * bx::kPi * hz * i / fs; c += y[(size_t) i] * std::cos (ph); s += y[(size_t) i] * std::sin (ph); } return 2 * std::sqrt (c * c + s * s) / n; };
    r.h1 = amp (f0); double sum = 0;
    for (int k = 2; k <= 10; ++k) { const double h = amp (f0 * k); sum += h * h; r.db[(size_t) k] = bx::gainToDb (h / std::max (r.h1, 1e-12)); }
    r.db[1] = 0; r.thd = std::sqrt (sum) / std::max (r.h1, 1e-12); r.valid = true;
    return r;
}

// ------------------------------------------------------------------------------------------------ presets e estado
void BurnixProcessor::applyPreset (int index)
{
    const auto& list = factoryPresets(); if (index < 0 || index >= (int) list.size()) return;
    const auto& pr = list[(size_t) index];
    undo.beginNewTransaction ("Preset " + juce::String (pr.name));
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm)) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : pr.values)
        if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    currentProgram = index; presetName = pr.name;
    apvts.state.setProperty ("engine", 13, nullptr); legacyEngine = false; loudReset = true;   // presets de fabrica: sempre motor 1.3
}

// Uma sessao (ou preset do utilizador, ou A/B, ou colar) gravada na 1.2 nao tem a propriedade "engine": abre com o motor
// antigo e com os novos parametros na posicao que soa igual (sem compensacao, curva classica, sem drift nem clip).
void BurnixProcessor::adoptState (juce::ValueTree st)
{
    const bool old = ! st.hasProperty ("engine") || (int) st.getProperty ("engine") < 13;
    if (! st.hasProperty ("seed")) st.setProperty ("seed", apvts.state.getProperty ("seed", 12345), nullptr);
    if (old)
    {
        auto setIfMissing = [&st] (const juce::String& id, float v)
        {
            for (auto c : st) if (c.hasType ("PARAM") && c.getProperty ("id").toString() == id) return;
            juce::ValueTree c ("PARAM"); c.setProperty ("id", id, nullptr); c.setProperty ("value", v, nullptr); st.appendChild (c, nullptr);
        };
        setIfMissing ("levelComp", 0.f); setIfMissing ("driveCurve", 1.f); setIfMissing ("drift", 0.f); setIfMissing ("clip", 0.f); setIfMissing ("renderQuality", 0.f);
        st.setProperty ("engine", 12, nullptr);
    }
    legacyEngine = old; loudReset = true;
    apvts.replaceState (st);
}

void BurnixProcessor::upgradeEngine()
{
    if (! legacyEngine.load()) return;
    undo.beginNewTransaction ("Motor 1.3");
    apvts.state.setProperty ("engine", 13, nullptr); legacyEngine = false;
    if (auto* p = apvts.getParameter ("levelComp")) p->setValueNotifyingHost (1.f);
    if (auto* p = apvts.getParameter ("driveCurve")) p->setValueNotifyingHost (0.f);
}
void BurnixProcessor::setCurrentProgram (int i) { applyPreset (i); }
const juce::String BurnixProcessor::getProgramName (int i)
{
    const auto& l = factoryPresets(); return (i >= 0 && i < (int) l.size()) ? juce::String (l[(size_t) i].name) : juce::String();
}

juce::Array<juce::File> BurnixProcessor::userPresets() const
{
    auto a = bxpaths::userPresetDir().findChildFiles (juce::File::findFiles, false, "*.burnix");
    a.sort(); return a;
}
bool BurnixProcessor::saveUserPreset (const juce::String& name)
{
    auto st = apvts.copyState(); st.setProperty ("presetName", name, nullptr);
    if (auto xml = st.createXml()) { presetName = name; return xml->writeTo (bxpaths::userPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".burnix")); }
    return false;
}
bool BurnixProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            undo.beginNewTransaction ("Preset " + f.getFileNameWithoutExtension());
            adoptState (juce::ValueTree::fromXml (*xml)); presetName = f.getFileNameWithoutExtension(); return true;
        }
    return false;
}
juce::String BurnixProcessor::stateAsText()
{
    auto st = apvts.copyState(); st.setProperty ("presetName", presetName, nullptr);
    if (auto xml = st.createXml()) return xml->toString();
    return {};
}
bool BurnixProcessor::loadStateFromText (const juce::String& text)
{
    if (auto xml = juce::XmlDocument::parse (text))
        if (xml->hasTagName (apvts.state.getType()))
        {
            undo.beginNewTransaction ("Colar");
            auto st = juce::ValueTree::fromXml (*xml); presetName = st.getProperty ("presetName", presetName).toString();
            adoptState (st); return true;
        }
    return false;
}
bool BurnixProcessor::saveAsDefault()
{
    auto st = apvts.copyState(); st.setProperty ("presetName", "Default", nullptr);
    if (auto xml = st.createXml()) return xml->writeTo (bxpaths::userDir().getChildFile ("default.burnix"));
    return false;
}
bool BurnixProcessor::clearDefault() { return bxpaths::userDir().getChildFile ("default.burnix").deleteFile(); }
bool BurnixProcessor::hasDefault() const { return bxpaths::userDir().getChildFile ("default.burnix").existsAsFile(); }

void BurnixProcessor::storeAB (int slot) { abState[juce::jlimit (0, 1, slot)] = apvts.copyState(); }
void BurnixProcessor::copyToOther() { abState[1 - ab] = apvts.copyState(); }
void BurnixProcessor::loadPresetIntoB (int index)
{
    if (ab == 0) { abState[0] = apvts.copyState(); ab = 1; }
    applyPreset (index);
}
int BurnixProcessor::currentFactoryIndex() const
{
    const auto& l = factoryPresets();
    return (currentProgram >= 0 && currentProgram < (int) l.size() && presetName == juce::String (l[(size_t) currentProgram].name)) ? currentProgram : -1;
}
juce::String BurnixProcessor::presetDescription() const
{
    const int i = currentFactoryIndex(); if (i < 0) return {};
    const auto& pr = factoryPresets()[(size_t) i];
    return juce::String (juce::CharPointer_UTF8 (AccessManager::get().language() == 1 ? pr.purposeEn : pr.purpose));
}
void BurnixProcessor::toggleAB()
{
    abState[ab] = apvts.copyState(); ab = 1 - ab;
    if (abState[ab].isValid()) adoptState (abState[ab].createCopy());
}

void BurnixProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("uiView", uiView, nullptr); st.setProperty ("uiBand", uiBand, nullptr); st.setProperty ("uiScale", uiScale, nullptr);
    st.setProperty ("presetName", presetName, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}
void BurnixProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto st = juce::ValueTree::fromXml (*xml);
            uiView = st.getProperty ("uiView", 0); uiBand = st.getProperty ("uiBand", 1); uiScale = (float) (double) st.getProperty ("uiScale", 1.0);
            presetName = st.getProperty ("presetName", "Init").toString();
            adoptState (st);
        }
}

juce::AudioProcessorEditor* BurnixProcessor::createEditor() { return new BurnixEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BurnixProcessor(); }
