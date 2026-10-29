# ADR 0032: Richieste di oggetti e di numeri; sequenze degli script di AutoCAD

- **Stato:** Accettato
- **Data:** 2026-10-29
- **Estende:** ADR 0008 (selezione), ADR 0014 (richieste di valori), ADR 0026
  (descrizione dei comandi)

## Contesto

I comandi di modifica chiedono cose che oggi il runner non sa chiedere:

- **un oggetto con il punto in cui lo si sceglie.** `TRIM` deve sapere quale pezzo
  togliere, `OFFSET` quale entità copiare, `FILLET` quale lato di due entità
  raccordare. La selezione dell'ADR 0008 dà solo un elenco di identificativi;
- **un numero puro**, come il fattore di `SCALE`. L'ADR 0014 lo prevedeva:
  «fattori di scala per `SCALE` si aggiungeranno nella 0.7 con un nuovo
  `PromptKind`»;
- **una selezione a metà comando**, e una selezione **scritta**: negli script
  `.scr` gli oggetti si scelgono scrivendo un punto o `W` con due angoli, e
  l'ADR 0008 prevedeva di aggiungere le opzioni scritte di selezione.

Preparando i DXF di prova in AutoCAD LT è emerso un altro fatto. Quando esegue uno
script `.scr`, AutoCAD usa per alcuni comandi **le sequenze di domande delle
versioni precedenti**, anche se l'utente ha scelto altrimenti:

- `TRIM` ed `EXTEND` lavorano nel modo standard anche con `TRIMEXTENDMODE` = 1;
- `FILLET` e `CHAMFER` si chiudono dopo aver impostato raggio o distanze;
- `COPY` fa una copia sola;
- `JOIN` chiede prima un oggetto sorgente, poi gli oggetti da unire (gli archi
  uno per clic);
- `TEXT` si chiude dopo una riga.

L'ADR 0011 (punto 3) promette che uno script `.scr` di AutoCAD con i comandi di
Tecnigrafo sia uno script valido. Per mantenerlo, Tecnigrafo deve riconoscere
quelle sequenze.

## Decisione

### Richieste di oggetti e di numeri

1. **`PromptKind::Object`.** Alla richiesta di un oggetto, un clic, oppure un
   punto scritto, sceglie l'entità sotto quel punto, con le regole di
   `pickEntity` (3 pixel, entità visibili, a pari distanza l'identificativo più
   alto). Il comando riceve `ObjectInput{id, point, shift}`: `point` è il punto
   del clic, `shift` dice se era premuto `Shift` (negli script
   `!click x,y remove`).
   - Se lì non c'è nessuna entità, la domanda resta, senza messaggio. In AutoCAD
     un clic nel vuoto apre un rettangolo di selezione: in Tecnigrafo resta
     fuori, con le opzioni *Crossing* e *Fence*.
   - Un'entità su un layer bloccato non si sceglie: compare il messaggio della
     selezione, `1 was on a locked layer.` (ADR 0023, punto 4), e la domanda
     resta. È lo stesso comportamento che AutoCAD ha mostrato con `TRIM`.
   - Lo snap e l'orto non correggono il punto: si sceglie, non si disegna.
2. **`PromptKind::Number`.** Un numero puro, senza unità né angolo (il fattore di
   `SCALE`). Un clic arriva come `PointInput`, come alle richieste di distanza: è
   il comando a misurare dal suo punto base.
3. **`PromptKind::Selection`: una selezione chiesta a metà comando.** Alcuni
   comandi chiedono una selezione dopo altre domande, con un testo loro: i bordi
   di `TRIM` nel modo standard (`Select objects or [mOde] <select all>:`) e gli
   oggetti da unire di `JOIN` negli script (`Select lines to join to source:`).
   Il runner accetta i gesti di selezione dell'ADR 0008 e la selezione scritta
   (punto 4), ripete la domanda del comando dopo ogni gesto, e con Invio passa
   al comando una `SelectionInput`, vuota se non si è scelto nulla: che cosa
   vuol dire una selezione vuota (`<select all>`) lo decide il comando. Le
   parole chiave della domanda funzionano come nelle altre richieste.
4. **Selezione scritta.** A ogni richiesta di selezione, quella del runner
   (`Select objects:`) e quella del punto 3, si può scrivere:
   - un punto: sceglie l'entità sotto quel punto, come un clic;
   - `W`, poi due punti (`Specify first corner:`, `Specify opposite corner:`):
     una finestra, in qualunque direzione;
   - `C`, poi due punti: un'intersezione, in qualunque direzione;
   - `ALL`: tutte le entità selezionabili (visibili e non su layer bloccati).

   I messaggi sono quelli dei gesti (`<n> found, <t> total`, layer bloccati).
   È ciò che gli script di AutoCAD usano più spesso per scegliere gli oggetti;
   le altre opzioni (*Last*, *Previous*, *Fence*, *WPolygon*…) restano fuori.
5. **Descrizione dei comandi.** `ValueType` riceve `Object` e `Selection`;
   `Real` serve già per i numeri. `tecnigrafo-mcp` e
   `tecnigrafo-script describe` li riportano.

### Sequenze degli script di AutoCAD

6. **Due sequenze.** Un comando che in AutoCAD cambia sequenza negli script ha
   in Tecnigrafo due sequenze: quella **interattiva**, di chi usa il programma, e
   quella **degli script**. Il runner sceglie con `setScriptSequences(bool)`, e
   il comando legge la scelta nel suo contesto (`CommandContext::scriptSequences`).
7. **Quando valgono le sequenze degli script:**
   - eseguendo un file `.scr` (riconosciuto dall'estensione), con
     `tecnigrafo-script` e con `--script` del programma;
   - in un file `.tgs`, con la direttiva `!scr on`, fino a `!scr off`. Senza la
     direttiva un `.tgs` simula chi usa il programma, quindi vale la sequenza
     interattiva. Così i casi di verifica provano entrambe.
8. **I comandi con due sequenze** sono quelli che AutoCAD tratta così, uno per
   uno, a mano a mano che Tecnigrafo li realizza. Nella 0.7:

   | Comando | Interattiva | Script |
   |---|---|---|
   | `TEXT` | una riga dopo l'altra, finché non si preme Invio su una riga vuota | una riga sola, poi il comando finisce |
   | `TRIM`, `EXTEND` | modo rapido o standard, come `TRIMEXTENDMODE` (ADR 0035) | sempre standard |
   | `FILLET`, `CHAMFER` | dopo *Radius*, *Distance*, *Angle* la domanda dei due oggetti torna | dopo quelle opzioni il comando finisce |
   | `COPY` | copie multiple fino a Invio | una copia, poi il comando finisce |
   | `JOIN` | più oggetti insieme | prima l'oggetto sorgente, poi gli oggetti da unire, con le regole di AutoCAD per tipo (ADR 0035) |

   La tabella si tiene in [docs/verifica/script.md](../verifica/script.md), e ogni
   ADR che porta un comando nuovo dice se ha due sequenze.
9. **Descrizione.** Le domande delle due sequenze stanno tutte nella descrizione
   del comando (ADR 0026), così il controllo di completezza vale per entrambe.

## Alternative considerate

- **La selezione dell'ADR 0008 con un punto in più:** chi sceglie un oggetto per
  `TRIM` non sta costruendo una selezione. Mischiare le due cose cambierebbe i
  messaggi `<n> found` e il comportamento di `Shift`.
- **Il comando che chiama `pickEntity` da sé su un `PointInput`:** ogni comando
  rifarebbe la regola dei layer bloccati e la tolleranza in pixel, che sono del
  runner (ADR 0008).
- **Una sola sequenza, quella moderna:** gli script `.scr` di AutoCAD con
  `TRIM`, `FILLET`, `COPY`, `JOIN` o `TEXT` darebbero risultati diversi o si
  fermerebbero a metà, contro l'ADR 0011.
- **Una sola sequenza, quella degli script:** chi usa il programma avrebbe il
  comportamento di AutoCAD di quindici anni fa.
- **La sequenza degli script anche nei `.tgs`:** i casi di verifica non
  proverebbero mai il comportamento interattivo, che è quello che conta per chi
  usa il programma.

## Conseguenze

- `Input` riceve `ObjectInput`; i comandi esistenti lo ignorano.
- Gli script `.scr` di AutoCAD che scelgono gli oggetti con un punto o con `W`
  e `C` funzionano anche in Tecnigrafo, per tutti i comandi con selezione.
- `TEXT` cambia nella sequenza degli script: un `.scr` che scrive un testo e poi
  un comando funziona come in AutoCAD.
- Il modulo `script` riconosce l'estensione `.scr` e la direttiva `!scr`.
- Ogni comando nuovo va confrontato con AutoCAD anche dentro uno script. Il modo
  più semplice è quello usato per i DXF di prova: lo stesso `.scr` eseguito in
  AutoCAD LT.
