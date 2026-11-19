// =============================================================================
//  BURNIX by PIRADEX — presets de fabrica (valores reais, nao normalizados). Cada preset parte do INIT
//  (todos os parametros por omissao) e altera so o que lista. Finalidade tecnica em "purpose".
// =============================================================================
#pragma once
#include <initializer_list>
#include <utility>
#include <vector>

namespace bxp
{
struct FactoryPreset
{
    const char* name; const char* category; const char* purpose;
    std::vector<std::pair<const char*, float>> values;
};

// indices de escolha: voice 0 MORPH 1 EMBER 2 TRIODE 3 PENTODE 4 IRON 5 VELVET 6 FIRE · route 0 L/R 1 MID 2 SIDE 3 M+S
// filter 0 NONE 1 HP 2 LP 3 BP 4 NOTCH 5 SHELF · mode 0 INSERT 1 PARALLEL · quality 0 ECO 1 HQ4 2 HQ8 3 ULTRA16
inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> p {
        { "Init", "Base", "Ponto de partida: 4 bandas, drive 3, morph perto de TRIODE BLOOM.", {} },
        { "Clean Analog Glue", "Mix Bus", "Cola suave numa banda, VELVET, ganho automatico.",
          { { "numBands", 1 }, { "b1_voice", 5 }, { "b1_driveM", 2.5f }, { "density", 20 }, { "autoGain", 1 } } },
        { "Vocal Silk", "Voz", "Calor discreto na voz, agudos sedosos, ataques preservados.",
          { { "numBands", 3 }, { "xo1", 200 }, { "xo2", 4000 }, { "b1_voice", 1 }, { "b1_driveM", 1 }, { "b2_voice", 1 }, { "b2_driveM", 3.5f },
            { "b3_voice", 5 }, { "b3_driveM", 2.5f }, { "air", 25 }, { "transient", 30 }, { "autoGain", 1 } } },
        { "Kizomba Vocal Heat", "Voz", "Corpo da voz com 2.o harmonico (TRIODE) so no MID, brilho controlado.",
          { { "numBands", 4 }, { "b1_voice", 1 }, { "b1_driveM", 1 }, { "b2_voice", 2 }, { "b2_route", 1 }, { "b2_driveM", 5 },
            { "b3_voice", 2 }, { "b3_driveM", 3.5f }, { "b4_voice", 5 }, { "b4_driveM", 2 }, { "color", 0.8f }, { "air", 15 }, { "density", 25 }, { "autoGain", 1 } } },
        { "G-Zouk Bass Presence", "Baixo", "Presenca do baixo em colunas pequenas: IRON CORE no sub, TRIODE nos medios-graves, graves em mono.",
          { { "numBands", 3 }, { "xo1", 100 }, { "xo2", 700 }, { "b1_voice", 4 }, { "b1_route", 1 }, { "b1_driveM", 4.5f },
            { "b2_voice", 2 }, { "b2_route", 1 }, { "b2_driveM", 6 }, { "b3_driveM", 0 }, { "monoBass", 1 }, { "monoFreq", 120 }, { "autoGain", 1 } } },
        { "Afro Kick Grit", "Bateria", "Mordida no kick: PENTODE em paralelo filtrado a 900 Hz, FIRESTORM nos agudos.",
          { { "numBands", 3 }, { "xo1", 80 }, { "xo2", 2500 }, { "b1_voice", 4 }, { "b1_driveM", 3 }, { "b2_voice", 3 }, { "b2_driveM", 6.5f },
            { "b2_mode", 1 }, { "b2_mix", 40 }, { "b2_filter", 3 }, { "b2_freq", 900 }, { "b2_q", 1 }, { "b3_voice", 6 }, { "b3_driveM", 4 }, { "b3_mix", 50 },
            { "transient", 50 } } },
        { "Stereo Side Sparkle", "Estereo", "Brilho so nas laterais (SIDE), centro intacto, graves em mono.",
          { { "numBands", 3 }, { "xo1", 300 }, { "xo2", 5000 }, { "b1_driveM", 0 }, { "b2_route", 2 }, { "b2_voice", 5 }, { "b2_driveS", 3.5f },
            { "b3_route", 2 }, { "b3_voice", 1 }, { "b3_driveS", 5 }, { "width", 115 }, { "monoBass", 1 } } },
        { "Mixbus Velvet", "Mix Bus", "Saturacao de bus macia e densa, sem aspereza.",
          { { "numBands", 1 }, { "b1_voice", 5 }, { "b1_driveM", 3.5f }, { "density", 30 }, { "autoGain", 1 } } },
        { "Master Subtle Color", "Master", "Cor minima para master: 3 bandas, HQ 8x, filtro linear.",
          { { "numBands", 3 }, { "xo1", 150 }, { "xo2", 5000 }, { "b1_driveM", 1.5f }, { "b2_driveM", 2 }, { "b3_driveM", 1.5f },
            { "morphX", 0.35f }, { "morphY", 0.4f }, { "quality", 2 }, { "osPhase", 1 }, { "autoGain", 1 } } },
        { "Drum Parallel Fire", "Bateria", "Saturacao paralela agressiva em duas bandas com 2.o estagio.",
          { { "numBands", 2 }, { "xo1", 200 }, { "b1_voice", 6 }, { "b1_driveM", 5 }, { "b1_mode", 1 }, { "b1_mix", 35 },
            { "b2_voice", 6 }, { "b2_driveM", 7 }, { "b2_mode", 1 }, { "b2_mix", 45 }, { "stageBlend", 30 }, { "autoGain", 1 } } },
        { "Low-Mid Sculpt", "Mistura", "So os medios-graves (120-900 Hz) ganham corpo, focado a 300 Hz.",
          { { "numBands", 4 }, { "b1_driveM", 0 }, { "b2_voice", 2 }, { "b2_driveM", 5 }, { "b2_filter", 3 }, { "b2_freq", 300 }, { "b2_q", 1.2f },
            { "b3_driveM", 0 }, { "b4_driveM", 0 } } },
        { "Air Without Harshness", "Mistura", "Harmonicos so acima de 10 kHz com VELVET, ataques protegidos.",
          { { "numBands", 3 }, { "xo1", 300 }, { "xo2", 6000 }, { "b1_driveM", 0 }, { "b2_driveM", 0 }, { "b3_voice", 5 }, { "b3_driveM", 4 },
            { "b3_filter", 5 }, { "b3_freq", 10000 }, { "transient", 40 } } },
        { "Mono-Safe Master", "Master", "Master com M+S: lado mais suave, graves em mono abaixo de 100 Hz, HQ 8x.",
          { { "numBands", 3 }, { "xo1", 120 }, { "xo2", 4000 }, { "b1_route", 3 }, { "b1_driveM", 2 }, { "b1_driveS", 0 }, { "b2_route", 3 }, { "b2_driveM", 2.5f },
            { "b2_driveS", 1.5f }, { "b3_route", 3 }, { "b3_driveM", 1.5f }, { "b3_driveS", 1.5f }, { "monoBass", 1 }, { "monoFreq", 100 }, { "quality", 2 }, { "autoGain", 1 } } },
    };
    return p;
}
} // namespace bxp
