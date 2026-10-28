# ADR 0036: Tolleranze del lato di Tecnigrafo

- **Stato:** Proposto (bozza: si completa e si accetta dopo la valutazione di
  solid-space della richiesta [#329](https://github.com/Matleyx78/tecnigrafo/issues/329))
- **Data:** 2026-10-28

## Contesto

L'ADR 0030 (punto 2) divide le tolleranze in due parti. La politica delle
tolleranze geometriche (quando due punti coincidono, quando due curve sono
allineate o tangenti) spetta a solid-space. Restano a Tecnigrafo le tolleranze
in pixel, quelle con cui i casi di verifica confrontano i numeri, e il
trattamento dei valori che l'utente scrive.

Oggi queste regole sono sparse:

- in pixel, negli ADR 0008, 0033 e 0034: scelta 3, snap 10, grip 10, polare 10;
- nel confronto dei casi, `1e-9` assoluto, cambiabile con `!tolerance` (ADR
  0011, punto 6);
- nelle soglie di `core` (ADR 0030, conseguenze).

La scheda della tappa 0.7 chiede di raccoglierle in un ADR, dopo la valutazione
di solid-space.

## Decisione (bozza)

1. **Pixel.** Le tolleranze in pixel si raccolgono qui, con i loro valori e il
   loro scopo: scelta di un'entità 3, apertura dello snap 10, quadrato dei grip
   10, aggancio del polare 10. Si cambiano solo con un ADR, perché i casi di
   verifica dipendono da loro attraverso `!view`.
2. **Confronto dei casi.** Il confronto assoluto `1e-9` resta per tutto ciò che
   Tecnigrafo calcola da sé. Per i risultati delle costruzioni di solid-space
   (intersezioni, raccordi, curve parallele) vale l'errore che solid-space
   dichiara, scritto nei casi con `!tolerance`.
   *Da completare con l'errore dichiarato dalla valutazione di #326, #327 e #328.*
3. **Valori scritti dall'utente.** Un raggio, una distanza o un fattore uguali a
   0, o non finiti, si trattano come in AutoCAD:
   - raggio 0 in `FILLET`: spigolo;
   - distanza 0 in `OFFSET`: rifiutata;
   - fattore 0 in `SCALE`: rifiutato.

   Il confine tra "zero" e "piccolo" per i valori scritti è il confronto esatto
   con 0: il numero scritto dall'utente è un dato, non un risultato da arrotondare.
4. **Risultati degeneri.** Quando un pezzo dopo un taglio, un segmento dopo un
   offset o un arco dopo un grip ha lunghezza nulla, decide la regola di
   solid-space.
   *Da completare con la valutazione di #329.*
5. **Soglie di `core`.** Le soglie oggi in `arc.cpp`, `arc_geometry.cpp`,
   `polyline.cpp` e `proximity.cpp` si sostituiscono con la regola di solid-space,
   in un pacchetto proprio. Il comportamento non deve cambiare nei casi di
   verifica esistenti; se cambia, il pacchetto si ferma e decide il responsabile.
   *Da completare: elenco delle soglie, e per ognuna la regola che la sostituisce
   o il motivo per cui resta.*

## Alternative considerate

- **Scrivere l'ADR prima della valutazione di solid-space:** fisserebbe numeri
  che solid-space potrebbe dover cambiare. La scheda della tappa lo esclude.
- **Lasciare le tolleranze sparse negli ADR:** chi scrive un caso di verifica non
  sa dove trovarle.

## Conseguenze

- Fino all'accettazione valgono le regole di oggi.
- Se il punto 2 cambia il confronto dei casi, serve un pacchetto nel modulo
  `script`, da aggiungere al piano in "Modifiche al piano".
