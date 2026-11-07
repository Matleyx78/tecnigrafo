# [0.7.5-P19] Icone e voci: comandi di disegno, con le varianti

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: le varianti eseguono i comandi con le risposte
  che un utente scriverebbe, già verificate dai casi dei comandi; test Qt e
  controllo delle icone
- **Dipende da:** P12, P16
- **Modulo:** `app` (più le icone)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1
  (area 3); [ICONE.md](../../ICONE.md) (riscritto da P11);
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punto 1;
  `app/src/action_registry.hpp`, `app/src/builtin_actions.cpp`,
  `app/src/ui_builder.cpp`, `app/src/main_window.cpp` (come un'azione avvia il
  suo comando); le richieste di `CIRCLE`, `ARC`, `LINE`, `PLINE` e `TEXT` in
  `cmd/src/commands/`

## File

- `app/src/action_registry.hpp`, `app/src/action_registry.cpp`
- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`, `app/src/main_window.cpp`
  (solo l'avvio delle varianti)
- `resources/icons/`: `draw-line`, `draw-pline`, `draw-text`, `draw-circle-cr`
  (fatta da P11), `draw-circle-cd`, `draw-circle-2p`, `draw-circle-3p`,
  `draw-circle-ttr`, `draw-arc-3p`, `draw-arc-sce`, ciascuna a 16 e 32 px; via i
  file da 24 px che queste sostituiscono
- `docs/ICONE.md` (l'elenco)
- `tests/app/action_registry_test.cpp`, `tests/app/ribbon_smoke_test.cpp`,
  `tests/app/icons_test.cpp`

## Cosa fare

1. **Varianti.** Un'azione può essere la variante di un'altra: `ActionSpec` dice
   di quale azione è variante e le **risposte** da dare al comando dopo il suo
   nome, una per richiesta, dove `\` vuol dire «aspetta una risposta
   dell'utente» (come le macro dei menu di AutoCAD). Esempi:
   - *Center, Radius*: `CIRCLE`, nessuna risposta;
   - *Center, Diameter*: `CIRCLE`, `\`, `D`;
   - *2-Point*: `CIRCLE`, `2P`; *3-Point*: `CIRCLE`, `3P`;
     *Tan, Tan, Radius*: `CIRCLE`, `T`;
   - *3-Point* di `ARC`: `ARC`, nessuna risposta;
     *Start, Center, End*: `ARC`, `\`, `C`.
   Le risposte entrano nel runner con `submitText`, come scritte nella riga di
   comando, e compaiono nella cronologia. Se il comando finisce o chiede altro
   prima, quelle che restano si scartano.
2. **Ribbon:** un pulsante grande con il menu a tendina delle varianti, con
   l'icona e il testo di ciascuna (ADR 0039, area 3); il pulsante ricorda
   l'ultima variante usata, come AutoCAD. **Menu classico:** un sottomenu per
   comando con le varianti.
3. **Icone** a ruoli, a 16 e 32 px, dei comandi e delle varianti qui sopra, con
   i punti e i centri dichiarati (`data-on`, `data-center-of`) dove ci sono.
   Nessuna forma copiata da altri programmi.

## Regole della tappa che valgono qui

- `builtin_actions.cpp`, `ui_builder.cpp`, `action_registry.*`: prima di te P12 e
  P16, dopo di te P20.
- Interfaccia in inglese; i nomi delle varianti come in AutoCAD in inglese.

## Test richiesti

- `action_registry_test.cpp`: una variante con `\` aspetta un punto e poi manda
  la risposta; un comando che finisce prima scarta le risposte rimaste.
- `ribbon_smoke_test.cpp`: `CIRCLE` ha cinque varianti nel menu a tendina, `ARC`
  due, ognuna con la sua icona; il pulsante ricorda l'ultima.
- `icons_test.cpp` e il controllo di P11 passano sulle icone nuove.

## Fatto quando

- I comandi di disegno e le loro varianti hanno icone a ruoli e voci nel ribbon e
  nei menu.
- I test qui sopra esistono e passano.
- La CI è verde.
