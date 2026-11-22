# [0.8.1-PT14] `cmd`: `-ATTDEF` e `AFLAGS`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT14]` (#…), agente claude
- **Dipende da:** PT15, RT14
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 2; `cmd/src/commands/text_command.cpp`
  (le domande di `TEXT` da riusare); `cmd/src/commands/pedit_command.cpp`
  (`PEDITACCEPT`, un valore della sessione); la cronologia
  `autocad-attributi.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/attdef_command.hpp`,
  `cmd/src/commands/attdef_command.cpp` (nuovi)
- `cmd/src/commands/text_command.cpp`, `cmd/src/text.hpp` (solo se serve
  esporre le domande del punto iniziale)
- `cmd/include/tg/cmd/command.hpp` (file comune: `CommandDefaults::attributeFlags`)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/attdef_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. `AFLAGS` come valore della sessione (`Enter new value for AFLAGS <16>:`,
   da 0 a 31), che all'avvio vale 16.
2. `-ATTDEF` (alias `ATT`, anche `ATTDEF`) con le richieste dell'ADR 0056,
   punto 2, nell'ordine della cronologia; *Justify* e *Style* come `TEXT`;
   l'`ATTDEF` entra nel disegno sul layer e con le proprietà correnti, con
   `lockPosition` (PT15) dal modo. La richiesta dell'etichetta non è di testo
   (ADR 0014, punto 3): nella riga di comando lo spazio vale Invio, come in
   AutoCAD, che con `DUE PAROLE` prende `DUE` e passa `PAROLE` alla richiesta
   dopo (seduta del 2026-11-22). Etichetta vuota: `Tag cannot be null.` e la
   richiesta si ripete, come AutoCAD; una riga con uno spazio che arriva intera
   (script al livello 1): `Invalid tag name.` e la richiesta si ripete.
3. Un passo di annullamento `Attdef`.

## Regole della tappa che valgono qui

- File comuni: `cmd/include/tg/cmd/command.hpp` (un campo in
  `CommandDefaults`, e la riga di `CommandRunner::setDocumentFile` non serve:
  `defaults` si copia intero), `cmd/src/builtin_commands.cpp`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.
- `AFLAGS` all'avvio vale 16, come AutoCAD 2021
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
  della 0.8.1», punto 4).

## Test richiesti

- Ognuno dei sei `ATTDEF` di `autocad-attributi`; *Constant* senza richiesta;
  spazi nella richiesta e nel predefinito; etichetta vuota; etichetta con uno
  spazio al livello 1.

## Fatto quando

- I casi di RT14 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/attdef/modi/script.tgs
AFLAGS
0
-ATTDEF

CODICE
Codice componente
K1
2,15
2.5
0
AFLAGS
0
-ATTDEF
C

COSTRUTTORE
ACME
2,7
2.5
0
AFLAGS
0
-ATTDEF
L

SIGLA
Sigla
S1
J
MC
30,5
2.5
10
```

Righe da controllare: in `entities:` tre `ATTDEF` (l'ultimo con
`lock-position=yes`, `horizontal=center vertical=middle`), le richieste dei modi
in `messages:`, in `undo:` tre passi `Attdef`.

- **Strumenti:** righe normali, su `main`; il campo `lock-position` di PT15.
- **Dipendenze:** PT15 (il campo); nient'altro.
- **Fonte:** `autocad-attributi.txt` (richieste, ordine, *Constant*); la seduta
  del 2026-11-22 per l'etichetta vuota e lo spazio
  ([dxf-di-prova.md](dxf-di-prova.md)).
- **Risposte condivise:** nessuna.
