# [0.7.5-P30b] `app`: il tipo di documento apre e salva

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: con il solo disegno nomi, messaggi, file scritti
  e file recenti restano quelli di oggi (i test e i casi esistenti, invariati,
  sono la prova); il tipo di prova di P30 lo dimostra con test Qt
- **Dipende da:** P30
- **Nata:** 2026-11-07, dalla decisione del responsabile su B6 del verbale della
  revisione delle interfacce (P23,
  [#563](https://github.com/Matleyx78/tecnigrafo/issues/563);
  [DECISIONI.md](../../DECISIONI.md)); separata da P30 per la misura del
  pacchetto. Con R2 della revisione della correzione
  ([#588](https://github.com/Matleyx78/tecnigrafo/pull/588)): il rifiuto di B3
  prima della conferma di sostituzione
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 3,
  7 e 9; [ADR 0021](../../adr/0021-comandi-dei-file.md); la scheda
  [P30](P30-ambiente-attivo-e-tipi.md) (registri, azioni globali, tipo di
  prova); [P28b](P28b-new-e-open.md) (un file aperto in un solo documento,
  decisione B3); `app/src/document_types.hpp` (P30), `app/src/main_window.cpp`
  (`handleFilePrompts`, `chooseFile`, `dialogPath`, file recenti, chiusura),
  `app/src/start_page.cpp` (P29), `app/src/builtin_actions.cpp`;
  `cmd/src/commands/file_commands.cpp` (che cosa fanno oggi `OPEN`, `QSAVE` e
  `SAVEAS`)

## Perché

L'ADR 0041, punto 7, dà al registro dei tipi «come crearne uno nuovo, aprirlo e
salvarlo», e fa passare dal registro la cornice dell'interfaccia: un tipo nuovo
(le parti, gli assiemi) non deve cambiare la cornice. P30 dà al tipo solo
`create`; il responsabile ha scelto di completare il tipo secondo l'ADR invece
di correggere l'ADR (decisione B6). Oggi la cornice apre e salva eseguendo
`OPEN`, `QSAVE` e `SAVEAS`, che sono comandi del disegno: con il tipo di prova il
dialogo conosce `tgtest`, ma il comando lo rifiuta, e in un ambiente diverso da
`drawing2d` il comando non parte (P27).

## File

- `app/src/document_types.hpp`, `app/src/document_types.cpp`
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/start_page.cpp` (*New* e *Open…* passano dal registro)
- `app/src/builtin_actions.cpp` (`file.save` e `file.saveas` globali)
- `cmd/include/tg/cmd/commands/file_commands.hpp` e
  `cmd/src/commands/file_commands.cpp`, solo se serve estrarre la lettura di
  `OPEN` (punto 2)
- `tests/app/environments_test.cpp`, `tests/app/file_menu_test.cpp`

## Cosa fare

### 1. Aprire e salvare nel tipo

`DocumentType` (P30) ha due funzioni in più:

```cpp
/// How a document is saved: on its own file, or as a file the user chose.
enum class SaveMode : std::uint8_t { Save, SaveAs };

/// Writes a line to the command line history.
using HistoryMessage = std::function<void(const QString&)>;

struct DocumentType {
    // ... the members of P30, create included ...

    /// Opens `path`, whose extension is one of `extensions`, as a new document
    /// of this type, in its default environment; nullptr if it cannot, after
    /// writing the reason to `message`.
    std::function<std::unique_ptr<DocumentSession>(const QString& path,
                                                   const HistoryMessage& message)>
        open;

    /// Saves `session`, a document of this type: with SaveMode::Save on its
    /// own file (`path` is its name), with SaveMode::SaveAs as `path`. On
    /// success the document has the file `path` and is not modified; false if
    /// it could not save, after writing the reason to `message`.
    std::function<bool(DocumentSession& session, SaveMode mode, const QString& path,
                       const HistoryMessage& message)>
        save;
};
```

Il dialogo lo mostra la cornice: il tipo riceve un percorso già scelto, e la
sostituzione di un file che esiste l'ha già confermata la cornice (punto 3,
*Salva con nome*), come oggi la conferma il dialogo in `handleFilePrompts`.

### 2. Il disegno

Per il tipo disegno, aprire e salvare fanno **esattamente** ciò che fanno oggi
`OPEN`, `QSAVE` e `SAVEAS` dal menu: stessi messaggi nella cronologia, stessi
file scritti (la versione DXF di `QSAVE` sul proprio file, R2018 con
`SAVEAS`), stesso nome e stato «modificato» dopo. `save` esegue `QSAVE`
(`SaveMode::Save`) o `SAVEAS` (`SaveMode::SaveAs`) nel runner della sessione,
rispondendo alle richieste come fa oggi `handleFilePrompts`. `open` legge il
file come `OPEN`, con gli stessi messaggi (errori di lettura, entità non
gestite); se per farlo serve, la lettura di `OPEN` si estrae in una funzione di
`cmd` che `OPEN` e il tipo usano tutti e due, senza cambiare un messaggio.

I comandi scritti nella riga di comando (`NEW`, `OPEN`, `SAVE`, `QSAVE`,
`SAVEAS`) restano comandi del disegno, nell'ambiente `drawing2d` (P27), con
l'ospite di P28b.

### 3. La cornice passa dal tipo

Nessun percorso della cornice esegue più `NEW`, `OPEN`, `QSAVE` o `SAVEAS`:

- **Nuovo** (il pulsante `+`, `file.new`, i pulsanti *New* della pagina
  *Start*): `create` del tipo; `+` e `file.new` creano il primo tipo del
  registro (oggi il disegno), un pulsante di *Start* il suo. Il disegno nuovo si
  chiama `Drawing<N>.cadx` come con `NEW` (P28b).
- **Apri** (`file.open`, *Open…* di *Start*, i file recenti): il dialogo offre
  le estensioni di tutti i tipi registrati (con il solo disegno, i filtri di
  oggi); il tipo si trova dall'estensione (`DocumentTypeRegistry`, per
  estensione). Un file già aperto si attiva con il messaggio
  `<name> is already open.`, come `OPEN` (P28b). Altrimenti `open` del tipo, e il
  documento nuovo si aggiunge in coda e diventa attivo, nel suo ambiente. Un
  file che nessun tipo apre non apre niente e lascia un messaggio nella forma
  `Cannot open <name>: …` (il testo lo sceglie chi implementa, in inglese).
- **Salva** (`file.save`, *Yes* alla richiesta di salvare quando si chiude):
  `save` con `SaveMode::Save` se il documento ha un file; altrimenti come *Save
  As*. **Salva con nome** (`file.saveas`), in quest'ordine:
  1. il dialogo con le estensioni del tipo del documento, che sceglie il
     percorso **senza** chiedere ancora se sostituire un file che esiste;
  2. un percorso aperto in un **altro** documento si rifiuta subito, con il
     messaggio di B3 (`Cannot save <name>: it is open in another document.`),
     una volta sola: niente domanda di sostituzione, niente si scrive, il tipo
     non è chiamato e il documento resta com'era (decisione B3: il rifiuto
     viene prima della domanda di sostituzione, anche nella cornice);
  3. se il file esiste, la conferma di sostituzione; *No* o *Cancel* non salva;
  4. `save` con `SaveMode::SaveAs`.

  Come ottenerlo con i dialoghi di Qt (per esempio senza la conferma del
  dialogo stesso) lo sceglie chi implementa.
- Un file aperto o salvato entra nei file recenti, come oggi, qualunque sia il
  suo tipo.

`file.save` e `file.saveas` diventano globali (`environments` vuoto, P30), con
`requiresDocument` vero: valgono in ogni ambiente, perché passano dal tipo.

## Regole della tappa che valgono qui

- `main_window.*`, `builtin_actions.cpp`, `document_types.*`: prima di te P30,
  dopo di te P31.
- Con il solo disegno l'interfaccia e i file sono quelli di prima: i test di
  `file_menu_test.cpp` di oggi passano invariati.
- Interfaccia in inglese.

## Test richiesti

In `environments_test.cpp`, con il tipo di prova di P30, che nel test ha `open`
e `save` su un file di testo suo:

- (B6) dalla scheda del tipo `test`, *Save As* `a.tgtest` chiama `save` del tipo
  con `SaveMode::SaveAs`: la scheda si chiama `a.tgtest` e non ha `*`; *Save*
  dopo una modifica chiama `save` con `SaveMode::Save` sullo stesso file;
- chiusa la scheda, *Open…* `a.tgtest` apre una scheda del tipo `test`,
  nell'ambiente `test`, con `open` del tipo; aprirlo di nuovo attiva la stessa
  scheda con il messaggio;
- dalla scheda `test`, `file.new` crea un disegno e `file.open` di un `.cadx`
  apre un disegno: nessun comando del disegno parte nel runner del tipo `test`,
  e il messaggio di P27 non compare;
- *Save As* dalla scheda `test` con il nome di un disegno aperto rifiuta con il
  messaggio di B3, e il tipo non è chiamato;
- un file con un'estensione che nessun tipo dichiara non apre niente e lascia il
  messaggio.

In `file_menu_test.cpp`, con due disegni veri aperti, A come `a.cadx` (che
esiste) e B come `b.cadx` (R2 della revisione della correzione): da B, *Save
As* con il file `a.cadx` scelto nel dialogo **non** mostra la conferma di
sostituzione; la cronologia ha il messaggio di B3 una volta sola; nessun
salvataggio parte e `a.cadx` sul disco è quello di prima; B resta `b.cadx`,
con lo stesso stato «modificato». Da B, *Save As* con un file `c.cadx` che
esiste e non è aperto chiede la conferma; con *No* non salva, con *Yes* salva.

In `file_menu_test.cpp`, con il disegno: i test di oggi passano invariati
(salva, apri, copia, titolo, recenti); dal menu, *Save* di un disegno aperto da
un DXF vecchio conserva la sua versione e *Save As* in DXF scrive R2018, come
prova oggi `CurrentDxfPreservesReadVersionAndSaveasWritesR2018` per i comandi.

## Fatto quando

- La cornice crea, apre e salva solo attraverso il tipo, e con il disegno niente
  cambia.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
