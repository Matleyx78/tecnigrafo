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
   scritto, vertici **antiorari** dal primo angolo dato, anche dato dall'alto a
   destra; *Chamfer* con la prima distanza sul lato verticale e la seconda
   sull'orizzontale; *Fillet* con i bulge `tan(22,5°)`; *Elevation* nel 38,
   *Thickness* nel 39 (negli `EntityExtras`, come `PLINE`); *Area* con
   lunghezza o larghezza; *Dimensions*; *Rotation* (con *Pick points*).
3. Le opzioni restano per i `RECTANG` successivi della sessione, rotazione e
   quote comprese, come AutoCAD.
4. Un passo di annullamento `Rectang`.

## Regole della tappa che valgono qui

- Angoli digitati: [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2,
  punto 2. Direzioni calcolate dalla rotazione con `cardinalSinCos` (sezione 2,
  punto 1), così una rotazione di 90° dà vertici esatti.
- Il bulge del raccordo con `solid_space::robust` (determinismo, ADR 0030).

## Test richiesti

- I dieci rettangoli di `autocad-rectang`; uno smusso più grande del lato
  (AutoCAD lo disegna senza smusso: dalla cronologia); larghezza e raccordo
  insieme.

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
  rotazione scritta e *Area* dalla lunghezza. *Pick points* di *Rotation*,
  *Area* calcolata dalla larghezza e *Width* insieme a *Fillet* sono **da
  registrare** nella seduta prima dell'apertura (sezione 5 del piano); fino ad
  allora RT35 non li scrive.
- **Risposte condivise:** nessuna.
