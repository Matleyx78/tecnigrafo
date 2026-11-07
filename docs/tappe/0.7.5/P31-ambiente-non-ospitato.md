# [0.7.5-P31] `app`: un ambiente che il documento attivo non ospita apre una scheda nuova

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: con il solo disegno non succede; test Qt con
  l'ambiente e il tipo di prova di P30
- **Dipende da:** P30
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 7 e
  8; `app/src/environments.hpp`, `app/src/document_types.hpp` (P30),
  `app/src/main_window.cpp` (sessioni e `activate`), `app/src/document_tabs.hpp`
  (P29); `tests/app/environments_test.cpp` (P30, l'ambiente di prova)

## File

- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/environments.hpp`, `app/src/environments.cpp` (il menu degli ambienti)
- `app/src/document_types.hpp`, `app/src/document_types.cpp` (la scelta del tipo)
- `tests/app/environments_test.cpp`

## Cosa fare

1. L'**indicatore dell'ambiente** (P30) apre un menu con gli ambienti
   registrati; quello attivo ha la spunta.
2. Scegliere un ambiente che il tipo del documento attivo ospita: lo diventa
   (con un solo ambiente per tipo, oggi non succede mai che ce ne siano due).
3. Scegliere un ambiente che il documento attivo **non** ospita (strada A, ADR
   0041, punto 8):
   - il documento attivo **resta aperto** nella sua scheda, così com'è, con il
     comando in corso, se c'è: niente si chiude e niente chiede di salvare;
   - se un solo tipo registrato ospita quell'ambiente, si crea un documento nuovo
     di quel tipo in una **scheda nuova**, che diventa attiva nell'ambiente
     scelto;
   - se più tipi lo ospitano, un dialogo chiede quale, nell'ordine del registro;
     *Cancel* non fa niente;
   - se c'è già una scheda aperta di un tipo che lo ospita, si crea comunque un
     documento nuovo: l'utente ha chiesto un ambiente, non un documento
     esistente.
4. La pagina *Start* non è un documento: con lei attiva, scegliere un ambiente
   crea il documento nuovo come al punto 3.

## Regole della tappa che valgono qui

- `main_window.*`, `environments.*`, `document_types.*`: prima di te P30.
- Interfaccia in inglese.

## Test richiesti

In `environments_test.cpp`, con l'ambiente e il tipo di prova:

- con un disegno modificato attivo, scegliere l'ambiente `test` apre una scheda
  nuova del tipo `test`, attiva nell'ambiente `test`; la scheda del disegno è
  ancora lì, modificata, e nessuna richiesta di salvare è comparsa;
- con due tipi di prova che ospitano lo stesso ambiente, il dialogo chiede quale,
  e *Cancel* non apre niente;
- scegliere `drawing2d` con la scheda `test` attiva apre un disegno nuovo.

## Fatto quando

- Chiedere un ambiente che il documento attivo non ospita apre un documento del
  tipo giusto in una scheda nuova, senza chiudere niente.
- I test qui sopra esistono e passano.
- La CI è verde.
