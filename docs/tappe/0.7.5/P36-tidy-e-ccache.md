# [0.7.5-P36] CI: confronto giusto per il tidy e cache di compilazione su Linux

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: infrastruttura; test e casi invariati
- **Dipende da:** P3 (tocca `.github/workflows/ci.yml`); P4 viene dopo questo
  pacchetto
- **Nata:** 2026-11-08, nella chat del responsabile, dai tempi della CI delle
  pull request: la #585, che tocca solo `cmd/` e `tests/`, ha fatto il tidy di
  tutto il progetto (62 min); in una pull request normale il job più lungo è
  quello con ASan (17–18 min), e i tre job Linux ricompilano tutto da zero
- **Modulo:** `.github/` (questa scheda lo autorizza), con `ccache` installato
  nei job Linux: è uno strumento della CI, non una dipendenza del programma,
  e lo ha ammesso il responsabile ([DECISIONI.md](../../DECISIONI.md),
  2026-11-08)
- **Da leggere:** `.github/workflows/ci.yml` (job `changes`, `linux`,
  `verifica`, `sanitizers`); `scripts/controlla.sh`, solo la parte che sceglie i
  file del tidy (`TG_TIDY_BASE`)

## File

- `.github/workflows/ci.yml`

## Cosa fare

1. **Confronto giusto.** In una pull request `actions/checkout` estrae il
   commit di prova che GitHub costruisce sopra il `main` più recente, mentre
   `github.event.pull_request.base.sha` può essere più vecchio. Oggi il passo
   «Choose the static analysis» confronta quel commit con la base vecchia
   (`git diff --name-only "$BASE"`), e conta come modifiche della pull request
   quelle entrate in `main` dopo: così la #585, partita dopo la fusione della
   #579 che cambiava `ci.yml`, ha fatto il tidy completo. Lo stesso confronto
   arriva a `scripts/controlla.sh` con `TG_TIDY_BASE`, e allarga il tidy dei
   file toccati.
   - Il passo e `TG_TIDY_BASE` usano come base il primo genitore del commit
     estratto, `git rev-parse HEAD^1`: il `main` su cui GitHub ha costruito il
     commit di prova. Così il confronto dà esattamente ciò che la pull request
     aggiunge al `main` di oggi.
   - Il passo stampa nel log l'elenco dei file confrontati.
   - Il commento spiega perché, con la #585 come esempio.
   - Gli inneschi del tidy completo restano quelli di oggi: `.clang-tidy`,
     `CMakeLists.txt` principale, `CMakePresets.json`, `cmake/` e
     `.github/workflows/ci.yml` (decisione del responsabile del 2026-11-08).
   - `scripts/controlla.sh` non cambia.
2. **ccache** nei job `linux`, `verifica` e `sanitizers`:
   - `ccache` si installa con `apt-get`, insieme agli altri pacchetti;
   - `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`
     vanno sulla riga di `cmake --preset`, senza toccare `cmake/` né i preset;
   - le variabili d'ambiente sono `CCACHE_DIR` in `build/ccache`,
     `CCACHE_MAXSIZE` a 500M e `CCACHE_COMPILERCHECK=content`;
   - la cache si **legge** sempre con `actions/cache/restore`, con chiave
     `<runner.os>-ccache-<job>-<github.sha>` e `restore-keys`
     `<runner.os>-ccache-<job>-`;
   - si **salva** con `actions/cache/save` solo dopo un push su `main` o
     nell'esecuzione notturna, anche se un passo successivo fallisce
     (`if: always()` con l'evento giusto). Le pull request non scrivono cache,
     così non consumano lo spazio del repository;
   - `ccache --show-stats` si stampa alla fine di ogni build.

   Il tidy completo (preset `tidy`) non usa ccache.

## Regole della tappa che valgono qui

- Nessun controllo si toglie e nessun avviso si disattiva.
- `.github/workflows/ci.yml` è in catena: P1 → P3 → P36 → P4 → P5 → P6.
- Windows e macOS restano come sono: con MSVC ccache chiederebbe di cambiare il
  formato delle informazioni di debug in `cmake/`, fuori da questo pacchetto.

## Test richiesti

Non ci sono test di unità: le prove sono esecuzioni della CI, che la pull
request riporta.

- La pull request stessa tocca `ci.yml`, quindi fa il tidy completo, come
  previsto. L'elenco dei file stampato al punto 1 coincide con quello della
  pull request su GitHub.
- Dopo la fusione, il primo push su `main` salva le tre cache: si vedono con
  `gh api repos/Matleyx78/tecnigrafo/actions/caches`.
- La prima pull request di codice successiva mostra in `ccache --show-stats`
  quasi tutti successi. Il responsabile la annota nella issue, perché arriva
  dopo il merge.

## Fatto quando

- L'elenco stampato da «Choose the static analysis» è quello dei file della
  pull request, anche quando `main` è andato avanti dopo la sua base. Si
  verifica sulla pull request stessa, se `main` si muove mentre è aperta;
  altrimenti con un ramo di prova che parte da un commit di `main` precedente a
  un altro cambio di `main` (il ramo di prova si cancella dopo).
- I cambi a `.clang-tidy`, `CMakeLists.txt` principale, `CMakePresets.json`,
  `cmake/` e `ci.yml` fanno ancora il tidy completo; gli altri, quello dei file
  toccati.
- Su `main` e di notte il tidy completo gira come prima.
- La pull request riporta i tempi dei passi di build dei tre job Linux, prima e
  dopo: quelli «dopo» li dà una pull request di prova su un solo `.cpp`,
  lanciata dopo che `main` ha salvato le cache; altrimenti la riga resta al
  responsabile, nella issue.
- CI verde.
