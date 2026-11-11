# [0.8-PT49] `cmd`: comando `ELLIPSE`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT49]` (#…), agente claude
- **Dipende da:** PT48, RT49
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0052](../../adr/0052-ellisse.md), punto 5;
  `cmd/src/commands/arc_command.cpp`, `circle_command.cpp` (anteprima e
  opzioni); la scheda [PT48](PT48-ellisse.md); la cronologia di `ELLIPSE` copiata
  dal responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/ellipse_command.hpp`,
  `cmd/src/commands/ellipse_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/ellipse_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

Richieste dalla cronologia di AutoCAD LT; quelle attese:

1. `Specify axis endpoint of ellipse or [Arc/Center]:`
   - un punto: `Specify other endpoint of axis:` (i due punti sono l'asse, il
     centro è nel mezzo), poi `Specify distance to other axis or [Rotation]:`
     (la distanza è metà dell'altro asse; *Rotation*: `Specify rotation around
     major axis:`, rapporto = coseno dell'angolo, da 0 a 89,4° come AutoCAD);
   - *Center*: `Specify center of ellipse:`, `Specify endpoint of axis:`, poi la
     distanza come sopra;
   - *Arc*: le stesse richieste per l'ellisse, poi `Specify start angle or
     [Parameter]:` e `Specify end angle or [Parameter/Included angle]:`, senza
     *Parameter* (ADR 0052, punto 5): la richiesta lo omette; *Included angle*
     sì.
2. L'asse indicato può essere il minore: l'entità ha sempre come `majorAxis`
   l'asse più lungo, con il rapporto ≤ 1, e gli angoli si riportano di
   conseguenza, come AutoCAD.
3. Anteprima durante ogni richiesta; un passo di annullamento: `Ellipse`.
4. `PELLIPSE` non c'è (sempre 0).

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `ellipse_command_test.cpp`: dall'asse e dalla distanza; da *Center*; con
  *Rotation*; un arco con angoli e con *Included angle*; asse indicato più
  corto dell'altro; `U`.

## Fatto quando

- I casi di RT49 passano; la CI è verde.
