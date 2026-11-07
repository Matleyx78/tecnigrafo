# [0.7.5-P20] Icone e voci: comandi di modifica della 0.7

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: le azioni avviano comandi già verificati dai loro
  casi; test Qt e controllo delle icone
- **Dipende da:** P19
- **Modulo:** `app` (più le icone)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1
  (area 3, gruppo *Modify*); [ICONE.md](../../ICONE.md);
  [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), «Comandi
  nell'interfaccia»; le note di rilascio [0.7.0](../../rilasci/0.7.md), «Limiti
  noti»; `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`

## Perché

I comandi di modifica della 0.7 si scrivono nella riga di comando ma non hanno
icone né voci di menu, ribbon e barre (limite noto della 0.7.0).

## File

- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp` (solo se il gruppo
  *Modify* chiede una disposizione nuova)
- `resources/icons/`: `edit-move`, `edit-copy`, `edit-rotate`, `edit-scale`,
  `edit-mirror`, `edit-explode`, `edit-trim`, `edit-extend`, `edit-offset`,
  `edit-fillet`, `edit-chamfer`, `edit-join`, ciascuna a 16 e 32 px
- `docs/ICONE.md` (l'elenco)
- `tests/app/action_registry_test.cpp`, `tests/app/ribbon_smoke_test.cpp`,
  `tests/app/icons_test.cpp`

Le icone contano come coppie 16/32: il pacchetto ha dodici coppie e tre file di
codice o di test.

## Cosa fare

1. Un'azione per comando, `edit.move` … `edit.join`, con il comando di `cmd`,
   la descrizione, l'icona, la voce nel menu classico *Modify* e il posto nel
   gruppo *Modify* del ribbon (`home/modify`), disposto come nell'ADR 0039: pile
   di tre pulsanti piccoli (*Move*, *Copy*, … ), `TRIM` ed `EXTEND` insieme,
   `FILLET` e `CHAMFER` insieme; anche nella barra *Modify* dell'interfaccia
   classica. Le scorciatoie, solo se AutoCAD ne ha una predefinita per quel
   comando (di norma nessuna).
2. Le icone a ruoli: l'oggetto già esistente in `esist`, il risultato in `geo`,
   i punti che l'utente indica in `pt`, direzioni e distanze in `cost`; con
   `data-on` e `data-center-of` dove ci sono punti e centri.
3. `TRIM` ed `EXTEND` dal ribbon partono nel modo che il comando usa quando lo
   si scrive (ADR 0035): nessuna risposta in più.

## Regole della tappa che valgono qui

- `builtin_actions.cpp`, `ui_builder.cpp`, `docs/ICONE.md`: prima di te P19, dopo
  di te P21.
- Interfaccia in inglese.

## Test richiesti

- `action_registry_test.cpp`: le dodici azioni esistono, ognuna con il suo
  comando e l'icona.
- `ribbon_smoke_test.cpp`: il gruppo *Modify* le mostra; nell'interfaccia
  classica c'è la barra *Modify*.
- Il controllo di P11 passa sulle icone nuove.

## Fatto quando

- Ogni comando di modifica della 0.7 ha icona e voci di menu, ribbon e barre.
- I test qui sopra esistono e passano.
- La CI è verde.
