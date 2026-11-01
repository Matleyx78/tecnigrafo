# [F4] `scripts`: le dipendenze tra Tecnigrafo e solid-space

- **Tappa:** fuori tappa, strumento del responsabile come
  `scripts/apri-tappa.sh` (decisione del 2026-11-01,
  [DECISIONI.md](../../DECISIONI.md))
- **Issue:** #450
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** forte; impegno medio
- **Riferimenti:** non servono: lo script non cambia il comportamento di
  Tecnigrafo. Lo provano i test di unità con testi e risposte dell'API
  costruiti apposta
- **Dipende da:** —
- **Modulo:** `scripts` (più un passo della CI e la guida del responsabile)
- **Da leggere:** [RICHIESTE-A-SOLID-SPACE.md](../../RICHIESTE-A-SOLID-SPACE.md);
  [GUIDA-RESPONSABILE.md](../../GUIDA-RESPONSABILE.md), B6 e B11;
  [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), sezione 2;
  [tappe/0000-modello.md](../0000-modello.md), sezioni 5 e 6;
  `scripts/apri-tappa.sh` (lettura della tabella, forme di "Dipende da",
  Project); come esempio di tabelle vere, il
  [piano 0.7](../0.7-modifiche.md) e il
  [piano 0.2 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/tappe/0.2-geometria-del-piano.md),
  sezione 6, con «Che cosa sblocca in Tecnigrafo»

## Perché

Le dipendenze di una issue di Tecnigrafo stanno in quattro posti: la tabella
del piano, la riga "Dipende da" della issue, la relazione «bloccata da» di
GitHub e lo stato nel Project. Quando dipendono da solid-space, chi le aggiorna
deve anche leggere il piano di solid-space. Oggi lo fa il responsabile a mano,
e il 2026-11-01 R14 (#356) e R22 (#372) erano ferme da giorni con tutti i loro
pacchetti di solid-space già uniti, perché "Dipende da" citava ancora le
richieste. Lo script fa questo controllo in modo meccanico.

## File

- `scripts/dipendenze.py` (nuovo)
- `scripts/dipendenze_test.py` (nuovo)
- `.github/workflows/ci.yml`: un job o un passo che esegue i test dello script
  (questa scheda lo autorizza)
- `docs/GUIDA-RESPONSABILE.md`: B6 (parte di solid-space, passo 5) e B11
  (punto 2), dove oggi il controllo è a mano

## Cosa fare

Uno script Python, solo libreria standard (Python 3.11 o più recente), che
chiama l'API di GitHub con `gh api`, come `apri-tappa.sh`. Si lancia dal PC del
responsabile, perché il Project si legge solo con GraphQL.

```text
scripts/dipendenze.py <tappa> [--applica] [--senza-project]
```

`<tappa>` è una tappa di Tecnigrafo (`0.7`). In testa al file, come in
`apri-tappa.sh`, un commento in inglese spiega uso, opzioni, permessi di `gh`
e codici di uscita.

### Che cosa legge

1. **Il piano di Tecnigrafo**, `docs/tappe/<tappa>-*.md` della cartella
   locale: la tabella della sezione 6 (Pacchetto, Issue, Dipende da, Agente),
   con le stesse regole e forme di `apri-tappa.sh` (`PN`, `RN`, `#N`,
   `owner/repo#N`), e la tabella «Richieste a solid-space» della sezione 5
   (Richiesta, Pacchetti che ne dipendono, Si chiude con). **Il piano fa fede**:
   issue e Project si allineano al piano, mai il contrario.
2. **Le issue di Tecnigrafo** delle righe del piano: stato, etichette, la riga
   `**Dipende da:**` del testo, le relazioni `dependencies/blocked_by`.
3. **solid-space, solo in lettura**: lo stato e le etichette delle issue citate;
   dalla tabella "Stato" di `docs/ROADMAP.md` la tappa di solid-space la cui
   colonna «Per la tappa di Tecnigrafo» comincia con `<tappa>`, il suo stato e
   il suo piano; dal piano la tabella dei pacchetti della sezione 6 (Pacchetto,
   Issue, Dipende da) e la tabella «Che cosa sblocca in Tecnigrafo». Nelle
   celle di quest'ultima valgono i nomi (`P8`, `R14a`), gli intervalli
   (`P4–P10`, con il trattino lungo o corto, nell'ordine della tabella dei
   pacchetti) e le parentesi (`P12, P13, P14 (con P4–P10)`: tutti servono).
   I file si leggono con l'API (`contents`), alla versione di `main`.
   solid-space è pubblico: se `gh api` rifiuta una lettura (per esempio in una
   sessione di un agente che non ha solid-space collegato), lo script ripiega
   su `curl` senza token, come `scripts/ci-errori.sh`.
4. **Il Project «Tecnigrafo»**, salvo `--senza-project`: il campo *Status*
   degli elementi che corrispondono alle issue.

### Che cosa controlla

Per ogni riga del piano con la issue **aperta**, nell'ordine del piano:

1. **Pronta o bloccata.** È pronta se tutte le issue delle sue dipendenze nel
   piano sono chiuse (una dipendenza `PN` o `RN` è la issue della sua riga).
   Altrimenti elenca le issue aperte che la bloccano, nei due repository. Se
   porta `AA-Presa in carico`, lo dice.
2. **Testo.** L'insieme dei riferimenti di "Dipende da" nel testo è diverso da
   quello del piano. Le annotazioni tra parentesi non contano.
3. **Relazioni.** L'insieme di «bloccata da» è diverso da quello del piano.
4. **Project.** Lo stato è *Bloccato* e la issue è pronta, oppure è *Da fare* o
   vuoto e la issue è bloccata. Gli altri stati non si toccano.
5. **Richiesta al posto dei pacchetti.** Una dipendenza del piano è una issue
   di solid-space con l'etichetta `richiesta`, e la tappa di solid-space
   corrispondente è aperta, quindi ha i suoi pacchetti. Lo script propone la
   cella "Dipende da" nuova, pronta da copiare nel piano: le dipendenze di
   Tecnigrafo della riga, più i pacchetti di solid-space della sua riga di «Che
   cosa sblocca», tolti quelli già compresi attraverso le dipendenze (le catene
   di "Dipende da" del piano di solid-space, e i pacchetti di solid-space già
   citati, direttamente o attraverso altre righe, dalle dipendenze di
   Tecnigrafo della riga). Le righe si trattano nell'ordine del piano, e per
   una dipendenza di Tecnigrafo che ha a sua volta una proposta vale la
   proposta. Se la issue non sta in «Che cosa sblocca», lo dice e non propone
   nulla.

Poi, una volta per tappa:

6. **Richieste da chiudere.** Una richiesta della tabella «Richieste a
   solid-space» è aperta e le issue di «Si chiude con» sono tutte chiuse: «verifica
   il "quando è soddisfatta" e chiudi Matleyx78/solid-space#N». Lo script non
   la chiude mai.
7. **Etichette.** L'etichetta `agente:…` della issue è diversa dalla colonna
   *Agente* del piano: solo un'informazione, perché decide l'etichetta
   (AGENTS.md) e il responsabile riassegna i pacchetti cambiandola. Non conta
   per il codice di uscita.

### Che cosa stampa

Un elenco in inglese, come `apri-tappa.sh`: prima le issue pronte (quelle da
spostare o da avviare), poi le bloccate con chi le blocca, poi le differenze
dei controlli 2–7, ciascuna con la correzione. Esce con **0** se non c'è niente
da correggere né da verificare (controlli 2–6), **1** se c'è, **2** se una
lettura fallisce o una tabella non si legge (messaggio con file e riga).

### Con `--applica`

Mostra le modifiche ai controlli 2, 3 e 4 e chiede `Go on? [s/N]`. Con `s`:
riscrive la riga "Dipende da" del testo dal piano, nella forma di
`apri-tappa.sh` (`#12 (P3)`; per solid-space `Matleyx78/solid-space#49 (0.2-P8)`,
con il tag preso dal titolo della issue), aggiunge e toglie le relazioni
«bloccata da», cambia lo stato nel Project. Non scrive mai il piano (le
proposte del controllo 5 si copiano nel piano con una pull request), non chiude
issue e **non scrive niente in solid-space**. Un errore su una issue non ferma
le altre: alla fine elenca quelle non riuscite ed esce con 2.

### Guida del responsabile

In B6, parte di solid-space, il passo 5 nuovo (sostituire le richieste con i
pacchetti) diventa: lanciare lo script, copiare nel piano le proposte del
controllo 5 con una pull request e, unita quella, lanciarlo con `--applica`. In
B11, punto 2, lo script sostituisce il conto a mano dell'ultimo pacchetto della
riga e lo spostamento delle schede nel Project.

## Regole della tappa che valgono qui

- Codice, commenti e messaggi dello script in inglese; la guida in italiano.
- Solo libreria standard di Python e `gh`: nessuna dipendenza nuova.
- In solid-space lo script legge soltanto (AGENTS.md, "Dove puoi scrivere").
- La lettura della tabella dei pacchetti segue le stesse regole di
  `apri-tappa.sh`: una tabella che uno dei due legge e l'altro no è un errore.

## Test richiesti

In `scripts/dipendenze_test.py`, con `unittest`, senza rete: la funzione che
legge l'API si sostituisce con risposte scritte nel test.

- La tabella del piano con dipendenze nelle quattro forme; una riga malformata
  dà un errore con il numero di riga.
- «Che cosa sblocca» con nomi, intervalli con i due trattini e parentesi.
- Pronta e bloccata, con dipendenze nei due repository.
- Testo e relazioni diversi dal piano: le differenze giuste, in tutti e due i
  versi; annotazioni tra parentesi ignorate.
- La proposta del controllo 5 con le righe vere della tappa 0.7 prima del
  2026-11-01: R14 → R4b, R8a, R8b, R9, R10a, R10b, R11; P14 → P0, ss P8, ss P11,
  R14; P17 → P14, ss P14, R17; P22 → P14, R22 (i pacchetti di solid-space
  già compresi spariscono).
- Una richiesta con le issue di «Si chiude con» chiuse è segnalata; con una
  aperta no.
- `--applica` costruisce le chiamate giuste (testo, relazioni, Project) e non
  ne fa nessuna verso solid-space.
- Codici di uscita 0, 1 e 2.

## Fatto quando

- Con i dati veri e il piano 0.7 di `main` (dopo la pull request #449),
  `scripts/dipendenze.py 0.7 --senza-project` gira e la sua uscita, incollata
  nella pull request, non segnala differenze di testo, relazioni né proposte
  sulle otto issue della seconda metà allineate il 2026-11-01.
- Tutti i test richiesti esistono e passano, anche nella CI.
- La guida del responsabile, B6 e B11, usa lo script.
- La CI è verde.
