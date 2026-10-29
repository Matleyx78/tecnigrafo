# Come chiedere qualcosa a solid-space

[solid-space](https://github.com/Matleyx78/solid-space) è il kernel geometrico di
Tecnigrafo ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md)) e cresce
**solo su richiesta di Tecnigrafo**. Quando a Tecnigrafo serve un comportamento
geometrico che non c'è, si scrive una richiesta.

La richiesta **vive solo in solid-space**, in una issue con l'etichetta
`richiesta`: in Tecnigrafo non se ne tiene una copia, e il piano e i pacchetti
citano quella issue. Le regole per scriverla e valutarla stanno là (modello:
[docs/richieste/0000-modello.md](https://github.com/Matleyx78/solid-space/blob/main/docs/richieste/0000-modello.md),
decisione:
[ADR 0007](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)).
Questo file dice soltanto che cosa si fa dal lato di Tecnigrafo.

## Quando

Si scrive una richiesta quando serve qualcosa che spetta a solid-space (ADR 0030,
punto 2: da che parte sta un punto, se due elementi si intersecano, sono tangenti
o coincidono, un'intersezione o un raccordo con un errore garantito, un vincolo,
una faccia riconosciuta dopo una modifica, una booleana). Non si scrive per ciò
che resta di Tecnigrafo: comandi, interfaccia, annulla e ripeti, formati di file,
tolleranze in pixel.

Il bisogno si cerca **quando si prepara il piano** di una tappa, e le richieste
entrano nella sezione "Richieste a solid-space" del piano
([tappe/0000-modello.md](tappe/0000-modello.md)). Se emerge **durante la
tappa**, l'agente che lo trova si ferma e ne scrive il testo nella issue del suo
pacchetto (AGENTS.md, "Se serve qualcosa a solid-space"); il responsabile decide
se aprirla. Conviene aprirla appena si vede il bisogno.

## Che cosa contiene

Sette campi, tutti obbligatori. Dicono **di che cosa c'è bisogno, non come
soddisfarlo**: niente «usiamo OCCT», niente nomi o firme di funzioni, niente
valori attesi. Se basta OCCT, se va corretto o se serve un pezzo nuovo lo decide
solid-space.

1. **Titolo:** «Richiesta: » e il bisogno in una frase.
2. **Da dove nasce:** la tappa di Tecnigrafo e la issue, la pull request o la
   discussione in cui è emerso.
3. **Il bisogno:** che cosa Tecnigrafo non riesce a fare o fa male, e che cosa
   succede oggi.
4. **Casi concreti:** ingressi, file, sequenze di operazioni in cui si presenta.
5. **Quando è soddisfatta:** una condizione che Tecnigrafo può osservare.
6. **Vincoli:** robustezza, prestazioni, sistemi operativi, dipendenze che
   Tecnigrafo non può avere (per esempio compilare senza OCCT).
7. **Che cosa blocca:** che cosa non va avanti in Tecnigrafo senza la richiesta,
   e da quando.

Ogni riferimento si scrive **in forma assoluta**, `Matleyx78/tecnigrafo#N`: in
una issue di solid-space un `#N` da solo punta a solid-space.

## Come si apre

In uno di due modi, sempre in solid-space:

- **dal sito:** in solid-space **Issues** → **New issue** → **Richiesta di
  Tecnigrafo** (il link c'è anche tra i modelli delle issue di Tecnigrafo);
- **con lo script**, dal tuo PC o da un agente a cui lo chiedi:

  ```sh
  scripts/richiesta.sh richiesta.md --tappa ss-0.2
  ```

  Lo script legge un file con il titolo e i sei campi (esempio sotto), controlla
  che ci siano tutti e che i riferimenti siano assoluti, mostra la issue e chiede
  conferma; poi la apre in solid-space, la mette nel Project «Tecnigrafo» con
  *Tipo* = richiesta e *Tappa* (se indicata) e stampa il riferimento da citare.
  Opzioni e permessi sono scritti in testa allo script.

Un agente di Tecnigrafo apre una richiesta solo se il responsabile glielo chiede
nella chat (AGENTS.md, "Dove puoi scrivere"); il file lo prepara dal testo che il
responsabile approva. In solid-space, il workflow `controlla-richiesta.yml`
commenta la issue se manca qualcosa.

## Che cosa succede dopo

1. solid-space scrive un **ADR di valutazione** con uno di quattro esiti (OCCT
   basta, OCCT va corretto, serve un pezzo nostro, non spetta a solid-space). Se
   l'esito aggiunge o cambia un'interfaccia, la stessa pull request porta il
   **contratto**: le intestazioni pubbliche in bozza, con l'errore garantito di
   ogni costruzione. Il responsabile le accetta insieme all'ADR.
2. A ogni tappa di Tecnigrafo che chiede qualcosa corrisponde **una tappa di
   solid-space**, che raccoglie tutte le sue richieste e avanza insieme a lei.
3. Nel piano di Tecnigrafo i pacchetti che ne dipendono hanno la scheda **dopo
   il contratto**, scritta sulle intestazioni accettate. In "Dipende da" citano
   la richiesta finché la tappa di solid-space non ha i suoi pacchetti, poi il
   **pacchetto di solid-space** che serve.
4. Quando quel pacchetto è unito, il pacchetto di Tecnigrafo può partire: sposta
   il sottomodulo al commit che lo chiude, come autorizza la sua scheda (ADR
   0030, punto 9).
5. I valori geometrici attesi dei riferimenti si prendono dall'archivio dei casi
   di solid-space, citando il caso (ADR 0030, punto 14).
6. Il responsabile controlla il «quando è soddisfatta» e chiude la issue della
   richiesta. Lo stato delle richieste si legge nella vista «Richieste» del
   Project, non in un documento.

## Esempio del file per lo script (inventato)

```markdown
# Richiesta: decidere in modo coerente da che parte sta un punto rispetto a un segmento

## Da dove nasce

Tappa 0.7 di Tecnigrafo (comandi di modifica), dal piano in Matleyx78/tecnigrafo#…

## Il bisogno

Il comando di taglio decide se due entità si intersecano e da che parte di un
segmento sta il pezzo da tenere. Con punti quasi allineati le due decisioni si
contraddicono, e lo stesso disegno dà risultati diversi su Windows, Linux e macOS.

## Casi concreti

Disegno `taglio-01.dxf`, entità 12 e 15; tre punti allineati a meno di 1e-12.

## Quando è soddisfatta

Il taglio tiene sempre il pezzo giusto, anche su punti quasi allineati, e la CI
dà lo stesso risultato sui tre sistemi operativi.

## Vincoli

Nessuna dipendenza dal 3D né da OCCT; compila su Windows, Linux e macOS.

## Che cosa blocca

I comandi di taglio e prolungamento della tappa 0.7.
```
