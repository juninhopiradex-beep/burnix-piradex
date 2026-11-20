# Changelog — BURNIX by PIRADEX

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
