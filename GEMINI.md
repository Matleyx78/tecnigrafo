# Tecnigrafo

Le istruzioni per gli assistenti IA sono in [AGENTS.md](AGENTS.md): prima di
qualunque modifica leggi il nucleo e, dalla sua tabella "Se ... leggi anche ...",
i file di [docs/agenti/](docs/agenti/) che riguardano il tuo lavoro. Seguili alla
lettera.

Questo file lo leggono Gemini CLI e Antigravity CLI, e tiene solo le note proprie
di Gemini.

Riferimenti e revisioni si fanno **a lotti**: una sessione prende di fila tutto
il lavoro pronto, con una pull request per pacchetto
([docs/agenti/lotti.md](docs/agenti/lotti.md)). Usa la cartella di build
`build-gemini` (`TG_BUILD_SUFFIX=-gemini`) e, se devi compilare o provare,
`scripts/controlla.sh` ([docs/agenti/comandi.md](docs/agenti/comandi.md)).

Se sei **Gemini 4 Argon**, il responsabile te lo dice nel messaggio: la tua
etichetta è `agente:gemini-argon`, non `agente:gemini`, e la tua cartella di
build è `build-gemini-argon` (`TG_BUILD_SUFFIX=-gemini-argon`). Prendi solo le
issue con la tua etichetta: quelle di `agente:gemini` sono di Gemini 3.8 Flash,
che lavora sullo stesso PC. Il piano non ti assegna niente: l'etichetta la
mette il responsabile sulle issue che ti passa, quando hai quota disponibile.
