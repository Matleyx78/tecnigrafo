# [0.5-P18] Manuale e architettura

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P16, P17
- **Modulo:** documentazione
- **Da leggere:** [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punto 1 (strati),
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punto 3,
  [ADR 0019](../../adr/0019-archivi-zip.md),
  [ADR 0021](../../adr/0021-comandi-dei-file.md); `docs/manuale/comandi.md`
  e `docs/ARCHITETTURA.md` per lo stile; le schede di
  [P15](P15-salvataggio.md), [P16](P16-apertura.md) e
  [P17](P17-menu-file.md) per domande e messaggi esatti

## File

- `docs/manuale/comandi.md`
- `docs/ARCHITETTURA.md`

## Cosa fare

### `docs/manuale/comandi.md`

Una sezione nuova, **"File"**, prima di "Selezione", con lo stesso tono del
resto del manuale (seconda persona, esempi di ciò che si scrive e di ciò che
risponde il programma):

- `NEW`, `OPEN`, `QSAVE`, `SAVE`, `SAVEAS`: a cosa servono, le domande
  (copiate esatte dalle schede), le scorciatoie e il menu File, le finestre
  di dialogo e quando non compaiono (negli script);
- la differenza tra `SAVE` e `SAVEAS`, come in AutoCAD;
- la domanda sulle modifiche non salvate, anche alla chiusura della
  finestra;
- formati: `.cadx` (formato di Tecnigrafo, con il rimando a
  `docs/formati/cadx.md`) e DXF; quale versione si scrive (ADR 0017, punto
  2);
- il messaggio delle entità non gestite: cosa vuol dire, che non si perdono
  e che si riscrivono identiche;
- `INSUNITS` con la tabella dei valori più usati (0 senza unità, 1 pollici,
  4 millimetri, 5 centimetri, 6 metri);
- file recenti.

### `docs/ARCHITETTURA.md`

- Il modulo `io` nella figura degli strati e nella tabella dei moduli (cosa
  fa, da cosa dipende: `core`, e zlib in privato), e `cmd` che dipende da
  `io`.
- `third_party/` nella sezione "Dove si trova cosa": solo zlib, perché, e il
  rimando all'ADR 0019.
- `tests/fuzz/` e `tests/verifica/oracolo/` nella stessa sezione.
- Gli ADR 0017–0022 nella sezione "Decisioni architetturali".

## Regole della tappa che valgono qui

- Le domande e i messaggi nel manuale sono quelli del programma, in inglese,
  copiati dalle schede: non tradurli né riformularli.
- Nessun file fuori dai due elencati (il catalogo di parità e le note di
  rilascio li aggiorna la pull request di chiusura).

## Test richiesti

Nessuno: è documentazione. Controlla che ogni domanda citata nel manuale
compaia identica nel codice (`cmd/src/commands/file_commands.cpp`).

## Fatto quando

- Il manuale descrive tutti i comandi dei file e `INSUNITS`.
- `docs/ARCHITETTURA.md` descrive `io`, `third_party/`, `tests/fuzz/` e
  l'oracolo, e cita gli ADR nuovi.
- La CI è verde.
