# [0.7-P2] `cmd`: richieste di oggetti, di numeri e di selezione; selezione scritta

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R2]` (#…), agente gemini (selezione scritta con
  `ERASE`). Le richieste di oggetto, di numero e di selezione nel comando non
  hanno ancora un comando che le usi: le provano i test di unità qui, e i casi
  dei comandi più avanti
- **Dipende da:** R2
- **Modulo:** `cmd` (più il nome del nuovo valore in `script` e `mcp`)
- **Da leggere:** [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 1–5; [ADR 0008](../../adr/0008-selezione-e-snap.md) punto 7 e
  "Conseguenze"; [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md);
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md);
  `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`,
  `cmd/include/tg/cmd/selection_set.hpp`

## File

- `cmd/include/tg/cmd/input.hpp`, `cmd/include/tg/cmd/command.hpp`,
  `cmd/include/tg/cmd/command_description.hpp`
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`,
  `cmd/src/command_registry.cpp`
- `script/tool/main.cpp`, `mcp/src/command_tools.cpp` (i nomi dei nuovi
  `ValueType`)
- `tests/cmd/command_runner_test.cpp`, `tests/cmd/selection_set_test.cpp` se
  serve
- `tests/verifica/in-attesa.txt` (togliere le righe di R2)

## Cosa fare

```cpp
// input.hpp
struct ObjectInput {
    core::EntityId id;
    core::Vec2 point;    ///< where the object was picked, in drawing coordinates
    bool shift{false};   ///< Shift held (PickAction::Remove)
    friend bool operator==(const ObjectInput&, const ObjectInput&) = default;
};
// Input gains ObjectInput, last.

// command.hpp
enum class PromptKind : std::uint8_t { Point, Distance, Angle, Text, Object, Number, Selection };

// command_description.hpp: ValueType gains Object and Selection (after Keyword).
```

- **Richiesta di oggetto** (ADR 0032, punto 1):
  - un `click` o un punto scritto (coordinate assolute o relative, come a una
    richiesta di punto) cercano l'entità con `pickEntity` entro 3 pixel;
  - se la trovano, il comando riceve `ObjectInput`, con `shift` per
    `PickAction::Remove`;
  - un'entità su un layer bloccato dà `1 was on a locked layer.`, e la domanda
    resta;
  - nessuna entità: la domanda resta, senza messaggio;
  - nessuna correzione di snap, orto o polare; `activeSnap()` è vuoto.
- **Richiesta di numero** (punto 2): un numero da solo diventa `ValueInput`
  (senza conversione in radianti); un clic o delle coordinate diventano
  `PointInput`, corretti dagli aiuti come alle richieste di distanza; il resto
  dà `Requires a number.` e la domanda resta.
- **Richiesta di selezione** (punto 3):
  - il runner fa i gesti di selezione dell'ADR 0008 e la selezione scritta, con
    i loro messaggi, e mostra il testo della domanda del comando;
  - Invio passa al comando una `SelectionInput` con gli identificativi scelti,
    anche vuota;
  - le parole chiave della domanda arrivano come `KeywordInput`;
  - `Esc` con un rettangolo aperto chiude il rettangolo, altrimenti annulla il
    comando, come alla domanda `Select objects:`.
- **Selezione scritta** (punto 4), alla domanda `Select objects:` e alle
  richieste di selezione:
  - un punto sceglie come un clic;
  - `W` e `C`, poi `Specify first corner:` e `Specify opposite corner:`, fanno
    finestra o intersezione in qualunque direzione;
  - `ALL` sceglie tutte le entità visibili e non su layer bloccati;
  - in maiuscolo o minuscolo, come le parole chiave. Il resto dà ancora
    `Invalid selection.`
- **Descrizioni:** `describe` (tecnigrafo-script e mcp) scrive `object` e
  `selection` per i nuovi `ValueType`; il registro li accetta come gli altri.

## Regole della tappa che valgono qui

- `command.hpp`, `command_runner.cpp` e `.hpp`: dopo di te li toccano P3, P7, P8
  e P10. Tieni ogni tipo di richiesta in una funzione separata.
- Testi in inglese, come AutoCAD.

## Test richiesti

- Un comando di prova con una richiesta di oggetto: clic su un'entità, nel vuoto,
  su un layer bloccato, su un layer spento; punto scritto; `Shift`; `Esc`; snap
  acceso che non sposta il clic.
- Un comando di prova con una richiesta di numero: `2`, `-1` (lo passa: decide il
  comando), `abc`, un clic.
- Un comando di prova con una richiesta di selezione: gesti, selezione scritta,
  Invio senza nulla, una parola chiave, `Esc`.
- Selezione scritta con `ERASE`: punto, `W`, `C`, `ALL`, layer bloccati, testo
  sbagliato.

## Fatto quando

- I casi di `[0.7-R2]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`; i test qui sopra esistono e passano.
- La CI è verde.
