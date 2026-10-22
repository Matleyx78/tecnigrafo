# ADR 0021: Comandi dei file

- **Stato:** Accettato (2026-10-22)
- **Data:** 2026-10-22

## Contesto

La tappa 0.5 porta `NEW`, `OPEN`, `SAVE`, `SAVEAS`, l'avviso di modifiche non
salvate, i file recenti e le unità del disegno (`INSUNITS`). Lettura e
scrittura stanno nel modulo `io` (ADR 0017); restano da decidere i comandi,
le domande che fanno, dove stanno le finestre di dialogo e come la modalità di
controllo apre e salva file senza diventare un rischio.

Vincoli:

- i comandi stanno in `cmd`, senza Qt (ADR 0005); le domande usano le parole di
  AutoCAD, così gli script `.scr` restano validi (ADR 0011, punto 3);
- uno script può fare solo ciò che farebbe l'utente; la modalità di controllo
  non apre nuovi punti di attacco (ADR 0011, punto 10);
- il rapporto cresce solo con campi scritti quando diversi da una sessione
  nuova (`docs/verifica/rapporto.md`, "Come cresce il formato").

## Decisione

### 1. Un disegno alla volta

Tecnigrafo tiene aperto un solo disegno, come AutoCAD in modalità a documento
singolo. `NEW` e `OPEN` sostituiscono il disegno corrente, dopo aver chiesto se
salvarlo quando ha modifiche. Il disegno nuovo si chiama `Drawing1.cadx`,
unità millimetri (`INSUNITS` 4). La cronologia di annullamento si azzera quando
si apre o si crea un disegno.

### 2. I comandi

Le domande sono quelle di AutoCAD con `FILEDIA` a 0: il nome del file si
scrive sulla riga di comando. Nell'interfaccia gli stessi comandi, lanciati
dal menu, dalle scorciatoie o dalla riga di comando senza argomenti, aprono la
finestra di dialogo del sistema; in modalità di controllo (livello 1 e
`--script`) usano sempre la riga di comando.

| Comando | Domande | Effetto |
|---|---|---|
| `NEW` | (modifiche non salvate) | disegno nuovo vuoto |
| `OPEN` | (modifiche non salvate), `Enter name of drawing to open:` | apre un `.cadx` o un `.dxf`, secondo l'estensione |
| `QSAVE` | `Save drawing as <Drawing1.cadx>:` solo se il disegno non ha ancora un file | salva nel file corrente, nel suo formato |
| `SAVE` | `Save drawing as <nome corrente>:` | salva con il nome dato; il disegno resta legato al nome corrente |
| `SAVEAS` | `Save drawing as <nome corrente>:` | salva con il nome dato, che diventa quello corrente |
| `INSUNITS` | `Enter new value for INSUNITS <4>:` | cambia le unità (intero da 0 a 24, come nel DXF); si annulla con `U` |

- **Modifiche non salvate:**
  `Save changes to Drawing1.cadx? [Yes/No/Cancel] <Yes>:`. `Yes` salva come
  `QSAVE`, `No` scarta, `Cancel` o `Esc` annulla il comando.
- **File che esiste già** (`SAVE`, `SAVEAS`):
  `A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:`.
- **Formato:** dall'estensione. `.cadx` scrive il formato nativo, `.dxf` un DXF
  (versione secondo l'ADR 0017, punto 2). Un'estensione mancante si completa
  con `.cadx`; un'estensione diversa dà un errore.
- **Errori:** un messaggio sulla cronologia che nomina il file e la causa
  (`Cannot open plan.dxf: line 812: expected a group code, found "abc".`);
  il disegno corrente non cambia. Un `.cadx` di una versione più recente:
  `Cannot open plan.cadx: it was written by a newer version of Tecnigrafo
  (format 3; this version reads up to 1).`
- **Entità non gestite:** dopo l'apertura, il messaggio dell'ADR 0018,
  punto 3.
- Il salvataggio è **atomico**: si scrive un file temporaneo nella stessa
  cartella e lo si rinomina solo a scrittura completata; un errore lascia
  intatto il file che c'era.

### 3. Stato del disegno

`edit::Editor` sa:

- se il disegno ha **modifiche non salvate**: confronta la posizione della
  pila di annullamento con quella del salvataggio, così annullare fino al
  punto salvato toglie l'avviso, come in AutoCAD;
- il **nome** del file (o `Drawing1.cadx` se non ne ha uno) e il suo formato.

`INSUNITS` è una modifica al documento e passa dall'editor, come le altre.

### 4. Accesso ai file

- `io` definisce un'interfaccia `io::FileAccess` (leggere tutti i byte di un
  file, scriverli in modo atomico, dire se esiste). I comandi la ricevono dal
  loro contesto; non aprono mai un file da soli.
- **Nell'interfaccia** (`app`) l'implementazione usa il file system senza
  limiti: è l'utente che sceglie.
- **In modalità di controllo** (`tecnigrafo-script` e `--script`)
  l'implementazione ammette solo:
  - percorsi relativi alla **cartella di uscita** del caso, senza `..`, per
    leggere e scrivere;
  - `@sources/<nome>`, in sola lettura: il file `<nome>` di
    `tests/verifica/sorgenti.json`, già scaricato e controllato.

  Ogni altro percorso dà `Invalid file name.`. Così uno script non può leggere
  né scrivere fuori dalla sua cartella.

### 5. File recenti

Solo nell'interfaccia: gli ultimi 9 file aperti o salvati, dal più recente, nel
menu File, salvati nelle impostazioni del programma. Un file che non esiste più
si toglie dall'elenco quando si prova ad aprirlo. In modalità di controllo
l'elenco non si legge e non si scrive.

### 6. Rapporto della modalità di controllo

Due campi nuovi, scritti solo quando sono diversi da una sessione nuova:

- `drawing:` il nome del file corrente e, se ci sono modifiche non salvate,
  `, modified` (`drawing: plan.dxf, modified`);
- `units:` il valore di `INSUNITS`, se diverso da 4.

Il formato preciso si scrive in `docs/verifica/rapporto.md` con il pacchetto
che li realizza.

## Alternative considerate

- **Più disegni aperti insieme** (schede, come AutoCAD a documenti multipli):
  utile, ma tocca interfaccia, comandi e modalità di controllo; si potrà
  aggiungere con un ADR dedicato.
- **Comandi dei file solo nell'interfaccia:** la modalità di controllo non
  potrebbe provarli al livello 1, e gli script `.scr` di AutoCAD che salvano
  non funzionerebbero.
- **Percorsi liberi negli script:** uno script ricevuto da altri potrebbe
  leggere o sovrascrivere file dell'utente.
- **`SAVE` uguale a `QSAVE`:** in AutoCAD sono diversi, e gli script `.scr`
  scritti per AutoCAD si aspettano le loro domande.
- **Unità predefinita "senza unità" (0)**, come un DXF che non dice niente:
  il disegno tecnico in Italia e in Europa è in millimetri, e AutoCAD con il
  modello metrico fa lo stesso.

## Conseguenze

- `cmd` dipende da `io`; il contesto dei comandi riceve un `io::FileAccess`.
- La modalità di controllo ha una cartella di uscita per ogni caso, che la CI
  allega agli artefatti come le immagini.
- Il catalogo di parità registra `NEW`, `OPEN`, `QSAVE`, `SAVE`, `SAVEAS`,
  `INSUNITS` alla chiusura della tappa.
- I modelli di disegno (`.dwt`) e la scelta del formato DXF in `SAVEAS`
  restano per dopo.
