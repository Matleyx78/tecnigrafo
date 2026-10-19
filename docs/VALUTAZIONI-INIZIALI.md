# Valutazioni iniziali

Riassunto della conversazione da cui è nato Tecnigrafo (15–16 ottobre 2026): le
valutazioni fatte prima di scrivere il codice, le scelte che ne sono uscite e i
temi rimasti aperti. Le decisioni definitive stanno negli ADR e nella ROADMAP; qui
c'è il perché.

## Obiettivo

Un CAD 2D libero nello spirito di AutoCAD e, sulla **stessa base**, un modellatore
3D parametrico nello spirito di Inventor, per Windows, Linux e macOS. Niente scopo
commerciale: il software deve restare di tutti. Un progetto con **una sola
direzione** e regole ferree, per evitare la dispersione vista in FreeCAD.

## Kernel geometrico: Open CASCADE

- **Perché FreeCAD non arriva a Inventor.** In parte per il kernel (OCCT è meno
  robusto di ShapeManager e Parasolid su raccordi, booleane, shell, geometria
  importata "sporca"), in parte per FreeCAD stesso: problema del naming
  topologico, assiemi arrivati tardi, architettura a workbench poco coerente.
- **OCCT è vivo.** Versione 8.0.1 del 30 luglio 2026: passaggio a C++17, molte
  ottimizzazioni (lettura STEP fino al 75% più veloce rispetto alla 7.7), test
  con GTest. Lo sviluppa Open Cascade SAS.
- **Portarlo al livello di Parasolid non è realistico** per un piccolo gruppo: la
  differenza sta in decenni di casi limite risolti. È realistico costruire
  un'ottima applicazione sopra OCCT, aggirarne i limiti (ShapeFix, strategie
  alternative, messaggi chiari) e contribuire correzioni mirate.
- **Il 2D non usa OCCT come motore principale**: un disegno ha centinaia di
  migliaia di entità semplici, e trasformarle in spigoli B-rep sarebbe pesante.
  Motore 2D proprio e leggero; OCCT per il 3D; le tavole dal 3D tramite la
  rimozione delle linee nascoste di OCCT (`HLRBRep`), convertite in entità 2D.
- Struttura di OCCT discussa: topologia (`TopoDS`: solid, shell, face, wire,
  edge, vertex) separata dalla geometria (`Geom`), spigoli condivisi, curve 3D
  più pcurve 2D sulle facce, tolleranze per elemento, OCAF per documenti e
  annullamento.

## Licenza

- LGPL (OCCT, Qt) permette anche prodotti chiusi con collegamento dinamico; la
  GPL obbliga tutto il programma a restare GPL.
- La vendita non è contemplata: scelta la **GPLv3 o successiva** (ADR 0003), che
  garantisce che il software resti libero, e permette di usare codice GPL come
  LibreDWG o parti di LibreCAD e QCAD.

## Strumenti

C++20, CMake, Qt 6 Widgets, GoogleTest, clang-format e clang-tidy 18, CI su
GitHub Actions per i tre sistemi (ADR 0002). Architettura a strati con il nucleo
senza Qt (ADR 0001). Alternative scartate: Rust (nessun kernel CAD maturo, binding
Qt immaturi), Python come linguaggio principale (troppo lento), Qt Quick
(inadatto a un'interfaccia CAD densa).

## Governo del progetto

Modello del "dittatore benevolo": il responsabile decide e unisce, le regole le
fanno rispettare gli strumenti (GOVERNANCE.md). Il progetto avanza per **tappe**,
una alla volta, ciascuna divisa in **pacchetti di lavoro** grandi quanto una pull
request; le scrive e le pubblica il responsabile (ROADMAP.md). Le IA possono
contribuire alle stesse regole, con una persona che firma e ne risponde
(AGENTS.md, CONTRIBUTING.md).

## Nome

"Tecnigrafo" (il tavolo da disegno con le squadre snodate) è il nome di lavoro,
giudicato forse **troppo italiano**. Alternative valutate, senza omonimi stretti
nelle ricerche fatte: **PantoCAD** (dal pantografo) e **GrafoCAD**. Scartate:
RigorCAD (troppo vicino a RogCAD) e TecniCAD (troppo vicino a TechCAD).
**Decisione ancora da prendere**, dopo aver verificato nome su GitHub, dominio
`.org` e marchi registrati (TMview).

## CADCraft e SolidCraft

Il 16 ottobre sono emersi due progetti simili, nati pochi giorni prima e scritti
quasi interamente da sciami di agenti IA:

- **[CADCraft](https://github.com/storytold/cadcraft)** (suite ArtCraft): clone di
  AutoCAD in Rust/egui, MIT o Apache 2.0. In dieci giorni ~184.000 righe, 54
  autori; già quote, retini, blocchi, layout, DWG. Pre-alpha per loro stessa
  ammissione.
- **[SolidCraft](https://github.com/bherbruck/solidcraft)** (progetto
  indipendente, non della suite): CAD 3D parametrico in stile Fusion 360, Rust,
  kernel Truck in sostituzione con uno proprio. ~378.000 righe, 82 autori.

**Analogie** con Tecnigrafo: riga di comando in stile AutoCAD, strati con
interfaccia sostituibile, regole per le IA, "tutto è un comando". **Differenze**:
sviluppo a sciame contro tappe ordinate; due programmi separati invece di una base
comune 2D+3D; nessun uso di OCCT (le loro regole vietano persino di leggerne il
codice); licenze permissive invece della GPL; piani di lavoro non pubblici.

**Prova fatta con i programmi ufficiali** (CADCraft 0.7.0, SolidCraft 0.13.1):

- CADCraft: comandi di base esatti, DXF e DWG senza perdite in andata e ritorno,
  robusto con file danneggiati, veloce (200.000 entità senza problemi). Però
  l'**offset di una polilinea a U produce una figura che si interseca** invece di
  due pezzi separati, senza avvisi; offset delle ellissi approssimato; un DXF
  troncato si apre vuoto senza messaggio.
- SolidCraft: i suoi 82 pezzi di riferimento passano; su pezzi costruiti per la
  prova (raccordi, svuotamento, foro, parametro) i volumi coincidono con i calcoli
  a mano; STEP con superfici esatte.

**Fork valutato e rimandato.** La licenza lo permetterebbe, ma significherebbe
ereditare oltre mezzo milione di righe scritte da agenti, inseguire centinaia di
modifiche al giorno, fondere due basi diverse e cambiare linguaggio. Decisione:
**tenerli d'occhio per 4–8 settimane**; se si fermano, un fork avrà senso; se
continuano, contribuire costa meno che competere.

## Formato dei file

Scelto il modello dati di DWG/DXF, la regola di conservare intatto ciò che non si
capisce, e il formato `.cadx` (ZIP con il disegno in DXF standard più i dati
propri): tutto in ADR 0006. Verificato che `.cadx` non risulta usata da altri
programmi.

## Modo di lavorare

- Gli agenti lavorano su GitHub: ramo, pull request, CI, revisione e merge del
  responsabile. Nessuna IA lavora direttamente sul PC dell'ufficio.
- Più agenti in parallelo: un pacchetto per agente, assegnato con le etichette
  `agente:…`, una pull request alla volta (LAVORO-IN-PARALLELO.md). Un modello
  diverso è utile soprattutto come revisore.
- La memoria del progetto è il repository, non la chat: una chat nuova per tappa
  o per argomento, raccolte in un progetto Claude "Tecnigrafo".

## Ancora aperto

- Scegliere il **nome definitivo** e rinominare il progetto.
- Tradurre in **inglese** almeno `README`, `CONTRIBUTING` e `AGENTS`.
- Rivalutare **CADCraft e SolidCraft** a metà dicembre 2026 (contribuire, fare un
  fork o proseguire da soli).
- Segnalare agli autori di CADCraft l'**errore di offset** sulla polilinea a U
  (polilinea chiusa 0,0 100,0 100,60 60,60 60,20 40,20 40,60 0,60; offset verso
  l'interno di 12: atteso due rettangoli, ottenuta una figura che si interseca).

## Fonti

- [Open Cascade Technology](https://en.wikipedia.org/wiki/Open_Cascade_Technology)
  e [OCCT 8.0.0, prestazioni e stabilità](https://dev.opencascade.org/content/occt-800-planned-q1-2026-performance-stability-and-support-services)
- [ArtCraft, la suite gratis che sfida Adobe e Microsoft](https://www.punto-informatico.it/artcraft-suite-gratis-sfida-adobe-microsoft/)
- [SolidCraft su Hacker News](https://zeli.app/story/50064840)
