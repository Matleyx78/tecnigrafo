# Tecnigrafo

Le istruzioni per gli assistenti IA sono in [AGENTS.md](AGENTS.md): leggile per
intero prima di qualunque modifica e seguile alla lettera.

Questo file lo leggono Gemini CLI e Antigravity CLI. Di norma Gemini lavora ai
**pacchetti di riferimenti** (`[X.Y-RN]`): le regole sono in AGENTS.md, sezione
"Modalità di controllo e riferimenti", e in
[docs/adr/0011-modalita-di-controllo.md](docs/adr/0011-modalita-di-controllo.md).
Fa anche le revisioni delle pull request, la pull request di chiusura della
tappa e i pacchetti facili.

Riferimenti e revisioni si fanno **a lotti**: una sessione prende di fila tutto
il lavoro pronto, con una pull request per pacchetto (AGENTS.md, "Lavoro a
lotti"). Usa la cartella di build `build-gemini` (`TG_BUILD_SUFFIX=-gemini`) e,
se devi compilare o provare, `scripts/controlla.sh` (AGENTS.md, "Comandi").

Se sei **Gemini 4 Argon**, il responsabile te lo dice nel messaggio: la tua
etichetta è `agente:gemini-argon`, non `agente:gemini`, e la tua cartella di
build è `build-gemini-argon` (`TG_BUILD_SUFFIX=-gemini-argon`). Prendi solo le
issue con la tua etichetta: quelle di `agente:gemini` sono di Gemini 3.8 Flash,
che lavora sullo stesso PC.
