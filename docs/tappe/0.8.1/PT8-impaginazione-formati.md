# [0.8.1-PT8] `core`: impaginazione di `MTEXT` con i formati

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT8]` (#…), agente codex, fascia forte
  (calcoli delicati)
- **Dipende da:** PT4, PT6, PT7, PT30, RT8
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
- `core/src/explode.cpp` (`shapeEntity` di un `FormattedText` è il suo `Text`,
  di un `TextLine` la sua `Line`)
- `core/src/snap_points.cpp`, `core/src/document_query.cpp` (un `TextLine` non
  dà snap e non è una curva per le intersezioni né per *Vicino*)
- `core/include/tg/core/mtext.hpp` (`shownLines`)
- `render/src/generated_drawing.cpp` (solo: un `FormattedText` si disegna come
  il suo `Text`, finché PT9 non porta le varianti)
- `script/src/new_entities_format.cpp` (`face=`, `~lines`, `~text`)
- `tests/core/mtext_layout_test.cpp`, `tests/core/generated_test.cpp`,
  `tests/script/report_test.cpp`

## Cosa fare

1. `GeneratedShape` ha due alternative nuove: `FormattedText { Text text;
   FontFace face; }` e `TextLine { Line line; }` (le righe di decorazione e delle
   frazioni). Le visite comuni (riquadro, distanza, finestra, trasformazione
   affine, che decompone il testo come oggi e tiene la variante) le trattano
   come il loro `Text` e la loro `Line`. **Nessuna delle due dà snap**: un
   `TextLine` non dà estremi né punti medi, non entra nelle intersezioni e non è
   una curva per *Vicino* (PT30), così una sottolineatura dentro un blocco o una
   quota non si confonde con uno spigolo (verbale della revisione del piano,
   [bloccante] 3).
2. `mtext_layout::layout` lavora sui pezzi di PT7 con le regole dell'ADR 0054,
   punto 3 (larghezze per variante, a capo, prima linea di base, interlinea,
   frazioni, decorazioni), e `generate()` di `MTEXT` dà, per riga, i pezzi
   nell'ordine di lettura: `Text` per i pezzi Regular, `FormattedText` per gli
   altri, `TextLine` per le decorazioni e le linee delle frazioni, con il colore
   del pezzo nelle proprietà effettive.
3. Un `MTEXT` senza formati dà esattamente la geometria di oggi.
4. Le quote misurano il testo con la stessa impaginazione (`dimension.cpp`).
5. `generated` scrive `face=…` dopo `vertical=` per un `FormattedText` (campo
   annunciato da PT2), e un `TextLine` come una `LINE`.
6. **Righe e testo mostrato** (verbale della revisione del piano, [bloccante]
   2): oggi `~lines` conta gli oggetti `Text` e `shownText` mette un a capo fra
   un `Text` e l'altro, quindi una riga con tre pezzi conterebbe tre righe e la
   quota perderebbe gli spazi fra i pezzi. In `core`:

   ```cpp
   // The text each layout row shows, pieces joined in reading order (a stack as
   // "top/bottom"), for an MTEXT or for the text of a dimension.
   [[nodiscard]] std::vector<std::u32string> shownLines(const Document&, const EntityRecord&);
   ```

   `~lines` di `MTEXT` è il numero di righe di `shownLines`, `~text` delle quote
   le righe unite con l'a capo.

## Regole della tappa che valgono qui

- **[D1]**: regole e numeri dell'ADR 0054, punto 3, come li ha decisi il
  responsabile prima dell'apertura.
- A capo: «supererebbe» è il confronto stretto `>` (ADR 0046, punto 2).
- File comuni: `script/src/new_entities_format.cpp` (solo il campo `face=`; gli
  altri campi della tappa li aggiungono PT15 e PT23).

## Test richiesti

- Un caso per ogni riga dell'ADR 0054, punto 1, con le posizioni dei pezzi;
  una parola con due formati che va a capo intera; frazioni dei tre tipi;
  quota lineare con testo sostitutivo `{\C1;<>} mm`; `shownLines` di una riga
  con tre pezzi; nessuno snap da un `TextLine`.

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

# casi/mtext/decorazioni-senza-snap/domande.tgq
!block "TARGA" base=0,0
T = MTEXT insertion=0,10 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="{\\Lsottolineato}"
L = LINE start=0,0 end=40,0
!endblock
I = INSERT block="TARGA" insertion=0,0 scale=1,1 rotation=0
? snap 0.1,7.0 aperture=0.5 modes=end,mid,int,nea -> …
? snap 0.1,0.1 aperture=0.5 modes=end,mid,int,nea -> …

# casi/mtext/formati-autocad/domande.tgq (struttura da AutoCAD)
D = DXF source=autocad-mtext-formati
? count D -> …
? entity D <handle del TEXT esploso «grassetto»> -> …
```

- **Strumenti:** `generated` (0.8) con il campo `face=` annunciato da PT2;
  `snap` con `modes=nea` (annunciato da PT2, realizzato da PT30, da cui PT8
  dipende): il primo punto, vicino alla sola sottolineatura, dà `none`, il
  secondo l'estremo della `LINE`;
  fontTools sui quattro TTF di `resources/fonts` (già nel repository), come
  RT6: RT8 calcola le larghezze da solo e non aspetta PT6;
  `count` ed `entity` dell'oracolo. `RT8` aggiunge `autocad-mtext-formati` a
  `sorgenti.json` (commit e SHA-256 in
  [dxf-di-prova.md](dxf-di-prova.md)).
- **Dipendenze:** nessun comando; i valori attesi dei pezzi si calcolano con le
  misure lette dai TTF in modo indipendente e con le regole di [D1]; da
  AutoCAD, secondo [D1], la struttura o anche le posizioni.
- **Risposte condivise** (controllate riga per riga): in
  `mtext/codici-semplici` cambiano `generated Groups` (`BC` sottolineato in un
  gruppo: tre pezzi e un `TextLine`), `generated Toggles` (sottolineato,
  sopralineato, barrato: tre pezzi e tre `TextLine`), `generated Arguments`
  (altezze, larghezze, obliqui, colori: un pezzo per formato) e `generated
  Stacks` (tre frazioni impilate invece del testo `1/2 3/4 5/6`); in
  `mtext/codici-letterali` nessuna riga (i codici malformati restano testo); in
  `dxf/mtext-proprieta` la riga `entity D 51` (`Seconda {\H2x;grande} riga`:
  `~lines=2` resta 2 con `shownLines`, ma `~box` cambia per la parola alta).
  Nessun `~text` delle quote di oggi ha codici di formato (controllato: 22
  casi, solo `<>` e testi semplici). RT8 scrive `; cambia con #<issue di PT8>`
  prima delle righe che cambiano, con i valori nuovi, e mette i due casi nel suo
  file di `tests/verifica/in-attesa/`; PT8 li toglie.
