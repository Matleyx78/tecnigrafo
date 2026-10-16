# Roadmap

Tecnigrafo cresce per **tappe**. Ogni tappa ha un obiettivo, dei confini precisi e
dei criteri di chiusura verificabili. Le tappe si percorrono una alla volta, in
ordine: la successiva si apre solo quando la precedente è chiusa.

Le tappe le definisce e le pubblica il responsabile del progetto. Chi contribuisce
lavora **solo** sui pacchetti di lavoro della tappa aperta.

## Stato

| Tappa | Versione | Obiettivo | Stato | Documento |
|---|---|---|---|---|
| 1 | 0.1 | Fondamenta: architettura, CI, polilinee, vista navigabile | **chiusa** | [0.1](tappe/0.1-fondamenta.md) |
| 2 | 0.2 | Riga di comando, primo comando di disegno, annullamento | **aperta** | [0.2](tappe/0.2-comandi-e-annullamento.md) |
| 3 | 0.3 | Selezione, cancellazione, snap e orto | in programma | [0.3](tappe/0.3-selezione-e-snap.md) |
| 4 | 0.4 | Entità base: linea, cerchio, arco, testo | in programma | [0.4](tappe/0.4-entita-base.md) |
| 5 | 0.5 | File: formato nativo e DXF | in programma | [0.5](tappe/0.5-file-e-dxf.md) |
| 6 | 0.6 | Layer, colori, tipi di linea, proprietà | in programma | [0.6](tappe/0.6-layer-e-proprieta.md) |
| 7 | 0.7 | Comandi di modifica | in programma | [0.7](tappe/0.7-modifiche.md) |
| 8 | 0.8 | Quote, retini, blocchi | in programma | [0.8](tappe/0.8-quote-retini-blocchi.md) |
| 9 | 0.9 | Stampa, PDF, traduzioni, installatori | in programma | [0.9](tappe/0.9-stampa-e-distribuzione.md) |
| 10 | 1.0 | Stabilità e prestazioni: CAD 2D per lavoro reale | in programma | [1.0](tappe/1.0-stabilita.md) |
| — | 2.x | Modellatore 3D parametrico | da definire | [2.x](tappe/2.x-modellatore-3d.md) |

## Regole delle tappe

### Stati

- **In programma**: esiste una *scheda* con obiettivo, confini, criteri di chiusura
  e progetto di massima. Non si scrive codice.
- **In preparazione**: il *piano dettagliato* è scritto, ma ADR, milestone e issue
  non sono ancora pubblicati. Si può discutere il piano, non si scrive codice.
- **Aperta**: il piano dettagliato è pubblicato, con il milestone su GitHub e una
  issue per ogni pacchetto di lavoro. È l'unica tappa su cui si lavora.
- **Chiusa**: tutti i criteri di chiusura sono verificati, la versione è pubblicata
  con le note di rilascio. Il documento della tappa diventa storia e non si modifica.

### Aprire una tappa

Solo il responsabile apre una tappa, e solo dopo aver chiuso la precedente:

1. trasforma la scheda in piano dettagliato, partendo da
   [tappe/0000-modello.md](tappe/0000-modello.md);
2. scrive e accetta gli ADR che la tappa richiede;
3. crea su GitHub il milestone `vX.Y` e una issue per ogni pacchetto di lavoro,
   con le etichette `tappa-X.Y`, `pacchetto` e la difficoltà;
4. aggiorna la tabella qui sopra.

### Chiudere una tappa

1. Tutti i pacchetti sono uniti e tutti i criteri di chiusura sono verificati, uno
   per uno, e spuntati nel documento della tappa.
2. La CI è verde sui tre sistemi operativi.
3. La prova manuale descritta nel documento è stata eseguita su Windows, Linux e
   macOS.
4. Si crea il tag `vX.Y.0` con le note di rilascio (`docs/rilasci/X.Y.md`).
5. Si aggiorna la tabella e si apre la tappa successiva.

### Cosa si può fare fuori dalla tappa aperta

- Correggere bug delle tappe chiuse: versioni `X.Y.Z` di sola correzione.
- Migliorare documentazione e test.
- Proporre idee per le tappe future, con una issue *Idea*. Le idee si discutono,
  non si implementano: se approvate, il responsabile le inserisce in una scheda.

Nient'altro. Un lavoro fuori da queste categorie viene chiuso senza revisione,
anche se è scritto bene.

### Spostare o cambiare una tappa

Il contenuto di una tappa *in programma* può cambiare con una decisione del
responsabile, registrata in questo file. Il contenuto della tappa *aperta* cambia
solo se un pacchetto si rivela impossibile o sbagliato: si aggiorna il piano e se
ne spiega il motivo nella sezione "Modifiche al piano" del documento della tappa.
