# [0.7.5-P23] Revisione delle interfacce delle schede P24–P31

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è una revisione di documenti, non cambia il
  comportamento del programma
- **Dipende da:** —
- **Modulo:** nessuno: il lavoro sta nella issue, non in una pull request
- **Da leggere:** la lista di controllo di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  punti 1–11; l'[ADR 0041](../../adr/0041-documenti-e-ambienti.md) per intero;
  le schede [P24](P24-ciclo-di-vita-del-file.md)–[P31](P31-ambiente-non-ospitato.md);
  il codice che le schede toccano: `edit/include/tg/edit/editor.hpp`,
  `cmd/include/tg/cmd/command.hpp`, `command_runner.hpp`,
  `command_description.hpp`, `command_registry.hpp`,
  `app/src/main_window.hpp`, `app/src/cad_view.hpp`,
  `app/src/command_line.hpp`, `app/src/layer_panel.hpp`,
  `app/src/properties_panel.hpp`, `app/src/script_session.hpp`,
  `script/include/tg/script/level1.hpp`

## Perché

Le schede P24–P31 non introducono tipi in `core`, quindi la regola della
revisione delle interfacce non le copre. Il responsabile l'ha voluta lo stesso
(DECISIONI.md, 2026-11-07): sulle interfacce di `edit`, `cmd` e `app` che queste
schede fissano cresceranno la 0.8 (stili, blocchi, quote, canale MCP) e il 3D.
I buchi più costosi stanno tra una funzione e l'altra (#359), qui tra la sessione,
il runner, l'editor e l'interfaccia.

## Che cosa si rivede

In quest'ordine, che è quello in cui le interfacce si usano l'una con l'altra:

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [P24](P24-ciclo-di-vita-del-file.md) | `edit::DocumentFile`; `Editor` senza nome e file | il runner, i comandi dei file, il rapporto degli script |
| [P25](P25-tabelle.md) | `edit::DocumentTables`, `Editor::tables()` | la stessa transazione di `Editor` |
| [P26](P26-protocollo-dei-comandi.md) | `cmd::ProtocolContext`, `command_protocol.hpp` | `CommandContext` del disegno (P24) |
| [P27](P27-ambienti-nei-comandi.md) | `CommandDescription::environments`, l'ambiente del runner | il registro dei comandi, P26 |
| [P28](P28-sessione-per-documento.md) | `cmd::DrawingSession`, `app::DocumentSession`, l'attivazione | P24, P26; vista, pannelli, riga di comando, script |
| [P29](P29-schede-e-pagina-iniziale.md) | le schede dei documenti e la pagina *Start* | le sessioni di P28 |
| [P30](P30-ambiente-attivo-e-tipi.md) | `app::DocumentTypeRegistry`, `app::EnvironmentRegistry`, l'ambiente attivo | P27, P29, il registro delle azioni |
| [P31](P31-ambiente-non-ospitato.md) | la scelta di un ambiente non ospitato | P30 |

## Come

Per ogni tipo e ogni funzione delle schede, e per ogni passaggio da una
all'altra, ogni punto della lista: è **determinato** dall'ADR 0041, dalla scheda
o dal codice di oggi (e allora citi dove), oppure è un **buco**. Insisti su:

- i punti 10 e 11 (scelte libere, che cosa arriva al chiamante), soprattutto per
  ciò che succede a un comando in corso quando la sessione cambia, a un listener
  dopo un `replaceDocument`, a «modificato» dopo un salvataggio;
- la proprietà e la durata degli oggetti (chi possiede la sessione, il runner,
  l'editor; chi li tiene per riferimento e fino a quando);
- che cosa vede uno script e che cosa il server MCP, con una sessione e con più
  sessioni.

Non rivedi lo stile né le scelte che l'ADR 0041 ha già preso: se una regola
esistente risponde, la citi e vai avanti
([revisione.md](../../agenti/revisione.md), «Che cosa blocca una revisione»).

## Il verbale

Nella issue, scheda per scheda: i buchi, ciascuno con un esempio e le scelte
possibili, senza scegliere; poi i punti determinati, in una riga. I buchi li
decide il responsabile in [DECISIONI.md](../../DECISIONI.md), e una pull request
li applica alle schede e chiude la issue; senza buchi, la chiude il responsabile
dopo il verbale. Il verbale dice strumento e modello.

## Fatto quando

- Il verbale è nella issue e copre le otto schede.
