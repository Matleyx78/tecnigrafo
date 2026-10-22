# [0.5-P14] `io`: formato `.cadx`

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.5-R14]` (#…), agente gemini
- **Dipende da:** P3, P10, P13, R14
- **Modulo:** `io`
- **Da leggere:** `docs/formati/cadx.md` (P3: è la specifica di questo
  pacchetto), [ADR 0006](../../adr/0006-modello-dati-dwg-e-formato-cadx.md)
  punto 3, [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punto 2
  (conversione a R2018), `io/src/zip.hpp` (P13), `io/include/tg/io/io.hpp`,
  `core/include/tg/core/raw_drawing.hpp` (`NativePart`)

## File

- `io/src/cadx.cpp` (nuovo)
- `io/src/json.hpp` (nuovo, interno)
- `io/src/json.cpp` (nuovo)
- `io/src/io.cpp` (tolte le versioni provvisorie di `readCadx` e `writeCadx`)
- `io/CMakeLists.txt` (anche la definizione `TG_VERSION`, come per
  `tecnigrafo-script`)
- `tests/io/cadx_test.cpp` (nuovo)
- `tests/io/json_test.cpp` (nuovo)
- `tests/fuzz/fuzz_cadx.cpp` (nuovo)
- `tests/fuzz/corpus/cadx/` (nuova: 2–3 `.cadx` minuscoli)
- `tests/fuzz/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R14)

## Cosa fare

`readCadx` e `writeCadx` seguono `docs/formati/cadx.md`.

### JSON (`json.hpp/.cpp`)

Un lettore e uno scrittore JSON piccoli e severi (RFC 8259), solo per
`manifest.json`: oggetti, array, testi, numeri, `true`, `false`, `null`.
Limiti: profondità 32, 1 MiB. Le chiavi sconosciute si conservano, in ordine,
e si riscrivono.

### Lettura (`readCadx`)

1. `readZip` (P13).
2. La prima parte è `mimetype`, *stored*, con il contenuto esatto
   `application/vnd.tecnigrafo.cadx`; altrimenti `Malformed`:
   `not a Tecnigrafo drawing (no valid mimetype)`.
3. `manifest.json`: un oggetto con `format` intero ≥ 1. Se `format` è
   maggiore di `kCadxFormat`: `Unsupported`,
   `written by a newer version of Tecnigrafo (format 3; this version reads up
   to 1)`. Manca o non è valido: `Malformed`.
4. `drawing.dxf` con `readDxf`; i suoi errori si riportano con il nome della
   parte (`drawing.dxf: line 12: …`).
5. `manifest.json` e ogni altra parte (`extensions.json`, `thumbnail.png`,
   sconosciute) finiscono in `RawDrawing::nativeParts`, byte per byte.

### Scrittura (`writeCadx`)

- `drawing.dxf` con `writeDxf(document, DxfVersion::R2018)`: un disegno nato
  da un DXF meno recente si converte (ADR 0006, decisione del 2026-10-22); un
  errore della conversione si riporta così com'è.
- `manifest.json` ricostruito dal manifesto conservato (se c'è) con
  `format = kCadxFormat`, `generator = "Tecnigrafo <versione>"` e `parts`
  aggiornati; le altre chiavi restano.
- Parti nell'ordine della specifica: `mimetype` (*stored*), `manifest.json`,
  `drawing.dxf`, poi le altre in ordine di nome. Stesso disegno, stessi byte.

### Fuzzing

`fuzz_cadx.cpp` chiama `readCadx`; il job di P7 lo trova da solo.

## Regole della tappa che valgono qui

- Ciò che non si capisce si conserva (ADR 0006): una parte sconosciuta si
  riscrive identica.
- `io/CMakeLists.txt`, `tests/fuzz/CMakeLists.txt`, `tests/CMakeLists.txt`
  sono file comuni.

## Test richiesti

- Andata e ritorno di disegni casuali (le entità di P11 e P12, seme fisso) in
  `.cadx`: modello uguale entro la tolleranza, parti conservate identiche.
- Un `.cadx` costruito a mano con lo strumento `zip` come dice la specifica:
  si apre.
- Rifiuti: senza `mimetype`, `mimetype` compresso o non primo, contenuto
  diverso, manifesto mancante o non valido, `format` 0, 2 e 3, `drawing.dxf`
  mancante o danneggiato.
- Un disegno nato da un DXF R2000 diventa R2018 nel `.cadx`.
- JSON: testi con escape e caratteri non ASCII, numeri, profondità oltre il
  limite, chiavi doppie.

## Fatto quando

- I casi di `[0.5-R14]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- Un `.cadx` di una versione più recente si rifiuta con il messaggio scritto
  sopra.
- Il job `Fuzzing (io)` esegue anche `fuzz_cadx` e passa.
- La CI è verde.
