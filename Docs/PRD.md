# BURNIX by PIRADEX — Product Requirements Document (v1.0.0)

**Tagline:** Add colorful, frequency-selective tube saturation with M/S processing.
**Produto:** plugin de áudio em tempo real, 64 bits — VST3 + AU (macOS universal Intel/Apple Silicon, 10.13+) e VST3 (Windows x64).
**Base:** brief "BURNIX by PIRADEX — Product & DSP Master Brief v1.0" (19 nov 2026) e a interface aprovada "Órbita v2".
**Criador e Desenvolvedor:** Ricardo Amorim / Juninho Piradex — Beatfreak Music.

---

## 1. Objetivo e princípios

Cor harmónica controlável por frequência e por imagem estéreo — não "mais distorção". Do subtil ao agressivo, utilizável em voz,
baixo, bateria, buses e master, sem defeitos não intencionais.

Princípios (todos verificados por testes automáticos, ver `Docs/TEST_RESULTS.md`):

* **Bypass verdadeiro** — a entrada sai intacta, atrasada exatamente pela latência reportada ao DAW (null digital).
* **Drive 0 = identidade exata** — com todas as bandas a 0 o DELTA é silêncio digital (−200 dBFS) em todas as qualidades.
* **Sem limitador escondido** — só proteção numérica (NaN/Inf → 0, clamp a ±64 = +36 dBFS para evitar explosões).
* **Ganho automático opt-in** (desligado por omissão), limitado a ±18 dB e lento, para nunca esconder distorção.
* **Nunca inventar medições** — a vista ENGINEER mostra harmónicos MEDIDOS num motor real (seno 1 kHz −6 dBFS).
* **Inspirar sem copiar** — Decapitator, Saturn 2 e HG-2 MS foram referências de conceitos gerais. Nenhum DSP, curva, preset,
  nome de modo, layout ou identidade foi reproduzido. Os seis voicings e a interface "Órbita" são originais.

## 2. Âmbito por versão

| Fase | Incluído | Estado |
|---|---|---|
| **MVP — som real** | estéreo/mono, 1–6 bandas, 6 voicings, drive, mix, auto gain, oversampling 1/4/8/16x, VST3 | **feito (1.0.0)** |
| **v1 — BURNIX PRO** | M/S completo por banda, filtros HP/LP/BP/NOTCH/SHELF, paralelo/insert, presets, AU, medições, instaladores | **feito (1.0.0)** |
| **v1.5 — Mastering** | crossovers de fase linear, LUFS integrado, mais presets de master, histórico de medições | por fazer |
| **v1.1 — Definições** | idioma PT/EN, 5 esquemas de cores, animação, espectro, dicas, predefinição própria, copiar/colar | **feito (1.1.0)** |
| **v2 — Creative** | moduladores (envolvente/LFO), macros, morph por banda, routing avançado, 32x, AAX | por fazer |

Decisões conscientes desta versão:
* **Oversampling da cadeia de bandas inteira** (não só da não-linearidade): o seco e o molhado passam pelo mesmo divisor de
  bandas e por oversamplers idênticos, por isso a mistura e o DELTA não têm filtro-pente. Custo: mais CPU (medido, secção 9).
* **32x não incluído**: a 16x o aliasing do caso extremo de teste já está a −73 dB do fundamental; 32x duplicaria o CPU.
* **Crossovers Linkwitz-Riley IIR** (24 ou 48 dB/oct): soma passa-tudo (módulo plano ±0,0000 dB medido), sem latência.
  A fase linear aplica-se só ao filtro de oversampling (opção LINEAR).
* **Interface em português e inglês** (Definições ⚙ → Idioma, desde a 1.1.0), com 5 esquemas de cores.
* **AAX** não incluído: exige SDK Avid e assinatura PACE (mesmo processo dos outros plugins Piradex).

## 3. Arquitetura

```
 entrada ─► InputConditioner (ganho INPUT, NaN/Inf → 0) ─┬─► atraso = latência ─────────────────────────► BYPASS / bloqueado
                                                          │
                                                          ▼
                              OversamplingManager (1x / 4x / 8x / 16x, MIN-PHASE IIR ou LINEAR FIR, latência inteira)
                                                          │  (taxa sobreamostrada)
                                                          ▼
                    BandSplitter (1–6 bandas, LR24/LR48 + passa-tudo de compensação)  ──►  DRY = soma sem processar
                                                          │
                    por banda: ChannelRouter L/R | MID | SIDE | M+S   (M = (L+R)/√2, S = (L−R)/√2)
                    por lane:  filtro seletivo F ─► NonlinearTubeStage (voicing/morph, bias, density, transient, 2.º estágio)
                               INSERT  y = x + mix·(Sat(Fx) − Fx)       PARALLEL  y = x + mix·Sat(Fx)
                                                          │
                                                          ▼  WET = soma das bandas
                                     descida (mesmo filtro no WET e no DRY → mesma latência e fase)
                                                          ▼
          ToneStage (COLOR tilt, BODY, AIR — só no WET) ─► MIX global ─► StereoStage (WIDTH, MONO-BASS) ─► LoudnessMatcher
                                                          ▼
                                        OUTPUT ─► medidores (peak, RMS, true peak 4x) ─► espectro para a GUI
```

Classes (todas testáveis; o DSP é C++ puro sem JUCE):

| Classe | Ficheiro | Papel |
|---|---|---|
| InputConditioner | `Plugin/PluginProcessor.cpp` (entrada) | ganho, proteção numérica, medidores de entrada, linha de atraso do bypass |
| OversamplingManager | `Plugin/PluginProcessor.*` | 3 fatores × 2 filtros × (wet, dry), preparados no `prepareToPlay`; troca com fade de 6 ms e nova latência |
| BandSplitter | `DSP/BandSplitter.h` | divisor Linkwitz-Riley em cascata com compensação de fase |
| ChannelRouter + HarmonicEngine | `DSP/BurnixEngine.h` | rota M/S por banda, filtros seletivos, modos, morph, medidores de atividade |
| NonlinearTubeStage | `DSP/TubeStage.h` | 6 voicings, normalização, bias, sag, transient preserve, 2.º estágio, bloqueio de DC |
| StereoProcessor | `DSP/PostStages.h` (StereoStage) | largura e mono-bass sem vazamento de graves para S |
| ToneStage | `DSP/PostStages.h` | tilt 1 kHz, body 120 Hz, air 12 kHz |
| LoudnessMatcher | `DSP/PostStages.h` | RMS de saída = RMS do seco alinhado (opt-in) |
| MeteringEngine | `DSP/PostStages.h` (TruePeak) + processador | peak, RMS, true peak 4x, CPU, espectro |
| PresetManager | `Plugin/FactoryPresets.h` + processador | 150 presets de fábrica em 19 categorias (`tools/gen_presets.py`), presets do utilizador (`.burnix`), A/B, undo/redo |
| BurnixEditor | `GUI/*` | Órbita v2: PERFORM / SCULPT / ENGINEER, login |

## 4. Modelos harmónicos (originais)

Normalização comum: `y = (f(s·x + b) − f(b)) / (f(s + b) − f(b))` — sem DC estática e com x = 1 → y = 1.
Drive 0–10 → `s = 0,35 · 10^(drive·2,7/20)`. Abaixo de drive 1 mistura com o limpo (drive 0 = identidade exata).
A componente acrescentada `y − x` passa por um bloqueador de DC a 5 Hz (o sinal limpo nunca é filtrado).

| Voicing | Curva | Objetivo | Medido (1 kHz, −6 dBFS, drive 7) |
|---|---|---|---|
| EMBER | `tanh`, bias interno 0,05 | suave, 3.º dominante, 2.º discreto | THD 12,8 %, H2 −34 dB, H3 −18 dB |
| TRIODE BLOOM | `tanh` assimétrico (negativo 0,6×, mais macio), bias 0,18 | 2.º harmónico proeminente | THD 14,8 %, H2 −18 dB, H3 −22 dB |
| PENTODE EDGE | `u / (1 + |u|^2,5)^0,4` | joelho mais duro, só ímpares | THD 13,9 %, H2 nulo, H3 −17 dB |
| IRON CORE | `tanh(u + 0,35·(u − m))`, m segue u a 250 Hz | densidade low-mid (aproximação, não modelo de transformador) | THD 17,9 % |
| VELVET | `atan` de joelho largo + passa-baixo 20 kHz → ~9 kHz com o drive | pouco THD aparente | THD 7,2 % |
| FIRESTORM | clipper de joelho quadrático (C1), limiares +1 / −0,8 | agressivo, sound design | THD 29,2 % |

Dinâmica: **DENSITY** (sag: drive −45 % e assimetria +0,25 com a envolvente 3/80 ms), **TRANSIENT PRESERVE** (menos saturação
quando a envolvente rápida 0,5/15 ms passa a lenta 15/150 ms), **2.º estágio** PENTODE com ganho INTERSTAGE e ordem trocável
(TOPOLOGY). **Morph**: quatro pontos cardeais (cima, direita, baixo, esquerda) com voicings escolhíveis; pesos = (1 − d/0,707)²
normalizados. Uma banda pode seguir o morph global ou ter um voicing fixo.

## 5. Matemática M/S e compatibilidade mono

`M = (L+R)/√2`, `S = (L−R)/√2`, `L = (M+S)/√2`, `R = (M−S)/√2` (ortonormal: a reconstrução é exata — null digital a drive 0).
Por banda: L/R (drive MID nos dois canais), MID, SIDE ou M+S (drives independentes).
**Mono-bass:** S passa por um passa-alto LR4 na frequência escolhida e M pelo passa-tudo equivalente, para que M e S fiquem em
fase acima do corte; abaixo do corte S é removido (medido: lado a 40 Hz atenuado 38 dB com corte em 120 Hz).

## 6. Contrato de parâmetros (IDs estáveis — nunca renomear; versão 1)

Globais: `input` (−24…24 dB), `heat` (0–10, 5 = valores das bandas), `morphX`, `morphY` (0–1), `cornerTop`, `cornerRight`,
`cornerBottom`, `cornerLeft` (voicing), `density` (0–100 %), `bias` (−100…100), `stageBlend` (0–100 %), `topology`,
`interstage` (−12…12 dB), `transient` (0–100 %), `color` (−6…6 dB), `body` (0–100 %), `air` (0–100 %), `width` (0–200 %),
`monoBass`, `monoFreq` (20–300 Hz), `mix` (0–100 %), `output` (−24…24 dB), `autoGain`, `delta`, `bypass`,
`quality` (ECO 1x, HQ 4x, HQ 8x, ULTRA 16x), `osPhase` (MIN-PHASE, LINEAR), `numBands` (1–6), `slope` (24/48 dB/oct),
`xo1`…`xo5` (Hz).
Por banda N = 1…6: `bN_on`, `bN_solo`, `bN_route` (L/R, MID, SIDE, M+S), `bN_driveM`, `bN_driveS` (0–10),
`bN_voice` (MORPH + 6 voicings), `bN_filter` (OFF, HP, LP, BP, NOTCH, SHELF), `bN_freq`, `bN_q`, `bN_mode` (INSERT, PARALLEL),
`bN_mix` (0–100 %).
Todos automatizáveis. Suavização por amostra: input, output, mix, drives e mix das bandas, bias e stage blend (20 ms).

## 7. Mapa de estados

* **Licença:** aberto até 31/12/2026 → depois MESTRE (senha master offline) ou licença PCC válida; bloqueado = áudio passa
  intacto e aparece o login. Revalidação PCC 1×/dia em segundo plano; 14 dias de graça sem rede.
* **Qualidade:** a troca faz fade-out de 6 ms → reinicia oversamplers e motor → reporta a nova latência → fade-in. Sem cliques.
* **Bypass:** crossfade de 10 ms; quando completo, a saída é a entrada atrasada (bit a bit).
* **Sessão:** guarda parâmetros + vista, banda selecionada, escala e nome do preset.

## 8. Interface "Órbita v2"

Janela 1200 × 740 a 100 %, escalável 75–200 % (menu ⚙ ou arrastar o canto, proporção fixa).
* **PERFORM:** anel com MID por fora e SIDE por dentro (arcos = drive, brilho = atividade medida), espectro de saída,
  medidores IN/OUT na abertura, morph de carácter no centro; painel HEAT, carácter, density, color, air, input, mix, output,
  rota e modo para todas as bandas, AUTO GAIN e DELTA.
* **SCULPT:** pegas brancas movem crossovers, clique num arco escolhe a banda, voicing da banda no centro; painel com rota,
  ON/SOLO, drives MID/SIDE, mix, filtro seletivo (frequência e Q), modo e inclinação.
* **ENGINEER:** radar dos harmónicos H2–H10 MEDIDOS (e referência guardada), oversampling na base do anel, THD no centro;
  painel com filtro do oversampling, topologia, stage blend, interstage, bias, transient, body, width, mono-bass e leituras
  reais (THD, latência, CPU, true peak, auto gain, fator).
Acessibilidade: foco de teclado visível (contorno cyan), setas nos controlos, duplo-clique repõe, Shift = ajuste fino, dicas.

## 9. Requisitos de desempenho e critérios de aceitação

| Ensaio | Critério | Resultado (ver TEST_RESULTS) |
|---|---|---|
| Null / bypass, 7 configurações | diferença 0 | **0** em todas |
| Delta a drive 0, 4 qualidades | < −100 dBFS | **−200 dBFS** |
| Latência reportada (LINEAR) | = pico do impulso ±1 | 61/65/67 = 61/65/67 |
| Reconstrução crossovers 2/3/6 bandas, LR24/LR48 | < 0,01 dB | **0,0000 dB** |
| Aliasing 15 kHz FIRESTORM drive 9 @48 kHz | 4x ≥ 10 dB melhor que 1x | 1x −9,6 · 4x −35,4 · 8x −44,5 · 16x −73,1 dB |
| 44,1–192 kHz × blocos 1–4096 | saída finita | ok (20/20) |
| Automação aleatória de todos os parâmetros | finita, sem crash | ok |
| Alocações na thread de áudio | 0 | **0** |
| CPU 6 bandas, 48 kHz (Xeon 2,1 GHz, 1 núcleo) | informativo | ECO 5 % · HQ 4x 20 % · HQ 8x 43 % · ULTRA 16x 89 % |

## 10. Riscos e trade-offs

* **CPU em ULTRA 16x com 6 bandas** é alto num núcleo lento; recomendação: HQ 4x para mistura, 8x/16x para master ou render.
* **MIN-PHASE** reporta a latência de grupo a baixa frequência (6 amostras); o pico do impulso pode cair 1 amostra depois.
  LINEAR tem latência exata e maior (61–67 amostras a 48 kHz).
* **PARALLEL** soma o ramo saturado ao sinal: o nível sobe (até +6 dB com mix 100 %). Use OUTPUT ou AUTO GAIN.
* **IRON CORE** e os restantes voicings são modelos originais, sem pretensão de reproduzir válvulas ou transformadores reais.
