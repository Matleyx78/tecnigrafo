# [0.8.1-PT30] `core` e `script`: snap *Vicino*

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT30]` (#…), agente codex, fascia forte
  (priorità e pareggi)
- **Dipende da:** PT2, PT5, RT30
- **Modulo:** `core` (più `!osnap nea`, `modes=nea` e il tipo `Nearest` in
  `script`)
- **Da leggere:** [ADR 0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md),
  punti 1 e 2;
  [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md), punto 5;
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 4, «Selezione e
  snap a oggetto»; `core/include/tg/core/snap_points.hpp`,
  `core/src/document_query.cpp` (`findSnap`), `core/include/tg/core/proximity.hpp`

## File

- `core/include/tg/core/snap_points.hpp` (`nearest`, `extension`, `parallel`
  in `SnapModes`; `Nearest`, `Extension`, `Parallel` in `SnapKind`: gli ultimi
  due li usa PT31)
- `core/src/document_query.cpp`, `core/include/tg/core/document_query.hpp`
  (anche il filtro: le forme di testo della geometria generata non danno
  *Vicino*; `generated.cpp` lo cambia PT8 e qui non si tocca)
- `script/src/script.cpp` (`nea`; `ext` e `par` restano annunciati),
  `script/src/questions.cpp` (`modes=nea`), `script/src/text_format.cpp`
  (`Nearest`)
- `tests/core/document_query_test.cpp`, `tests/script/script_test.cpp`

## Cosa fare

1. I tre modi nuovi, spenti per difetto; solo `nearest` fa qualcosa in
   questo pacchetto.
2. In `findSnap`, il livello di *Vicino* fra gli snap di punto e centro e
   inserimento (ADR 0057, punto 2): fra le curve entro l'apertura dal puntatore
   (anche dentro la geometria generata di inserimenti e quote, e i lati di un
   `SOLID`), escluse le forme di testo, quella più vicina; il punto è
   `closestPoint` della curva o `nearest()` di solid-space per l'ellisse. Come
   AutoCAD nella seduta del 2026-11-22: sull'inserimento, sulla linea di quota e
   sul lato del `SOLID` *Vicino* aggancia.
3. Nessun *Vicino* su `TEXT`, `MTEXT`, `ATTDEF`, `ATTRIB`, sui testi generati e
   sui retini: come AutoCAD, che su `TEXT` e sul retino (con `OSNAPHATCH` 0, il
   valore di fabbrica) risponde «Punto non valido».

## Regole della tappa che valgono qui

- Priorità: [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 4,
  «Selezione e snap a oggetto», punto 2.
- Pareggi di *Vicino*: fra due curve l'id più alto, dentro una polilinea il
  primo segmento (stessa sezione, punto 2, «Pareggi di *Vicino*»).

## Test richiesti

- I sei oggetti di `autocad-vicino` (testo compreso); centro e *Vicino* sul
  bordo di un cerchio; un estremo entro l'apertura che vince su *Vicino*; due
  linee alla stessa distanza; i cinque oggetti di `autocad-vicino-altri`
  (inserimento, quota, `SOLID`, retino e `TEXT`).

## Fatto quando

- I casi di RT30 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/snap/vicino-autocad/script.tgs
OPEN
@sources/autocad-vicino
ERASE
!click 20,-29
!click 77.25,-29
!click 118.9,-29
!click 155,-29
!click 180.3,-29
!click 217.5,-29

!view 100,0 0.05 800x600
!osnap nea
LINE
!move 20,5.3
!report
!move 155,0.3
!report
!esc

# casi/snap/vicino-priorita/domande.tgq
C = CIRCLE center=0,0 radius=10
? snap 10.2,0.3 aperture=1 modes=cen,nea -> …
? snap 10.2,0.3 aperture=1 modes=cen -> …
L = LINE start=0,20 end=10,20
? snap 9.9,20.2 aperture=1 modes=end,nea -> …
T = TEXT position=0,40 alignment-point=0,40 height=2.5 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="AB"
? snap 1,41 aperture=1 modes=nea -> …
```

- **Strumenti:** `!osnap nea` e `modes=nea` annunciati da PT2; `ERASE`,
  `!click`, `!view`, `!move`, `!report`, `snap` su `main`. RT30 aggiunge
  `autocad-vicino` a `sorgenti.json`.
- **Geometria e risultati** (verbale della revisione del piano, [bloccante]
  7): il DXF ha anche le sei `LINE` disegnate da AutoCAD con `_nea`, che vanno
  da ogni oggetto fino a y = −30. Lo script le cancella per prime, scegliendole
  a y = −29, dove non c'è altro (i cerchi e gli archi scendono al più a −10):
  così il punto atteso non può venire da una di loro (la `LINE` che parte da
  155,0 darebbe proprio 155,0). Le coordinate attese sono i **punti iniziali**
  di quelle linee, letti con l'oracolo (`entity D <handle>`).
- **Dipendenze:** nessun comando nuovo.
- **Fonte:** `autocad-vicino.dxf` e la sua cronologia (punti su linea, arco,
  cerchio, polilinea, ellisse; niente sul `MTEXT`); la seduta della #920, punto
  5 (centro contro *Vicino*); `autocad-vicino-altri.dxf` e la sua cronologia
  (seduta del 2026-11-22, [dxf-di-prova.md](dxf-di-prova.md)): punti
  sull'inserimento (50,0), sulla linea di quota (88,10) e sul `SOLID` (140,0),
  «Punto non valido» sul retino e sul `TEXT`, con la stessa geometria di
  `autocad-vicino` (le `LINE` da y = −30). RT30 aggiunge anche
  `autocad-vicino-altri` a `sorgenti.json`.
- **Risposte condivise:** nessuna: con *Vicino* spento per difetto la domanda
  `snap` senza `nea` e `active snap:` non cambiano.
