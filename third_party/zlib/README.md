<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# zlib

Compressione e decompressione deflate e CRC-32 per gli archivi `.cadx`
([ADR 0019](../../docs/adr/0019-archivi-zip.md)). Questo `README.md` è
l'unico file nostro della cartella; tutti gli altri sono di zlib e **non sono
modificati**.

| | |
|---|---|
| Progetto | zlib, di Jean-loup Gailly e Mark Adler, <https://zlib.net/> |
| Versione | 1.3.2 (17 febbraio 2026) |
| Archivio originale | `zlib-1.3.2.tar.gz` |
| SHA-256 | `bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16` |
| Licenza | zlib, testo in [`LICENSE`](LICENSE) |

L'archivio è stato scaricato dalla pagina dei rilasci del repository ufficiale
(<https://github.com/madler/zlib/releases/tag/v1.3.2>) e il suo SHA-256 è
stato confrontato con quello pubblicato su <https://zlib.net/>: coincidono.
La firma `zlib-1.3.2.tar.gz.asc` è della chiave `783FCD8E58BCAFBA` di Mark
Adler.

## File presi

Solo quelli che servono a deflate, inflate e CRC-32, copiati così come sono
dalla radice dell'archivio:

- `adler32.c`, `crc32.c`, `crc32.h`, `deflate.c`, `deflate.h`, `inffast.c`,
  `inffast.h`, `inffixed.h`, `inflate.c`, `inflate.h`, `inftrees.c`,
  `inftrees.h`, `trees.c`, `trees.h`, `zutil.c`, `zutil.h`, `zconf.h`,
  `zlib.h`;
- `gzguts.h`, solo perché `zutil.c` lo include: le funzioni `gz*` (i file
  `gz*.c`) non ci sono;
- `LICENSE`.

Niente `contrib/`, esempi, test, script di build né documentazione.

## Come si compila

[`../CMakeLists.txt`](../CMakeLists.txt) ne fa la libreria statica `tg_zlib`,
collegata solo da `tg_io` e in privato. Non riceve gli avvisi del progetto né
`clang-tidy`, ma i sanitizer e il fuzzing la compilano come il resto. Con
`Z_PREFIX` tutti i simboli di zlib prendono il prefisso `z_`, così non si
scontrano con la zlib di sistema che Qt carica su Linux.

## Aggiornare

Un aggiornamento è un pacchetto a sé: si sostituiscono gli stessi file con
quelli della nuova versione, senza modificarli, e si aggiornano versione e
SHA-256 in questa pagina.
