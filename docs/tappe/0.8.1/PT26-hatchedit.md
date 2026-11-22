# [0.8.1-PT26] `cmd`: `-HATCHEDIT` e contorni evidenziati in `-HATCH`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT26]` (#…), agente claude
- **Dipende da:** PT3, PT5, PT23, PT34, RT26
- **Modulo:** `cmd` (più gli id dei contorni trovati in `core`)
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punto 6;
  `cmd/src/commands/hatch_command.cpp`; le schede [PT23](PT23-proprieta-retini-quote.md)
  (`withPattern`, `withPatternOrigin`) e [PT34](PT34-last-previous.md)
  (`Command::highlighted()`); la cronologia `autocad-hatchedit.txt`
  ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/hatchedit_command.hpp`,
  `cmd/src/commands/hatchedit_command.cpp` (nuovi)
- `cmd/src/commands/hatch_command.cpp`, `cmd/include/tg/cmd/commands/hatch_command.hpp`
  (`highlighted()`, l'origine di `HPORIGIN`)
- `cmd/include/tg/cmd/command.hpp` (file comune: `CommandDefaults::hatchOrigin`)
- `core/include/tg/core/hatch_boundary.hpp`, `core/src/hatch_boundary.cpp`
  (`DetectedBoundary::sources`; verbale della revisione PT5, [bloccante] 11),
  `tests/core/hatch_boundary_test.cpp`
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/hatchedit_command_test.cpp` (nuovo), `tests/cmd/hatch_command_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **`-HATCHEDIT`** (alias `HE`, anche `HATCHEDIT`): `Select hatch object:`
   (un retino; altro: `The selected object must be an associative hatch
   object.` e la domanda si ripete, come la prima riga di AutoCAD nella seduta
   del 2026-11-22; la seconda, sui blocchi di tratteggio delle versioni vecchie,
   in Tecnigrafo non ha senso), poi `Enter hatch
   option [Style/Properties/Origin] <Properties>:`.
   - *Properties*: le tre domande di *Properties* di `-HATCH` (motivo, scala,
     angolo), proponendo i valori del retino, con lo stesso elenco di motivi
     (`ANSI31`, `NET`, `SOLID`; `withPattern` di PT23, con l'origine di
     `patternOrigin` degli extras e il `$MEASUREMENT` del disegno); aggiornano
     anche `HPNAME`, `HPSCALE`, `HPANG`, come AutoCAD, e il `-HATCH` successivo
     li propone. Scala e angolo di un motivo non generabile trasformano le sue
     famiglie in proporzione (`withPattern` di PT23;
     [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 9).
     **`HPNAME` si aggiorna solo con un nome che `-HATCH` sa creare** (`ANSI31`,
     `NET`, `SOLID`): con un motivo importato non generabile resta il nome di
     prima, mentre `HPSCALE` e `HPANG` si aggiornano comunque (decisione del
     responsabile sulla #927, [decisione] 6, presa uguale anche sulla #928,
     [decisione] 7). Esempio: `HPNAME` `ANSI31`, la
     scala di un `ANSI37` portata a 2 → il `-HATCH` successivo propone
     `<ANSI31>` e la scala 2.
   - *Style*: `Enter hatching style [Ignore/Outer/Normal] <Normal>:` (il valore
     del retino); aggiorna `HPISLANDDETECTION`.
   - *Origin*: `[Use current origin/Set new origin] <Use current origin>:`;
     *Set new origin*: `Specify origin point:` e `Store this as default origin?
     [Yes/No] <N>:` (con *Yes* `HPORIGIN` della sessione, che `-HATCH` usa per i
     retini nuovi). L'origine nuova sposta le famiglie con `withPatternOrigin`
     e scrive l'XDATA con `withPatternOriginXdata` (PT23); un motivo non
     generabile tiene gli scarti delle sue famiglie dall'origine (PT23,
     regola D10; #928, [decisione] 7). *Default to boundary
     extents* resta fuori.
   Il retino modificato si mette nel disegno con `Editor::replace(id, entity,
   extras)` di PT22, così famiglie ed XDATA si annullano insieme (verbale della
   revisione delle interfacce PT4, #927, [bloccante] 6).
   Un passo `Hatchedit`.
2. **`-HATCH` evidenzia** i contorni e le isole trovati, o gli oggetti scelti,
   fino a `Invio` o `Esc`: `highlighted()` dà gli identificativi delle curve
   (PT34); il disegno lo fa PT33. **Da dove vengono** ([bloccante] 11): con un
   punto interno, `core::DetectedBoundary` ha un campo nuovo
   `std::vector<EntityId> sources`, le entità delle curve dei suoi loop, ognuna
   **una volta** (più segmenti della stessa polilinea danno un id solo),
   nell'ordine in cui compaiono la prima volta; si ricava da
   `RegionLoop::origins` di solid-space, uno o due indici per pezzo, con la
   mappa candidati → `EntityId` che `hatch_boundary.cpp` ha già. Con *Select
   objects* gli id sono quelli dei contorni accettati
   (`ChosenContours::taken`). `highlighted()` torna vuoto quando il comando
   finisce o si annulla.

## Regole della tappa che valgono qui

- File comuni: `cmd/include/tg/cmd/command.hpp`, `cmd/src/builtin_commands.cpp`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- Revisione PT5: quattro `LINE` attorno a un punto e un cerchio dentro come
  isola, `-HATCH` con il punto interno: `highlighted()` dà i cinque id; una
  polilinea chiusa come contorno: il suo id una volta sola; *Select objects*
  su due contorni: i loro id; vuoto dopo `Invio` e dopo `Esc`
  ([bloccante] 11).
- Un retino `ANSI37` letto dal DXF: *Properties* con `ANSI37` confermato e
  scala 2 lascia `HPNAME` com'era (il `-HATCH` dopo propone `<ANSI31>`, se
  era quello) e cambia `HPSCALE`; *Set new origin* trasla le famiglie con i
  loro scarti, anche con il retino senza XDATA dell'origine ([decisione] 7).
- Le modifiche di `autocad-hatchedit` con `NET` al posto di `ANSI37`; *Store*
  con *Yes* e il `-HATCH` dopo; `HPNAME` proposto dal `-HATCH` dopo un
  `-HATCHEDIT` *Properties*; `HPNAME` che resta `ANSI31` dopo la scala di un
  `ANSI37` letto dal DXF, con `HPSCALE` aggiornata; *Origin* e `U`, che
  rimette famiglie e XDATA; `highlighted()` durante e dopo `-HATCH`; una linea
  scelta al posto del retino.

## Fatto quando

- I casi di RT26 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/hatchedit/opzioni/script.tgs
LINE
0,0
40,0
40,20
0,20
C
-HATCH
10,10

-HATCHEDIT
!click 10,10
P
NET
2
30
-HATCH
P

!esc
-HATCHEDIT
!click 10,10
O
S
245,5
N
!report
```

Righe da controllare: in `entities:` il `HATCH` con `pattern="NET"`,
`scale=2`, `angle=30` e `origin=245,5` (campo di PT23); in `messages:` le
richieste, e quella di `-HATCH` subito dopo con `<NET>` come predefinito;
`undo:` con due `Hatchedit`.

- **Strumenti:** righe normali, `!click`, `!report`, su `main`.
- **Dipendenze:** `-HATCH` su `main`; PT23 per le funzioni e il campo
  `origin=`.
- **Fonte:** `autocad-hatchedit.txt`; il messaggio su un oggetto che non è un
  retino dalla seduta del 2026-11-22 ([dxf-di-prova.md](dxf-di-prova.md)).
- **Risposte condivise:** nessuna: `highlighted()` non entra nel rapporto.
