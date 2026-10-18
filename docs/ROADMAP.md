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
| 4 | 0.4 | Entità base: linea, cerchio, arco, testo | in preparazione | [0.4](tappe/0.4-entita-base.md) |
| 5 | 0.5 | File: formato nativo e DXF | in programma | [0.5](tappe/0.5-file-e-dxf.md) |
| 6 | 0.6 | Layer, colori, tipi di linea, proprietà; impianto dell'interfaccia | in programma | [0.6](tappe/0.6-layer-e-proprieta.md) |
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
3. crea su GitHub il milestone `vX.Y`, una issue per ogni pacchetto di lavoro,
   con le etichette `tappa-X.Y`, `pacchetto`, la difficoltà e l'agente, e una
   issue `[X.Y-RN]` con l'etichetta `riferimenti` per ogni pacchetto che ha
   riferimenti (ADR 0011, [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md));
4. aggiorna la tabella qui sopra.

### Chiudere una tappa

1. Tutti i pacchetti sono uniti e tutti i criteri di chiusura sono verificati, uno
   per uno, e spuntati nel documento della tappa. Lo fa la pull request di
   chiusura, che prepara anche le note di rilascio
   ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), "Chiudere la tappa").
2. La CI è verde sui tre sistemi operativi: su Windows, Linux e macOS il programma
   si compila senza avvisi e tutti i test automatici passano.
3. La prova manuale descritta nel documento è stata eseguita su Windows e Linux.
   Su macOS si esegue quando c'è un revisore che lo usa, e l'esito si annota nelle
   note di rilascio; la sua mancanza non blocca la chiusura (vedi "Decisioni").
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

## Decisioni

Decisioni del responsabile che cambiano le regole o le tappe, in ordine di data.

- **2026-10-16 — Prova manuale su macOS non obbligatoria.** Il responsabile non
  dispone di un Mac. macOS resta un sistema supportato: la CI lo compila e ne
  esegue tutti i test a ogni pull request, come per Windows e Linux. La prova
  manuale su macOS (scorciatoie con `Cmd`, menu, schermi Retina, trackpad) si fa
  quando un revisore che lo usa è disponibile, e il suo esito, o la sua mancanza,
  si scrive nelle note di rilascio. Vale dalla tappa 0.2 (vedi "Modifiche al
  piano" in [tappe/0.2](tappe/0.2-comandi-e-annullamento.md)).
- **2026-10-16 — Impianto dell'interfaccia nella tappa 0.6.** Il responsabile
  sceglie: ribbon come in AutoCAD, con la possibilità per l'utente di passare
  all'interfaccia classica a menu e barre; pannelli agganciabili; icone tutte
  nostre in un unico stile; tema chiaro o scuro a scelta; scorciatoie
  personalizzabili. Le aree di lavoro si decideranno insieme agli ambienti del 3D.
  Il lavoro entra nella tappa 0.6, che introduce i primi pannelli, secondo
  l'[ADR 0009](adr/0009-interfaccia-grafica.md) (proposto, da accettare
  all'apertura della tappa).
- **2026-10-18 — Modalità di controllo, con la tappa 0.4 come banco di prova.**
  Il responsabile sceglie la verifica automatica proposta nell'[ADR 0011](adr/0011-modalita-di-controllo.md)
  (proposto, da accettare all'apertura della tappa 0.4):
  per ogni pacchetto che cambia il comportamento, un altro agente scrive prima i
  casi di verifica, e il lavoro è finito quando la CI conferma che il programma li
  rispetta. Di norma i riferimenti li scrive Gemini; Claude lavora ai pacchetti
  medi e impegnativi, Codex a quelli facili e medi
  ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md)). La tappa 0.4 comincia con i
  quattro pacchetti che costruiscono il sistema di verifica (ADR 0011, punto 13);
  alla sua chiusura si valuta se il metodo ha alleggerito il lavoro. I file DXF e,
  in futuro, STEP presi da raccolte pubbliche non si copiano nel repository: si
  scaricano con hash fissato (ADR 0011, punto 6). Cambia di conseguenza il rischio
  annotato nella scheda della [tappa 0.5](tappe/0.5-file-e-dxf.md).
- **2026-10-18 — Piano della tappa 0.4 e scelte collegate.** Il piano dettagliato
  della [tappa 0.4](tappe/0.4-entita-base.md) è in preparazione, con gli ADR
  0012–0015 proposti e l'[ADR 0011](adr/0011-modalita-di-controllo.md) esteso
  alle **domande al modello**, che danno riferimenti anche alla geometria non
  ancora raggiungibile dai comandi, in 2D e in futuro per superfici e solidi. Il
  responsabile sceglie: font incluso **Liberation Sans** (licenza OFL, ADR 0013);
  **`Esc` conserva il lavoro già completato** in `LINE`, `PLINE` e `TEXT`, come
  in AutoCAD (ADR 0015, che sostituisce in parte l'ADR 0005); interlinea del
  testo 5/3 dell'altezza, da rivedere. La [tappa 0.5](tappe/0.5-file-e-dxf.md)
  eredita il disegno di tutti gli allineamenti del testo, dell'obliquo e della
  specchiatura, e la gestione di quota, spessore ed estrusione diversi dai
  valori predefiniti (ADR 0012 e 0013).
