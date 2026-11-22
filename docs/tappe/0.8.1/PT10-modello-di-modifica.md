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
   dall'impaginazione di PT8 (anche ruotata e con l'allineamento). Ogni
   posizione sta fra due **unità** di PT7 (`MTextRun::units`), e il suo punto
   viene dai confini delle unità che dà l'impaginazione di PT8 (punto 3):
   l'origine di un carattere nel valore e i codici da conservare si leggono
   dalle unità, mai dagli avanzamenti o dal numero di caratteri (verbale della
   revisione delle interfacce PT4, #927, [bloccante] 4).
   - **Frazione impilata** (decisione del responsabile sulla #927,
     [decisione] 2): un'unità sola, come in AutoCAD. Il cursore la salta (una
     posizione prima e una dopo, nessuna dentro), la scelta la prende intera o
     per niente, `Backspace` e `Canc` la cancellano intera con i suoi byte.
     Numeratore e denominatore si cambiano cancellando e riscrivendo la
     frazione, o dal campo del valore nel pannello (PT25).
   - **Misura di una quota** (decisione del responsabile sulla rilettura
     della #993, seconda [decisione]): per il testo di una quota il modello
     riceve anche il testo che sostituisce `<>`, già formattato
     (`TextEditRequest::measurement` di PT11), anche quando il valore di
     partenza non ha `<>`, così si mostra appena lo si scrive. Come
     `dimensionText`, si sostituisce solo la **prima** occorrenza di `<>` del
     valore: le altre sono testo, carattere per carattere (verbale del secondo
     giro della #993, [bloccante] 2). Quella prima occorrenza si impagina con
     il testo che la sostituisce, come nella grafica, ma è **un'unità sola**
     con i due byte di `<>`, come una frazione e come il campo di AutoCAD: il
     cursore la salta, la scelta la prende intera o per niente, `Backspace` e
     `Canc` tolgono `<>` intero. Esempi: misura `1234`, valore `<> mm`: si vede
     `1234 mm`, le posizioni sono prima e dopo `1234`, poi fra le lettere di
     ` mm`; un clic fra il `2` e il `3` va alla posizione più vicina delle due;
     cancellare la misura dà ` mm`. Misura `30`, valore `<> / <>`: si vede
     `30 / <>`, l'unità della misura sono i byte `[0,2)` e il secondo `<>` sono
     due unità di testo. Vale anche per *Mtext* durante la creazione della
     quota.

     Se il testo che sostituisce `<>` va su più righe (un `DIMPOST` con `\P`,
     o un a capo automatico), l'unità ha **sempre e solo** le sue due
     posizioni: prima, all'inizio della sua prima riga, e dopo, alla fine
     dell'ultima. Le righe che stanno tutte dentro l'unità non hanno posizioni
     proprie: un clic su di loro va alla più vicina delle due, `Home` e `End`
     su una di quelle righe portano alla posizione prima e a quella dopo
     dell'unità (verbale del secondo giro della #993, [decisione],
     applicazione della decisione appena presa: un'unità sola, con due
     posizioni).
   - **A capo automatico:** gli spazi dove la riga va a capo non si disegnano
     (PT8, come oggi) ma restano unità come le altre (PT7), con una posizione
     prima e una dopo ciascuno: in `A B`, con una lettera per riga, `Canc` dopo
     la `A` e `Backspace` prima della `B` tolgono lo spazio e danno `AB`; con
     più spazi consecutivi se ne toglie uno per volta, nei due versi, e una
     scelta che li attraversa li prende (verbale della rilettura della #993,
     prima [decisione], applicazione di questa regola: ogni unità ha le sue
     posizioni). Il punto disegnato delle posizioni intorno agli spazi
     assorbiti, e quale punto tiene il cursore quando un confine ne ha due
     (fine della riga e inizio della seguente), sono buchi non visibili (#927,
     [decisione] 2): li sceglie chi implementa, li scrive nel commento
     dell'interfaccia e li fissa con un test.
2. Operazioni: inserire testo, cancellare a sinistra e a destra, nuovo
   paragrafo, scegliere un intervallo, applicare o togliere un formato
   dell'ADR 0054, punto 1, sull'intervallo; ogni operazione dà il valore nuovo
   con i codici. I codici che non si mostrano (`\A`, `\p…;`, gli altri codici
   noti saltati, e quelli con un argomento fuori dominio, che PT7 nasconde e
   ignora) restano nel valore nel punto in cui stavano: cancellare il carattere
   accanto non li toglie. Un codice sconosciuto o senza `;` non è un codice:
   PT7 lo legge come testo, quindi i suoi caratteri sono unità come le altre
   (#927, [decisione] 1, applicazione della regola dell'ADR 0054, punto 1).
   Scrivere nella posizione fra due unità separate da un codice nascosto
   (`A` e `B` di `A\A1;B`) mette il testo prima o dopo il codice: è un buco non
   visibile, perché il disegno di Tecnigrafo non cambia; lo sceglie chi
   implementa e lo fissa con un test.
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
- In `A\A1;B` cancellare `A` o `B` lascia `\A1;`; in `é\U+0042` cancellare la
  `B` toglie i sette byte di `\U+0042` e niente altro; la scelta del punto
  d'inserimento fra `A` e `\A1;B` fissata da un test.
- `x\S1/2;y`: tre unità e quattro posizioni; `Canc` dopo la `x` toglie tutta la
  frazione e dà `xy`; il punto più vicino a un punto sopra la frazione
  (punto 3) è la posizione prima o quella dopo, mai una dentro.
- Un paragrafo che va a capo da solo: il cursore sul confine, prima e dopo una
  scrittura, con la regola scelta.
- Misura `1234` con valore `<> mm`: due posizioni ai bordi di `1234`; il clic
  fra il `2` e il `3` va a una delle due; `Canc` prima di `1234` dà ` mm`; il
  valore senza modifiche torna `<> mm`.
- Misura `30` con valore `<> / <>`: si vede `30 / <>`; la misura è un'unità
  sola (`[0,2)`), il secondo `<>` due unità di testo.
- Valore di partenza `FISSO` senza `<>`: scrivere `<>` al posto di `FISSO` fa
  vedere subito la misura ricevuta, prima della conferma.
- `DIMPOST` `<> mm\Pnominale\Pquota` con valore `<>`: tre righe, due sole
  posizioni (prima di `30`, dopo `quota`); un clic sulla riga `nominale` e
  `Home`/`End` su quella riga portano a una delle due; scrivere `X` dopo dà
  `<>X`, prima dà `X<>`.
- `A B` con una lettera per riga: `Canc` dopo la `A` dà `AB`, `Backspace`
  prima della `B` dà `AB`; `A  B` (due spazi): un tasto toglie uno spazio
  solo, in tutti e due i versi.

## Fatto quando

- I test passano; la CI è verde.
