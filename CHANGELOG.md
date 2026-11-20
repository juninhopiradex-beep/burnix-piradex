# Changelog — BURNIX by PIRADEX

## 2.0.0 — 2026-11-20 (Creative)
- **MOTION** (4.º separador): 3 ligações de modulação com fontes ENV (entrada), LFO (seno/triângulo/quadrado/aleatório,
  0,05–20 Hz) e SIDECHAIN, e destinos HEAT, MORPH X, MORPH Y, MIX e DENSITY. O ponto do morph mostra a posição modulada ao vivo.
- **Sidechain**: entrada lateral estéreo opcional; a pista de controlo pode, por exemplo, fazer o BURNIX recuar quando o kick toca.
- **Dinâmica por banda (DYN)**: só nos ataques; + dá punch, − amacia. **ATAQUE por banda**: ±12 dB de drive só nos ataques.
- **Harmonic Draw**: novo voicing DRAW; desenham-se H2–H7 no ENGINEER (modo DESENHAR). **Capturas**: importar/exportar
  `.bnxcap` (ou CSV com H2..H7), por ex. medições feitas no ANALYSER de pré-amplificadores, consolas ou fita.
- **A/B MORPH**: leva o som atual até ao outro lado do A/B (automatizável: verso → refrão).
- **Modo TELEMÓVEL** (monitorização) e medidor **GRAVE AUDÍVEL** (quanto o grave passou a ouvir-se em colunas pequenas).
- **Ferramentas** (⚙): teste A/B cego (X/Y e revelar), BURNIX Link (saturação de cada instância da sessão), Assistente
  (ouve 10 s, identifica a fonte e sugere presets) e Match de referência (densidade banda a banda contra uma faixa → drive).
- **CLAP** (Bitwig, Reaper, FL Studio, Studio One 7) em macOS e Windows, além de VST3 e AU.
- Sessões e presets anteriores abrem iguais: os novos parâmetros começam desligados.

## 1.3.1 — 2026-11-20 (interface)
- PERFORM: medidores IN / OUT / HARM em dB (HARM = o que o BURNIX acrescenta), HEAT e MIX em botões grandes, grupos
  CARÁCTER e NÍVEL, GANHO COMPENSADO / AUTO GAIN / DELTA PURO / CLIP 0 dBFS, legenda dos anéis e dica legível.
- Descrição do preset por baixo do cabeçalho; A/B em dois segmentos e botão A→B (copiar).
- Navegador de presets: pesquisa, favoritos, categorias, intensidade, etiquetas, descrição, carregar no B para comparar.
- ENGINEER: harmónicos H2–H10 em barras (pares/ímpares), oversampling e qualidade ao exportar à vista, grupo ANALÓGICO
  (drift, curva do drive, ganho compensado) e SAÍDA E ESTÉREO.
- Visita guiada de 3 passos na primeira abertura (repete-se em ⚙ › Visita guiada). Contraste dos textos discretos ≥ 4,5:1.

## 1.3.0 — 2026-11-20 (motor)
- Ganho compensado (ligado por omissão): o drive já não sobe o volume (antes +10 a +16 dB no máximo; agora ≤ 1,9 dB).
  Os 150 presets ficam a menos de 0,7 dB do volume de entrada.
- Curva do drive progressiva: o meio do botão já satura (EMBER 1,7 % → 3,0 % de THD a −12 dBFS); FIRESTORM sem degrau.
- ADAA em todas as curvas (tabelas com Hermite): aliasing em HQ 4x de −35 para −62 dB, ECO 1x de −10 para −26 dB, e menos CPU
  (6 bandas: 4x 28,6 % → 21,4 %, 16x 96,5 % → 85,9 % no servidor de testes).
- IRON CORE: saturação de transformador nos graves com histerese leve. Drift analógico (bias dinâmico + tolerância L/R com semente).
- DELTA puro, qualidade ao exportar (8x/16x), CLIP 0 dBFS, crossovers 6 e 12 dB/oitava, Auto Gain com arranque rápido.
- Sessões e presets do utilizador gravados na 1.2 abrem com o motor antigo, idêntico amostra a amostra; atualização no menu ⚙ › Som.
- Textos dos presets com acentos e em inglês. Licenças: master comum dos plugins Piradex, beta até 30/06/2027 e licenças PCC.

## 1.2.0 — 2026-11-20
- 150 presets de fábrica em 19 categorias (Voz, Rap & Trap, Coros, Baixo, 808 & Sub, Kick, Snare & Clap, Bateria, Percussão,
  Guitarra, Teclas, Synths, Mix Bus, Mistura, Master, Estéreo, Afro & Angola, Criativo, Base). O menu mostra quantos há em
  cada categoria e traduz os nomes das categorias para inglês. Gerados por `tools/gen_presets.py`.
- Logótipo: "BURN" em vermelho-fogo e "by PIRADEX" encostado ao BURNIX (medido pela fonte, igual em qualquer escala).
- Corrigido "PortuguÃªs" no menu de idioma (texto UTF-8); estado da sessão (MESTRE/LICENÇA/BETA) traduzido em inglês.

## 1.1.0 — 2026-11-20
- Roda dentada ⚙ (Definições): idioma Português / English; 5 esquemas de cores (Órbita, Brasa, Gelo, Índigo seguro para
  daltonismo, Clara); escala da janela; animação 60 / 30 / 15 fps (económica para Macs antigos); espectro no anel on/off;
  dicas on/off; usar as regulações atuais ao abrir o BURNIX (predefinição própria) ou voltar à de fábrica; copiar e colar
  regulações entre instâncias; repor tudo. As preferências ficam guardadas para todas as instâncias.

## 1.0.0 — 2026-11-19
- Primeira versão: motor multibanda (1–6 bandas) com M/S por banda, 6 voicings originais e morph, filtro seletivo, INSERT/PARALLEL,
  2.º estágio, density, transient preserve, oversampling 1/4/8/16x (min-phase/linear), auto gain, delta, bypass real.
- Interface "Órbita v2" com PERFORM / SCULPT / ENGINEER, medição real de harmónicos, escala 75–200 %.
- 13 presets de fábrica, presets do utilizador, A/B, undo/redo.
- Login/licenças iguais aos outros plugins Piradex (período aberto até 31/12/2026, master offline, PCC online).
- Instaladores .pkg (macOS) e .exe (Windows) com o ecrã de boas-vindas Piradex.
