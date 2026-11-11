# [0.8-PT10] `core` e `io`: entità `SOLID`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT10]` (#…), agente claude
- **Dipende da:** PT4 (revisione delle interfacce), PT8, PT9, RT10
- **Modulo:** `core` (più il codec in `io`)
- **Da leggere:** [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punti 1, 2 e 7; [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md),
  punti 3–5; la scheda [PT6](PT6-scheletro-core.md), punti 2 e 4;
  `core/src/solid.cpp`; `io/src/dxf_line.cpp` come esempio di codec;
  `io/src/dxf_entities.cpp`, `io/src/dxf_entity_common.hpp`

## File

- `core/include/tg/core/solid.hpp`, `core/src/solid.cpp`
- `io/src/dxf_solid.cpp` (nuovo), `io/CMakeLists.txt`,
  `io/src/dxf_entities.cpp`, `io/src/dxf_entity_common.hpp` (file comuni: una
  riga ciascuno)
- `tests/core/solid_test.cpp`, `tests/io/dxf_solid_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const Solid&)` restituisce `true`.
2. Funzioni di `Solid` (ADR 0044, punto 7): ingombro dei quattro vertici;
   geometria generata = la forma stessa con le proprietà effettive;
   trasformazione = i quattro vertici trasformati (una riflessione non cambia
   l'ordine: la figura `v0, v1, v3, v2` resta la stessa regione); grip sui
   quattro vertici (un vertice ripetuto, come nel caso da tre, ha un grip solo:
   il primo dei due), e spostare il grip del terzo vertice di un `SOLID` da tre
   sposta anche il quarto; snap estremo sui vertici distinti; distanza 0 dentro
   la figura (i due triangoli `v0 v1 v3` e `v0 v3 v2`), altrimenti la distanza
   dal bordo; finestra e intersezione con il rettangolo sui due triangoli.
3. Codec DXF `SOLID`: gruppi 10/20, 11/21, 12/22, 13/23 (13 mancante = 12),
   quota (30…), spessore (39) ed estrusione come dice l'ADR 0018, punto 5 (con
   `(0,0,-1)` la geometria arriva nel sistema del disegno, `x → −x`), il resto in
   `extras`. Un `SOLID` con tutti i vertici uguali è degenere e resta non
   gestito.
4. Il rapporto e le domande li scrive già PT9.

## Regole della tappa che valgono qui

- Tocchi solo i file di `Solid` e il codec: le visite comuni chiamano già le
  funzioni di `solid.cpp` (PT6).
- Niente funzioni trigonometriche di `<cmath>`.
- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `solid_test.cpp`: ingombro, trasformazione con riflessione, grip del caso da
  tre vertici, distanza dentro e fuori da un `SOLID` «a farfalla».
- `dxf_solid_test.cpp`: lettura e scrittura di un `SOLID` da tre e da quattro
  vertici, con estrusione `(0,0,-1)` e con gruppi sconosciuti conservati.

## Fatto quando

- I casi di RT10 passano (il loro file è tolto da `tests/verifica/in-attesa/`).
- La CI è verde.
