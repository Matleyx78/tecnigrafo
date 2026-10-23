# [0.5-P13] `io`: contenitore ZIP e zlib

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #169
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: strato interno senza effetti osservabili; lo
  provano i test con archivi creati da altri strumenti, il fuzzing e i casi di
  R14
- **Dipende da:** P7
- **Modulo:** `io` (più `third_party/zlib/`)
- **Da leggere:** [ADR 0019](../../adr/0019-archivi-zip.md) (tutto),
  [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md) punto 1,
  [ADR 0003](../../adr/0003-licenza.md); `io/include/tg/io/result.hpp` (P4),
  `tests/fuzz/` (P7); la specifica APPNOTE di PKWARE, sezioni 4.3 e 4.4

## File

- `third_party/zlib/` (nuova: i sorgenti di zlib che servono, **non
  modificati**, più `LICENSE` e `README.md` con fonte, versione e SHA-256
  dell'archivio originale); conta come un solo elemento
- `third_party/CMakeLists.txt` (nuovo)
- `CMakeLists.txt` (una riga: `add_subdirectory(third_party)` prima di `io`)
- `io/CMakeLists.txt` (collega `tg_zlib` in privato)
- `io/src/zip.hpp` (nuovo, interno)
- `io/src/zip.cpp` (nuovo)
- `tests/io/zip_test.cpp` (nuovo)
- `tests/io/dati/zip/` (nuova: archivi di prova e `crea.sh`, lo script che li
  ha creati)
- `tests/fuzz/fuzz_zip.cpp` (nuovo)
- `tests/fuzz/corpus/zip/` (nuova: 3–4 archivi minuscoli)
- `tests/fuzz/CMakeLists.txt`
- `tests/CMakeLists.txt`

L'ADR 0019 autorizza la copia di zlib: è il permesso esplicito che AGENTS.md
chiede per il codice di altri progetti.

## Cosa fare

### zlib

- L'ultima versione stabile pubblicata su <https://zlib.net>, scaricata
  dall'archivio ufficiale e controllata con la firma o l'hash pubblicati; nel
  repository solo i file `.c` e `.h` che servono a deflate, inflate e
  CRC-32 (niente `contrib/`, esempi, `gz*`).
- `third_party/CMakeLists.txt`: `enable_language(C)`, libreria statica
  `tg_zlib` con i suoi include, **senza** `tg_configure_target` (niente
  avvisi del progetto né `clang-tidy`, ADR 0019); i sanitizer e il fuzzing la
  compilano come il resto, perché le loro opzioni sono globali.
- Solo `tg_io` collega `tg_zlib`, in privato: nessun header di zlib compare
  negli header pubblici di `io`.

### Il contenitore (`zip.hpp`)

```cpp
namespace tg::io::detail {
struct ZipPart {
    std::string name;    ///< UTF-8
    std::string bytes;   ///< uncompressed content
    bool stored{false};  ///< read: method 0; write: do not compress
};
struct ZipLimits {
    std::size_t maxParts{256};
    std::uint64_t maxTotalSize{std::uint64_t{1} << 30};  ///< 1 GiB uncompressed
    std::uint32_t maxRatio{1000};                         ///< per part
};
/// Parts in the order of the central directory (docs/adr/0019, point 2).
[[nodiscard]] Result<std::vector<ZipPart>> readZip(std::string_view archive,
                                                   const ZipLimits& limits = {});
/// Parts in the given order, with a fixed date (1980-01-01 00:00).
[[nodiscard]] std::string writeZip(std::span<const ZipPart> parts);
}
```

Lettura:

- dal record di chiusura (cercato dalla fine, commento compreso) all'elenco
  centrale; ogni intestazione locale si controlla contro l'elenco centrale;
  le dimensioni si prendono dall'elenco centrale (archivi con descrittore
  dei dati, bit 3, compresi);
- metodi 0 e 8; cifratura, ZIP64, più dischi, altri metodi: `Unsupported`
  con il motivo;
- nomi: UTF-8 valido, con o senza bit 11 (lo strumento `zip` di Info-ZIP
  su Linux scrive i nomi UTF-8 senza bit 11; deciso in #190), relativi,
  senza `..` né `\`, senza duplicati: altrimenti `Malformed`;
- limiti **prima** di decomprimere (numero di parti, somma delle dimensioni
  dichiarate, rapporto) e **durante** (inflate si ferma appena supera la
  dimensione dichiarata): `TooLarge` o `Malformed`;
- CRC-32 controllato.

Scrittura: intestazioni locali, elenco centrale, record di chiusura;
`stored` per le parti che lo chiedono, deflate (livello 6) per le altre;
nessun campo extra; bit 11 per i nomi non ASCII.

### Fuzzing

`fuzz_zip.cpp` chiama `readZip` su un input qualunque; il job di P7 lo trova
da solo.

## Regole della tappa che valgono qui

- I file di zlib non si modificano: un aggiornamento è un pacchetto a sé.
- `tests/fuzz/CMakeLists.txt`, `io/CMakeLists.txt`, `CMakeLists.txt`,
  `tests/CMakeLists.txt` sono file comuni: solo le tue righe.

## Test richiesti

- Archivi creati da `zip` (con e senza compressione, con descrittore dei
  dati), dal modulo `zipfile` di Python e, se possibile, da Esplora file di
  Windows: lettura corretta di nomi e contenuti. `crea.sh` dice come sono
  stati fatti.
- Scrittura e rilettura di parti casuali, anche vuote e grandi (qualche MB);
  due scritture uguali danno byte uguali.
- Ogni rifiuto: cifrato, ZIP64, metodo 12, nome con `..`, nome assoluto,
  duplicato, CRC sbagliato, dimensione dichiarata falsa, rapporto 2000:1,
  257 parti, archivio troncato a ogni lunghezza.

## Fatto quando

- zlib compila sui tre sistemi senza avvisi che fermino la build e senza
  `clang-tidy`; `README.md` riporta fonte, versione e hash.
- `readZip` e `writeZip` passano i test, anche con i sanitizer.
- Il job `Fuzzing (io)` esegue anche `fuzz_zip` e passa.
- La CI è verde.
