# [0.7.5-P16] `app`: immissione dinamica vicino al cursore

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: il testo che entra nel runner lo provano i test
  di P15; qui test Qt e prova manuale
- **Dipende da:** P12, P15
- **Modulo:** `app`
- **Da leggere:** [ADR 0042](../../adr/0042-immissione-dinamica.md) per intero;
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1
  (aree 7 e 10); `cmd/include/tg/cmd/dynamic_input.hpp` (P15);
  `app/src/cad_view.hpp` e `.cpp`, `app/src/command_line.hpp` e `.cpp`,
  `app/src/main_window.cpp` (barra di stato, impostazioni),
  `app/src/builtin_actions.cpp`

## File

- `app/src/dynamic_input_box.hpp`, `app/src/dynamic_input_box.cpp` (nuovi: il
  riquadro)
- `app/src/cad_view.hpp`, `app/src/cad_view.cpp`
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/builtin_actions.cpp` (l'azione `aids.dynmode`)
- `resources/icons/aids-dynmode-16.svg`, `aids-dynmode-32.svg` (nuove, con le
  regole di ICONE.md)
- `app/CMakeLists.txt`
- `tests/app/dynamic_input_test.cpp` (nuovo), `tests/CMakeLists.txt`

## Cosa fare

1. **Riquadro** vicino al cursore, in basso a destra con uno scarto fisso, che
   non esce dalla vista: la richiesta (`DynamicInput::prompt`) e i campi, il
   campo attivo nel colore d'accento, gli altri come campi normali (ADR 0039,
   area 7). I campi non scritti mostrano `dynamicInputShown` e si aggiornano a
   ogni movimento.
2. **Tasti**, quando il riquadro è visibile e la riga di comando è vuota:
   - cifre, punto, segno meno, virgola vanno nel campo attivo e lo bloccano;
   - `Tab` passa al campo successivo, ciclicamente;
   - `Invio` manda `dynamicInputText` con `submitText` (la riga equivalente
     compare nella cronologia), poi i campi si svuotano;
   - `Esc` vale come nella riga di comando;
   - una lettera va alla riga di comando, come oggi, e il riquadro resta
     com'è.
3. **Interruttore** `aids.dynmode` nella barra di stato (dove P12 ha lasciato il
   posto tra gli interruttori) e `F12`, con `<Dynamic input on>` e `<Dynamic input
   off>` nella cronologia come per l'orto. Acceso per difetto, salvato nelle
   impostazioni dell'interfaccia; spento nelle sessioni di script.
4. Senza un comando in corso il riquadro non c'è.

## Regole della tappa che valgono qui

- `cad_view.*`, `main_window.*`: prima di te P12, dopo di te P18 e P28.
  `builtin_actions.cpp`: dopo di te P19.
- Interfaccia in inglese.

## Test richiesti

`dynamic_input_test.cpp` (test Qt, con `input_events.hpp`):

- `LINE`, primo punto con X e Y scritti e `Invio`, secondo punto con lunghezza,
  `Tab`, angolo e `Invio`: le entità sono quelle di `LINE 10,10` e `@50<30`, e la
  cronologia ha quelle righe;
- `Invio` senza campi scritti termina il comando;
- una lettera (`C` in `LINE`) va alla riga di comando;
- `F12` spegne il riquadro e lo scrive nella cronologia; spento, i tasti vanno
  alla riga di comando come oggi.

## Fatto quando

- Il riquadro funziona come nell'ADR 0042 e si accende e spegne.
- I test qui sopra esistono e passano.
- La CI è verde.
