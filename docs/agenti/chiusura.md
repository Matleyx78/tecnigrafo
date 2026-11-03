# Chiusura della tappa

Da leggere quando il ramo è `docs/chiusura-X.Y`, o quando il messaggio chiede la
pull request di chiusura della tappa ([AGENTS.md](../../AGENTS.md), "Se ...
leggi anche ..."). Vale insieme al nucleo.

## Che cosa fa la pull request di chiusura

Non si ripete qui. Che cosa porta la pull request (criteri spuntati, note di
rilascio, catalogo di parità, stato, versione nel `CMakeLists.txt`) è in
[docs/LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md), "Chiudere la tappa";
che cosa fa poi il responsabile è in [docs/ROADMAP.md](../ROADMAP.md),
"Chiudere una tappa".

## Le regole del nucleo nella chiusura

- Valgono tutte, tranne due voci di "Prima di dichiarare il lavoro finito" che
  riguardano un pacchetto: i test richiesti dal pacchetto e il file dei
  riferimenti da togliere da `tests/verifica/in-attesa/`.
- La pull request cambia la versione in `CMakeLists.txt`, quindi non tocca solo
  documenti: la regola delle pull request senza compilazione locale
  ([comandi.md](comandi.md)) non si applica.
