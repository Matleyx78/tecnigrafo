# [F6] `app`: l'aspetto dell'interfaccia sul riferimento, in locale con il responsabile

- **Tappa:** fuori tappa: correzione dell'aspetto dell'interfaccia della 0.7.5
  ([ROADMAP.md](../../ROADMAP.md), "Cosa si può fare fuori dalla tappa
  aperta"), in parallelo alla tappa 0.8, che non aspetta questo pacchetto e
  non ne è aspettata ([DECISIONI.md](../../DECISIONI.md), 2026-11-11,
  «L'aspetto dell'interfaccia si corregge fuori tappa, in locale»)
- **Issue:** #658
- **Difficoltà:** impegnativa
- **Agente:** claude, con Claude Code **in locale** sul PC Debian del
  responsabile; non si fa da una sessione cloud, dove Qt non c'è
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: aspetto, il comportamento non cambia. Fanno
  fede le immagini di `docs/adr/0039/` e l'occhio del responsabile; test Qt,
  galleria e prova manuale
- **Dipende da:** —
- **Misura:** oltre la quindicina di file e le 600 righe, per decisione del
  responsabile (stessa voce di DECISIONI.md): è un lavoro solo, visto e
  approvato area per area
- **Modulo:** `app` (più i suoi test e le icone di `resources/icons/`)
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md)
  per intero e il [README delle sue immagini](../../adr/0039/README.md);
  [ADR 0040](../../adr/0040-icone-a-ruoli.md) e [ICONE.md](../../ICONE.md),
  solo quando si tocca un'icona; [CODING_STYLE.md](../../CODING_STYLE.md);
  `docs/tappe/fuori-tappa/F6-stato.md` (sotto, "Lo stato del lavoro"), se
  esiste; in `app/src`: `theme.cpp` (gettoni), `style.cpp` (foglio di stile e
  `Style`), `ui_builder.cpp` (ribbon, menu, barre), `main_window.cpp`; in
  `app/tool/gallery.cpp` la galleria; in `third_party/saribbon/SARibbon.cpp`,
  **solo da leggere**, le parti del ribbon che servono

## Perché

La 0.7.5 ha rifatto l'interfaccia sul riferimento visivo dell'ADR 0039, ma
l'aspetto non è ancora quello del riferimento
([#655](https://github.com/Matleyx78/tecnigrafo/issues/655)). La causa è nel
metodo: chi ha scritto `app` non ha mai visto il programma, perché nelle
sessioni cloud Qt non c'è e la galleria della CI non si scarica; l'unico a
vedere era il responsabile, con un giro di CI a ogni tentativo.

Questo pacchetto cambia il giro: si lavora sul PC del responsabile, dove
l'agente compila, lancia la galleria e **guarda le immagini**, e il
responsabile guarda la finestra vera. Si modifica e si guarda finché l'area è
a posto, e a GitHub torna solo il lavoro finito.

## Preparare la cartella (il responsabile, una volta)

Sul PC Debian, già preparato come dice
[AMBIENTE-SVILUPPO.md](../../AMBIENTE-SVILUPPO.md), sezione 2. Una cartella
dedicata, diversa da quella dove si provano i pacchetti della 0.8:

```sh
sudo apt install ccache librsvg2-bin imagemagick
git clone https://github.com/Matleyx78/tecnigrafo ~/progetti/tecnigrafo-grafica
cd ~/progetti/tecnigrafo-grafica
git submodule update --init third_party/solid-space
git config user.email   # deve essere l'indirizzo noreply di GitHub
git switch -c f6-grafica
git push -u origin f6-grafica
cmake --preset debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
cmake --build --preset debug --target tecnigrafo tecnigrafo-galleria
./build/debug/app/tecnigrafo
code .
```

- `ccache` rende rapide le ricompilazioni; `rsvg-convert` (di `librsvg2-bin`)
  trasforma in PNG le immagini di riferimento; `compare` di ImageMagick
  confronta due PNG.
- Il ramo pubblicato **non avvia la CI**: parte solo per `main` e per le pull
  request. Pubblicarlo serve da copia di sicurezza.
- `--target tecnigrafo tecnigrafo-galleria` compila il programma e la galleria
  con ciò che serve loro, e non i test degli altri moduli: la prima volta
  richiede qualche minuto, poi si ricompila solo ciò che si tocca. Nessun test
  si cancella.
- La cartella di build è `build/`, senza `TG_BUILD_SUFFIX`: la cartella è
  dedicata a questo pacchetto.
- In VS Code si apre la cartella e si avvia Claude Code; la prima frase è:
  «Lavora al pacchetto F6 (#658): leggi
  `docs/tappe/fuori-tappa/F6-aspetto-dell-interfaccia.md` e segui "Una
  sessione"».

## Perimetro

Si toccano **solo**:

- `app/src/`, `app/tool/gallery.cpp`, `app/CMakeLists.txt`;
- `resources/icons/*.svg`, con le regole di [ICONE.md](../../ICONE.md);
- `tests/app/` e le righe di `tests/CMakeLists.txt` che lo riguardano;
- `docs/tappe/fuori-tappa/F6-stato.md`;
- alla chiusura: [manuale/interfaccia.md](../../manuale/interfaccia.md), se
  descrive un aspetto che è cambiato; le immagini di `docs/adr/0039/`, solo per
  gli scostamenti dal riferimento che il responsabile ha deciso (sotto,
  "Scostamenti dal riferimento"); in `.github/workflows/ci.yml` solo il numero
  delle immagini della galleria, se cambia.

**Mai:** `core`, `render`, `edit`, `cmd`, `io`, `script`, `mcp`, `cmake/`,
`third_party/` (SARibbon resta la 2.9.5 copiata senza modifiche:
`third_party/saribbon/README.md`), `tests/verifica/`, il testo degli ADR.
Nessuna dipendenza nuova.

**Il comportamento non cambia:** nessun comando, nessuna voce e nessuna
funzione nuova; ciò che un controllo fa resta com'è. Restano anche i nomi
(`objectName`) che i test e la galleria usano. Se per ottenere l'aspetto serve
uscire dal perimetro (per esempio SARibbon non si lascia disegnare come il
riferimento chiede, o serve un colore in `render`), l'agente **si ferma e lo
chiede al responsabile**, con le strade possibili.

## Una sessione

Una sessione locale di Claude Code comincia da zero: lo stato è nel ramo e in
`F6-stato.md`, non nella chat.

1. **Presa in carico.** Alla prima sessione l'etichetta `AA-Presa in carico`
   va sulla #658, con il commento, e **resta fino alla fine del pacchetto**:
   ci lavora un solo agente, in una sola cartella.
2. **Base aggiornata.** `git status` pulito, poi "Seguire `main`" (sotto), se
   `main` è avanzato.
3. **Lettura.** `F6-stato.md`: l'area in corso e i punti aperti.
4. **Il giro**, sull'area che il responsabile indica:
   1. modifica;
   2. `cmake --build --preset debug --target tecnigrafo tecnigrafo-galleria`;
   3. galleria e confronto (sotto, "Guardare");
   4. il responsabile guarda la finestra vera
      (`./build/debug/app/tecnigrafo`) e dice che cosa non va;
   5. da capo, finché dice che l'area è a posto.
5. **Commit** sul ramo a ogni passo che il responsabile approva (Conventional
   Commits, in inglese, con `Assisted-by:`, senza `-s`), e `git push` a fine
   sessione. Niente pull request.
6. **Fine sessione:** `F6-stato.md` aggiornato e pubblicato con il resto.

Durante il lavoro **non si apre la pull request e non si aspetta la CI**: la
sessione finisce quando lo dice il responsabile. "Quando fermarsi" di
[AGENTS.md](../../AGENTS.md) vale solo per la pull request finale.

Durante il giro i test di `tests/app` che fissano l'aspetto (colori, misure,
numero di immagini) possono restare rossi in locale: si sistemano alla
chiusura, e nessuno si cancella o si disattiva.

Se manca uno strumento, l'agente non lo installa: lo chiede al responsabile,
che è lì.

### Guardare

```sh
QT_QPA_PLATFORM=offscreen build/debug/app/tecnigrafo-galleria build/galleria
mkdir -p build/riferimento
rsvg-convert -w 1600 docs/adr/0039/finestra-scuro.svg -o build/riferimento/finestra-scuro.png
```

L'agente apre le due immagini (galleria e riferimento) e scrive le
differenze **area per area**, con i numeri delle aree dell'ADR 0039, punto 1:
misure in pixel, colori come gettoni, allineamenti. Le immagini non sono uguali
al pixel e non devono esserlo: il riferimento mostra `MOVE` in corso, icone
segnaposto e un altro font; si confrontano superfici, bordi, densità,
allineamenti e proporzioni. `compare` aiuta sui ritagli di una stessa area.

La galleria gira con la piattaforma `offscreen`, quindi con la cornice
propria; sotto Wayland la finestra vera ha la cornice del sistema (ADR 0039,
punto 4), che la galleria mostra solo in `ribbon-cornice-sistema-*.png`. La
finestra vera la giudica il responsabile.

La galleria si può estendere, dentro `app/tool/gallery.cpp`, per mostrare ciò
che serve confrontare (per esempio un comando in corso, come nel riferimento).

### Ricarica dello stile senza ricompilare (facoltativa)

Se il giro sui colori e sulle misure del foglio di stile risulta lento, si può
aggiungere un aiuto **di solo sviluppo**: con la variabile d'ambiente
`TG_DEV_STYLE=<file>` il programma legge da quel file il modello del foglio di
stile, al posto di quello compilato, e lo ricarica quando il file cambia.
Senza la variabile non cambia nulla. **Si toglie prima della pull request
finale**, salvo che il responsabile decida di tenerlo: allora entra con il suo
test.

### Lo stato del lavoro

`docs/tappe/fuori-tappa/F6-stato.md` nasce nel ramo alla prima sessione, che
confronta tutta la galleria con il riferimento e scrive la lista delle
differenze. Contiene, in breve:

- una tabella delle **aree** (le dieci dell'ADR 0039, punto 1, più
  l'interfaccia classica, le finestre di dialogo e le icone), ognuna con lo
  stato: da fare, in corso, approvata dal responsabile (con la data);
- per l'area in corso, i **punti aperti**;
- gli **scostamenti dal riferimento** decisi dal responsabile;
- le **cose rimandate** e perché (per esempio un limite di SARibbon).

L'ordine delle aree lo sceglie il responsabile. Un'area approvata non si
ritocca senza che lo chieda lui.

### Scostamenti dal riferimento

Il riferimento è l'ADR 0039 con le sue immagini. Dove il responsabile, vedendo
il risultato, sceglie diversamente (un colore, una misura, una disposizione),
la scelta si scrive in `F6-stato.md` e vince sul riferimento. Alla chiusura le
immagini di `docs/adr/0039/` si aggiornano per questi soli punti, e la pull
request li elenca: il testo dell'ADR non si tocca, e fa fede la voce di
DECISIONI.md che la pull request finale aggiunge. I valori dei gettoni si
possono ritoccare tenendo i nomi, come l'ADR già permette (punto 2).

## Seguire `main`

La tappa 0.8 lavora intanto su `main`, e i suoi pacchetti toccano anche `app/`:
azioni e voci dei comandi nuovi, icone, finestre. Il ramo assorbe `main` strada
facendo, così la pull request finale non ha conflitti.

Dopo ogni pull request unita in `main`, e comunque all'inizio di ogni sessione:

```sh
git fetch origin
git merge origin/main        # merge commit, mai rebase
git submodule update --init third_party/solid-space
cmake --build --preset debug --target tecnigrafo tecnigrafo-galleria
```

- **Conflitti:** si tiene ciò che `main` aggiunge (azioni, voci, icone,
  comportamento) e l'aspetto del ramo. Se un conflitto chiede di scegliere fra
  un comportamento di `main` e l'aspetto del ramo, vince `main`, e l'aspetto si
  riadatta.
- **Dopo il merge:** la galleria si rilancia. Ciò che la 0.8 ha aggiunto
  (pulsanti, finestre, icone nuove) si porta all'aspetto del ramo, se serve, e
  si annota in `F6-stato.md`.
- I pacchetti della 0.8 **non aspettano** questo ramo e non lo guardano:
  lavorano su `main` com'è.

## Chiusura

Quando il responsabile ha approvato tutte le aree:

1. Ultimo merge di `main` nel ramo, come sopra.
2. Si toglie la ricarica dello stile, se c'è e non si tiene.
3. I test di `tests/app` si allineano all'aspetto approvato; nessun test si
   toglie. La galleria passa, e il numero delle sue immagini in
   `.github/workflows/ci.yml` si aggiorna se è cambiato.
4. Documenti: `F6-stato.md` nella forma finale (resta come verbale), manuale e
   immagini di `docs/adr/0039/` secondo "Perimetro", e la voce di DECISIONI.md
   con gli scostamenti decisi.
5. `scripts/controlla.sh --tidy` (preset `ci`: avvisi come errori). Se il tidy
   rifiuta per il numero dei file, lo si lascia alla CI e lo si scrive nella
   pull request.
6. **Una sola pull request**, da `f6-grafica` a `main`, con `Closes #658`; da
   qui vale "Quando fermarsi" di AGENTS.md: CI verde, poi revisione di Codex.
   Il ramo non si unisce a pezzi.
7. **Prova manuale** del responsabile prima dello squash: il passo 2 della
   [prova della 0.7.5](../0.7.5-correzione-di-rotta.md#7-prova-manuale)
   (aspetto nei due temi, ribbon e interfaccia classica) su Linux e **su
   Windows**, dove il lavoro non è mai stato visto, più i passi 3, 4 e 7 per
   controllare che niente si sia rotto.

Dopo il merge il responsabile decide come esce (versione di correzione o
dentro il rilascio della tappa in corso) e che cosa resta della #655, compresa
la galleria approvata che la CI confronta.

## Regole che valgono qui

- Solo `app` dipende da Qt (ADR 0001); nessun colore dell'interfaccia fuori
  da `app/src/theme.cpp` (ADR 0039, punto 2).
- Nessuna libreria nuova per lo stile (ADR 0039, punto 4): foglio di stile,
  `QProxyStyle` e disegno proprio.
- Testi dell'interfaccia in inglese (le traduzioni sono della 0.9).
- Le icone seguono ICONE.md e il tratto da 1 px
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-10); `scripts/icone.py` deve
  passare.
- Le deroghe di questa scheda (sessioni locali senza CI, etichetta tenuta,
  una sola pull request alla fine, misura) valgono solo per F6; il resto di
  AGENTS.md vale com'è.

## Test richiesti

- Tutti i test di `tests/app` passano, con i valori dell'aspetto approvato.
- `app_gallery` passa e la galleria produce tutte le sue immagini nei due
  temi.
- Se la ricarica dello stile resta: un test che, senza la variabile, il foglio
  di stile è quello compilato, e con la variabile è quello del file.

## Fatto quando

- Ogni area di `F6-stato.md` è approvata dal responsabile, o rimandata con il
  motivo scritto.
- La pull request finale tocca solo i file del perimetro e non cambia il
  comportamento.
- `main` è già dentro il ramo: la pull request non ha conflitti.
- CI verde, revisione di Codex fatta, prova manuale su Linux e Windows
  superata.
