# [0.8.1-PT8] `core`: impaginazione di `MTEXT` con i formati

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT8]` (#…), agente codex, fascia forte
  (calcoli delicati)
- **Dipende da:** PT4, PT6, PT7, RT8
- **Modulo:** `core` (più il campo `face=` delle risposte di `generated` in
  `script` e il ramo minimo di `render` che fa compilare la forma nuova)
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punti 1, 3 e 4, con la decisione **[D1]** presa dal responsabile;
  [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md), punto 2;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 2;
  `core/src/mtext_layout.cpp`, `core/src/mtext.cpp`, `core/src/dimension.cpp`,
  `core/src/generated.cpp`; le schede sorelle [PT7](PT7-codici-di-formato.md)
  e [PT10](PT10-modello-di-modifica.md)

## File

- `core/include/tg/core/generated.hpp`, `core/src/generated.cpp`
  (`FormattedText`)
- `core/src/mtext_layout.hpp`, `core/src/mtext_layout.cpp`, `core/src/mtext.cpp`
- `core/src/dimension.cpp` (le misure del testo passano dai pezzi)
- `core/src/explode.cpp` (`shapeEntity` di un `FormattedText` è il suo `Text`)
- `render/src/generated_drawing.cpp` (solo: un `FormattedText` si disegna come
  il suo `Text`, finché PT9 non porta le varianti)
- `script/src/new_entities_format.cpp` (`face=`)
- `tests/core/mtext_layout_test.cpp`, `tests/core/generated_test.cpp`,
  `tests/script/report_test.cpp`

## Cosa fare

1. `GeneratedShape` ha l'alternativa `FormattedText { Text text; FontFace face; }`;
   le visite comuni (riquadro, distanza, finestra, intersezione, trasformazione
   affine, che decompone il testo come oggi e tiene la variante) la trattano
   come il suo `Text`. Nessuno snap, come le righe di `MTEXT`.
2. `mtext_layout::layout` lavora sui pezzi di PT7 con le regole dell'ADR 0054,
   punto 3 (larghezze per variante, a capo, prima linea di base, interlinea,
   frazioni, decorazioni), e `generate()` di `MTEXT` dà, per riga, i pezzi
   nell'ordine di lettura: `Text` per i pezzi Regular, `FormattedText` per gli
   altri, `Line` per le decorazioni e le linee delle frazioni, con il colore del
   pezzo nelle proprietà effettive.
3. Un `MTEXT` senza formati dà esattamente la geometria di oggi.
4. Le quote misurano il testo con la stessa impaginazione (`dimension.cpp`).
5. `generated` scrive `face=…` dopo `vertical=` per un `FormattedText` (campo
   annunciato da PT2), e le `LINE` delle decorazioni come le altre forme.

## Regole della tappa che valgono qui

- **[D1]**: regole e numeri dell'ADR 0054, punto 3, come li ha decisi il
  responsabile prima dell'apertura.
- A capo: «supererebbe» è il confronto stretto `>` (ADR 0046, punto 2).
- File comuni: `script/src/new_entities_format.cpp` (solo il campo `face=`; gli
  altri campi della tappa li aggiungono PT15 e PT23).

## Test richiesti

- Un caso per ogni riga dell'ADR 0054, punto 1, con le posizioni dei pezzi;
  una parola con due formati che va a capo intera; frazioni dei tre tipi;
  quota lineare con testo sostitutivo `{\C1;<>} mm`.

## Fatto quando

- I casi di RT8 passano, i casi di oggi senza formati restano verdi; la CI è
  verde.

## Prova a secco dei riferimenti

```text
# casi/mtext/formati/domande.tgq
B = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="Normale {\\fArial|b1|i0|c0|p34;grassetto} normale"
? generated B -> …
H = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="Normale {\\H2x;doppia} normale"
? generated H -> …
S = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="Frazione \\S1/2; fine"
? generated S -> …
O = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="Normale {\\Osopralineato} normale"
? generated O -> …
C = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="{\\c16744448;blu}"
? generated C -> …

# casi/mtext/formati-autocad/domande.tgq (struttura da AutoCAD)
D = DXF source=autocad-mtext-formati
? count D -> …
? entity D <handle del TEXT esploso «grassetto»> -> …
```

- **Strumenti:** `generated` (0.8) con il campo `face=` annunciato da PT2;
  `count` ed `entity` dell'oracolo. `RT8` aggiunge `autocad-mtext-formati` a
  `sorgenti.json` (commit e SHA-256 in
  [dxf-di-prova.md](dxf-di-prova.md)).
- **Dipendenze:** nessun comando; i valori attesi dei pezzi si calcolano con le
  misure di PT6 e le regole di [D1]; da AutoCAD, secondo [D1], la struttura o
  anche le posizioni.
- **Risposte condivise:** cambia `generated` dei casi di oggi con codici di
  formato: `mtext/codici-semplici` (`Toggles`, `Arguments`, `Stacks`),
  `mtext/codici-letterali` e `dxf/mtext-proprieta`. RT8 vi scrive `; cambia
  con #<issue di PT8>` prima delle righe che cambiano, con i valori nuovi, e li
  mette nel suo file di `tests/verifica/in-attesa/`; PT8 li toglie.
