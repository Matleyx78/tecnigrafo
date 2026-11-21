# [0.8.1-PT36] `cmd`: `POLYGON`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT36]` (#…), agente claude
- **Dipende da:** RT36
- **Modulo:** `cmd`
- **Da leggere:** la cronologia `autocad-polygon.txt` e i disegni
  ([dxf-di-prova.md](dxf-di-prova.md), «Che cosa si è visto nei disegni»);
  la scheda sorella [PT35](PT35-rectang.md) (le regole sulle direzioni)

## File

- `cmd/include/tg/cmd/commands/polygon_command.hpp`,
  `cmd/src/commands/polygon_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (file comune: l'ultimo numero di lati)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/polygon_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. `POLYGON` (alias `POL`): `Enter number of sides <4>:` (da 3 a 1024; fuori:
   il messaggio della cronologia), `Specify center of polygon or [Edge]:`,
   `Enter an option [Inscribed in circle/Circumscribed about circle] <I>:`,
   `Specify radius of circle:`; *Edge*: `Specify first endpoint of edge:` e
   `Specify second endpoint of edge:`.
2. Una `LWPOLYLINE` chiusa, antioraria, come AutoCAD: con il raggio scritto il
   poligono poggia su un lato orizzontale (lati dispari: un vertice a 90°; pari:
   il primo vertice a 90° + 180°/n); *Circumscribed*: vertici a raggio
   `r / cos(180°/n)`; con il raggio dato da un punto, *Inscribed* mette il primo
   vertice nel punto, *Circumscribed* fa del punto il centro di un lato; *Edge*:
   il primo vertice è il primo punto, il poligono a sinistra del verso dello
   spigolo.
3. Numero di lati e opzione restano per il `POLYGON` successivo. Un passo
   `Polygon`.

## Regole della tappa che valgono qui

- **[D8]** come [PT35](PT35-rectang.md): direzioni con `cardinalSinCos`, se il
  responsabile lo decide; `cos` e `sin` di `solid_space::robust`.

## Test richiesti

- I nove poligoni di `autocad-polygon`; 1024 lati; 2 e 1025 rifiutati.

## Fatto quando

- I casi di RT36 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/polygon/varianti/script.tgs
POLYGON
6
0,0
I
20
POLYGON

50,0
C
20
POLYGON
5
E
100,0
120,0
```

Righe da controllare: le tre `LWPOLYLINE` con i vertici di AutoCAD (dal DXF con
l'oracolo); la richiesta `Enter number of sides <6>:` del secondo.

- **Strumenti:** righe normali su `main`; `entity` dell'oracolo. RT36
  aggiunge `autocad-polygon` a `sorgenti.json`.
- **Dipendenze:** nessuna.
- **Fonte:** `autocad-polygon.dxf` e la sua cronologia.
- **Risposte condivise:** nessuna.
