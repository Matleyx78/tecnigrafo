# Istruzioni per GitHub Copilot

Valgono [AGENTS.md](../AGENTS.md) e i file della sua tabella, alla lettera: sei
un agente come gli altri. Qui ci sono solo le differenze del tuo ambiente; non
ripetono le regole e non le allargano. Se una richiesta le contraddice, fermati
e scrivilo nella pull request.

## Prendere in carico

- Ti assegna la issue il responsabile, con "Assign to Copilot": l'assegnazione
  vale come "il tuo nome come assegnatario" del nucleo. Non lavori su issue che
  non ti sono assegnate.
- Etichette e commenti della issue li gestisce il responsabile: non toccarli.
  Sulla issue assegnata a te, `AA-Presa in carico` (con `agente:copilot`) l'ha
  messa il responsabile per te: non vuol dire che ci lavora un altro agente.
  Il punto 1 di "Per prima cosa: prendi in carico" per te è già fatto: lavora.
- Il tuo nome di agente, nei commenti e nella pull request, è **Copilot**.

## Ambiente

Prima di te ha girato `.github/workflows/copilot-setup-steps.yml`: Qt, compilatori,
`clang-format-18`, `clang-tidy-18`, il sottomodulo di solid-space e l'oracolo
sono installati, e il setup ha tentato di configurare e compilare `main` in
`build-copilot/` (se non c'è riuscito, `scripts/controlla.sh` completa la
build). Non installare niente.

```sh
export TG_BUILD_SUFFIX=-copilot
export CMAKE_PREFIX_PATH=/home/runner/qt/Qt/6.8.3/gcc_64
git submodule update --init third_party/solid-space
scripts/controlla.sh --tidy
```

La build è in `build-copilot/`. Le altre regole dei comandi sono in
[comandi.md](../docs/agenti/comandi.md).

## Quando fermarsi

- Nella tua sessione non puoi aspettare la CI: `scripts/ci-attendi.sh` e
  `scripts/ci-errori.sh` non si usano. Al loro posto `scripts/controlla.sh
  --tidy` deve passare prima che tu pubblichi; se un passo non si può eseguire,
  la pull request dice quale e perché.
- La CI la segue il responsabile: se fallisce, ti scrive nella pull request che
  cosa correggere.
- Gli altri motivi per fermarsi sono quelli del nucleo e dei file della tabella.

## Commit e pull request

- Nei commit la riga `Assisted-by: GitHub Copilot`.
- La descrizione segue il modello della pull request, in italiano, con la
  sezione sull'uso dell'IA compilata e `Closes #N`.
- Leggi solo i file che il lavoro richiede: ogni file letto consuma crediti.
- Committa solo i file del pacchetto: niente log o file generati (per esempio
  `aqtinstall.log`). Controlla `git status` prima di pubblicare.
