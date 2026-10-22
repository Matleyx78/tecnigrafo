# [0.5-P4] `io`: modulo, gruppi DXF e interfaccia

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #157
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: legge e scrive gruppi senza ancora costruire un
  disegno; lo provano i suoi test, il fuzzing (P7) e i casi di R10
- **Dipende da:** P1
- **Modulo:** `io` (nuovo)
- **Da leggere:** [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) (tutto),
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punto 1,
  [ADR 0021](../../adr/0021-comandi-dei-file.md) punto 4 (`FileAccess`),
  `core/include/tg/core/raw_group.hpp` (da P1), `docs/ARCHITETTURA.md`
  sezione "Moduli"; la *DXF Reference* di Autodesk, capitoli "DXF Format"
  e "Group Codes in Numerical Order"

## File

- `CMakeLists.txt` (una riga: `add_subdirectory(io)` dopo `core`, e il
  commento sugli strati)
- `io/CMakeLists.txt` (nuovo)
- `io/include/tg/io/result.hpp` (nuovo)
- `io/include/tg/io/file_access.hpp` (nuovo)
- `io/include/tg/io/dxf_groups.hpp` (nuovo)
- `io/include/tg/io/io.hpp` (nuovo)
- `io/src/file_access.cpp` (nuovo)
- `io/src/dxf_groups.cpp` (nuovo)
- `io/src/encoding.cpp` (nuovo)
- `io/src/io.cpp` (nuovo)
- `tests/io/dxf_groups_test.cpp` (nuovo)
- `tests/io/encoding_test.cpp` (nuovo)
- `tests/io/file_access_test.cpp` (nuovo)
- `tests/CMakeLists.txt`

## Cosa fare

Nasce il modulo `io` (`tg_io`, alias `Tecnigrafo::io`), che dipende solo da
`Tecnigrafo::core` e usa `tg_configure_target` come gli altri. Questo
pacchetto fa lo strato dei **gruppi** (righe codice/valore) e fissa
l'interfaccia pubblica di tutto il modulo; la costruzione del disegno arriva
con P10, il formato `.cadx` con P14.

### Risultati ed errori (`result.hpp`)

```cpp
struct Error {
    enum class Kind : std::uint8_t { Malformed, Unsupported, TooLarge, File, NotImplemented };
    Kind kind{Kind::Malformed};
    std::size_t line{0};   ///< line of the file, 1-based; 0 if not about a line
    std::string message;   ///< English, says what was expected and what was found
};

/// A value or an Error (no exception ever leaves the module, docs/adr/0017).
template <typename T> class Result { /* ok(), value(), error(), from T and from Error */ };
```

### Accesso ai file (`file_access.hpp`)

```cpp
/// How io and the file commands reach files (docs/adr/0021, point 4).
/// Names are UTF-8.
class FileAccess {
public:
    virtual ~FileAccess() = default;
    [[nodiscard]] virtual Result<std::string> read(std::string_view name) = 0;
    /// Writes atomically: a temporary file in the same folder, renamed when
    /// complete. On failure the old file, if any, is untouched.
    [[nodiscard]] virtual std::optional<Error> write(std::string_view name,
                                                     std::string_view bytes) = 0;
    [[nodiscard]] virtual bool exists(std::string_view name) const = 0;
};

/// The real file system, without restrictions (the application).
class DiskFiles final : public FileAccess { … };
```

`DiskFiles` usa `std::filesystem`; i nomi UTF-8 diventano percorsi con
`std::u8string`, così funzionano anche su Windows.

### Gruppi (`dxf_groups.hpp`)

```cpp
enum class TextEncoding : std::uint8_t { Utf8, Ansi1252, Ansi1250 };

struct GroupFile {
    core::RawGroups groups;           ///< values decoded to UTF-8
    std::vector<std::size_t> lines;   ///< line of each group code
    std::string version;              ///< $ACADVER, "" if absent
    std::string codePage;             ///< $DWGCODEPAGE, "" if absent
    std::vector<std::string> warnings;
};

[[nodiscard]] Result<GroupFile> readGroups(std::string_view bytes);
[[nodiscard]] std::string writeGroups(std::span<const core::RawGroup> groups,
                                      TextEncoding encoding);
[[nodiscard]] std::string formatNumber(double value);
[[nodiscard]] std::optional<double> parseNumber(std::string_view text);
```

Lettura (`readGroups`):

- righe con `\n` o `\r\n`; spazi iniziali e finali del codice ignorati, del
  valore conservati (tranne `\r`);
- il codice è un intero tra −5 e 1071; il valore si controlla secondo
  l'intervallo del codice (testo, numero reale, intero a 16, 32 o 64 bit,
  booleano, handle esadecimale, dati binari esadecimali): un valore non
  valido è un errore `Malformed` con la sua riga;
- un numero reale si legge con `parseNumber`: punto decimale, indipendente
  dalla lingua del sistema; infinito e `NaN` non sono validi;
- DXF binario (`AutoCAD Binary DXF`), versione precedente a `AC1015` o
  sconosciuta: errore `Unsupported` che nomina ciò che ha trovato;
- codifica (ADR 0017, punto 3): UTF-8 da `AC1021`; prima, la code page di
  `$DWGCODEPAGE` (`ANSI_1252` e `ANSI_1250`; un'altra dà un avviso e si legge
  come `ANSI_1252`); in tutte le versioni `\U+XXXX` diventa il carattere;
  byte UTF-8 non validi: errore;
- limiti (ADR 0017, punto 7): riga oltre 64 KiB, file che finisce a metà di
  un gruppo: errore. Nessuna memoria riservata in base a numeri del file.

Scrittura (`writeGroups`): codice allineato a destra su tre caratteri come
fa AutoCAD, `\r\n`, valori nella codifica data (in `ANSI_*` i caratteri che
mancano diventano `\U+XXXX`). `formatNumber` dà la rappresentazione più breve
che `parseNumber` rilegge uguale, sempre con almeno una cifra decimale
(`1.0`, non `1`).

### Interfaccia del modulo (`io.hpp`)

```cpp
enum class DxfVersion : std::uint8_t { R2000, R2004, R2007, R2010, R2013, R2018 };

struct ReadReport {
    std::map<std::string, int> unhandled;  ///< entity type -> count
    std::vector<std::string> warnings;
};
struct ReadResult { core::Document document; ReadReport report; };

[[nodiscard]] Result<ReadResult> readDxf(std::string_view bytes);
[[nodiscard]] Result<std::string> writeDxf(const core::Document& document, DxfVersion version);
[[nodiscard]] Result<ReadResult> readCadx(std::string_view bytes);
[[nodiscard]] Result<std::string> writeCadx(const core::Document& document);
inline constexpr int kCadxFormat = 1;
```

In questo pacchetto `readDxf` chiama `readGroups` (così il fuzzing prova già
il lettore dei gruppi) e poi restituisce `NotImplemented`; le altre tre
funzioni restituiscono subito `NotImplemented`. I messaggi dicono quale
pacchetto le realizza.

## Regole della tappa che valgono qui

- `io` non usa Qt né altre librerie (zlib arriva con P13, con l'ADR 0019).
- Nessuna eccezione esce dal modulo; per i dati del file non se ne usano.
- `CMakeLists.txt` e `tests/CMakeLists.txt` sono file comuni: solo le tue
  righe; chi arriva dopo si aggiorna.

## Test richiesti

- Un DXF minimo valido, con `\n` e con `\r\n`: gruppi, righe, versione.
- Ogni tipo di valore: valido, non valido, ai limiti (`32767`, `-32768`,
  handle di 16 cifre esadecimali).
- Numeri: `1`, `1.0`, `-0.0`, `1e300`, `1e400` (errore), `nan`, `,5`
  (errore); `formatNumber` rilegge uguale per valori casuali.
- Codifica: `ANSI_1252` e `ANSI_1250` con lettere accentate, `\U+00E8`,
  `\U+2300`, code page sconosciuta (avviso), UTF-8 non valido (errore);
  scrittura e rilettura nelle tre codifiche.
- DXF binario, `AC1009`, versione sconosciuta: errori che nominano la
  versione.
- File troncato a ogni lunghezza tra 0 e la lunghezza intera di un DXF di
  prova: sempre un errore o un risultato, mai un crash.
- `DiskFiles`: scrittura atomica (il file vecchio resta se la scrittura
  fallisce), nomi con lettere accentate.

## Fatto quando

- Il modulo `io` compila sui tre sistemi, senza Qt, e i suoi test passano.
- `readDxf` legge i gruppi e restituisce `NotImplemented`; le altre funzioni
  restituiscono `NotImplemented`.
- Ogni errore ha un messaggio in inglese con la riga, quando c'è.
- La CI è verde, compreso il job dei sanitizer.
