# ADR 0030: solid-space, il kernel geometrico di Tecnigrafo

- **Stato:** Accettato
- **Sostituisce in parte:** ADR 0001 (moduli futuri: `modeling` su Open CASCADE);
  ADR 0002 (riga "Kernel 3D previsto")
- **Data:** 2026-10-28
- **Corretto:** 2026-10-29 — il responsabile cambia la cucitura tra i due
  progetti, insieme alla correzione dell'ADR 0007 di solid-space; nessun codice
  unito attua ancora questo ADR (eccezione di [GOVERNANCE.md](../../GOVERNANCE.md)).
  `core` usa anche il nuovo componente `geom2d` (punti 4 e 5); i target di
  solid-space non si compilano con `-ffast-math` né `/fp:fast` (punto 7); il
  sottomodulo entra con un pacchetto piccolo all'inizio della tappa 0.7, P0
  (punto 8); la versione fissata la aggiorna il pacchetto che usa il pezzo
  nuovo (punto 9);
  la richiesta si scrive solo in solid-space, e "Dipende da" cita la richiesta e
  poi il pacchetto di solid-space che serve (punti 11–13); i valori geometrici
  attesi si prendono dall'archivio dei casi di solid-space (punto 14, nuovo).
  Di conseguenza cambiano anche il contesto, le alternative e le conseguenze.

## Contesto

Gli ADR 0001 e 0002 nominano Open CASCADE (OCCT) come kernel 3D, da collegare
direttamente al futuro modulo `modeling`. Sono stati scritti prima di una
valutazione più approfondita (idee #104 e #217), da cui il responsabile ha tratto
la decisione definitiva: OCCT resta la base per ciò che fa bene, ma da solo non
basta per avvicinarsi a Inventor e SolidWorks. Vanno aggiunti o migliorati i
predicati geometrici robusti, il solver dei vincoli, il naming topologico stabile
e la robustezza delle operazioni solide.

Questo lavoro sta in [solid-space](https://github.com/Matleyx78/solid-space), un
repository separato dello stesso responsabile. È **il kernel di Tecnigrafo**: è
separato solo perché altri lo possano usare così com'è, ma Tecnigrafo non ha basi
geometriche né 3D senza di lui. È lo stesso rapporto che c'è tra Inventor e il suo
kernel. solid-space ha già fissato la sua parte
([ADR 0001](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0001-scopo-e-confini.md),
[ADR 0002](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0002-componenti-e-dipendenze.md),
[ADR 0004](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0004-robustezza-e-verifica.md),
[ADR 0007](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)):
cinque componenti (`robust`, `naming`, `solver`, `kernel`, `features`), OCCT e
libslvs collegati in privato, intestazioni pubbliche senza dipendenze esterne,
tappe che nascono dalle richieste di Tecnigrafo. Il suo ADR 0002, punto 8, lascia
a Tecnigrafo la scelta di come incorporarlo.

**La correzione del 2026-10-29.** Le prime richieste, quelle della tappa 0.7
(solid-space#20–#23), hanno mostrato tre attriti: il testo della richiesta viveva
in due copie, Tecnigrafo avrebbe scritto le schede dei suoi pacchetti senza
sapere quali funzioni avrebbe ricevuto, e avrebbe aspettato la fine di intere
tappe di solid-space. Il responsabile tiene il metodo e cambia la cucitura:
l'[ADR 0007 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md),
corretto lo stesso giorno, apre una tappa di solid-space per ogni tappa di
Tecnigrafo che chiede qualcosa, fa portare a ogni valutazione il contratto (le
intestazioni pubbliche in bozza) e rende utilizzabile ogni pacchetto unito. Per
il 2D le valutazioni hanno scelto un componente nuovo di solid-space, `geom2d`:
curve del piano (segmento, arco, cerchio, bulge, polilinea), intersezioni, lato,
tangenze e curve parallele, sopra `robust` e con la sola libreria standard. Lo
fissa un ADR di solid-space. Questo ADR adegua il lato di Tecnigrafo.

Senza una decisione scritta qui, le regole di Tecnigrafo vietano di usarlo:
`core` non ha dipendenze esterne (ADR 0001) e gli agenti non aggiungono
dipendenze (AGENTS.md).

## Decisione

### Che cosa è solid-space per Tecnigrafo

1. **solid-space è il kernel geometrico di Tecnigrafo**, non una libreria esterna.
   Tutto ciò che è geometria robusta, vincoli, naming e solidi si chiede a lui;
   Tecnigrafo non ne scrive una versione propria.
2. **Confine dei compiti.** A solid-space spettano: decisioni geometriche che
   dipendono dal segno di una grandezza vicina a zero (da che parte sta un
   punto, se due elementi si intersecano, sono tangenti o coincidono),
   costruzioni con un errore garantito (punti d'intersezione, raccordi), politica
   delle tolleranze geometriche, solver dei vincoli, naming topologico, albero
   delle feature, operazioni solide, STEP. A Tecnigrafo restano: comandi,
   interfaccia, annulla e ripeti, formati di file (DXF, `.cadx`), disegno sullo
   schermo, tolleranze in pixel e tolleranze dei casi di verifica.
3. **Non si espone OCCT.** Tecnigrafo usa solo le intestazioni pubbliche di
   solid-space, che non includono OCCT né libslvs. Nessun modulo di Tecnigrafo
   collega OCCT direttamente.

### Quale modulo usa quale componente

4. | Modulo di Tecnigrafo | Componenti di solid-space |
   |---|---|
   | `core` | `robust`, `geom2d` |
   | `modeling` (fase 2) | `kernel`, `features`, `naming`, `solver` |

   Il componente che servirà ai vincoli 2D (`solver`) e il modulo che lo collega
   si decidono con l'ADR della tappa che introduce i vincoli. Ogni altro
   collegamento richiede un ADR.
5. **`core` resta senza librerie esterne.** `solid_space::robust` e
   `solid_space::geom2d` usano solo la libreria standard, quindi `core` continua
   a compilare senza Qt, OCCT o altro.
   Il CAD 2D non compila mai `kernel` né `features`
   (`SOLID_SPACE_BUILD_KERNEL=OFF`), quindi non richiede OCCT.

### Come si incorpora

6. **Sottomodulo git** in `third_party/solid-space`, fissato a un commit preciso
   di `main` (a un tag, quando solid-space li pubblica). Si inizializza senza
   `--recursive`: i sottomoduli di solid-space (OCCT, SolveSpace) si scaricano
   solo quando Tecnigrafo compila un componente che ne ha bisogno.
7. **Si compila con `add_subdirectory`.** I test di solid-space non si compilano
   dentro Tecnigrafo: li esegue la CI di solid-space. Le sue intestazioni non si
   includono come `SYSTEM`: è codice nostro, con le stesse regole di avvisi.
   Tecnigrafo non compila i target di solid-space con `-ffast-math` né con
   `/fp:fast`, e non passa loro opzioni che li includano: i risultati di
   `robust` e `geom2d` devono essere gli stessi, bit per bit, sui tre sistemi
   operativi
   ([ADR 0008 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0008-componente-geom2d.md),
   punto 5).
8. **Il sottomodulo entra con un pacchetto suo, piccolo, all'inizio della
   tappa** che per prima usa solid-space: P0 della tappa 0.7, nella prima metà
   del piano, quando `robust` è ancora vuoto. P0 porta il sottomodulo, lo
   collega a CMake e fa scaricare i sottomoduli al checkout della CI: modifica
   `cmake/` e `.github/`, e la sua scheda lo dice esplicitamente, come eccezione
   alla regola di AGENTS.md. I pacchetti che poi usano i componenti non toccano
   né `cmake/` né `.github/`.
9. **Aggiornare la versione fissata** spetta al pacchetto di Tecnigrafo che usa
   il pezzo nuovo: sposta il sottomodulo al commit di `main` di solid-space che
   chiude il pacchetto di solid-space che gli serve, senza aspettare la fine
   della tappa di solid-space (ogni pacchetto unito là è utilizzabile, ADR 0007
   di solid-space, punto 16). La sua scheda lo autorizza e cita quel pacchetto;
   la CI di Tecnigrafo deve essere verde con la nuova versione. Fuori da un
   pacchetto, l'aggiornamento è una pull request che decide il responsabile.
10. **Le correzioni si fanno in solid-space**, mai nella copia dentro Tecnigrafo.

### Come si chiede

11. **Ogni bisogno nasce da Tecnigrafo e diventa una richiesta, scritta solo in
    solid-space**, con le regole di
    [RICHIESTE-A-SOLID-SPACE.md](../RICHIESTE-A-SOLID-SPACE.md): una sola issue
    di solid-space con l'etichetta `richiesta`, scritta con il suo modulo. La
    apre il responsabile, oppure un agente di Tecnigrafo a cui lui lo chiede, con
    `scripts/richiesta.sh`. In Tecnigrafo non ne resta una copia: il piano e i
    pacchetti citano la issue di solid-space. La richiesta dice di che cosa c'è
    bisogno, non come soddisfarlo; solid-space risponde con obiettivi
    raggiungibili e, per gli esiti che cambiano un'interfaccia, con il contratto:
    le intestazioni pubbliche in bozza, accettate insieme all'ADR di
    valutazione.
12. **Due momenti.** Il bisogno si cerca quando si prepara il piano della tappa
    (sezione "Richieste a solid-space" di [tappe/0000-modello.md](../tappe/0000-modello.md)),
    ma può emergere anche durante la tappa: allora l'agente si ferma e scrive il
    testo della richiesta nella issue del suo pacchetto (AGENTS.md, "Se serve
    qualcosa a solid-space"), e il responsabile decide se aprirla.
13. **Comanda il piano di Tecnigrafo, e le due tappe avanzano insieme.** A ogni
    tappa di Tecnigrafo che chiede qualcosa corrisponde una tappa di solid-space
    (ADR 0007 di solid-space, punti 10–12). Le schede dei pacchetti di
    Tecnigrafo che dipendono da solid-space si scrivono sulle intestazioni
    accettate, non prima; finché il contratto manca, il piano li elenca con una
    riga ciascuno. In "Dipende da" un pacchetto cita la richiesta
    (`Matleyx78/solid-space#N`) finché la tappa di solid-space non ha i suoi
    pacchetti, poi il pacchetto di solid-space che gli serve: Tecnigrafo aspetta
    quel pacchetto, non la fine della tappa. Intanto vanno avanti i pacchetti che
    non ne dipendono.

### Da dove vengono i valori attesi

14. **La verità geometrica sta nell'archivio dei casi di solid-space**
    (`tests/corpus/`, ADR 0007 di solid-space, punto 17). I riferimenti di
    Tecnigrafo prendono da lì i valori geometrici attesi (intersezioni, punti di
    tangenza, curve parallele, lati), **citando il caso**, invece di rifare i
    calcoli; verificano la traduzione dalle entità alle curve e i comandi. I
    casi concreti e i DXF di AutoCAD che servono a una richiesta entrano in
    quell'archivio. Se un riferimento di Tecnigrafo e l'archivio non
    concordano, si apre una issue in solid-space e decide l'archivio, corretto
    là se è sbagliato.

## Alternative considerate

- **OCCT collegato direttamente da `modeling`** (ADR 0001 e 0002): nessuna
  possibilità di correggerne o sostituirne un pezzo senza toccare l'applicazione,
  e nessun posto per predicati, solver e naming propri.
- **Il kernel dentro il repository di Tecnigrafo** (come proponeva la #217): il
  2D erediterebbe OCCT e il kernel non si potrebbe provare e misurare da solo.
  Scartato da solid-space nel suo ADR 0001.
- **Copia dei file in `third_party/`**, come zlib e SARibbon (ADR 0019 e 0009):
  adatta a librerie esterne che cambiano di rado. solid-space cresce con
  Tecnigrafo e va aggiornato spesso, e la copia renderebbe gli aggiornamenti
  difficili da controllare.
- **`FetchContent` alla configurazione**: richiede la rete a ogni configurazione
  pulita e nasconde la versione in un file CMake invece che nella storia di git.
- **Predicati scritti in `core`**: più rapido per la tappa 0.7, ma duplicherebbe
  in Tecnigrafo proprio ciò per cui solid-space esiste.
- **La richiesta scritta in Tecnigrafo e ricopiata in solid-space** (il testo
  del 2026-10-28, con l'etichetta `richiesta-solid-space`): due testi da tenere
  allineati a mano e un passaggio in più a ogni richiesta.
- **Il sottomodulo con il primo pacchetto che usa un componente** (il testo del
  2026-10-28): quel pacchetto, già impegnativo, avrebbe portato anche CMake e la
  CI, e l'integrazione sarebbe arrivata tutta insieme a metà tappa.
- **Aggiornare la versione fissata alla chiusura della tappa di solid-space**
  (il testo del 2026-10-28): Tecnigrafo avrebbe aspettato tutti i pacchetti di
  solid-space anche quando gliene serviva uno.
- **Calcolare i valori geometrici attesi anche in Tecnigrafo**: i calcoli
  delicati si farebbero due volte, e due riferimenti diversi per lo stesso caso
  non dicono quale sia giusto.

## Conseguenze

- Gli agenti di Tecnigrafo non scrivono predicati, soglie geometriche proprie,
  solver o naming: chiedono a solid-space.
- I pacchetti di Tecnigrafo possono dover aspettare un pacchetto di solid-space.
  Lo limitano la richiesta scritta presto, il contratto fissato prima delle
  schede, il piano che mette prima i pacchetti indipendenti e i pacchetti di
  solid-space utilizzabili appena uniti.
- Un piano di Tecnigrafo che chiede qualcosa si scrive in due tempi: subito i
  pacchetti che non dipendono da solid-space, dopo l'accettazione dei contratti
  le schede di quelli che ne dipendono.
- I riferimenti dei pacchetti che usano solid-space si accorciano: citano i casi
  dell'archivio di solid-space e verificano facciata e comandi.
- Le richieste e i loro stati non stanno in Tecnigrafo: si leggono nelle issue
  di solid-space e nel Project «Tecnigrafo» del responsabile, che raccoglie i
  due repository. L'etichetta `richiesta-solid-space` resta solo sulle issue
  storiche.
- La scheda della fase 2 ([tappe/2.x](../tappe/2.x-modellatore-3d.md)) e
  l'[ARCHITETTURA](../ARCHITETTURA.md) riflettono questa decisione.
- Le soglie geometriche già presenti in `core` (per esempio in `arc.cpp`,
  `arc_geometry.cpp`, `polyline.cpp` e `proximity.cpp`) restano finché una
  richiesta a solid-space non le sostituisce.
- Le idee #104 (chiusa) e #217 si considerano risolte da questo ADR e dagli
  ADR 0001 e 0002 di solid-space.
