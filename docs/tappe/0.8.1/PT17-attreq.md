# [0.8.1-PT17] `cmd`: `ATTREQ` e le domande degli attributi in `-INSERT`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT17]` (#…), agente claude
- **Dipende da:** PT3, RT17
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 3; [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punto 6;
  `cmd/src/commands/insert_command.cpp`; la cronologia
  `autocad-attributi.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/src/commands/insert_command.cpp`,
  `cmd/include/tg/cmd/commands/insert_command.hpp`
- `cmd/include/tg/cmd/command.hpp` (file comune: `CommandDefaults::attributeRequest`)
- `cmd/src/builtin_commands.cpp` (file comune: il comando `ATTREQ`)
- `tests/cmd/insert_command_test.cpp`

## Cosa fare

1. `ATTREQ`, valore della sessione, 1 all'avvio (`Enter new value for ATTREQ
   <1>:`, 0 o 1).
2. Con 1, dopo la rotazione di `-INSERT`, le domande dell'ADR 0056, punto 3,
   nell'ordine della definizione: `Enter attribute values`, poi per ogni
   attributo non costante e non preimpostato `<richiesta> <predefinito>:`
   (Invio: il predefinito; la risposta accetta gli spazi); poi, se ci sono
   attributi *Verify*, `Verify attribute values` e di nuovo quelli, con il valore
   dato. `Esc` durante le domande annulla l'inserimento, come le altre domande
   di `-INSERT`.
3. Con 0 si fa come nella 0.8; anche con 1 un blocco senza attributi da
   chiedere non fa domande.

## Regole della tappa che valgono qui

- File comuni: `cmd/include/tg/cmd/command.hpp`, `cmd/src/builtin_commands.cpp`.

## Test richiesti

- I tre `-INSERT` di `autocad-attributi`: valori scritti, Invio a ogni domanda,
  *Verify*; `ATTREQ` 0; `Esc` durante le domande.

## Fatto quando

- I casi di RT17 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/insert/attributi-domande/script.tgs
OPEN
componente.dxf
-INSERT
COMPONENTE
0,-40
1

0
S10
230V
montaggio a quadro
K10
230V
```

`componente.dxf` è un DXF del caso scritto a mano con il blocco `COMPONENTE` e
i sei `ATTDEF` di `autocad-attributi` (nell'ordine della definizione: `SIGLA`,
`POSIZIONE`, `TENSIONE`, `COSTRUTTORE`, `NOTA`, `CODICE`). Righe da
controllare: in `messages:` `Enter attribute values`, le quattro domande
(`Sigla <S1>:` …), `Verify attribute values` e `Tensione <230V>:`; in
`entities:` gli `ATTRIB` con i valori.

- **Strumenti:** `OPEN` di un file del caso, righe normali, su `main`.
- **Dipendenze:** `-INSERT` su `main`; nessun `-ATTDEF` (il blocco viene dal
  file).
- **Fonte:** `autocad-attributi.txt`, i tre `-INSERT`.
- **Risposte condivise:** con `ATTREQ` 1 cambiano solo gli inserimenti di
  blocchi con attributi da chiedere: i cinque casi di oggi con `-INSERT`
  (`insert/errori-e-unita`, `insert/ridefinizione-esc`, `insert/file-e-uguale`,
  `insert/nome-opzioni`, `explode/inserimento-trasformato`) non ne hanno
  (nessun `ATTRIB` nei loro `atteso.txt`), quindi non cambiano.
