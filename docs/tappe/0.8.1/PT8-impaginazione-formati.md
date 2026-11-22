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
  punti 1, 3 e 4, con l'impaginazione decisa dal responsabile
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
  della 0.8.1», punto 1);
  [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md), punto 2;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 2;
  `core/src/mtext_layout.cpp`, `core/src/mtext.cpp`, `core/src/dimension.cpp`,
  `core/src/generated.cpp`; le schede sorelle [PT7](PT7-codici-di-formato.md) e
  [PT10](PT10-modello-di-modifica.md)

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
- `script/src/report.cpp` (solo `writeGeneratedItem`: un `FormattedText` si
  scrive con la sua variante e il suo riquadro, non con quelli del `Text` di
  `shapeEntity`; verbale della revisione delle interfacce PT4, #927,
  [bloccante] 3)
- `tests/core/mtext_layout_test.cpp`, `tests/core/generated_test.cpp`,
  `tests/core/document_query_test.cpp`, `tests/core/mtext_test.cpp`,
  `tests/script/report_test.cpp`

## Cosa fare

1. `GeneratedShape` ha due alternative nuove: `FormattedText { Text text;
   FontFace face; double tracking{1}; }` e `TextLine { Line line; }` (le righe
   di decorazione e delle frazioni). `tracking` è la spaziatura di `\T`:
   moltiplica gli **avanzamenti** dei caratteri, mentre il fattore di larghezza
   del `Text` (`\W`) allarga anche i glifi. Un pezzo resta un `Text` solo se è
   Regular **e** con spaziatura 1; ogni altro pezzo è un `FormattedText`
   (verbale della revisione delle interfacce PT4, #927, [bloccante] 1).

   ```cpp
   // The box of a formatted piece: textBox() of its Text, with the advances
   // of its face multiplied by its tracking and its ascent and descent.
   [[nodiscard]] std::array<Vec2, 4> formattedTextBox(const FormattedText& piece);
   ```

   Le visite comuni trattano un `TextLine` come la sua `Line` e un
   `FormattedText` con **la sua variante e la sua spaziatura**, mai con le
   misure Regular del suo `Text`: riquadro ed estensione, distanza, finestra e
   attraversamento (selezione) usano `formattedTextBox`; la trasformazione
   affine decompone il testo come oggi e tiene variante e spaziatura. Lo stesso
   vale per il disegno (`render`, il riquadro a zoom basso compreso, PT9).
   **Nessuna delle due dà snap di suo**: un `TextLine` non dà estremi né punti
   medi, non entra nelle intersezioni e non è una curva per *Vicino* (PT30),
   così una sottolineatura dentro un blocco o una quota non si confonde con uno
   spigolo (verbale della revisione del piano, [bloccante] 3); un
   `FormattedText` non dà i punti di un `Text` (nemmeno l'inserimento), dentro
   un blocco come in una quota. Per selezione e distanza invece un `TextLine`
   visibile conta come la sua `Line`.
2. **Snap del `MTEXT` con i pezzi** (#927, [bloccante] 2). `findSnap` e
   `appendSnapPoints` non passano da `shapeEntity` per queste due forme: le
   saltano. Dove oggi una riga di un `MTEXT` (diretto o nella grafica di una
   quota) rende candidato il suo punto d'inserimento alla distanza dal riquadro
   della riga (`considerRows`; ADR 0046, punto 2;
   [0.8-PT13](../0.8/PT13-mtext-core.md), punto 6), lo fa ogni pezzo: un `Text`
   con il suo riquadro, un `FormattedText` con `formattedTextBox`; un `TextLine`
   non rende candidato niente. Il punto d'inserimento del multiriga resta quello
   di oggi.
3. `mtext_layout::layout` lavora sui pezzi di PT7 con le regole dell'ADR 0054,
   punto 3 (larghezze per variante e spaziatura, a capo, prima linea di base,
   interlinea, frazioni, decorazioni), e `generate()` di `MTEXT` dà, per riga, i
   pezzi nell'ordine di lettura: `Text` per i pezzi Regular con spaziatura 1,
   `FormattedText` per gli altri, `TextLine` per le decorazioni e le linee delle
   frazioni, con il colore del pezzo nelle proprietà effettive.

   L'impaginazione dà anche, per PT10, **dove sta ogni unità di PT7** dopo gli
   a capo: per ogni pezzo impaginato la riga, il paragrafo e il pezzo di PT7 da
   cui nasce, l'indice della sua prima unità in `MTextRun::units`, il numero di
   unità e l'ascissa di ogni confine fra un'unità e l'altra (#927, [bloccante]
   4). Una frazione è un'unità sola, con due confini: prima e dopo.
4. **Frazione diagonale** (decisione del responsabile sulla #927, [decisione]
   3): in `\S<a>#<b>;` di altezza `h` e larghezza `w`, che parte da `x` in una
   riga con linea di base 0, la linea va da `(x, 0)` a `(x + w, 7/3 · h)`, la
   cima delle maiuscole del numeratore (`4/3 · h + h`); il suo punto medio è a
   `7/6 · h`, come la linea di `/`.
5. **Altezze delle righe e delle quote:** contano le altezze nominali dei
   pezzi (ADR 0054, punto 3, regole 3 e 4), anche per una frazione, che conta
   `h` anche se il numeratore sporge. Le quote misurano il testo con la stessa
   impaginazione (`dimension.cpp`): la larghezza è quella della riga più larga,
   l'altezza va dall'allineamento in alto alla linea di base dell'ultima riga,
   con la formula di oggi quando le altezze sono tutte uguali (#927, [decisione]
   3, applicazione della regola).
6. **Limite dell'a capo:** il confronto stretto `>` sulla larghezza è una
   soglia, con la fascia di una soglia (ADR 0013 di solid-space, punto 4;
   [revisione.md](../../agenti/revisione.md), «La fascia di una soglia»):
   quando la larghezza della riga sta entro `4 · ε · L` dalla larghezza di
   riferimento `L` (`ε` l'epsilon del `double`, l'errore di una somma di pochi
   avanzamenti già arrotondati), l'esito è libero ma deterministico. Un `MTEXT`
   con un solo pezzo Regular per paragrafo si misura come oggi
   (`textWidth` della riga per altezza e fattore), così la 0.8 non cambia;
   l'esempio del verbale (`A A` e `A {\C1;A}` di altezza 3 con la larghezza di
   riferimento `9903/1409`) sta nella fascia (#927, [decisione] 3,
   applicazione della regola).
7. **Altezze assolute sotto una similitudine**, come AutoCAD (prova in
   AutoCAD del responsabile, 2026-11-22, che corregge la decisione sulla #927,
   [decisione] 5): `transformed()` di un `MTEXT` cambia l'altezza dell'entità
   come oggi e **non tocca il valore**, nemmeno le altezze assolute `\H<n>;`.
   Esempio: altezza 2, `A{\H3;B}`, `SCALE` per 2 → altezza 4, valore
   `A{\H3;B}`, e B resta alto 3, come in AutoCAD (`LIST` prima e dopo). Lo
   stesso vale per `EXPLODE` di un blocco in scala uniforme, che passa da
   `transformed()`; dentro un inserimento non esploso, invece, i pezzi generati
   si trasformano con il blocco, come ogni forma.
8. Un `MTEXT` senza formati dà esattamente la geometria di oggi.
9. `generated` scrive `face=…` dopo `vertical=` per un `FormattedText` (campo
   annunciato da PT2), con il riquadro `~box` di `formattedTextBox`, e un
   `TextLine` come una `LINE`. Il formattatore effettivo è
   `detail::writeGeneratedItem` di `script/src/report.cpp`, che oggi passa da
   `shapeEntity` e perde la variante: per un `FormattedText` chiama una
   funzione di `new_entities_format.cpp` che scrive il `Text`, `face=` e le
   derivate dal riquadro con la variante (#927, [bloccante] 3). La spaziatura
   non ha un campo suo: si vede nelle posizioni e nel `~box`.
10. **Righe e testo mostrato** (verbale della revisione del piano, [bloccante]
   2): oggi `~lines` conta gli oggetti `Text` e `shownText` mette un a capo fra
   un `Text` e l'altro, quindi una riga con tre pezzi conterebbe tre righe e la
   quota perderebbe gli spazi fra i pezzi. In `core`:

   ```cpp
   // The text each layout row shows, pieces joined in reading order (a stack as
   // "top/bottom"), for an MTEXT or for the text of a dimension.
   [[nodiscard]] std::vector<std::u32string> shownLines(const Document&, const EntityRecord&);
   ```

   `~lines` di `MTEXT` è il numero di righe di `shownLines`, `~text` delle quote
   le righe unite con l'a capo. Le righe vuote dell'impaginazione (`A\P\PB\P`
   ne ha quattro, due vuote) restano per le posizioni e per il cursore, ma
   `shownLines` non le dà: `~lines` e `~text` contano solo le righe con testo,
   come oggi, così i casi della 0.8 senza formati non cambiano (punto 8; #927,
   [decisione] 2, applicazione della regola).

## Regole della tappa che valgono qui

- Regole e numeri dell'ADR 0054, punto 3: nostri, in unità dell'altezza, con le
  misure di Liberation Sans; da AutoCAD solo la struttura
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
  della 0.8.1», punto 1).
- A capo: «supererebbe» è il confronto stretto `>` (ADR 0046, punto 2).
- File comuni: `script/src/new_entities_format.cpp` (solo il campo `face=`; gli
  altri campi della tappa li aggiungono PT15 e PT23). `script/src/report.cpp` è
  della catena PT27 → PT28 → PT31: PT8 vi cambia solo `writeGeneratedItem`,
  come in un file comune, e chi arriva dopo si aggiorna.
- La posizione del cursore su un a capo automatico non è affare di PT8: la
  sceglie PT10 (#927, [decisione] 2).

## Test richiesti

- Un caso per ogni riga dell'ADR 0054, punto 1, con le posizioni dei pezzi;
  una parola con due formati che va a capo intera; frazioni dei tre tipi;
  quota lineare con testo sostitutivo `{\C1;<>} mm`; `shownLines` di una riga
  con tre pezzi; nessuno snap da un `TextLine`.
- **Variante e spaziatura nelle visite** (#927, [bloccante] 1): `A{\T2;AB}` e
  `A{\W2;AB}` danno riquadri diversi (con `\T` i glifi restano larghi come il
  Regular, cambiano gli avanzamenti); un `{\T2;A}` Regular è un
  `FormattedText`, non un `Text`; un punto sul bordo destro del riquadro di un
  pezzo Bold, fuori da quello Regular dello stesso testo, è a distanza 0 e la
  finestra che lo taglia lo attraversa.
- **Snap** (#927, [bloccante] 2): gli stessi pezzi (un Bold e un sottolineato)
  in un `MTEXT` diretto, in un blocco e nella grafica di una quota, con
  `modes=insertion`, `end`, `mid`, `int` e `nea`: il punto d'inserimento del
  multiriga c'è per il diretto e per la quota, nessun punto da un `TextLine` né
  dall'inserimento di un `FormattedText`, e un `TextLine` visibile conta per
  `distance` e la finestra.
- **Unità impaginate** (#927, [bloccante] 4): in `A\A1;B` le due unità e i tre
  confini; dopo un a capo automatico, la prima unità della seconda riga.
- **Frazione diagonale** (#927, [decisione] 3): `\S1#2;` di altezza 2.5 dà un
  `TextLine` da `(x, 0)` a `(x + w, 5.833333333333333)` rispetto alla linea di
  base; una quota con `\S1#2;` nel testo ha l'altezza nominale.
- **A capo nella fascia** (#927, [decisione] 3): `A A` di altezza 3 con la
  larghezza di riferimento `9903/1409` resta su una riga come oggi; `A {\C1;A}`
  con la stessa larghezza dà un esito deterministico (lo stesso in due
  chiamate).
- **Righe vuote** (#927, [decisione] 2): `A\P\PB\P` ha `shownLines` di due
  righe, `~lines=2`, e le due `Text` alle posizioni di oggi.
- **Similitudine** (prova in AutoCAD del 2026-11-22): `A{\H3;B}` alto 2
  scalato per 2 dà altezza 4 e lo stesso valore `A{\H3;B}`, con B alto 3;
  `EXPLODE` di un blocco in scala 2 con quel `MTEXT` dà lo stesso valore.

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
? snap 0.1,7.0 aperture=0.5 modes=insertion -> …
M = MTEXT insertion=0,30 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="{\\Lsotto} {\\fArial|b1;Grassetto}"
? snap 0.1,27.5 aperture=0.5 modes=insertion,endpoint,midpoint -> …
? snap 0.1,27.0 aperture=0.5 modes=end,mid,int,nea -> …

# casi/mtext/spaziatura/domande.tgq
T = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="A{\\T2;AB}"
? generated T -> …
W = MTEXT insertion=0,0 height=2.5 width=0 attachment=top-left rotation=0 style="Standard" value="A{\\W2;AB}"
? generated W -> …

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
  misure lette dai TTF in modo indipendente e con le regole dell'ADR 0054,
  punto 3; da AutoCAD solo la struttura (quali pezzi, quali formati, che cosa
  sta sulla stessa riga), mai le posizioni.
- **Risposte condivise** (controllate riga per riga): in
  `mtext/codici-semplici` cambiano `generated Groups` (`BC` sottolineato in un
  gruppo: tre pezzi e un `TextLine`), `generated Toggles` (sottolineato,
  sopralineato, barrato: tre pezzi e tre `TextLine`), `generated Arguments`
  (altezze, larghezze, obliqui, colori: un pezzo per formato) e `generated
  Stacks` (tre frazioni impilate invece del testo `1/2 3/4 5/6`); in
  `mtext/codici-letterali` nessuna riga (i codici malformati restano testo); in
  `dxf/mtext-proprieta` nessuna riga: la risposta di `entity D 51`
  (`Seconda {\H2x;grande} riga`) finisce con `~lines=2`, che resta 2 con
  `shownLines`, e non ha `~box`; la parola alta cambia solo la geometria
  generata, che quel caso non chiede (#927, [bloccante] 3).
  Nessun `~text` delle quote di oggi ha codici di formato (controllato: 22
  casi, solo `<>` e testi semplici). RT8 scrive `; cambia con #<issue di PT8>`
  prima delle righe che cambiano, con i valori nuovi, e mette i due casi nel suo
  file di `tests/verifica/in-attesa/`; PT8 li toglie.
