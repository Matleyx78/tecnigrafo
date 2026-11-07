# [0.7.5-P17] Analisi: pannelli che si nascondono da soli (#534)

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: è un'analisi, un documento
- **Dipende da:** —
- **Modulo:** `docs`
- **Da leggere:** la [#534](https://github.com/Matleyx78/tecnigrafo/issues/534);
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punto 3 («Se `QDockWidget`
  si rivelerà insufficiente, si valuterà con un nuovo ADR»);
  [ADR 0002](../../adr/0002-linguaggio-e-strumenti.md) (le dipendenze);
  [ADR 0003](../../adr/0003-licenza.md); [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punto 4; `app/src/main_window.cpp` (`addPanel`, salvataggio della
  disposizione), `app/src/layer_panel.hpp`, `app/src/properties_panel.hpp`

## Perché

Il responsabile vuole che i pannelli agganciati ai lati si nascondano da soli,
come in AutoCAD (un bordo con il nome, il pannello che scorre sopra l'area di
disegno al passaggio del puntatore). I `QDockWidget` di Qt non lo fanno. Decide
il responsabile, sull'analisi; se approva, il pacchetto P18 lo realizza.

## File

- `docs/tappe/0.7.5/analisi-autohide.md` (nuovo)

## Cosa fare

L'analisi confronta almeno due strade, con pro, contro, rischi e una stima in
pacchetti:

1. **Con ciò che abbiamo:** un pannello nascosto diventa una linguetta su una
   barra al bordo della finestra; al passaggio del puntatore o al clic il suo
   contenuto si mostra **dentro** la finestra principale, sopra l'area di
   disegno, e si richiude quando il puntatore esce. Come convive con
   `QDockWidget`, con il salvataggio della disposizione (`saveState`) e con il
   passaggio tra ribbon e classica; perché il contenuto non deve essere una
   finestra separata (sotto Wayland una finestra mobile non si posiziona).
2. **Con una libreria di aggancio** (per esempio Qt Advanced Docking System):
   licenza e compatibilità con la GPLv3, versione minima di Qt, compatibilità
   con SARibbon e con la finestra senza cornice di P12, che cosa cambia nel
   codice dei pannelli e nel salvataggio della disposizione, chi la mantiene.
   Una dipendenza nuova chiede un ADR (ADR 0002): l'analisi dice che cosa
   dovrebbe decidere.

Poi una **raccomandazione**, con il comportamento proposto (che cosa si vede,
quando si apre e si chiude, come si fissa di nuovo un pannello) e i rischi da
provare prima di scrivere la scheda di P18. Dove un fatto non si può verificare
leggendo la documentazione, lo dici.

## Regole della tappa che valgono qui

- Nessun codice: un prototipo, se ti serve, resta fuori dalla pull request.

## Test richiesti

Nessuno: è un documento.

## Fatto quando

- Il documento confronta le due strade e dà una raccomandazione.
- La CI è verde.
