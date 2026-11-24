# BURNIX by PIRADEX — resultados reais dos testes (v1.1.0)

Gerados pelos testes automáticos do repositório (não editados à mão). O CI volta a corrê-los em cada commit e guarda os logs como artefacto `BURNIX-test-logs`.

Máquina destes resultados: Linux x86_64, Intel Xeon @ 2,1 GHz (2 vCPU, núcleo partilhado), compilação Release.

## 1. DSP puro (`Tests/test_dsp.cpp`)

```
BURNIX by PIRADEX — testes DSP (resultados reais)

[1] Reconstrucao do BandSplitter (modulo da soma, 20 Hz a 20 kHz)
    LR24  2 bandas: desvio maximo 0.0000 dB
    LR24  3 bandas: desvio maximo 0.0000 dB
    LR24  6 bandas: desvio maximo 0.0000 dB
    LR48  2 bandas: desvio maximo 0.0000 dB
    LR48  3 bandas: desvio maximo 0.0000 dB
    LR48  6 bandas: desvio maximo 0.0000 dB

[2] Drive 0 -> wet identico a dry (motor completo, 6 bandas, M+S)
    diferenca maxima wet-dry: 4.44e-16

[3] Harmonicos por voicing (1 kHz, -6 dBFS, sem oversampling extra: fs = 384 kHz)
    voicing        drive     THD %     H2 dB     H3 dB     H4 dB     H5 dB
    EMBER            3.0     1.868     -39.8     -36.1     -69.8     -70.7
    EMBER            7.0    12.804     -34.4     -18.1     -45.5     -33.9
    TRIODE BLOOM     3.0     4.082     -28.2     -37.9     -71.4     -64.9
    TRIODE BLOOM     7.0    14.816     -18.4     -21.5     -37.2     -38.4
    PENTODE EDGE     3.0     1.350    -240.0     -37.4    -240.0     -66.1
    PENTODE EDGE     7.0    13.890    -240.0     -17.3    -240.0     -32.4
    IRON CORE        3.0     3.082     -36.2     -31.5     -61.5     -61.3
    IRON CORE        7.0    17.926     -33.3     -15.3     -41.1     -27.9
    VELVET           3.0     1.058     -46.1     -40.6     -77.2     -76.3
    VELVET           7.0     7.152     -40.7     -23.1     -54.4     -41.3
    FIRESTORM        3.0     1.058     -43.5     -44.9     -47.0     -49.9
    FIRESTORM        7.0    29.162     -28.1     -11.4     -31.3     -20.1

[4] THD cresce com o drive
    EMBER         ok (THD a drive 10: 29.73 %)
    TRIODE BLOOM  ok (THD a drive 10: 27.06 %)
    PENTODE EDGE  ok (THD a drive 10: 29.57 %)
    IRON CORE     ok (THD a drive 10: 33.86 %)
    VELVET        ok (THD a drive 10: 17.10 %)
    FIRESTORM     ok (THD a drive 10: 39.83 %)

[5] DC residual com bias +1 e TRIODE BLOOM drive 8
    DC medio: -1.17e-05

[6] Robustez numerica
    saida sempre finita: sim

[7] Mono-bass 120 Hz: lado a 40 Hz atenuado
    nivel do lado a 40 Hz: -38.3 dB

Resultado: 31 verificacoes ok, 0 falhadas
```

## 2. Integração do plugin (`Tests/smoke.cpp`)

```
BURNIX by PIRADEX — teste de integracao (medicoes reais)

ok      bypass ECO 1x    MIN-PHASE: latencia 0 amostras, diferenca maxima 0 (null exato)
ok      bypass HQ 4x     MIN-PHASE: latencia 6 amostras, diferenca maxima 0 (null exato)
ok      bypass HQ 4x     LINEAR   : latencia 61 amostras, diferenca maxima 0 (null exato)
ok      bypass HQ 8x     MIN-PHASE: latencia 6 amostras, diferenca maxima 0 (null exato)
ok      bypass HQ 8x     LINEAR   : latencia 65 amostras, diferenca maxima 0 (null exato)
ok      bypass ULTRA 16x MIN-PHASE: latencia 6 amostras, diferenca maxima 0 (null exato)
ok      bypass ULTRA 16x LINEAR   : latencia 67 amostras, diferenca maxima 0 (null exato)
ok      delta a drive 0 (ECO 1x): pico -200.0 dBFS
ok      delta a drive 0 (HQ 4x): pico -200.0 dBFS
ok      delta a drive 0 (HQ 8x): pico -200.0 dBFS
ok      delta a drive 0 (ULTRA 16x): pico -200.0 dBFS
INFO    latencia HQ 4x MIN-PHASE: reportada 6 (atraso de grupo a baixa frequencia), pico do impulso a 6 (fase minima)
ok      latencia HQ 4x LINEAR: reportada 61, pico do impulso a 61
INFO    latencia HQ 8x MIN-PHASE: reportada 6 (atraso de grupo a baixa frequencia), pico do impulso a 7 (fase minima)
ok      latencia HQ 8x LINEAR: reportada 65, pico do impulso a 65
INFO    latencia ULTRA 16x MIN-PHASE: reportada 6 (atraso de grupo a baixa frequencia), pico do impulso a 7 (fase minima)
ok      latencia ULTRA 16x LINEAR: reportada 67, pico do impulso a 67

INFO    aliasing ECO 1x   : energia fora do fundamental = -9.6 dB relativo ao fundamental
INFO    aliasing HQ 4x    : energia fora do fundamental = -35.4 dB relativo ao fundamental
INFO    aliasing HQ 8x    : energia fora do fundamental = -44.5 dB relativo ao fundamental
INFO    aliasing ULTRA 16x: energia fora do fundamental = -73.1 dB relativo ao fundamental
ok      4x reduz o aliasing pelo menos 10 dB face a 1x (25.8 dB)
ok      16x nao piora face a 4x (-73.1 vs -35.4 dB)

INFO    THD EMBER         drive 5:   5.15 %   H2  -35.9 dB   H3  -26.2 dB   H5  -50.7 dB
ok      medicao de harmonicos valida (EMBER)
INFO    THD TRIODE BLOOM  drive 5:   8.11 %   H2  -22.7 dB   H3  -29.2 dB   H5  -51.1 dB
ok      medicao de harmonicos valida (TRIODE BLOOM)
INFO    THD PENTODE EDGE  drive 5:   5.20 %   H2 -240.0 dB   H3  -25.7 dB   H5  -57.1 dB
ok      medicao de harmonicos valida (PENTODE EDGE)
INFO    THD IRON CORE     drive 5:   8.11 %   H2  -33.2 dB   H3  -22.2 dB   H5  -42.5 dB
ok      medicao de harmonicos valida (IRON CORE)
INFO    THD VELVET        drive 5:   2.94 %   H2  -42.3 dB   H3  -30.9 dB   H5  -57.1 dB
ok      medicao de harmonicos valida (VELVET)
INFO    THD FIRESTORM     drive 5:  14.49 %   H2  -24.8 dB   H3  -17.6 dB   H5  -43.0 dB
ok      medicao de harmonicos valida (FIRESTORM)

ok      fs 44.1 kHz bloco 1: saida finita (latencia 6)
ok      fs 44.1 kHz bloco 32: saida finita (latencia 6)
ok      fs 44.1 kHz bloco 480: saida finita (latencia 6)
ok      fs 44.1 kHz bloco 4096: saida finita (latencia 6)
ok      fs 48.0 kHz bloco 1: saida finita (latencia 6)
ok      fs 48.0 kHz bloco 32: saida finita (latencia 6)
ok      fs 48.0 kHz bloco 480: saida finita (latencia 6)
ok      fs 48.0 kHz bloco 4096: saida finita (latencia 6)
ok      fs 88.2 kHz bloco 1: saida finita (latencia 6)
ok      fs 88.2 kHz bloco 32: saida finita (latencia 6)
ok      fs 88.2 kHz bloco 480: saida finita (latencia 6)
ok      fs 88.2 kHz bloco 4096: saida finita (latencia 6)
ok      fs 96.0 kHz bloco 1: saida finita (latencia 6)
ok      fs 96.0 kHz bloco 32: saida finita (latencia 6)
ok      fs 96.0 kHz bloco 480: saida finita (latencia 6)
ok      fs 96.0 kHz bloco 4096: saida finita (latencia 6)
ok      fs 192.0 kHz bloco 1: saida finita (latencia 6)
ok      fs 192.0 kHz bloco 32: saida finita (latencia 6)
ok      fs 192.0 kHz bloco 480: saida finita (latencia 6)
ok      fs 192.0 kHz bloco 4096: saida finita (latencia 6)
ok      automacao aleatoria de todos os parametros (3000 blocos, input/output ate +24 dB): saida finita, pico 36.1 dBFS
ok      alocacoes durante processBlock (200 blocos, ULTRA 16x, 6 bandas): 0

INFO    CPU ECO 1x    6 bandas: 5.4 % do tempo real (4 s de audio em 0.218 s)
INFO    CPU HQ 4x     6 bandas: 21.4 % do tempo real (4 s de audio em 0.856 s)
INFO    CPU HQ 8x     6 bandas: 44.3 % do tempo real (4 s de audio em 1.772 s)
INFO    CPU ULTRA 16x 6 bandas: 85.6 % do tempo real (4 s de audio em 3.425 s)

ok      estado guardado e reposto (preset Kizomba Vocal Heat + alteracao)
INFO    preset Init                   pico de saida -2.6 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Clean Analog Glue      pico de saida -5.1 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Vocal Silk             pico de saida -4.3 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Kizomba Vocal Heat     pico de saida -0.7 dBFS (entrada -6 dBFS), latencia 6
INFO    preset G-Zouk Bass Presence   pico de saida 1.8 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Afro Kick Grit         pico de saida -3.7 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Stereo Side Sparkle    pico de saida -4.6 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Mixbus Velvet          pico de saida -4.6 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Master Subtle Color    pico de saida -4.2 dBFS (entrada -6 dBFS), latencia 65
INFO    preset Drum Parallel Fire     pico de saida 1.5 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Low-Mid Sculpt         pico de saida -1.4 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Air Without Harshness  pico de saida -6.0 dBFS (entrada -6 dBFS), latencia 6
INFO    preset Mono-Safe Master       pico de saida -2.2 dBFS (entrada -6 dBFS), latencia 6
ok      todos os 150 presets de fabrica processam audio finito
ok      captura editor_perform.png (1200x740)
ok      captura editor_sculpt.png (1200x740)
ok      captura editor_engineer.png (1200x740)
ok      escala 75 %: 900x555
ok      escala 200 %: 2400x1480
ok      captura do esquema de cores 0
ok      captura do esquema de cores 1
ok      captura do esquema de cores 2
ok      captura do esquema de cores 3
ok      captura do esquema de cores 4
ok      captura em ingles (SCULPT)
ok      copiar/colar regulacoes entre instancias
INFO    captura editor_login.png

Resultado: 58 verificacoes ok, 0 falhadas
```

## 3. Validação nos sistemas-alvo (GitHub Actions — v1.0.0 execução 44640620819; v1.1.0 execução 44646006630, commit 761194e: mesmos resultados)

| Verificação | Resultado |
|---|---|
| macOS: Xcode 16.4, binário universal x86_64 + arm64 | ok |
| macOS: mínimo da fatia x86_64 (Catalina e anteriores) | **10.13** (a fatia arm64 é 11.0, o mínimo do Apple Silicon) |
| `auval -v aufx PxBx Prdx` | **AU VALIDATION SUCCEEDED** (avisos de arredondamento float no valor por omissão de parâmetros com escala logarítmica, sem efeito) |
| pluginval VST3 nível 5, macOS | **SUCCESS** |
| pluginval VST3 nível 5, Windows x64 | **SUCCESS** |
| Testes DSP + integração no Linux do CI | 31/31 e 51/51 ok |
| Instaladores | `BURNIX-1.0.0-macOS.pkg`, `BURNIX-1.0.0-Windows-Setup.exe` (+ .zip e SHA256SUMS) na release v1.0.0 |

CPU medido no runner Linux do CI (6 bandas, 48 kHz): ECO 6,8 % · HQ 4x 26,5 % · HQ 8x 52,2 % · ULTRA 16x 103 % de um núcleo.
Os binários não estão assinados com Developer ID / Authenticode (ver `Docs/SIGNING.md`); o instalador macOS remove a quarentena.


## Versão 1.3.0 — motor (20/11/2026)

Testes DSP (`build/test_dsp`) — reconstrução dos crossovers (agora com 6 e 12 dB/oitava) e verificações novas da 1.3:
```
[1] Reconstrucao do BandSplitter (modulo da soma, 20 Hz a 20 kHz)
    LR24  2 bandas: desvio maximo 0.0000 dB
    LR24  3 bandas: desvio maximo 0.0000 dB
    LR24  6 bandas: desvio maximo 0.0000 dB
    LR48  2 bandas: desvio maximo 0.0000 dB
    LR48  3 bandas: desvio maximo 0.0000 dB
    LR48  6 bandas: desvio maximo 0.0000 dB
    6 dB  2 bandas: desvio maximo 0.0000 dB
    6 dB  3 bandas: desvio maximo 0.0000 dB
    6 dB  6 bandas: desvio maximo 0.0000 dB
    LR12  2 bandas: desvio maximo 0.0000 dB
    LR12  3 bandas: desvio maximo 0.0000 dB
    LR12  6 bandas: desvio maximo 0.0000 dB

[13a] Motor legado (sessoes da 1.2) contra o TubeStage original da 1.2.0, amostra a amostra
    6 voicings x 5 regulacoes x 40000 amostras: diferenca maxima 6.22e-15

[13b] Antiderivadas em tabela (ADAA) contra a forma fechada
    erro maximo tanh/triode/velvet 5.1e-11 ; derivada do PENTODE 8.6e-09

[13c] Ganho compensado: nivel do fundamental de drive 1 a 10 (seno 1 kHz a -12 dBFS)
    EMBER          d1   +0.0  d3   +0.0  d5   +0.1  d7   +0.4  d10  +1.2  dB
    TRIODE BLOOM   d1   +0.0  d3   +0.0  d5   +0.1  d7   +0.2  d10  +0.9  dB
    PENTODE EDGE   d1   +0.0  d3   +0.0  d5   +0.1  d7   +0.4  d10  +1.3  dB
    IRON CORE      d1   -0.1  d3   -0.1  d5   -0.0  d7   +0.2  d10  +1.1  dB
    VELVET         d1   -0.0  d3   -0.0  d5   +0.0  d7   +0.1  d10  +0.6  dB
    FIRESTORM      d1   +0.0  d3   +0.0  d5   +0.2  d7   +0.8  d10  +1.9  dB

[13d] Curva progressiva: THD no meio do botao (drive 5, -12 dBFS)
    EMBER         THD  3.00 % no meio, 17.11 % no maximo
    TRIODE BLOOM  THD  5.64 % no meio, 17.91 % no maximo
    PENTODE EDGE  THD  2.62 % no meio, 18.18 % no maximo
    IRON CORE     THD  4.35 % no meio, 18.05 % no maximo
    VELVET        THD  1.69 % no meio,  9.34 % no maximo
    FIRESTORM     THD  6.50 % no meio, 31.58 % no maximo

Resultado: 51 verificacoes ok, 0 falhadas
```

Teste de integração (`BurnixSmoke`) — medições da 1.3 (aliasing, THD a drive 5, CPU, DELTA, sessões antigas, render, CLIP, senhas):
```
INFO    aliasing ECO 1x   : energia fora do fundamental = -25.7 dB relativo ao fundamental
INFO    aliasing HQ 4x    : energia fora do fundamental = -61.8 dB relativo ao fundamental
INFO    aliasing HQ 8x    : energia fora do fundamental = -84.0 dB relativo ao fundamental
INFO    aliasing ULTRA 16x: energia fora do fundamental = -91.1 dB relativo ao fundamental
INFO    THD EMBER         drive 5:   8.95 %   H2  -34.6 dB   H3  -21.2 dB   H5  -40.4 dB
INFO    THD TRIODE BLOOM  drive 5:  11.72 %   H2  -19.9 dB   H3  -24.6 dB   H5  -43.6 dB
INFO    THD PENTODE EDGE  drive 5:   9.69 %   H2 -240.0 dB   H3  -20.3 dB   H5  -40.0 dB
INFO    THD IRON CORE     drive 5:   9.69 %   H2  -32.9 dB   H3  -20.6 dB   H5  -41.3 dB
INFO    THD VELVET        drive 5:   5.06 %   H2  -41.0 dB   H3  -26.1 dB   H5  -47.4 dB
INFO    THD FIRESTORM     drive 5:  21.56 %   H2  -26.1 dB   H3  -13.8 dB   H5  -27.8 dB
INFO    CPU ECO 1x    6 bandas: 5.6 % do tempo real (4 s de audio em 0.223 s)
INFO    CPU HQ 4x     6 bandas: 22.0 % do tempo real (4 s de audio em 0.878 s)
INFO    CPU HQ 8x     6 bandas: 43.4 % do tempo real (4 s de audio em 1.736 s)
INFO    CPU ULTRA 16x 6 bandas: 105.2 % do tempo real (4 s de audio em 4.206 s)
ok      instancia nova usa o motor 1.3
ok      sessao legada continua legada depois de gravada de novo
ok      atualizar a sessao para o motor 1.3
ok      exportacao em ULTRA 16x com a mesma latencia (6 -> 6 amostras)
ok      CLIP 0 dBFS: pico de saida 0.00 dBFS com +12 dB de saida
INFO    DELTA: sem compensacao -12.1 dB, DELTA puro com ganho compensado -19.9 dB (relativo a entrada)
ok      Init com ganho compensado: o volume muda -0.50 dB (era +2.5 dB na 1.2)
ok      senha master partilhada reconhecida (entrada 0)
ok      senha errada recusada
```
Auditoria dos 150 presets (ruído rosa a −18 dBFS RMS, `BX_AUDIT=1`): variação de volume média −0,02 dB, entre −0,5 e +0,7 dB
(exceto *Delta Harmonics Only*, que só deixa passar os harmónicos: −19 dB); pico máximo −3,6 dBFS.

## Versão 2.0.0 — Creative (20/11/2026)

### DSP (`Tests/test_dsp.cpp`, 55 verificações ok, 0 falhadas)
```
[20a] HARMONIC DRAW: um seno a fundo de escala sai com os harmonicos desenhados
    desenhado H2 -20 dB, H3 -26 dB, H5 -34 dB ; medido H2 -20.2, H3 -28.4, H4 -132.3, H5 -35.4 dB

[20b] Dinamica por banda: ataques de um sinal em rajadas
    ataque acima do sustain: DYN 0 10.5 dB, DYN +80 % 21.2 dB (mais punch), DYN -80 % 6.8 dB (mais sustain)

[20c] Ataque por banda: +12 dB de drive nos ataques
    energia acrescentada nos ataques: 0 dB -> 10.1 ; +12 dB -> 80

[20d] A/B morph: a meio, os parametros numericos ficam a meio
    heat 5.0, drive 5.0, crossover 200 Hz

Resultado: 55 verificacoes ok, 0 falhadas
```
Nota: no DRAW os harmónicos mais altos saem 1–2 dB abaixo do desenhado (o seno de teste entra a 0,95 da escala, e Tₖ desce com
a amplitude); a tolerância do teste é ±2,5 dB. A barra do H4 desligada mede −132 dB.

### Integração (`Tests/smoke.cpp`, 87 verificações ok, 0 falhadas)
```
INFO    LFO -> HEAT: H3 varia 0.00 dB sem modulacao e 36.6 dB com LFO 2 Hz a 100 %
ok      modulacao LFO -> HEAT mexe o carater ao longo do tempo
ok      modulacao ativa visivel para a interface (ponto fantasma)
INFO    SIDECHAIN -> MIX -100 %: H3 -58.2 dB com o SC calado, -72.1 dB com o SC a tocar
ok      sidechain ligado e a controlar a saturacao
ok      modo TELEMOVEL corta o sub (60 Hz desce 56 dB)
INFO    A/B MORPH: H3 do A -45.9 dB, A com morph 100 % -69.2 dB, B sozinho -69.2 dB
ok      A/B morph leva o A ate ao B
ok      BURNIX Link ve 4 instancias e o HARM de cada uma
ok      assistente: 55 Hz sustentado = '808 & Sub' (graves 97 %)
ok      captura editor_perform.png (1200x740)
ok      captura editor_sculpt.png (1200x740)
ok      captura editor_engineer.png (1200x740)
ok      captura do navegador de presets
ok      captura da visita guiada
ok      captura do separador MOTION
ok      captura do modo DESENHAR
ok      captura do BURNIX Link
ok      captura do Assistente
ok      captura do Match de referencia
ok      teste A/B cego ativo (X/Y)
ok      revelar devolve o A e o B aos seus lados
```
CPU com 6 bandas, sem modulação (servidor de testes, 2 vCPU partilhados): ECO 1x 5,6 %, HQ 4x 21,5 %, HQ 8x 41,9 %, ULTRA 16x 88–94 %
(igual à 1.3: a 2.0 não pesa quando as funções novas estão desligadas).

### CLAP (`clap-validator` 0.3.2, Linux)
21 testes: 15 passaram, 5 não se aplicam, 1 falhou de propósito: `state-reproducibility-flush` compara o estado de duas instâncias
novas e o BURNIX grava uma semente diferente em cada uma (a tolerância analógica do DRIFT: cada instância é uma "unidade" diferente,
como dois aparelhos reais). Não afeta guardar/abrir sessões, que repõe a semente gravada (testes `state-reproducibility-basic` e
`-null-cookies` passaram).
