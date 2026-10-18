# ADR 0014: Richieste di distanza, angolo e testo

- **Stato:** Accettato (2026-10-18)
- **Data:** 2026-10-18

## Contesto

L'ADR 0005 ha dato ai comandi quattro tipi di input: punto, parola chiave,
conferma e annullamento; l'ADR 0008 ha aggiunto la selezione. I comandi della
tappa 0.4 chiedono anche altro:

- una **distanza**: il raggio e il diametro di `CIRCLE`, l'altezza di `TEXT`;
- un **angolo**: la rotazione di `TEXT`;
- un **testo**: le righe di `TEXT`, con spazi e qualunque carattere.

Oggi un numero scritto da solo è una "distanza diretta", cioè un punto nella
direzione del puntatore; uno spazio nella riga di comando vale `Invio`, come in
AutoCAD; e il runner toglie gli spazi all'inizio e alla fine. Nessuna di queste
regole va bene per le nuove richieste. Inoltre AutoCAD propone tra `< >` un
valore predefinito (l'ultimo usato) che si accetta con `Invio`.

L'ADR 0005 resta valido: i comandi non leggono testo, e la sintassi è la stessa
per tutti i comandi perché la interpreta il runner. Questo ADR lo integra.

## Decisione

### 1. Tipo di richiesta

`Prompt` dichiara cosa chiede: `PromptKind::Point` (come oggi, predefinito),
`Distance`, `Angle` o `Text`. Il runner interpreta ciò che l'utente scrive in base
al tipo:

| Scritto | `Point` | `Distance` | `Angle` | `Text` |
|---|---|---|---|---|
| vuoto (`Invio`) | conferma | conferma | conferma | conferma |
| parola chiave | parola chiave | parola chiave | parola chiave | testo |
| numero da solo (`50`) | distanza diretta | **valore** | **valore**, in gradi | testo |
| altra coordinata | punto | punto | punto | testo |
| altro | messaggio | messaggio | messaggio | testo |

- Due nuovi input: `ValueInput { double value }` (unità di disegno per le
  distanze, radianti per gli angoli) e `TextInput { std::string text }` (mai
  vuoto).
- I numeri hanno la sintassi delle coordinate (ADR 0005): punto decimale,
  segno, esponente, indipendenti dalla lingua del sistema. Gli angoli si scrivono
  in gradi, in senso antiorario da +X. Il segno lo controlla il comando.
- Messaggi: `Requires a distance, a point or an option.` e
  `Requires an angle, a point or an option.`; la domanda resta.

### 2. Punti alle richieste di distanza e angolo

Un punto (scritto, o un clic) arriva al comando come `PointInput`: è il comando
a misurare distanza o angolo dal proprio `basePoint()`, come fa AutoCAD con il
raggio "dato con il mouse". Snap e orto valgono come per ogni punto (ADR 0008),
con base `basePoint()`; un comando che fa una richiesta di distanza o di angolo
deve quindi avere un punto base.

### 3. Richiesta di testo

- Il testo arriva **così come è scritto**: il runner non toglie gli spazi e non
  riconosce parole chiave né coordinate. Il testo vuoto è una conferma; `Esc`
  annulla come sempre.
- `CommandRunner::acceptsText()` dice alla vista che il comando chiede un testo:
  nella riga di comando lo spazio diventa un carattere invece di `Invio`.
- Negli script (ADR 0011) una riga normale si passa senza togliere spazi; a una
  richiesta di testo diventa la riga di testo.

### 4. Valori predefiniti

- `CommandContext` riceve `CommandDefaults`, che vive quanto il runner:
  altezza del testo (2,5 all'avvio), rotazione del testo (0), raggio del
  cerchio (nessuno prima del primo cerchio). Il comando aggiorna il valore che ha
  usato.
- Il valore compare nella domanda tra `< >`, scritto con `formatLength` (quattro
  decimali, come AutoCAD per le lunghezze: `<2.5000>`) o `formatAngle` (gradi,
  fino a quattro decimali senza zeri finali: `<0>`, `<37.5>`).
- `Invio` a una domanda con valore predefinito usa quel valore; senza valore
  predefinito il comando termina senza effetti.
- I valori non si salvano tra una sessione e l'altra: se ne riparla con le
  impostazioni della tappa 0.9 e con le variabili del disegno della 0.5.

## Alternative considerate

- **I comandi leggono il testo da soli:** già scartato dall'ADR 0005; ogni comando
  avrebbe la sua sintassi.
- **Numero sempre come distanza diretta:** il comando dovrebbe ricavare il
  raggio da un punto costruito nella direzione del puntatore; con il puntatore
  sul centro il valore scritto andrebbe perso.
- **Una finestra di dialogo per il testo:** non si pilota da uno script, e
  allontana dal modo di lavorare di AutoCAD.
- **Togliere gli spazi anche al testo:** AutoCAD li conserva, e un testo con
  spazi iniziali è un caso reale (incolonnamenti).

## Conseguenze

- `Input` ha due alternative in più; i comandi esistenti le ignorano.
- La vista deve chiedere `acceptsText()` prima di trattare lo spazio come
  `Invio`.
- Gli script `.scr` di AutoCAD che usano `CIRCLE` o `TEXT` funzionano senza
  modifiche.
- Altri tipi di richiesta (numeri interi per le serie, fattori di scala per
  `SCALE`) si aggiungeranno nella 0.7 allo stesso modo, con un nuovo
  `PromptKind`.
- Le domande con un valore tra `< >` contengono un numero dentro una frase fissa:
  la traduzione della 0.9 dovrà prevederlo.
