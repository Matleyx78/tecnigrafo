# Come contribuire

Grazie dell'interesse. Tecnigrafo cresce per **tappe** pianificate dal
responsabile del progetto: chi contribuisce non deve inventare cosa fare, ma
sceglie un compito già definito nei dettagli. Leggi tutto questo file prima di
iniziare: le pull request che non lo rispettano vengono chiuse senza revisione.

## In cinque minuti

1. Apri [docs/ROADMAP.md](docs/ROADMAP.md) e guarda quale tappa è **aperta**.
2. Apri il documento della tappa in `docs/tappe/`: contiene i **pacchetti di
   lavoro**. Ognuno dice quali file toccare, quali interfacce rispettare, quali
   test scrivere e quando è "fatto".
3. Su GitHub, nel milestone della tappa, scegli la issue di un pacchetto libero.
   Se è la prima volta, cerca l'etichetta `buon-primo-contributo`.
4. Scrivi nella issue che vuoi occupartene e **aspetta l'assegnazione**.
5. Lavora, apri la pull request, aspetta la revisione.

Come si leggono i riferimenti (`[0.2-P4]`, `#5`, `Closes #5`) e come lavorano più
persone o agenti in parallelo: [docs/LAVORO-IN-PARALLELO.md](docs/LAVORO-IN-PARALLELO.md).

## Il percorso di una modifica

1. **Assegnazione.** Un pacchetto è assegnato a una sola persona alla volta. Se
   non dai notizie per 14 giorni, il pacchetto torna libero.
2. **Ramo.** Crea un ramo dal nome `pX.Y-NN-descrizione` (esempio:
   `p0.2-04-coordinate-parser`).
3. **Pull request.** Una PR realizza **un solo pacchetto** e cita la sua issue
   (`Closes #123`). Niente modifiche "già che c'ero": se trovi altro da sistemare,
   apri una issue a parte.
4. **CI verde.** Formattazione, compilazione senza avvisi su tre sistemi, analisi
   statica e test devono passare. Una PR con la CI rossa non viene revisionata.
5. **Revisione.** Almeno un'approvazione del responsabile o di un revisore
   dell'area. Le osservazioni si risolvono con nuovi commit.
6. **Merge.** Lo fa il revisore, con *squash* in un unico commit.

### Cosa si può fare senza un pacchetto

- Correggere un bug di una versione pubblicata: apri prima una issue *Bug*.
- Migliorare documentazione e test esistenti.
- Proporre un'idea per le tappe future con una issue *Idea*: si discute, ma non si
  scrive codice finché il responsabile non la inserisce in una tappa.

Nient'altro, anche se il codice è ben scritto.

### Se il piano di un pacchetto è sbagliato

Capita. Non aggirarlo nel codice: scrivi nella issue cosa non funziona e proponi
una correzione. Il responsabile aggiorna il piano della tappa e la sezione
"Modifiche al piano".

## Regole tecniche obbligatorie

### Architettura

- Rispetta gli ADR in `docs/adr/` e la mappa in
  [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md). Se non sei d'accordo con un ADR,
  proponi un nuovo ADR; non aggirarlo nel codice.
- Dipendenze a senso unico tra i moduli. Solo `app` include Qt.
- Nessuna nuova dipendenza esterna senza un ADR approvato.

### Codice

- C++20. Stile descritto in [docs/CODING_STYLE.md](docs/CODING_STYLE.md).
- Codice, commenti, nomi e messaggi di commit in **inglese**, per permettere a
  chiunque nel mondo di leggerli. La documentazione per gli utenti può essere
  in più lingue.
- Formattazione con `clang-format` 18 usando il `.clang-format` del progetto.
- Nessun avviso del compilatore e nessuna segnalazione di `clang-tidy`.
- Ogni file sorgente inizia con l'intestazione SPDX della licenza.

### Test

- Ogni modifica a `core`, `render`, `edit` e `cmd` arriva con i suoi test. Una
  correzione di bug arriva con il test che lo riproduce.
- I test non dipendono dall'ordine di esecuzione, dalla rete o dall'ora.

### Commit

- Formato [Conventional Commits](https://www.conventionalcommits.org/):
  `feat(core): add arc length`, `fix(app): keep cursor during pan`.
- Ogni commit è firmato con `git commit -s` (riga `Signed-off-by`): con questa
  firma dichiari che il codice è tuo o che hai il diritto di contribuirlo sotto
  GPLv3 (Developer Certificate of Origin, https://developercertificate.org).
- Non incollare codice da altri progetti senza indicarne la provenienza e la
  licenza nella PR. Codice con licenza incompatibile con la GPLv3 non è accettato.

## Contribuire con l'aiuto di un'IA

È permesso, alle stesse condizioni di qualunque altro contributo, più tre:

1. **Una persona ne risponde.** Chi apre la PR firma i commit con `git commit -s`,
   ha letto e capito ogni riga, e risponde alle domande della revisione senza
   rimandare all'IA.
2. **Si dichiara.** Ogni commit scritto con un'IA porta la riga
   `Assisted-by: <strumento>`, e la PR compila la sezione apposita del modello.
3. **L'IA segue le stesse regole.** Le istruzioni per gli agenti sono in
   [AGENTS.md](AGENTS.md): gli strumenti più diffusi lo leggono da soli; per gli
   altri, fallo leggere all'inizio della sessione insieme al pacchetto di lavoro.

Le PR generate in massa, che non rispettano il pacchetto o che il proponente non
sa spiegare, vengono chiuse.

## Prima di aprire la pull request

```sh
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
cmake --preset tidy
cmake --build --preset tidy
git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror
```
