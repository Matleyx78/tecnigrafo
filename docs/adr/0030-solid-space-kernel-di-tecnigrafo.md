# ADR 0030: solid-space, il kernel geometrico di Tecnigrafo

- **Stato:** Accettato
- **Sostituisce in parte:** ADR 0001 (moduli futuri: `modeling` su Open CASCADE);
  ADR 0002 (riga "Kernel 3D previsto")
- **Data:** 2026-10-28

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
   | `core` | `robust` |
   | `modeling` (fase 2) | `kernel`, `features`, `naming`, `solver` |

   Il componente che servirà ai vincoli 2D (`solver`) e il modulo che lo collega
   si decidono con l'ADR della tappa che introduce i vincoli. Ogni altro
   collegamento richiede un ADR.
5. **`core` resta senza librerie esterne.** `solid_space::robust` usa solo la
   libreria standard, quindi `core` continua a compilare senza Qt, OCCT o altro.
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
8. **Il sottomodulo entra con il primo pacchetto** che usa un componente. Quel
   pacchetto modifica `cmake/` e `.github/` (il checkout della CI con i
   sottomoduli), e la sua scheda lo dice esplicitamente, come eccezione alla
   regola di AGENTS.md.
9. **Aggiornare la versione fissata** è una pull request di Tecnigrafo, di solito
   alla chiusura di una tappa di solid-space nata da una richiesta. La pull
   request cita la tappa e la richiesta, e la CI di Tecnigrafo deve essere verde
   con la nuova versione. Solo il responsabile la decide.
10. **Le correzioni si fanno in solid-space**, mai nella copia dentro Tecnigrafo.

### Come si chiede

11. **Ogni bisogno nasce da Tecnigrafo e diventa una richiesta**, con le regole di
    [RICHIESTE-A-SOLID-SPACE.md](../RICHIESTE-A-SOLID-SPACE.md): una issue di
    Tecnigrafo con l'etichetta `richiesta-solid-space`, che il responsabile
    ricopia in solid-space. La richiesta dice di che cosa c'è bisogno, non come
    soddisfarlo; solid-space risponde con obiettivi raggiungibili.
12. **Due momenti.** Il bisogno si cerca quando si prepara il piano della tappa
    (sezione "Richieste a solid-space" di [tappe/0000-modello.md](../tappe/0000-modello.md)),
    ma può emergere anche durante la tappa: allora l'agente si ferma e scrive il
    testo della richiesta nella issue (AGENTS.md, "Quando fermarsi").
13. **Comanda il piano di Tecnigrafo.** Una richiesta che blocca la tappa aperta
    di Tecnigrafo fa nascere una tappa di solid-space. Intanto in Tecnigrafo
    vanno avanti solo i pacchetti che non ne dipendono; quelli che ne dipendono
    citano la issue di solid-space in "Dipende da".

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

## Conseguenze

- Gli agenti di Tecnigrafo non scrivono predicati, soglie geometriche proprie,
  solver o naming: chiedono a solid-space.
- Le tappe di Tecnigrafo possono dover aspettare una tappa di solid-space.
  Lo limitano la richiesta scritta presto, il piano che mette prima i pacchetti
  indipendenti e le tappe piccole di solid-space.
- La scheda della fase 2 ([tappe/2.x](../tappe/2.x-modellatore-3d.md)) e
  l'[ARCHITETTURA](../ARCHITETTURA.md) riflettono questa decisione.
- Le soglie geometriche già presenti in `core` (per esempio in `arc.cpp`,
  `arc_geometry.cpp`, `polyline.cpp` e `proximity.cpp`) restano finché una
  richiesta a solid-space non le sostituisce.
- Le idee #104 (chiusa) e #217 si considerano risolte da questo ADR e dagli
  ADR 0001 e 0002 di solid-space.
