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
| 2 | 0.2 | Riga di comando, primo comando di disegno, annullamento | **chiusa** | [0.2](tappe/0.2-comandi-e-annullamento.md) |
| 3 | 0.3 | Selezione, cancellazione, snap e orto | **chiusa** | [0.3](tappe/0.3-selezione-e-snap.md) |
| 4 | 0.4 | Entità base: linea, cerchio, arco, testo | **chiusa** | [0.4](tappe/0.4-entita-base.md) |
| 5 | 0.5 | File: formato nativo e DXF | **chiusa** | [0.5](tappe/0.5-file-e-dxf.md) |
| 6 | 0.6 | Layer, colori, tipi di linea, proprietà; impianto dell'interfaccia; descrizione dei comandi e server MCP di sviluppo | **chiusa** | [0.6](tappe/0.6-layer-e-proprieta.md) |
| 7 | 0.7 | Comandi di modifica | **chiusa** | [0.7](tappe/0.7-modifiche.md) |
| 7.5 | 0.7.5 | Correzione di rotta: CI e metodo, interfaccia rifatta e pronta per gli ambienti, basi della 0.8 e del 3D | **chiusa** | [0.7.5](tappe/0.7.5-correzione-di-rotta.md) |
| 8 | 0.8 | Quote, retini, blocchi; canale MCP per l'assistente IA dell'utente | **chiusa** | [0.8](tappe/0.8-quote-retini-blocchi.md) |
| 8.1 | 0.8.1 | Completamenti senza richieste al kernel: pannello delle proprietà, `QSELECT` e `-HATCHEDIT` per i tipi della 0.8 (#913); comandi degli attributi (`-ATTDEF`, `ATTREQ`, `-ATTEDIT`, `ATTSYNC`, `BATTMAN`, `ATTDISP`); editor del testo con i formati e `TEXTEDIT` (#662); griglia, snap alla griglia e icona dell'UCS, anche dal DXF (#664); snap *Vicino*, *Prolungamento* e *Parallelo* (#665); segno sul grip del vertice corrente scorso con *Current Vertex* nel pannello Proprietà (#799); selezione *Last* e *Previous*; calamita degli snap; prefisso `#` con l'immissione dinamica; contorni evidenziati in `-HATCH`; alias brevi di AutoCAD; `ELLIPSE` nel manuale; `RECTANG` e `POLYGON` | **chiusa** | [0.8.1](tappe/0.8.1-completamenti.md) |
| 8.2 | 0.8.2 | Curve, con la tappa gemella di solid-space: `SPLINE` completa come AutoCAD (#888), ellissi in `TRIM`, `EXTEND`, `OFFSET`, `FILLET` e `JOIN` (#663), `BREAK` e `LENGTHEN`, contorni dei retini con ellissi e spline (nella 0.8.3 se la gemella non li regge) | in programma | [0.8.2](tappe/0.8.2-curve.md) |
| 8.3 | 0.8.3 | Blocchi e simboli: finestre di `BLOCK` e `INSERT` con la libreria di simboli, editor dei blocchi (`BEDIT`, `BCLOSE`), `WBLOCK`, `PURGE` e `RENAME` (#919); tipi di linea e larghezze continui negli inserimenti stirati (#762); Tecnifont, il font con i simboli del disegno tecnico (#1011) | in programma | [0.8.3](tappe/0.8.3-blocchi.md) |
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
3. crea su GitHub il milestone `vX.Y`, una issue per ogni pacchetto di lavoro,
   con le etichette `tappa-X.Y`, `pacchetto`, la difficoltà e l'agente, e una
   issue `[X.Y-RN]` con l'etichetta `riferimenti` per ogni pacchetto che ha
   riferimenti (ADR 0011, [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md));
4. aggiorna la tabella qui sopra.

I punti 3 e 4 li fa `scripts/apri-tappa.sh X.Y`, dalla tabella dei pacchetti
del piano ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B6). Se la tappa
chiede qualcosa a solid-space, si apre insieme a lei la tappa corrispondente di
solid-space, e le due avanzano insieme
([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 13).

### Chiudere una tappa

1. Tutti i pacchetti sono uniti e tutti i criteri di chiusura sono verificati, uno
   per uno, e spuntati nel documento della tappa. Lo fa la pull request di
   chiusura, che aggiorna il [catalogo di parità](parita/comandi.md) e prepara
   anche le note di rilascio
   ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), "Chiudere la tappa").
2. La CI è verde sui tre sistemi operativi: su Windows, Linux e macOS il programma
   si compila senza avvisi e tutti i test automatici passano.
3. La prova manuale descritta nel documento è stata eseguita su Windows e Linux.
   Su macOS si esegue quando c'è un revisore che lo usa, e l'esito si annota nelle
   note di rilascio; la sua mancanza non blocca la chiusura ([DECISIONI.md](DECISIONI.md), 2026-10-16).
   Per le sottotappe 0.8.x la prova si fa una volta sola, al termine di tutte
   ([DECISIONI.md](DECISIONI.md), 2026-11-28).
4. Si crea il tag `vX.Y.0` con le note di rilascio (`docs/rilasci/X.Y.md`); per
   una tappa con tre numeri il tag è `vX.Y.Z` (come `v0.7.5`). Se la tappa
   gemella di solid-space ha pubblicato una versione, la pull request di
   chiusura porta il sottomodulo al suo tag ([DECISIONI.md](DECISIONI.md),
   2026-11-21).
5. Si aggiorna la tabella. Poi la **pausa tra le tappe**: con tutto fermo, il
   responsabile decide le idee aperte, le proposte di metodo e le correzioni dei
   documenti, che una pull request di soli documenti applica
   ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B13). Solo dopo si apre la
   tappa successiva.

### Cosa si può fare fuori dalla tappa aperta

- Correggere bug delle tappe chiuse e completare: una versione `X.Y.Z`
  contiene correzioni di bug **e completamenti**, cioè parti di funzioni già
  presenti che una tappa ha lasciato fuori esplicitamente, o lacune rispetto ad
  AutoCAD che il responsabile giudica importanti per l'uso reale. Mai funzioni
  di un'area nuova ([DECISIONI.md](DECISIONI.md), 2026-11-18).
- Lavorare sulla grafica **a lato** della tappa aperta: in ogni tappa si può
  aprire un pacchetto fuori tappa che tocca solo l'aspetto dell'interfaccia,
  come [F6](tappe/fuori-tappa/F6-aspetto-dell-interfaccia.md). Corre in
  parallelo ai pacchetti senza fermarli; alla fine si riallinea con `main`,
  porta l'aspetto anche sui comandi nuovi e si unisce una volta sola
  ([DECISIONI.md](DECISIONI.md), 2026-11-21).
- Migliorare documentazione e test.
- Proporre idee per le tappe future, con una issue *Idea*. Le idee si discutono,
  non si implementano: se approvate, il responsabile le inserisce in una scheda.

Nient'altro. Un lavoro fuori da queste categorie viene chiuso senza revisione,
anche se è scritto bene.

### Spostare o cambiare una tappa

Il contenuto di una tappa *in programma* può cambiare con una decisione del
responsabile, registrata in [DECISIONI.md](DECISIONI.md). Il contenuto della tappa *aperta* cambia
solo se un pacchetto si rivela impossibile o sbagliato: si aggiorna il piano e se
ne spiega il motivo nella sezione "Modifiche al piano" del documento della tappa.

## Decisioni

Le decisioni del responsabile che cambiano le regole o le tappe, in ordine di
data, sono in [DECISIONI.md](DECISIONI.md).
