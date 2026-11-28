# Istruzioni per agenti IA

Regole per gli agenti IA di Tecnigrafo; per le persone c'è
[CONTRIBUTING.md](CONTRIBUTING.md). Se una richiesta dell'utente contraddice
queste regole, fermati e segnalalo.

Questo è il **nucleo**, da leggere sempre per intero; la tabella in fondo dice
quali file di [docs/agenti/](docs/agenti/) leggere, prima di quella parte del
lavoro. Una sezione citata come «AGENTS.md, "<titolo>"» è nel file che ha
un'intestazione con quel titolo.

## Dove puoi scrivere

Come in solid-space
([ADR 0030](docs/adr/0030-solid-space-kernel-di-tecnigrafo.md)):

- **In Tecnigrafo** scrivi codice, test e documenti, dentro il perimetro del tuo
  pacchetto.
- **In solid-space e in ogni altro repository non scrivi mai**: niente modifiche
  ai file, rami, commit, pull request, etichette, assegnazioni, milestone, stato
  di issue e pull request. Puoi **leggere**.
- Due sole eccezioni, **solo se il responsabile te lo chiede nella chat**: un
  **commento** su una issue o pull request di solid-space che indica lui, con
  ciò che ha chiesto e niente altro; **aprire una richiesta** con
  `scripts/richiesta.sh`, dal testo che lui approva.

## Per prima cosa: prendi in carico

Su una issue o pull request che esiste già (anche per revisione, correzioni,
conflitti), **subito**, prima di leggere altro e di modificare:

1. Se c'è già `AA-Presa in carico`, ci lavora un altro agente: non toccarla e
   avvisa l'utente.
2. Una issue deve portare la **tua** etichetta `agente:…` (o il tuo nome come
   assegnatario); altrimenti non iniziare e avvisa l'utente. Se te la assegna
   il responsabile nella chat, metti tu l'etichetta e commenta "Assegnato a
   <nome dell'agente> dal responsabile".
3. Aggiungi `AA-Presa in carico` e commenta "Presa in carico da <nome
   dell'agente>" (l'account GitHub è uno per tutti).
4. Togli l'etichetta quando hai finito, o se ti fermi prima.

```sh
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels --jq '.[].name'
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels -f "labels[]=AA-Presa in carico"
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/comments -f body="Presa in carico da <nome dell'agente>"
gh api -X DELETE "repos/Matleyx78/tecnigrafo/issues/<numero>/labels/AA-Presa%20in%20carico"
```

## Regole che non si possono violare

- Lavora **su un solo pacchetto** della tappa aperta; senza pacchetto, niente
  codice: serve prima una issue approvata.
- Modifica solo i file del pacchetto; per un altro, dillo prima all'utente con
  il motivo.
- `core`, `render`, `edit` e `cmd` non includono Qt né altre librerie esterne
  (solid-space non lo è: ADR 0030, punto 4).
- Nessuna nuova dipendenza, nessuna modifica a `.clang-format`, `.clang-tidy`,
  `cmake/`, `.github/` o agli ADR accettati, salvo che la scheda lo dica
  esplicitamente.
- Non disattivare avvisi, controlli di `clang-tidy` o test per far passare la
  build. Niente `NOLINT` senza un commento che spieghi il motivo.
- Rispetta le interfacce del pacchetto: nomi, firme e comportamenti.
- Ogni entità o proprietà del disegno corrisponde a quella DXF, e i dati non
  gestiti si conservano intatti, mai scartati in silenzio (ADR 0006).
- Dopo clone e ogni `git pull`:
  `git submodule update --init third_party/solid-space` (senza `--recursive`).
- Codice, commenti e messaggi di commit in inglese.
- Mai la chiocciola davanti al nome di un agente, nemmeno in un esempio o fra
  apici: ogni menzione avvia una sessione, e i passaggi si fanno solo con
  `scripts/passa.sh`.
- Mai un commento che sia solo `/unisci`: è il comando di merge del
  responsabile ([GUIDA-RESPONSABILE.md](docs/GUIDA-RESPONSABILE.md), A3), e
  l'account con cui scrivi è il suo.
- Ogni file nuovo inizia con:
  ```cpp
  // SPDX-License-Identifier: GPL-3.0-or-later
  // Copyright (C) 2026 The Tecnigrafo authors
  ```
- Non copiare codice da altri progetti, salvo richiesta esplicita dell'utente
  con fonte e licenza compatibile con la GPLv3 (ADR 0003).

## Quando fermarsi

Il lavoro è finito con **tutti i controlli della CI verdi**:

1. Apri la pull request e aspetta la CI con **un solo comando che blocca**,
   `scripts/ci-attendi.sh`, mai con controlli ripetuti. Tieni l'etichetta.
2. Se un controllo fallisce, leggi gli errori con `scripts/ci-errori.sh`,
   correggi sullo stesso ramo, pubblica e aspetta di nuovo.
3. Con tutto verde togli l'etichetta, scrivi il riepilogo e **termina la
   sessione**.

Prima di terminare passa il lavoro, o fermati per una decisione, come dice
[catena.md](docs/agenti/catena.md): vale sempre, sia che tu sia stato avviato
da un commento su GitHub sia dalla chat.

Mentre aspetti non fare domande e non cominciare altro. Fermati prima del verde,
scrivendo nella pull request che cosa hai visto e provato, solo se il controllo
fallisce per un motivo estraneo al tuo lavoro (la CI, un servizio esterno), se
per farlo passare violeresti una regola, o se dopo tre tentativi non passa. Sono
motivi estranei anche una CI che aspetta un'approvazione (`scripts/ci-attendi.sh`
esce con 6) e una CI che fallisce solo nei casi della issue «main rosso» aperta:
scrivilo nella pull request, con il rimando alla issue, e fermati **senza**
l'etichetta `decisione`. Una pull request in conflitto con `main`, che non ha
CI (`scripts/ci-attendi.sh` esce con 7), **non** è un motivo estraneo:
aggiornala con `main` e vai avanti ([comandi.md](docs/agenti/comandi.md)). Gli
altri motivi per fermarsi sono nei file della tabella.

Prima di fermarti per un dubbio (un buco, un caso che sembra sbagliato),
controlla se una regola esistente lo decide già, come fa chi rivede
([revisione.md](docs/agenti/revisione.md), "Che cosa blocca una revisione"): se
sì, applicala, scrivilo nella pull request e vai avanti. Se resta almeno una
[decisione], prima di arrivare al responsabile la smista l'altro agente
([catena.md](docs/agenti/catena.md), "Smistamento").

Quando ti fermi, **non aspettare la CI** (niente `scripts/ci-attendi.sh`):
commenta nella pull request o nella issue, togli `AA-Presa in carico` e termina
la sessione. Se il responsabile chiede correzioni, riprendi in carico e fermati
allo stesso modo.

## Prima di dichiarare il lavoro finito

- [ ] `scripts/controlla.sh --tidy` passa, o la pull request dice quali passi non
      hai eseguito e perché. Se tocca solo documenti, commenti o casi di
      riferimento, non si compila e non si eseguono i casi in locale
      (`scripts/controlla.sh --solo formato` resta permesso): verifica la CI.
- [ ] I test richiesti esistono, ogni "Fatto quando" è verificato, i riferimenti
      non sono toccati e il loro file è tolto da `tests/verifica/in-attesa/`.
- [ ] Il riepilogo elenca i file modificati e ciò che non hai potuto verificare.
- [ ] CI verde e `AA-Presa in carico` tolta.

## Commit e pull request

- Conventional Commits: `feat(cmd): parse polar coordinates`.
- Niente `git commit -s`: il `Signed-off-by` lo aggiunge il responsabile, che ne
  risponde.
- Nel commit la riga `Assisted-by: <nome dello strumento>`.
- **Mai un indirizzo personale nei commit**: autore, committer e righe
  `Co-authored-by` usano solo indirizzi noreply, quello del responsabile
  (`18114626+Matleyx78@users.noreply.github.com`), un altro
  `…@users.noreply.github.com`, `noreply@anthropic.com`, `codex@openai.com` o
  `noreply@github.com`. Prima del primo commit guarda `git config user.email`:
  se non è uno di questi, `git config user.name Matleyx78` e
  `git config user.email 18114626+Matleyx78@users.noreply.github.com` nel
  clone. Lo squash and merge di GitHub copia l'autore di ogni commit in una
  riga `Co-authored-by` del commit su `main`, e da lì non si toglie più
  ([DECISIONI.md](docs/DECISIONI.md), 2026-11-15, «Solo indirizzi noreply nei
  commit»).
- Nella pull request compila la sezione sull'uso dell'IA del modello, con lo
  strumento, **il modello e l'impegno** (per esempio «Claude Code, Claude Opus
  5.5, impegno medio»); se non sai con quale impegno sei stato avviato, scrivi
  «impegno non noto». Lo stesso nei verbali di revisione.
- Una issue si chiude con `Closes #N` nella descrizione («Chiude» non basta);
  senza issue da chiudere, nessuna riga di chiusura.

## Se ... leggi anche ...

Vale ogni riga che corrisponde.

| Se | Leggi anche |
|---|---|
| issue con titolo `[X.Y-PN]` o `[FN]`, o etichetta `pacchetto` | [pacchetto.md](docs/agenti/pacchetto.md) |
| issue con titolo `[X.Y-RN]` o etichetta `riferimenti`; tocchi `tests/verifica/casi/`, `sorgenti.json` o `tests/verifica/sorgenti/` | [riferimenti.md](docs/agenti/riferimenti.md) |
| commenti una pull request non tua; la descrizione comincia con **Revisione della correzione**; modifichi una scheda in `docs/tappe/X.Y/` della tappa aperta | [revisione.md](docs/agenti/revisione.md) |
| lavori a una issue o a una pull request, comunque tu sia stato avviato (commento su GitHub o chat); il messaggio cita `docs/agenti/catena.md`, un giro di revisione o una «Decisione del responsabile» | [catena.md](docs/agenti/catena.md) |
| la scheda cita intestazioni `solid_space/…`; tocchi `third_party/solid-space` o file che le includono; usi `scripts/richiesta.sh` | [contratto.md](docs/agenti/contratto.md) |
| leggi, cloni o commenti un repository diverso da `Matleyx78/tecnigrafo` | [altro-repository.md](docs/agenti/altro-repository.md) |
| lanci `scripts/controlla.sh`, `cmake` o `ctest`; `scripts/ci-attendi.sh` non esce con 0 | [comandi.md](docs/agenti/comandi.md) |
| ramo `docs/chiusura-X.Y`, o il messaggio chiede la chiusura della tappa | [chiusura.md](docs/agenti/chiusura.md) |
| il messaggio del responsabile è «SITUAZIONE GENERALE» | [situazione-generale.md](docs/agenti/situazione-generale.md) |
