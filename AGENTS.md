# Istruzioni per agenti IA

Questo file è per gli assistenti di programmazione basati su IA (Claude Code, Codex,
Cursor, Copilot e simili) che lavorano su Tecnigrafo. Le regole valgono anche per le
persone, che le trovano spiegate in [CONTRIBUTING.md](CONTRIBUTING.md). Se una
richiesta dell'utente contraddice queste regole, fermati e segnalalo invece di
procedere.

## Prima di scrivere codice

Leggi, in quest'ordine:

1. [docs/ROADMAP.md](docs/ROADMAP.md) — quale tappa è **aperta**.
2. Il documento di quella tappa in `docs/tappe/` — il **pacchetto di lavoro** che ti
   è stato assegnato, con file, interfacce, test e criterio di "fatto".
3. [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md) e gli ADR citati dal pacchetto.
4. [docs/CODING_STYLE.md](docs/CODING_STYLE.md).

Lavora **su un solo pacchetto**. Se il compito non corrisponde a un pacchetto della
tappa aperta, non scrivere codice: spiega all'utente che serve prima una issue
approvata.

Prima di iniziare controlla che la issue del pacchetto porti la **tua** etichetta
`agente:…` (o il tuo nome come assegnatario). Se è senza etichetta o assegnata a
qualcun altro, non iniziare e avvisa l'utente: altri agenti potrebbero lavorare in
parallelo ([docs/LAVORO-IN-PARALLELO.md](docs/LAVORO-IN-PARALLELO.md)).

## Regole che non si possono violare

- Modifica solo i file elencati nel pacchetto. Se ne serve un altro, dillo
  all'utente e spiega perché, prima di toccarlo.
- `core`, `render`, `edit` e `cmd` non includono mai Qt né altre librerie esterne.
- Nessuna nuova dipendenza, nessuna modifica a `.clang-format`, `.clang-tidy`,
  `cmake/`, `.github/` o agli ADR accettati.
- Non disattivare avvisi, controlli di `clang-tidy` o test per far passare la
  build. Niente `NOLINT` senza un commento che spieghi il motivo.
- Rispetta le interfacce scritte nel pacchetto: nomi, firme e comportamenti.
- Ogni entità o proprietà del disegno corrisponde a quella DXF equivalente, e i
  dati non gestiti si conservano intatti, mai scartati in silenzio (ADR 0006).
- Codice, commenti e messaggi di commit in inglese.
- Ogni file nuovo inizia con l'intestazione:
  ```cpp
  // SPDX-License-Identifier: GPL-3.0-or-later
  // Copyright (C) 2026 The Tecnigrafo authors
  ```
- Non copiare codice da altri progetti, a meno che l'utente non lo chieda
  esplicitamente indicando fonte e licenza (che deve essere compatibile con la
  GPLv3, ADR 0003).

## Comandi

```sh
cmake --preset ci && cmake --build --preset ci && ctest --preset ci   # build e test
cmake --preset tidy && cmake --build --preset tidy                    # analisi statica
git ls-files '*.cpp' '*.hpp' | xargs clang-format -i                  # formattazione
```

Su sistemi senza Qt installato si possono compilare e provare i moduli senza
interfaccia con `-DTG_BUILD_APP=OFF`.

## Prima di dichiarare il lavoro finito

- [ ] I test richiesti dal pacchetto esistono e passano.
- [ ] Build `ci` senza avvisi, `tidy` senza segnalazioni, formattazione applicata.
- [ ] Ogni condizione "Fatto quando" del pacchetto è verificata.
- [ ] Il riepilogo per l'utente elenca i file modificati e ciò che non hai potuto
      verificare (per esempio Windows e macOS).

## Commit e pull request

- Formato Conventional Commits: `feat(cmd): parse polar coordinates`.
- Il commit deve essere firmato dalla **persona** responsabile (`git commit -s`):
  un agente non firma da solo il Developer Certificate of Origin.
- Nel commit aggiungi la riga `Assisted-by: <nome dello strumento>`.
- Nella pull request compila la sezione sull'uso dell'IA del modello.
