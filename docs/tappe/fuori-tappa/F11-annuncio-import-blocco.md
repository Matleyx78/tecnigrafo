# [F11] `script`: annunciare l'importazione di un blocco

- **Tappa:** fuori tappa; sblocca RT19 della
  [tappa 0.8](../0.8-quote-retini-blocchi.md) (decisione del 2026-11-17,
  [DECISIONI.md](../../DECISIONI.md), «Importazione di un blocco annunciata
  prima di PT19»)
- **Issue:** #842
- **Difficoltà:** facile
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: cambia solo l'annuncio della definizione e
  delle domande, provato dai test di unità di `script`
- **Dipende da:** —
- **Modulo:** `script` (più il catalogo `docs/verifica/domande.md`)
- **Da leggere:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md),
  punto 4; [verifica/domande.md](../../verifica/domande.md),
  [verifica/casi.md](../../verifica/casi.md), punto 6 di «Scrivere un caso»;
  la scheda [PT19](../0.8/PT19-file-come-blocco.md)

## Perché

RT19 scrive i casi di PT19 prima del lavoro, ma il lettore non conosce ancora
la definizione `BLOCK-IMPORT` e la tabella non conosce le domande `import-*`.
Senza un annuncio i casi sarebbero non validi anche con `--pending`, invece di
restare in attesa del pacchetto che li realizza.

## File

- `docs/verifica/domande.md` (la sezione «Importazione di un file come blocco»)
- `script/src/entity_reader.cpp` (solo l'annuncio di `BLOCK-IMPORT`)
- `script/src/question_table.cpp` (solo le dieci righe annunciate)
- `tests/script/case_check_test.cpp`

## Cosa fare

1. Una riga `BLOCK-IMPORT …` ferma il file con `not implemented yet`: un caso
   che la usa è **in attesa**, non **non valido**.
2. In `kQuestions` entrano le dieci domande `import-*` del catalogo, con
   funzione `nullptr`; rispondono `error: question not implemented yet`.
3. PT19 realizza la definizione e le domande e toglie questi annunci.
4. La pull request di F11 porta su una riga a sé `Avvia dopo il merge: #700`.

## Test richiesti

- `case_check_test.cpp` prova che un caso in attesa con `BLOCK-IMPORT` e le
  dieci domande `import-*` resta in attesa, mentre una definizione o una
  domanda sconosciuta rende ancora il caso non valido.

## Fatto quando

- I casi di RT19 che usano la definizione e le domande sono in attesa e non
  non validi; la CI è verde.
