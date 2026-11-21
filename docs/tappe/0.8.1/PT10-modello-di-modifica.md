# [0.8.1-PT10] `core`: modello di modifica del testo formattato

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: nessun comportamento del programma cambia
  finché l'editor ([PT12](PT12-editor.md)) non lo usa; test di unità
- **Dipende da:** PT4, PT7, PT8
- **Modulo:** `core`
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punto 5; le schede sorelle [PT7](PT7-codici-di-formato.md),
  [PT8](PT8-impaginazione-formati.md) e [PT12](PT12-editor.md)

## Perché a parte

L'editor nella finestra (PT12) deve modificare il valore senza perdere codici,
e sapere dove sta ogni carattere. Messo in `core`, si prova senza Qt e l'editor
resta solo interazione.

## File

- `core/include/tg/core/mtext_edit.hpp`, `core/src/mtext_edit.cpp` (nuovi)
- `core/CMakeLists.txt` (file comune)
- `tests/core/mtext_edit_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. `MTextEditModel`, costruito da un `MText`: il testo visibile come sequenza
   di **posizioni del cursore** (fra un carattere e l'altro, e ai capi di ogni
   paragrafo), con, per ciascuna, il punto del disegno e l'altezza del cursore
   dall'impaginazione di PT8 (anche ruotata e con l'allineamento).
2. Operazioni: inserire testo, cancellare a sinistra e a destra, nuovo
   paragrafo, scegliere un intervallo, applicare o togliere un formato
   dell'ADR 0054, punto 1, sull'intervallo; ogni operazione dà il valore nuovo
   con i codici. I codici che non si mostrano (`\A`, `\p…;`, quelli sconosciuti)
   restano nel valore nel punto in cui stavano: cancellare il carattere accanto
   non li toglie.
3. Il punto più vicino a un punto del disegno (per il clic nell'editor).
4. Il valore senza modifiche torna **identico** a quello iniziale.

## Regole della tappa che valgono qui

- A pari distanza da due posizioni vince quella a sinistra (prima nell'ordine
  di lettura); è una scelta di interfaccia, non una regola numerica del
  disegno.

## Test richiesti

- Andata e ritorno identici sui valori di `autocad-mtext-formati.dxf`; scrivere
  dentro un grassetto resta grassetto; cancellare l'ultimo carattere di un
  gruppo lascia il gruppo vuoto o lo toglie senza toccare i codici vicini;
  applicare grassetto a metà di un pezzo corsivo dà grassetto corsivo.

## Fatto quando

- I test passano; la CI è verde.
