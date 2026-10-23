<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# Reader fuzzing

Install CMake, Ninja, Clang 18 with its compiler-rt/libFuzzer runtime, and
GoogleTest (otherwise configuration fetches the pinned release). No Qt is needed.
From the repository root:

```sh
export TG_BUILD_SUFFIX=-codex
cmake --preset fuzz
cmake --build --preset fuzz
mkdir -p "build${TG_BUILD_SUFFIX}/fuzz-corpus/dxf"
cp tests/fuzz/corpus/dxf/* "build${TG_BUILD_SUFFIX}/fuzz-corpus/dxf/"
# Use the same runtime checks as the asan test preset (ADR 0016).
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:detect_stack_use_after_return=1:strict_string_checks=1:check_initialization_order=1
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
export LSAN_OPTIONS="suppressions=$PWD/tests/sanitizer/lsan.supp:print_suppressions=0"
"build${TG_BUILD_SUFFIX}/fuzz/tests/fuzz/fuzz_dxf" \
  "build${TG_BUILD_SUFFIX}/fuzz-corpus/dxf" \
  -max_total_time=180 -timeout=10 -rss_limit_mb=2048
```

Use a writable copy of the corpus: libFuzzer adds interesting inputs to its
first corpus directory. The five hand-written DXFs cover an empty drawing,
all five supported entity types, ANSI_1252 bytes, DXF Unicode escapes, and an
unknown section. Their license and copyright are DXF group 999 comments.
The preset instruments all code, including `io` and `core`; only the reader
executables link the libFuzzer main. GCC is rejected at configuration time.

CI runs only for changes to `io/`, `third_party/`, or `tests/fuzz/` (or when
no base commit is available). It copies the seeds and the pinned public files
already downloaded to `TG_VERIFICA_SOURCES_DIR`, runs each reader for 180
seconds, and reports failures with a hex dump of up to 4096 bytes and the
complete input in the `fuzz-crash` artifact.

To reproduce a failure, download and extract that artifact, then pass the
individual input file instead of a corpus directory, with the runtime options
above still exported:

```sh
"build${TG_BUILD_SUFFIX}/fuzz/tests/fuzz/fuzz_dxf" /path/to/crash-input -runs=1
```

For a temporary fault-injection check, copy the target outside the repository,
add an intentional out-of-bounds access, and compile it with Clang 18 and
`-fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all
-fno-omit-frame-pointer -g -O1`. Run it with a nonempty seed and verify that
ASan stops execution and libFuzzer writes a failing input. Never commit the
fault or suppress a report.

New readers add `fuzz_<reader>.cpp` to this directory and optional seeds in
`corpus/<reader>/`. CMake and CI discover them automatically. Link any extra
reader-specific libraries in `tests/fuzz/CMakeLists.txt`.

Every discovered crash must be fixed and become a permanent verification case
written by another agent, as required by ADR 0020 and ADR 0011.
