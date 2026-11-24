// =============================================================================
//  BURNIX by PIRADEX — AudioProcessor
//  Cadeia: InputConditioner (ganho, protecao numerica) -> OversamplingManager (1x/4x/8x/16x, min-phase ou linear)
//          -> BurnixEngine (bandas, M/S, valvulas) -> descida -> ToneStage (cor) -> mistura com o seco alinhado
//          -> StereoStage (largura, mono-bass) -> LoudnessMatcher (opt-in) -> saida -> medidores.
//  O BYPASS entrega a entrada intacta atrasada exatamente pela latencia reportada ao DAW.
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "Params.h"
#include "FactoryPresets.h"
#include "../DSP/BurnixEngine.h"
#include "../DSP/PostStages.h"
#include "../DSP/Loudness.h"
#include "../DSP/Analysis.h"

// BURNIX Link (2.0): registo partilhado por todas as instancias carregadas no mesmo processo do anfitriao
struct BurnixLink
{
    static constexpr int kMax = 64;
    struct Entry { std::atomic<bool> used { false }; std::atomic<float> harm { -120.f }, out { -120.f }; char name[64] {}; std::atomic<int> nameSeq { 0 }; };
    static std::array<Entry, kMax>& entries() { static std::array<Entry, kMax> e; return e; }
    static int claim() { for (int i = 0; i < kMax; ++i) { bool f = false; if (entries()[(size_t) i].used.compare_exchange_strong (f, true)) return i; } return -1; }
    static void release (int i) { if (i >= 0) { auto& e = entries()[(size_t) i]; e.harm = -120.f; e.name[0] = 0; e.used = false; } }
};

class BurnixProcessor : public juce::AudioProcessor
{
public:
    BurnixProcessor();
    ~BurnixProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return (int) bxp::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int i) override;
    const juce::String getProgramName (int i) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    void updateTrackProperties (const TrackProperties&) override;

    // ---- API para a GUI e para os testes
    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undo;
    bx::EngineParams makeEngineParams() const;            // le os parametros (seguro em qualquer thread)
    void applyPreset (int index);
    juce::String currentPresetName() const { return presetName; }
    void setPresetName (const juce::String& n) { presetName = n; }
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::Array<juce::File> userPresets() const;
    juce::String stateAsText();                            // copiar definicoes (XML) para a area de transferencia
    bool loadStateFromText (const juce::String& xml);      // colar definicoes
    bool saveAsDefault();                                  // estas regulacoes passam a ser as de arranque de novas instancias
    bool clearDefault();
    bool hasDefault() const;
    void storeAB (int slot);                               // A/B: guarda e troca
    void toggleAB();
    int abSlot() const { return ab; }
    void copyToOther();                                    // A -> B (ou B -> A): o outro lado fica igual ao atual
    void loadPresetIntoB (int index);                      // guarda o atual em A e carrega o preset em B (comparar)
    juce::String presetDescription() const;                // finalidade do preset atual no idioma da interface (vazio se nao houver)
    int currentFactoryIndex() const;

    // ---- 2.0
    std::atomic<bool> phoneMode { false };                 // monitorizacao: soar como um telemovel (mono, sem graves, sem topo)
    struct ModLive { std::atomic<float> env { 0.f }, lfo { 0.f }, sc { 0.f }, heat { 5.f }, mx { 0.5f }, my { 0.3f }; std::atomic<bool> scConnected { false }, active { false }; } modLive;
    std::atomic<float> bassAudibleDb { 0.f };              // GRAVE AUDIVEL: o que o BURNIX acrescentou a 150–800 Hz face ao sub
    void startAnalysis (double seconds);                   // assistente / match: ouve a entrada durante N segundos
    bool analysisDone() const { return analysisState.load() == 2; }
    float analysisProgress() const;
    bx::Analyzer::Result analysisResult() const { return anaResult; }
    void swapAB();                                         // A/B cego: troca o conteudo dos dois lados
    void refreshMorphTarget();                             // A/B morph: recalcula o alvo (o outro lado)
    int linkSlot() const { return linkIndex; }
    juce::String trackName() const { return trackNm; }
    void setDrawFromDb (const std::array<float, 6>& db);   // capturas: H2..H7 em dB

    struct Meters
    {
        std::atomic<float> inPeak { -120.f }, inRms { -120.f }, outPeak { -120.f }, outRms { -120.f }, outTp { -120.f }, autoGainDb { 0.f }, cpu { 0.f },
                           harmRms { -120.f };   // HARM: nivel do que o BURNIX acrescenta (sinal menos o seco com o mesmo ganho)
        // 2.1 MASTERING: loudness BS.1770-4 da saida (LUFS) e pico verdadeiro maximo desde o ultimo RESET
        std::atomic<float> lufsI { -120.f }, lufsS { -120.f }, lufsM { -120.f }, lra { 0.f }, tpMax { -120.f }, lufsSecs { 0.f };
    } meters;
    void resetLoudness() { lufsReset.store (true); }
    // historico de medicoes (thread de mensagens): "data|LUFS-I|LRA|TPmax|preset", guardado na sessao (ate 12)
    juce::StringArray measureHistory;
    void storeMeasurement();
    int  latency() const { return currentLatency; }
    bool linearXo() const;                                // 2.1: crossovers de fase linear ligados
    int  latencyFor (int q, int phase, bool linear) const;
    int  activeFactor() const { return bxp::qualityFactor (activeQ); }
    const bx::BurnixEngine& activeEngine() const { return engines[(size_t) activeQ]; }
    bool usesLegacyEngine() const { return legacyEngine.load(); }
    int activeQuality() const { return activeQ; }
    void upgradeEngine();                                 // sessao da 1.2 -> motor 1.3 (pedido no menu)

    // espectro de saida (mono) para o anel da GUI
    bool popSpectrum (float* dest, int n);                // copia as ultimas n amostras (n <= 4096)

    // medicao real de harmonicos (vista ENGINEER): motor novo, seno 1 kHz a -6 dBFS, 192 kHz. Thread de fundo.
    struct HarmReport { std::array<double, 11> db {}; double thd = 0, h1 = 0; bool valid = false; };
    static HarmReport measureHarmonics (const bx::EngineParams& p, double levelDb = -6.0);

    // estado da GUI guardado com a sessao
    int uiView = 0, uiBand = 1; float uiScale = 1.0f;

private:
    template <typename T> void processT (juce::AudioBuffer<T>&);
    void processChunk (const float* const* in, float* const* out, int numCh, int n);
    void switchConfig (int q, int phase);
    int effectiveQuality (int wantQ, int phase) const;

    std::array<bx::BurnixEngine, 4> engines;                // 1x, 4x, 8x, 16x
    std::array<std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2>, 4> osWet, osDry;   // [q][phase] (q=0 sem)
    bx::LoudnessMeter lufsMeter; std::atomic<bool> lufsReset { false }; float tpMaxV = -120.f; int lufsTick = 0;
    bx::ToneStage tone; bx::StereoStage stereo; bx::LoudnessMatcher loud; bx::TruePeak tp;
    juce::AudioBuffer<float> work, wetBuf, dryBuf, rawDelay;
    int rawPos = 0, maxBlock = 512, currentLatency = 0, activeQ = 1, activePhase = 0;
    double sr = 48000;
    juce::SmoothedValue<float> inGain, outGain, mixS, bypassMix;
    float fade = 1.f; int fadeState = 0, pendingQ = 1, pendingPhase = 0;    // 0 normal, 1 a descer, 2 a subir

    // FIFO do espectro
    std::array<float, 8192> specRing {}; std::atomic<int> specWrite { 0 };

    int currentProgram = 0; juce::String presetName { "Init" };
    int ab = 0; juce::ValueTree abState[2];

    // ponteiros diretos para os parametros (leitura atomica)
    struct BandPtrs { std::atomic<float> *on, *solo, *route, *driveM, *driveS, *voice, *filter, *freq, *q, *mode, *mix; };
    std::array<BandPtrs, bx::kMaxBands> bp {};
    std::atomic<float> *pInput, *pHeat, *pMx, *pMy, *pCorner[4], *pDensity, *pBias, *pStage, *pTopo, *pInter, *pTrans, *pColor, *pBody, *pAir,
                       *pWidth, *pMono, *pMonoF, *pMix, *pOut, *pAuto, *pDelta, *pBypass, *pQuality, *pPhase, *pBands, *pSlope, *pXoLin = nullptr, *pXo[bx::kMaxX],
                       *pComp, *pCurve, *pDrift, *pClip, *pRender;

    // motor: 13 = versao 1.3 (sessoes novas e presets), 12 = sessao gravada na 1.2 (som identico ao da 1.2)
    std::atomic<bool> legacyEngine { false }, loudReset { false };
    void adoptState (juce::ValueTree st);                  // decide o motor de uma sessao/preset carregado e aplica-o
    double deltaW[2] {}, deltaD[2] {}, deltaC = 0.001;     // DELTA puro: ganho linear igualado por canal
    // 2.0: modulacao, sidechain, A/B morph, telemovel, grave audivel, Link, analise
    std::atomic<float> *pH[6] {}, *pModSrc[3] {}, *pModDst[3] {}, *pModAmt[3] {}, *pLfoRate {}, *pLfoShape {}, *pEnvA {}, *pEnvR {}, *pAbMorph {};
    struct BandPtrs2 { std::atomic<float> *dyn, *attack; };
    std::array<BandPtrs2, bx::kMaxBands> bp2 {};
    mutable std::array<float, 6> lastDrawDb { 1, 1, 1, 1, 1, 1 }; mutable int drawVer = 0; mutable std::array<double, 8> drawCache {};
    double envV = 0, scV = 0, lfoPh = 0, lfoRnd = 0, lfoRndT = 0; unsigned int rndState = 22222;
    float scTmp[2][512] {}; bool scPresent = false;
    struct MorphTarget { bx::EngineParams e; double color = 0, body = 0, air = 0, width = 1, mix = 1, out = 0, in = 0; bool valid = false; };
    std::array<MorphTarget, 2> morphT; std::atomic<int> morphIdx { 0 };
    MorphTarget targetFrom (const juce::ValueTree& st) const;
    std::array<bx::Biquad, 4> phoneF[2]; std::array<bx::Biquad, 2> abLo[2], abHi[2]; double abE[4] {}, abAcc[4] {};
    int linkIndex = -1; juce::String trackNm;
    bx::Analyzer analyzer; std::atomic<int> analysisState { 0 }; std::atomic<float> anaProgress { 0.f }; double analysisSecs = 10; bx::Analyzer::Result anaResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BurnixProcessor)
};
