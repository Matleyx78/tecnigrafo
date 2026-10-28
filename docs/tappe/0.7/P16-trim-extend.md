# [0.7-P16] `cmd`: `TRIM`, `EXTEND`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R16]` (#…), agente gemini 4 Argon (calcoli
  delicati; valori da `autocad-taglio.dxf`)
- **Dipende da:** P1, P8, P14, R16
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1–6, 16 e 17; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti
  5, 6, 9 e 10; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 1, 3 e 8; [dxf-di-prova.md](dxf-di-prova.md) (`autocad-taglio.dxf`, e
  "Che cosa resta da controllare", punto 1);
  `core/include/tg/core/intersections.hpp` (P14)

## File

- `cmd/include/tg/cmd/commands/trim_extend_commands.hpp`,
  `cmd/src/commands/trim_extend_commands.cpp` (nuovi; anche `TRIMEXTENDMODE`)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::trimExtendMode`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/trim_extend_commands_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R16)

## Cosa fare

**Modo rapido** (`TRIMEXTENDMODE` = 1, il valore all'avvio). Prima il messaggio
`Current settings: Trim mode = Quick, Projection=UCS, Edge=None`, poi:

```
Select object to trim or shift-select to extend or [mOde]:
Select object to trim or shift-select to extend or [mOde/Undo]:
```

**Modo standard**, e sempre nella sequenza degli script. Prima i messaggi
`Current settings: Projection=UCS, Edge=None, Mode=Standard` e
`Select cutting edges ...`, poi:

```
Select objects or [mOde] <select all>:
Select object to trim or shift-select to extend or [mOde]:
Select object to trim or shift-select to extend or [mOde/Undo]:
```

Per `EXTEND` le domande sono le stesse, con *extend* e *trim* scambiati (e
`Select boundary edges ...` al posto di `Select cutting edges ...`).

- **Bordi:**
  - nel modo rapido, tutte le linee, gli archi, i cerchi e le polilinee visibili;
  - nel modo standard, quelli scelti (`PromptKind::Selection`); con una scelta
    vuota, tutti come nel modo rapido;
  - il testo non è mai un bordo;
  - **una polilinea è bordo di se stessa e i suoi vertici delimitano i pezzi**
    (ADR 0035, punto 2).
- **Oggetto da tagliare** (`PromptKind::Object`): con la regola dell'ADR 0035,
  punto 3. `intersect` e `stationOf` danno i bordi lungo l'entità; il pezzo da
  togliere è quello tra i due bordi più vicini alla stazione del clic.
- **Prolungare** (`EXTEND`, o `TRIM` con `Shift`): l'estremo più vicino al clic
  arriva al primo bordo lungo il prolungamento (ADR 0035, punto 4), con
  `Extension::Full` sull'entità da prolungare.
- **Entità coinvolte:**
  - modifica sul posto con `replace` (anche cerchio → arco);
  - il secondo pezzo con `addCopy` dall'originale;
  - nel modo rapido, un oggetto senza bordi si cancella;
  - nel modo standard un oggetto senza bordi dà il messaggio inglese di AutoCAD
    (da controllare, ADR 0035 punto 17) e non cambia.
- **Opzioni:** *mOde* chiede `Enter a trim mode option [Quick/Standard] <Quick>:`
  e cambia `TRIMEXTENDMODE`; *Undo* torna all'ultimo punto di ripristino.
- **`TRIMEXTENDMODE`** come comando: `Enter new value for TRIMEXTENDMODE <1>:`,
  0 o 1.
- **Annulla ed `Esc`:** ogni oggetto tagliato è un'operazione; il comando è un
  passo di annulla `Trim` o `Extend`; `Esc` conserva i tagli fatti (ADR 0015).

## Regole della tappa che valgono qui

- Prima di cominciare controlla nella issue l'esito della prova a mano del modo
  rapido ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da controllare",
  punto 1). Se smentisce l'ADR 0035, punto 2, fermati: si corregge prima l'ADR.
- `builtin_commands.cpp`, `command_descriptions.cpp`: prima di te P8, dopo di te
  P18.
- Nessuna decisione geometrica propria: tutte vengono da `intersections.hpp`.

## Test richiesti

- Ogni caso di `autocad-taglio.dxf` nei due modi.
- Taglio nel mezzo di linea, arco, segmento di polilinea; cerchio; polilinea
  chiusa; bordo tangente; oggetto senza bordi nei due modi; testo scelto.
- Prolungamento di linea, arco, polilinea aperta; cerchio (nulla); senza bordo.
- `Shift`, *mOde*, *Undo*, `Esc` dopo due tagli; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R16]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
