# ADR 0042: Immissione dinamica vicino al cursore

- **Stato:** Accettato
- **Data:** 2026-11-07

## Contesto

Il riferimento visivo della tappa 0.7.5
([ADR 0039](0039-decisione-visiva-dell-interfaccia.md), punto 1.7) mostra,
vicino al cursore, un riquadro con la richiesta del comando e due campi,
lunghezza e angolo, come l'immissione dinamica di AutoCAD (`DYNMODE`). Il
responsabile ha deciso (2026-11-07) che entra nella 0.7.5, anche se è una
funzione e non grafica.

Oggi l'utente risponde ai comandi solo con la riga di comando e con il mouse. Il
runner (`cmd::CommandRunner`) legge il testo con `submitText` e i punti con
`submit`; le richieste di ogni comando, con le opzioni, vengono dalla sua
descrizione ([ADR 0026](0026-descrizione-dei-comandi.md)) e da
`Command::prompt()`. Script e casi di verifica parlano al runner con il testo
([ADR 0011](0011-modalita-di-controllo.md)).

Il rischio è avere due modi di immettere che danno risultati diversi: per
questo l'immissione dinamica non deve essere un secondo interprete.

## Decisione

### 1. Un solo interprete: il testo della riga di comando

L'immissione dinamica **compone il testo** che l'utente avrebbe scritto nella
riga di comando e lo passa al runner con `submitText`, come la riga di comando.
Non chiama i comandi né il runner in altro modo. Quindi:

- i risultati sono gli stessi della riga di comando, per costruzione;
- la cronologia mostra la riga equivalente (per esempio `@56.39<61`), così si
  vede che cosa è entrato;
- script, casi di verifica e server MCP non cambiano.

La composizione del testo è una funzione di `cmd`, senza Qt, provata con test
del runner che confrontano le due immissioni; `app` disegna il riquadro e
raccoglie i tasti.

### 2. Che cosa mostra

Secondo la richiesta in corso:

| Richiesta | Campi |
|---|---|
| un punto senza punto base | coordinate X e Y del puntatore |
| un punto con un punto base (`Command::basePoint()`) | lunghezza e angolo dal punto base |
| distanza, angolo, numero | il valore |
| testo, oggetto, selezione | nessun campo: solo la richiesta |

Sopra i campi la richiesta del comando con le sue opzioni, come nella riga di
comando. I campi seguono il puntatore finché l'utente non ci scrive; i numeri
sono formattati come nella riga di comando.

### 3. Come si usa

- Scrivere un numero lo mette nel campo attivo e lo **blocca**: il puntatore
  non lo cambia più. `Tab` passa al campo successivo, `Invio` conferma.
- Con un punto base, lunghezza e angolo diventano `@<lunghezza><<angolo>`
  (relative polari, come AutoCAD per il secondo punto); scrivere una virgola
  nel campo della lunghezza passa a `@<dx>,<dy>`. Senza punto base, X e Y
  diventano `<x>,<y>` (assolute).
- Un campo bloccato e uno libero: il libero prende il valore del puntatore al
  momento di `Invio`, già corretto da orto, polare e snap come per un clic.
- Nessun campo scritto: `Invio` vale come `Invio` nella riga di comando (testo
  vuoto), che per esempio termina `LINE`; per dare il punto del puntatore c'è il
  clic, come oggi.
- Una lettera che non è un numero (un'opzione, un comando) va alla riga di
  comando, come oggi.
- `Esc` vale come nella riga di comando.

### 4. Acceso o spento

Un interruttore nella barra di stato, con `F12` come in AutoCAD, accende e
spegne l'immissione dinamica. È un'impostazione del programma, salvata come le
altre dell'interfaccia, non un dato del disegno. Accesa per difetto. Negli
script e nelle prove non c'è.

### 5. Che cosa non fa

- Non mostra i suggerimenti delle quote sulle entità selezionate (in AutoCAD
  `DYNDIGRIP`): arriverà, se serve, con le quote.
- Non sostituisce la riga di comando, che resta e non si può togliere
  ([ADR 0009](0009-interfaccia-grafica.md), punto 2).
- Non introduce coordinate o unità diverse da quelle della riga di comando.

## Alternative considerate

- **Campi che chiamano direttamente il runner con punti e numeri:** più
  semplici da scrivere, ma sarebbero un secondo interprete, da tenere uguale al
  primo per sempre.
- **Solo un suggerimento in sola lettura vicino al cursore** (coordinate e
  richiesta): meno lavoro, ma non è il riferimento, e lascia l'utente a
  guardare due posti.
- **Lunghezza e angolo assoluti per il secondo punto:** non è ciò che fa
  AutoCAD per difetto, e confonderebbe chi viene da lì.

## Conseguenze

- Il runner deve dire, in sola lettura, il punto base della richiesta in corso,
  se c'è; il pacchetto di `cmd` (0.7.5-P15) lo aggiunge se manca.
- Ogni comando nuovo funziona con l'immissione dinamica senza lavoro in più,
  perché la sua richiesta viene dalla descrizione.
- La barra di stato ha un interruttore in più.
- La prova manuale controlla che le due immissioni diano gli stessi risultati.
