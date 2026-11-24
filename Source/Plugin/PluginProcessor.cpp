#include "PluginProcessor.h"
#include <map>
#include "Access.h"
#include "AppPaths.h"
#include "../GUI/PluginEditor.h"

using namespace bxp;

BurnixProcessor::BurnixProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "BURNIX", createLayout())
{
    auto P = [this] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); jassert (p != nullptr); return p; };
    pInput = P ("input"); pHeat = P ("heat"); pMx = P ("morphX"); pMy = P ("morphY");
    pCorner[0] = P ("cornerTop"); pCorner[1] = P ("cornerRight"); pCorner[2] = P ("cornerBottom"); pCorner[3] = P ("cornerLeft");
    pDensity = P ("density"); pBias = P ("bias"); pStage = P ("stageBlend"); pTopo = P ("topology"); pInter = P ("interstage"); pTrans = P ("transient");
    pColor = P ("color"); pBody = P ("body"); pAir = P ("air"); pWidth = P ("width"); pMono = P ("monoBass"); pMonoF = P ("monoFreq");
    pMix = P ("mix"); pOut = P ("output"); pAuto = P ("autoGain"); pDelta = P ("delta"); pBypass = P ("bypass");
    pQuality = P ("quality"); pPhase = P ("osPhase"); pBands = P ("numBands"); pSlope = P ("slope"); pXoLin = P ("xoLinear");
    for (int k = 0; k < bx::kMaxX; ++k) pXo[k] = P ("xo" + juce::String (k + 1));
    pComp = P ("levelComp"); pCurve = P ("driveCurve"); pDrift = P ("drift"); pClip = P ("clip"); pRender = P ("renderQuality");
    for (int k = 0; k < 6; ++k) pH[k] = P ("h" + juce::String (k + 2));
    for (int m = 0; m < 3; ++m) { pModSrc[m] = P ("mod" + juce::String (m + 1) + "Src"); pModDst[m] = P ("mod" + juce::String (m + 1) + "Dst"); pModAmt[m] = P ("mod" + juce::String (m + 1) + "Amt"); }
    pLfoRate = P ("lfoRate"); pLfoShape = P ("lfoShape"); pEnvA = P ("envAttack"); pEnvR = P ("envRelease"); pAbMorph = P ("abMorph");
    for (int b = 0; b < bx::kMaxBands; ++b) bp2[(size_t) b] = { P (bandId (b, "dyn")), P (bandId (b, "attack")) };
    linkIndex = BurnixLink::claim();
    apvts.state.setProperty ("engine", 13, nullptr);
    apvts.state.setProperty ("seed", (int) (juce::Random::getSystemRandom().nextInt() & 0x7fffffff), nullptr);
    AccessManager::get();          // inicializa ja aqui (thread de mensagens): nunca a primeira vez na thread de audio
    if (hasDefault()) { loadUserPreset (bxpaths::userDir().getChildFile ("default.burnix")); presetName = "Default"; undo.clearUndoHistory(); }
    for (int b = 0; b < bx::kMaxBands; ++b)
        bp[(size_t) b] = { P (bandId (b, "on")), P (bandId (b, "solo")), P (bandId (b, "route")), P (bandId (b, "driveM")), P (bandId (b, "driveS")),
                           P (bandId (b, "voice")), P (bandId (b, "filter")), P (bandId (b, "freq")), P (bandId (b, "q")), P (bandId (b, "mode")), P (bandId (b, "mix")) };
}

BurnixProcessor::~BurnixProcessor() { BurnixLink::release (linkIndex); }

bool BurnixProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out) return false;
    if (! (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo())) return false;
    }
    return true;
}

void BurnixProcessor::updateTrackProperties (const TrackProperties& t)
{
    trackNm = t.name.value_or (juce::String());
    if (linkIndex >= 0) { auto& e = BurnixLink::entries()[(size_t) linkIndex]; trackNm.copyToUTF8 (e.name, sizeof (e.name)); ++e.nameSeq; }
}

bx::EngineParams BurnixProcessor::makeEngineParams() const
{
    bx::EngineParams e;
    auto ld = [] (const std::atomic<float>* p) { return (double) p->load(); };
    auto li = [] (const std::atomic<float>* p) { return (int) std::lround (p->load()); };
    e.numBands = juce::jlimit (1, bx::kMaxBands, li (pBands));
    for (int k = 0; k < bx::kMaxX; ++k) e.xo[k] = ld (pXo[k]);
    { const int sl = li (pSlope); e.slope = sl == 1 ? bx::Slope::LR48 : sl == 2 ? bx::Slope::LR6 : sl == 3 ? bx::Slope::LR12 : bx::Slope::LR24; }
    if (pXoLin->load() > 0.5f && ! legacyEngine.load()) e.slope = bx::Slope::Linear;   // 2.1: crossovers de fase linear
    e.legacy = legacyEngine.load(); e.levelComp = pComp->load() > 0.5f; e.progressive = li (pCurve) == 0; e.drift = ld (pDrift) / 100.0;
    {   // HARMONIC DRAW: H2..H7 em dB -> amplitudes; a versao so muda quando o desenho muda
        bool changed = false;
        for (int k = 0; k < 6; ++k) { const float v = pH[k]->load(); if (v != lastDrawDb[(size_t) k]) { lastDrawDb[(size_t) k] = v; changed = true; } }
        if (changed) { drawCache[0] = 0; drawCache[1] = 1; for (int k = 0; k < 6; ++k) drawCache[(size_t) k + 2] = lastDrawDb[(size_t) k] <= -79.5f ? 0.0 : std::pow (10.0, lastDrawDb[(size_t) k] / 20.0); ++drawVer; }
        e.draw = drawCache; e.drawVersion = drawVer;
    }
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
        d.dyn = ld (bp2[(size_t) b].dyn) / 100.0; d.attackDb = ld (bp2[(size_t) b].attack);
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
    for (auto& f : phoneF) { f[0].set (bx::design::highpass (300, 0.9, sr)); f[1].set (bx::design::highpass (300, 0.9, sr)); f[2].set (bx::design::lowpass (7000, 0.7, sr)); f[3].set (bx::design::highShelf (2500, 0.7, 3.0, sr)); for (auto& b : f) b.reset(); }
    for (int c = 0; c < 2; ++c) { abLo[c][0].set (bx::design::lowpass (100, 0.7071, sr)); abLo[c][1].set (bx::design::lowpass (100, 0.7071, sr));
                                  abHi[c][0].set (bx::design::highpass (150, 0.7071, sr)); abHi[c][1].set (bx::design::lowpass (800, 0.7071, sr)); for (auto& b : abLo[c]) b.reset(); for (auto& b : abHi[c]) b.reset(); }
    for (auto& v : abE) v = 1e-9;
    analyzer.prepare (sr); envV = scV = 0; lfoPh = 0;
    tone.prepare (sr); stereo.prepare (sr); loud.prepare (sr); tp.reset();
    lufsMeter.prepare (sr); tpMaxV = -120.f;
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
    if (linearXo()) return wantQ;          // fase linear: a latencia dos crossovers muda com a qualidade, por isso o bounce usa a mesma
    const int r = (int) std::lround (pRender->load());
    return juce::jmax (wantQ, r == 1 ? 2 : (r == 2 ? 3 : wantQ));
}

void BurnixProcessor::switchConfig (int q, int phase)
{
    activeQ = q; activePhase = phase;
    if (q > 0) { osWet[(size_t) q][(size_t) phase]->reset(); osDry[(size_t) q][(size_t) phase]->reset(); }
    engines[(size_t) q].reset();
    currentLatency = latencyFor (q, phase, linearXo());
    setLatencySamples (currentLatency);
}

bool BurnixProcessor::linearXo() const { return pXoLin->load() > 0.5f && ! legacyEngine.load(); }

// latencia total a taxa do anfitriao: oversampling + crossovers de fase linear (B + D a taxa do motor, sempre multiplo de 16)
int BurnixProcessor::latencyFor (int q, int phase, bool linear) const
{
    int lat = q > 0 ? (int) std::lround (osWet[(size_t) q][(size_t) phase]->getLatencyInSamples()) : 0;
    if (linear) lat += engines[(size_t) q].linearLatency() / qualityFactor (q);
    return lat;
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
    const bool scOn = getBusCount (true) > 1 && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled() && getBus (true, 1)->getNumberOfChannels() > 0;
    auto scBuf = scOn ? getBusBuffer (buffer, true, 1) : juce::AudioBuffer<T>();
    scPresent = scOn && scBuf.getNumChannels() > 0; modLive.scConnected = scPresent;
    for (int pos = 0; pos < total; )
    {
        const int n = juce::jmin (total - pos, maxBlock, 512);
        for (int c = 0; c < numCh; ++c) { auto* s = buffer.getReadPointer (c, pos); for (int i = 0; i < n; ++i) inTmp[c][i] = (float) s[i]; }
        if (scPresent) for (int c = 0; c < 2; ++c) { auto* s = scBuf.getReadPointer (juce::jmin (c, scBuf.getNumChannels() - 1), pos); for (int i = 0; i < n; ++i) scTmp[c][i] = (float) s[i]; }
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
    auto base = makeEngineParams();
    double color = pColor->load(), body = pBody->load() / 100.0, air = pAir->load() / 100.0, width = pWidth->load() / 100.0;
    double mixV = pMix->load() / 100.0, outDb = pOut->load(), inDb = pInput->load();
    // A/B MORPH (2.0): o som atual vai ate ao outro lado do A/B
    {
        const double t = pAbMorph->load() / 100.0; const auto& mt = morphT[(size_t) morphIdx.load()];
        if (t > 0.0 && mt.valid)
        {
            base = bx::lerpEngine (base, mt.e, t);
            auto lp = [t] (double a, double b) { return a + t * (b - a); };
            color = lp (color, mt.color); body = lp (body, mt.body); air = lp (air, mt.air); width = lp (width, mt.width); mixV = lp (mixV, mt.mix); outDb = lp (outDb, mt.out); inDb = lp (inDb, mt.in);
        }
    }
    // MODULACAO (2.0): ENV (entrada), LFO, SIDECHAIN -> HEAT, MORPH X/Y, MIX, DENSITY (3 ligacoes)
    struct Slot { int src = 0, dst = 0; double amt = 0; } slots[3]; bool anyMod = false, mixMod = false;
    for (int m = 0; m < 3; ++m)
    {
        slots[m] = { (int) std::lround (pModSrc[m]->load()), (int) std::lround (pModDst[m]->load()), pModAmt[m]->load() / 100.0 };
        if (slots[m].src > 0 && slots[m].amt != 0.0) { anyMod = true; mixMod = mixMod || slots[m].dst == 3; }
    }
    eng.setParams (base);
    {   // 2.1: ligar/desligar a fase linear muda a latencia -> avisar o anfitriao
        const int want = latencyFor (activeQ, activePhase, base.slope == bx::Slope::Linear);
        if (want != currentLatency) { currentLatency = want; setLatencySamples (want); eng.reset(); }
    }
    tone.set (color, body, air);
    stereo.set (width, pMono->load() > 0.5f, pMonoF->load());
    inGain.setTargetValue (juce::Decibels::decibelsToGain ((float) inDb));
    outGain.setTargetValue (juce::Decibels::decibelsToGain ((float) outDb));
    mixS.setTargetValue ((float) mixV);
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
    // fontes de modulacao por amostra; o analisador (assistente / match) ouve a entrada crua
    float modV[3][512];      // ENV, LFO, SC (0..1, -1..1, 0..1)
    {
        const double ea = bx::envCoeff (pEnvA->load(), sr), er = bx::envCoeff (pEnvR->load(), sr);
        const double rate = pLfoRate->load() / sr; const int shape = (int) std::lround (pLfoShape->load()); const double rs = bx::envCoeff (5.0, sr);
        auto norm = [] (double e) { return (float) juce::jlimit (0.0, 1.0, (bx::gainToDb (e) + 48.0) / 48.0); };
        const int st = analysisState.load();
        if (st == 3) { analyzer.reset(); analysisState = 1; }
        for (int i = 0; i < n; ++i)
        {
            const double a = 0.5 * std::fabs (work.getSample (0, i) + work.getSample (1, i)); envV += (a > envV ? ea : er) * (a - envV);
            const double s = scPresent ? 0.5 * std::fabs (scTmp[0][i] + scTmp[1][i]) : 0.0; scV += (s > scV ? ea : er) * (s - scV);
            lfoPh += rate; if (lfoPh >= 1.0) { lfoPh -= 1.0; rndState = rndState * 1664525u + 1013904223u; lfoRndT = (rndState >> 8) / 8388608.0 - 1.0; }
            lfoRnd += rs * (lfoRndT - lfoRnd);
            const double lv = shape == 0 ? std::sin (2.0 * bx::kPi * lfoPh) : shape == 1 ? 4.0 * std::fabs (lfoPh - 0.5) - 1.0 : shape == 2 ? (lfoPh < 0.5 ? 1.0 : -1.0) : lfoRnd;
            modV[0][i] = norm (envV); modV[1][i] = (float) lv; modV[2][i] = norm (scV);
            if (analysisState.load() == 1) analyzer.push (in[0][i], in[1][i]);
        }
        modLive.env = modV[0][n - 1]; modLive.lfo = modV[1][n - 1]; modLive.sc = modV[2][n - 1];
        if (analysisState.load() == 1) { anaProgress = (float) (analyzer.seconds() / analysisSecs); if (analyzer.seconds() >= analysisSecs) { anaResult = analyzer.result(); analysisState.store (2, std::memory_order_release); } }
    }
    auto modParams = [&] (int i)
    {
        auto e = base;
        for (const auto& s : slots)
        {
            if (s.src <= 0 || s.amt == 0.0) continue;
            const double v = modV[s.src - 1][i] * s.amt;
            switch (s.dst)
            {
                case 0: e.heat = juce::jlimit (0.0, 10.0, e.heat + 5.0 * v); break;
                case 1: e.morphX = juce::jlimit (0.0, 1.0, e.morphX + 0.5 * v); break;
                case 2: e.morphY = juce::jlimit (0.0, 1.0, e.morphY + 0.5 * v); break;
                case 4: e.density = juce::jlimit (0.0, 1.0, e.density + v); break;
                default: break;
            }
        }
        return e;
    };
    { const auto le = anyMod ? modParams (n - 1) : base; modLive.heat = (float) le.heat; modLive.mx = (float) le.morphX; modLive.my = (float) le.morphY; modLive.active = anyMod; }
    auto mixModAt = [&] (int i) { double d = 0; for (const auto& s : slots) if (s.src > 0 && s.dst == 3) d += modV[s.src - 1][i] * s.amt; return d; };
    meters.inPeak.store (juce::Decibels::gainToDecibels (ip, -120.f));
    meters.inRms.store ((float) juce::Decibels::gainToDecibels (std::sqrt (is / (2.0 * n)), -120.0));

    // ---- motor (sobreamostrado). wet e dry saem com a mesma latencia e a mesma fase
    if (activeQ == 0)
    {
        for (int i = 0; i < n; ++i)
        {
            if (anyMod && (i & 31) == 0) eng.setParams (modParams (i));
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
        const int fac = juce::jmax (1, N / juce::jmax (1, n));
        for (int j = 0; j < N; ++j)
        {
            if (anyMod && (j % (32 * fac)) == 0) eng.setParams (modParams (juce::jmin (n - 1, j / fac)));
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
        const double m = mixMod ? juce::jlimit (0.0, 1.0, (double) mixS.getNextValue() + mixModAt (i)) : (double) mixS.getNextValue();
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

        {   // GRAVE AUDIVEL: 150–800 Hz contra <100 Hz, a saida comparada com a entrada
            const double xi = 0.5 * (in[0][i] + in[1][i]), xo = 0.5 * (ol + orr);
            const double li = abLo[0][1].process (abLo[0][0].process (xi)), hi = abHi[0][1].process (abHi[0][0].process (xi));
            const double lo2 = abLo[1][1].process (abLo[1][0].process (xo)), ho = abHi[1][1].process (abHi[1][0].process (xo));
            abAcc[0] += li * li; abAcc[1] += hi * hi; abAcc[2] += lo2 * lo2; abAcc[3] += ho * ho;
        }
        lufsMeter.process (ol, orr, numCh < 2);                  // 2.1: LUFS do que sai (antes da monitorizacao TELEMOVEL)
        if (phoneMode.load (std::memory_order_relaxed))
        {   // monitorizacao TELEMOVEL: mono, sem graves (HP 300 Hz), sem topo (LP 7 kHz), medios-agudos realcados
            double mo = 0.5 * (ol + orr); for (auto& f : phoneF[0]) mo = f.process (mo); ol = orr = mo;
        }
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
    if (lufsReset.exchange (false)) { lufsMeter.reset(); tpMaxV = -120.f; meters.lufsI.store (-120.f); meters.lra.store (0.f); }
    tpMaxV = juce::jmax (tpMaxV, juce::Decibels::gainToDecibels (tpk, -120.f));
    meters.tpMax.store (tpMaxV); meters.lufsM.store ((float) lufsMeter.momentary()); meters.lufsS.store ((float) lufsMeter.shortTerm());
    meters.lufsSecs.store ((float) lufsMeter.seconds());
    if ((++lufsTick & 7) == 0) { meters.lufsI.store ((float) lufsMeter.integrated()); meters.lra.store ((float) lufsMeter.range()); }
    for (int k = 0; k < 4; ++k) { abE[k] = 0.97 * abE[k] + abAcc[k] / n; abAcc[k] = 0; }
    if (abE[0] > 1e-9 && abE[2] > 1e-9 && abE[1] > 1e-12 && abE[3] > 1e-12)
        bassAudibleDb.store ((float) (10.0 * std::log10 (abE[3] / abE[2]) - 10.0 * std::log10 (abE[1] / abE[0])));
    if (linkIndex >= 0) { auto& le = BurnixLink::entries()[(size_t) linkIndex]; le.harm.store (meters.harmRms.load()); le.out.store (meters.outRms.load()); }
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

void BurnixProcessor::storeAB (int slot) { abState[juce::jlimit (0, 1, slot)] = apvts.copyState(); refreshMorphTarget(); }
void BurnixProcessor::copyToOther() { abState[1 - ab] = apvts.copyState(); refreshMorphTarget(); }

void BurnixProcessor::swapAB()
{
    auto cur = apvts.copyState();
    if (abState[1 - ab].isValid()) adoptState (abState[1 - ab].createCopy());
    abState[1 - ab] = cur; refreshMorphTarget();
}

BurnixProcessor::MorphTarget BurnixProcessor::targetFrom (const juce::ValueTree& st) const
{
    MorphTarget t; std::map<juce::String, float> v;
    for (auto c : st) if (c.hasType ("PARAM")) v[c.getProperty ("id").toString()] = (float) c.getProperty ("value");
    auto g = [&] (const juce::String& id) { auto it = v.find (id); if (it != v.end()) return (double) it->second; auto* p = apvts.getParameter (id); return p ? (double) p->convertFrom0to1 (p->getDefaultValue()) : 0.0; };
    auto gi = [&] (const juce::String& id) { return (int) std::lround (g (id)); };
    auto& e = t.e;
    e.numBands = juce::jlimit (1, bx::kMaxBands, gi ("numBands"));
    for (int k = 0; k < bx::kMaxX; ++k) e.xo[k] = g ("xo" + juce::String (k + 1));
    { const int sl = gi ("slope"); e.slope = sl == 1 ? bx::Slope::LR48 : sl == 2 ? bx::Slope::LR6 : sl == 3 ? bx::Slope::LR12 : bx::Slope::LR24; }
    if (g ("xoLinear") > 0.5 && ! legacyEngine.load()) e.slope = bx::Slope::Linear;
    e.legacy = legacyEngine.load(); e.levelComp = g ("levelComp") > 0.5; e.progressive = gi ("driveCurve") == 0; e.drift = g ("drift") / 100.0;
    e.heat = g ("heat"); e.morphX = g ("morphX"); e.morphY = g ("morphY");
    const char* cn[4] { "cornerTop", "cornerRight", "cornerBottom", "cornerLeft" }; for (int i = 0; i < 4; ++i) e.corner[(size_t) i] = gi (cn[i]);
    e.bias = g ("bias") / 100.0; e.density = g ("density") / 100.0; e.transient = g ("transient") / 100.0;
    e.stageBlend = g ("stageBlend") / 100.0; e.interstageDb = g ("interstage"); e.swapOrder = gi ("topology") == 1;
    for (int b = 0; b < bx::kMaxBands; ++b)
    {
        auto& d = e.band[(size_t) b];
        d.on = g (bandId (b, "on")) > 0.5; d.solo = g (bandId (b, "solo")) > 0.5; d.route = (bx::Route) juce::jlimit (0, 3, gi (bandId (b, "route")));
        d.driveM = g (bandId (b, "driveM")); d.driveS = g (bandId (b, "driveS")); d.voice = gi (bandId (b, "voice")) - 1;
        d.filter = (bx::FilterType) juce::jlimit (0, 5, gi (bandId (b, "filter"))); d.fHz = g (bandId (b, "freq")); d.fQ = g (bandId (b, "q"));
        d.mode = gi (bandId (b, "mode")) == 1 ? bx::Mode::Parallel : bx::Mode::Insert; d.mix = g (bandId (b, "mix")) / 100.0;
        d.dyn = g (bandId (b, "dyn")) / 100.0; d.attackDb = g (bandId (b, "attack"));
    }
    e.draw[0] = 0; e.draw[1] = 1; for (int k = 0; k < 6; ++k) { const double db = g ("h" + juce::String (k + 2)); e.draw[(size_t) k + 2] = db <= -79.5 ? 0.0 : std::pow (10.0, db / 20.0); }
    t.color = g ("color"); t.body = g ("body") / 100.0; t.air = g ("air") / 100.0; t.width = g ("width") / 100.0; t.mix = g ("mix") / 100.0; t.out = g ("output"); t.in = g ("input");
    t.valid = true; return t;
}

void BurnixProcessor::refreshMorphTarget()
{
    const auto& other = abState[1 - ab];
    const int w = 1 - morphIdx.load();
    morphT[(size_t) w] = other.isValid() ? targetFrom (other) : MorphTarget();
    morphIdx.store (w);
}

void BurnixProcessor::setDrawFromDb (const std::array<float, 6>& db)
{
    undo.beginNewTransaction ("Captura");
    for (int k = 0; k < 6; ++k) if (auto* p = apvts.getParameter ("h" + juce::String (k + 2))) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-80.f, 0.f, db[(size_t) k]))); p->endChangeGesture(); }
}

void BurnixProcessor::startAnalysis (double seconds) { analysisSecs = juce::jmax (1.0, seconds); anaProgress = 0.f; analysisState = 3; }
float BurnixProcessor::analysisProgress() const { return analysisState.load() == 2 ? 1.f : juce::jlimit (0.f, 1.f, anaProgress.load()); }
void BurnixProcessor::loadPresetIntoB (int index)
{
    if (ab == 0) { abState[0] = apvts.copyState(); ab = 1; }
    applyPreset (index); refreshMorphTarget();
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
    refreshMorphTarget();
}

void BurnixProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("uiView", uiView, nullptr); st.setProperty ("uiBand", uiBand, nullptr); st.setProperty ("uiScale", uiScale, nullptr);
    st.setProperty ("presetName", presetName, nullptr);
    st.setProperty ("measureHistory", measureHistory.joinIntoString ("\n"), nullptr);
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
            measureHistory = juce::StringArray::fromLines (st.getProperty ("measureHistory", "").toString()); measureHistory.removeEmptyStrings();
            adoptState (st);
        }
}

// 2.1: guarda a medicao atual (LUFS integrado, LRA, pico verdadeiro maximo) no historico da sessao
void BurnixProcessor::storeMeasurement()
{
    const auto now = juce::Time::getCurrentTime().formatted ("%d/%m/%Y %H:%M");
    measureHistory.insert (0, now + "|" + juce::String (meters.lufsI.load(), 1) + "|" + juce::String (meters.lra.load(), 1)
                               + "|" + juce::String (meters.tpMax.load(), 1) + "|" + presetName);
    while (measureHistory.size() > 12) measureHistory.remove (measureHistory.size() - 1);
}

juce::AudioProcessorEditor* BurnixProcessor::createEditor() { return new BurnixEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BurnixProcessor(); }
