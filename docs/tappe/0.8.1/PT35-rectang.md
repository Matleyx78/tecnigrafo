# [0.8.1-PT35] `cmd`: `RECTANG`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT35]` (#…), agente claude
- **Dipende da:** RT35
- **Modulo:** `cmd`
- **Da leggere:** [DECISIONI.md](../../DECISIONI.md), 2026-11-21, «Testo
  formattato, griglia, snap e attributi della 0.8.1», punto 9;
  `cmd/src/commands/pline_command.cpp` (una `LWPOLYLINE` con larghezza, quota e
  spessore); la cronologia `autocad-rectang.txt` e i disegni
  ([dxf-di-prova.md](dxf-di-prova.md), «Che cosa si è visto nei disegni»)

## File

- `cmd/include/tg/cmd/commands/rectang_command.hpp`,
  `cmd/src/commands/rectang_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (file comune: le opzioni che `RECTANG`
  ricorda in `CommandDefaults`)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/rectang_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. `RECTANG` (alias `REC`, anche `RECTANGLE`), con le richieste della
   cronologia tradotte: `Current rectangle modes: …` quando un modo non è 0,
   `Specify first corner point or [Chamfer/Elevation/Fillet/Thickness/Width]:`,
   `Specify other corner point or [Area/Dimensions/Rotation]:` e le domande di
   ogni opzione.
2. Una `LWPOLYLINE` chiusa, con il gruppo 43 (larghezza costante) sempre
   scritto. **Vertici**, come AutoCAD: il primo angolo `P`, poi `P + dx · u`,
   l'angolo opposto, poi `P + dy · v`, con `u` e `v` le direzioni del
   rettangolo (x e y ruotate di *Rotation*) e `dx`, `dy` le quote con il segno.
   Il verso è antiorario solo se `dx` e `dy` hanno lo stesso segno: dato dal
   basso a destra all'alto a sinistra, o dall'alto a sinistra al basso a
   destra, è orario (seduta del 2026-11-22, casi g e h). Un angolo opposto
   **scritto** con *Rotation* dà `dx` e `dy` come proiezioni di `Q − P` su `u`
   e `v` (caso f). *Chamfer* con la prima distanza sul lato verticale e la
   seconda sull'orizzontale; *Fillet* con i bulge `tan(22,5°)`, anche insieme
   a *Width*; *Elevation* nel 38, *Thickness* nel 39 (negli `EntityExtras`,
   come `PLINE`); *Area* con lunghezza o larghezza (l'altra quota è area /
   quota data); *Dimensions*; *Rotation*, con l'angolo scritto o con *Pick
   points* (la direzione da un punto all'altro).
3. **Smusso o raccordo che non ci sta** (`2 · d1 > |dy|` o `2 · d2 > |dx|`
   per *Chamfer*, `2 · r > min(|dx|, |dy|)` per *Fillet*): nessun messaggio, e
   una `LWPOLYLINE` di **8 vertici** con ogni angolo ripetuto due volte, nello
   stesso ordine, senza bulge, come AutoCAD
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Esiti della seconda
   seduta in AutoCAD della 0.8.1», punto 5). All'uguaglianza vale la fascia
   della soglia (ADR 0013 di solid-space, punto 4).
4. Le opzioni restano per i `RECTANG` successivi della sessione, rotazione e
   quote comprese, come AutoCAD; `Current rectangle modes:` elenca solo quelle
   diverse da 0 (`Chamfer=25.0000 x 25.0000`, `Fillet=4.0000`,
   `Width=1.0000`, `Rotation=30`). **Valori proposti**, come AutoCAD (punto 6
   della stessa voce): la seconda distanza di cimatura propone la prima appena
   scritta; dopo un *Fillet* `r` le due distanze di cimatura proposte sono `r`
   e `r`; dopo una cimatura `a`, `b` il raggio proposto del *Fillet* è `a`
   (prova del 2026-11-22: dopo 3 e 5 propone 3, e con Invio il modo diventa
   `Fillet=3.0000`).
5. Un passo di annullamento `Rectang`.

## Regole della tappa che valgono qui

- Angoli digitati: [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2,
  punto 2. Direzioni calcolate dalla rotazione con `cardinalSinCos` (sezione 2,
  punto 1), così una rotazione di 90° dà vertici esatti.
- Il bulge del raccordo con `solid_space::robust` (determinismo, ADR 0030).

## Test richiesti

- I dieci rettangoli di `autocad-rectang` e gli otto di
  `autocad-rectang-opzioni` (*Pick points*, *Area* dalla larghezza, *Width* con
  *Fillet*, smusso e raccordo che non ci stanno, angolo opposto scritto con
  *Rotation*, i due orari); i valori proposti dopo *Fillet* e dopo *Chamfer*.

## Fatto quando

- I casi di RT35 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/rectang/opzioni/script.tgs
RECTANG
0,0
40,20
RECTANG
C
3
5
60,0
100,20
RECTANG
C
0
0
F
4
120,0
160,20
RECTANG
F
0
540,0
R
30
D
40
20
600,40
```

*Rotation* e *Dimensions* si danno alla seconda richiesta, dopo il primo angolo,
come nella cronologia (verbale della revisione del piano, [bloccante] 8).

Righe da controllare: le quattro `LWPOLYLINE` (vertici, bulge, `width`), i
messaggi `Current rectangle modes:`; i valori da `autocad-rectang.dxf` con
l'oracolo (`entity D <handle>`).

- **Strumenti:** righe normali su `main`; `entity` dell'oracolo per i valori.
  RT35 aggiunge `autocad-rectang` a `sorgenti.json`.
- **Dipendenze:** nessuna.
- **Fonte:** `autocad-rectang.dxf` e la sua cronologia, che provano la
  rotazione scritta e *Area* dalla lunghezza; `autocad-rectang-opzioni.dxf` e
  la sua cronologia (seduta del 2026-11-22, [dxf-di-prova.md](dxf-di-prova.md)):
  *Pick points*, *Area* dalla larghezza, *Width* con *Fillet*, smusso e
  raccordo che non ci stanno, *Rotation* con l'angolo opposto scritto, i due
  rettangoli orari. RT35 aggiunge anche `autocad-rectang-opzioni` a
  `sorgenti.json`.
- **Risposte condivise:** nessuna.
