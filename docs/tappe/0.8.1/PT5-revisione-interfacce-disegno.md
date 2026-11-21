# [0.8.1-PT5] Revisione delle interfacce: stato del disegno, snap, attributi, retini

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è una revisione di documenti; verbale nella
  issue
- **Dipende da:** —
- **Modulo:** nessuno: il lavoro sta nella issue, non in una pull request
- **Da leggere:** la lista di controllo di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  punti 1–11; gli ADR [0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  [0056](../../adr/0056-comandi-degli-attributi.md) e
  [0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md) per intero;
  [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md), punto 5;
  [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md);
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezioni 3 e 4; le schede
  qui sotto e il codice che toccano

## Perché

Le schede qui sotto cambiano `Document` (lo stato della `VPORT` e
`$ATTMODE`), `SnapModes` e `SnapKind`, il tracciamento, le definizioni dei
blocchi e i comandi in corso (`Command::highlighted()`), e il disegno della
griglia in `render`: interfacce che più pacchetti usano insieme
([DECISIONI.md](../../DECISIONI.md), 2026-11-07: revisione anche fuori da
`core`).

## Che cosa si rivede

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [PT15](PT15-attributi-core.md) | `lockPosition`, `synchronizedAttributes`, `pickAttribute` | ADR 0045, `insert.hpp` |
| [PT16](PT16-definizioni-edit.md) | `reorderAttributeDefinitions`, `replaceAttributeDefinition`, `removeAttributeDefinition`, `synchronizeInserts` | PT15; `edit::Editor` |
| [PT27](PT27-stato-del-disegno.md) | `GridSettings`, `AttributeDisplay`, i loro campi in `Document` e nel DXF | `dxf_tables`, `dxf_header`, `cadx` |
| [PT29](PT29-griglia-render.md) | `AxisFrame`, i campi nuovi di `SceneStyle` | PT27 |
| [PT30](PT30-snap-vicino.md) | `SnapModes::nearest`, `SnapKind::Nearest`, il livello di *Vicino* in `findSnap` | ADR 0033, punto 5 |
| [PT31](PT31-prolungamento-parallelo.md) | le acquisizioni di prolungamenti e direzioni, le guide nuove in `TrackingState` | PT30, PT28; `object_tracking` |
| [PT34](PT34-last-previous.md) | `Command::highlighted()`, la selezione precedente | `CommandRunner` |
| [PT26](PT26-hatchedit.md) | `-HATCHEDIT`, l'evidenziazione di `-HATCH` | PT23, PT34 |

## Come

Ogni punto della lista, per ogni tipo e funzione e per ogni passaggio da una
all'altra. Insisti su:

- pareggi e priorità degli snap e delle guide nuove: dove la scheda rimanda a
  REGOLE-NUMERICHE.md la regola c'è; le proposte [D3], [D6], [D7] degli ADR
  vanno segnate come [decisione] se il responsabile non le ha ancora prese;
- lo stato del disegno fra documenti diversi (una sessione per documento,
  0.7.5-P28): chi cambia che cosa con `F7` e `F9` e che cosa si salva;
- la vita degli `ATTRIB` nella sincronizzazione: handle, ordine, proprietà,
  inserimenti annidati, blocchi non esplodibili; l'annullamento delle
  operazioni sulle definizioni;
- che cosa arriva a `render` dall'icona dell'UCS quando l'origine è sul bordo.

Non rivedi lo stile né le scelte già prese: se una regola esistente risponde,
la citi e vai avanti ([revisione.md](../../agenti/revisione.md), «Che cosa
blocca una revisione»).

## Il verbale

Nella issue, con le classi [bloccante], [decisione], [facoltativa]; i buchi li
decide il responsabile in [DECISIONI.md](../../DECISIONI.md) e una pull request
li applica alle schede. Senza buchi chiude il responsabile.
