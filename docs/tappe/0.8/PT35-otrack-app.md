# [0.8-PT35] `app`: `F11`, interruttore e guide del tracciamento

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: test Qt e prova manuale
- **Dipende da:** PT34
- **Modulo:** `app`
- **Da leggere:** [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md),
  punti 1, 2 e 9; [ADR 0040](../../adr/0040-icone-a-ruoli.md) e
  [ICONE.md](../../ICONE.md) (tratto da 1 px); `app/src/cad_view.cpp` (come si
  disegna la guida polare), `app/src/builtin_actions.cpp`,
  `app/src/ui_builder.cpp` (barra di stato e scorciatoie); la scheda
  [PT34](PT34-otrack-cmd.md)

## File

- `app/src/cad_view.hpp`, `app/src/cad_view.cpp`
- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`, `app/src/main_window.cpp`
- l'icona dell'interruttore (a 16 e 32 px, una coppia) in `resources/`
- `docs/ICONE.md`
- `tests/app/…` (test Qt), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Interruttore:** un'azione `OTRACK` nella barra di stato accanto a *OSNAP*
   e *POLAR*, con la sua icona a ruoli, e la scorciatoia `F11`; scrive
   `<Object Snap Tracking on>` / `off` nella cronologia; segue lo stato del
   runner della sessione attiva.
2. **Acquisizione con la pausa:** in `CadView`, quando il puntatore resta fermo
   per 500 ms (un `QTimer` a colpo singolo, riavviato a ogni movimento) durante
   una richiesta di punto, si chiama `pointerHeld` del runner.
3. **Disegno:** un segno `+` (7 pixel) su ogni punto acquisito; le guide attive
   tratteggiate come quella polare, da un bordo all'altro della vista; il
   suggerimento vicino al puntatore `<Snap>: <distanza> < <angolo>°` dal punto
   acquisito della guida, come quello del polare.
4. **Icona:** con le regole di ICONE.md, registrata lì.

## Regole della tappa che valgono qui

- `builtin_actions.cpp`, `ui_builder.cpp`, `main_window.*` e `ICONE.md` sono in
  catena: PT35 → PT37 → PT38 → PT39 → PT45.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- Test Qt: `F11` cambia lo stato del runner e il pulsante; un puntatore fermo
  500 ms su un estremo con il tracciamento acceso acquisisce il punto (timer
  simulato); la vista chiede di disegnare il segno e la guida.
- Il controllo della geometria delle icone passa.

## Fatto quando

- Interruttore, `F11`, acquisizione con la pausa e guide funzionano; la CI è
  verde; la prova manuale (piano, sezione 7, punto 6) resta al responsabile.
