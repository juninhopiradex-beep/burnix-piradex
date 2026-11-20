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

class BurnixProcessor : public juce::AudioProcessor
{
public:
    BurnixProcessor();
    ~BurnixProcessor() override = default;

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

    struct Meters
    {
        std::atomic<float> inPeak { -120.f }, inRms { -120.f }, outPeak { -120.f }, outRms { -120.f }, outTp { -120.f }, autoGainDb { 0.f }, cpu { 0.f };
    } meters;
    int  latency() const { return currentLatency; }
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
                       *pWidth, *pMono, *pMonoF, *pMix, *pOut, *pAuto, *pDelta, *pBypass, *pQuality, *pPhase, *pBands, *pSlope, *pXo[bx::kMaxX],
                       *pComp, *pCurve, *pDrift, *pClip, *pRender;

    // motor: 13 = versao 1.3 (sessoes novas e presets), 12 = sessao gravada na 1.2 (som identico ao da 1.2)
    std::atomic<bool> legacyEngine { false }, loudReset { false };
    void adoptState (juce::ValueTree st);                  // decide o motor de uma sessao/preset carregado e aplica-o
    double deltaW[2] {}, deltaD[2] {}, deltaC = 0.001;     // DELTA puro: ganho linear igualado por canal

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BurnixProcessor)
};
