# ADR 0038: La CI dei due progetti

- **Stato:** Accettato
- **Data:** 2026-11-07

## Contesto

La CI di Tecnigrafo e quella di solid-space sono nate separate e non provano le
stesse cose ([#521](https://github.com/Matleyx78/tecnigrafo/issues/521), con il
confronto voce per voce del 2026-11-05):

- Tecnigrafo compila solo in Debug (preset `ci`), solid-space anche in Release
  con gli avvisi come errori (preset `release`);
- solid-space fissa le opzioni di virgola mobile (`-ffp-contract=off`,
  `/fp:precise` su MSVC, rifiuto di `-ffast-math` e simili) per avere gli stessi
  numeri su tutti i sistemi (ADR 0008 di solid-space); Tecnigrafo no, e `core`
  calcola sopra i risultati di solid-space (tappa 0.7, P14–P22);
- solid-space confronta i risultati su Linux, Windows e macOS, Tecnigrafo no;
- le immagini e le versioni delle azioni sono diverse;
- il controllo incrociato non è simmetrico: ogni pull request di solid-space
  compila Tecnigrafo `main` (`tecnigrafo-core.yml`, Debug, solo Linux, senza
  `app`), mentre Tecnigrafo prova `main` di solid-space una volta a settimana,
  come avviso (`solid-space-main.yml`).

Il responsabile ha deciso le immagini il 2026-11-07 (DECISIONI.md) e ha chiesto
che da qui in avanti i due progetti si muovano insieme anche nella CI.

## Decisione

### 1. Immagini

I due progetti usano `ubuntu-24.04`, `windows-2025-vs2026` e `macos-26`, mai
le etichette `-latest`. Un'immagine cambia nei due repository nella stessa
tappa, o nello stesso giro fuori tappa.

### 2. Azioni

Le azioni comuni ai due repository (`actions/checkout`,
`actions/upload-artifact`, `actions/download-artifact`, `actions/cache`,
`actions/setup-python`) hanno la stessa versione principale nei due. La prima
volta si sceglie l'ultima versione principale pubblicata al momento del
pacchetto (0.7.5-P1), con il collegamento alla sua nota di rilascio nella pull
request; poi si aggiornano insieme.

### 3. Build Release con gli avvisi come errori

Tecnigrafo aggiunge un preset `ci-release` (Release, avvisi come errori, come
il `release` di solid-space; il `release` di Tecnigrafo resta quello di chi
sviluppa, senza avvisi come errori):

- **Linux** tiene i lavori di oggi (Debug con `clang-tidy`, sanitizer,
  fuzzing, verifica) e aggiunge un lavoro Release con test e verifica;
- **Windows e macOS** compilano in Release, con test e verifica, al posto di
  Debug: sono i lavori più lenti, e Debug resta provato su Linux.

### 4. Virgola mobile

Tutti i moduli di Tecnigrafo si compilano con le stesse opzioni di virgola
mobile di solid-space: niente contrazione di moltiplicazioni e somme
(`-ffp-contract=off` con GCC e Clang, `/fp:precise` con MSVC), e la
configurazione si rifiuta se trova `-ffast-math`, `-Ofast`, `/fp:fast` o
simili. Le opzioni stanno in un file di `cmake/` usato da tutti i target. Il
motivo è lo stesso di solid-space: i casi di verifica confrontano numeri, e lo
stesso calcolo deve dare gli stessi numeri sui tre sistemi.

### 5. Confronto dei risultati tra sistemi

Un lavoro della CI raccoglie, dai lavori Release di Linux, Windows e macOS, i
dati numerici dei rapporti della verifica (non le immagini, che si confrontano
già con la loro tolleranza) e li confronta. Il confronto **blocca**: una
differenza tra sistemi si corregge, oppure si scrive nell'elenco delle
eccezioni di `docs/verifica/sistemi.md` (lo scrive 0.7.5-P5), un caso per riga,
con il motivo. L'elenco nasce vuoto.

### 6. Controllo incrociato simmetrico

Le due direzioni provano le stesse cose: build Debug e Release con gli avvisi
come errori su Linux,
senza `app`, con test di `core` e di solid-space.

- **solid-space → Tecnigrafo:** ogni pull request di solid-space compila
  Tecnigrafo `main` con il commit della pull request (`tecnigrafo-core.yml`,
  esiste già; si aggiunge `ci-release`). Blocca, come oggi.
- **Tecnigrafo → solid-space:** ogni pull request di Tecnigrafo che tocca
  `core`, `cmake/` o `third_party/solid-space` compila anche con `main` di
  solid-space, e lo stesso una volta al giorno su `main`. È un **avviso**, non
  blocca: Tecnigrafo punta un commit fissato di solid-space
  ([ADR 0030](0030-solid-space-kernel-di-tecnigrafo.md), punto 9), e il
  puntatore non si sposta mai da solo. L'avviso di `main` apre o aggiorna una
  issue come fa oggi `solid-space-main.yml`.

### 7. I due progetti si muovono insieme

Chi cambia in uno dei due repository immagini, compilatori, preset, opzioni di
compilazione o versioni delle azioni comuni apre la modifica corrispondente
nell'altro, nella stessa tappa (o nello stesso giro fuori tappa), con un
rimando incrociato nelle due pull request. Le differenze volute stanno nella
tabella qui sotto; una differenza nuova si aggiunge con il suo motivo.

| Voce | Solo in | Perché |
|---|---|---|
| Qt, SARibbon, test di `app`, galleria di schermate | Tecnigrafo | solid-space non ha interfaccia |
| `clang-tidy` | Tecnigrafo | regole di stile di Tecnigrafo (`.clang-tidy`); solid-space decide le sue |
| Fuzzing dei lettori di file | Tecnigrafo | solid-space non legge file |
| Verifica con `tecnigrafo-script` e oracolo ezdxf | Tecnigrafo | i casi di solid-space stanno nel suo archivio (`tests/corpus/`) |
| Insiemi di avvisi (`cmake/TecnigrafoWarnings.cmake`, `cmake/SolidSpaceWarnings.cmake`) | Tecnigrafo: `-Wimplicit-fallthrough`, `/utf-8` | confrontati da 0.7.5-P4: Tecnigrafo ha preso da solid-space `-Wcast-align` e `-Wformat=2`, che il codice regge senza correzioni, e con MSVC i due usano `/W4 /permissive-`. Restano solo in Tecnigrafo `-Wimplicit-fallthrough` (con GCC è già in `-Wextra`, con Clang no) e `/utf-8` di MSVC, che dice la codifica dei sorgenti e non è un avviso |

## Alternative considerate

- **Lasciare le due CI diverse** e contare sul controllo settimanale: una
  rottura in Release o una differenza numerica tra sistemi si scoprirebbe
  tardi, e solo su Linux.
- **Una sola CI** che costruisce i due progetti: più facile da tenere uguale, ma
  lega i repository più di quanto l'ADR 0030 voglia, e solid-space deve restare
  compilabile da solo.
- **Debug e Release su tutti e tre i sistemi:** raddoppia i tempi delle due
  immagini più lente, per un guadagno piccolo, visto che Debug resta su Linux
  con i sanitizer.
- **Confronto tra sistemi solo come avviso:** non ferma niente, quindi una
  differenza resterebbe fino a quando qualcuno la guarda.
- **Controllo verso `main` di solid-space che blocca:** fermerebbe le pull
  request di Tecnigrafo per un lavoro di solid-space non ancora puntato.

## Conseguenze

- La CI di Tecnigrafo dura di più su Linux (un lavoro in più) e il primo giro
  può portare correzioni al codice, come è successo a solid-space sulle immagini
  precedenti (solid-space#199).
- Una differenza numerica tra sistemi diventa un errore visibile nella pull
  request che la introduce.
- Una modifica della CI costa due pull request, una per repository; il
  responsabile le unisce insieme.
- Le pull request di solid-space che toccano la CI citano questo ADR; in
  solid-space la regola gemella sta nel suo registro delle decisioni.
