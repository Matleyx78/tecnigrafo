# Comandi, build e CI

Da leggere quando lanci `scripts/controlla.sh`, `cmake` o `ctest`, o quando
`scripts/ci-attendi.sh` non esce con 0 ([AGENTS.md](../../AGENTS.md), "Se ...
leggi anche ..."). Vale insieme al nucleo.

## Comandi

Prima di ogni comando imposta la tua cartella di build, così più agenti sullo
stesso PC non si bloccano a vicenda: `export TG_BUILD_SUFFIX=-claude` (o `-codex`;
una seconda sessione di Claude sullo stesso PC usa `-claude-2`); i preset compilano in `build-<nome>/<preset>`, e i percorsi
`./build/...` della documentazione diventano i tuoi.

Dopo il clone e dopo ogni `git pull` esegui (il nucleo lo dice per ogni
lavoro, in "Regole che non si possono violare")
`git submodule update --init third_party/solid-space`, **senza** `--recursive`, così
il sottomodulo di solid-space è sempre al commit che `main` fissa
([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 6). Se lo
salti, la configurazione di CMake si ferma e ti scrive il comando.

Le prove locali si fanno con **un solo comando**, nell'ordine della CI, che si
ferma al primo errore e ne stampa la coda del log (log completi in
`build<TG_BUILD_SUFFIX>/controlla/`):

```sh
scripts/controlla.sh               # formato, compila, test, verifica
scripts/controlla.sh --tidy        # anche clang-tidy sui file toccati, prima della pull request
scripts/controlla.sh --solo test   # un solo passo (formato, compila, test, verifica, tidy)
scripts/controlla.sh --app-off     # senza Qt
```

Il tidy su tutto il progetto (`--tidy-tutto`) richiede decine di minuti: lo fa la
CI, non lanciarlo in locale. `--tidy` rifiuta più di 25 file (per esempio dopo
aver toccato un'intestazione inclusa ovunque): scrivi nella pull request che il
tidy è lasciato alla CI. Gli altri passi durano pochi minuti; se un comando
locale supera i 10 minuti non aspettarlo con controlli ripetuti: fermalo, scrivi
nella pull request quale passo non hai eseguito e pubblica.

Se nel tuo ambiente mancano gli strumenti di build (Qt, compilatori, CMake o
altri pacchetti di sistema), **non installarli**. Vale per qualunque agente e
ambiente: un'installazione dura a lungo, può bloccarsi e far cadere la sessione,
e il lavoro non ancora pubblicato va perso. Esegui solo i passi possibili con
quello che c'è (per esempio `clang-format`, o `--app-off` se manca solo Qt),
pubblica il ramo e apri la pull request: la verifica completa la fa la CI. Nella
pull request scrivi quali passi non hai eseguito.

Errori di memoria e comportamenti indefiniti li cerca il job `Linux (ASan,
UBSan)` della CI ([ADR 0016](../adr/0016-sanitizer-in-ci.md)); in locale:
`cmake --preset asan && cmake --build --preset asan && ctest --preset asan`. Un
errore nel nostro codice si corregge, non si sopprime.

## Pull request senza compilazione locale

Una pull request che tocca solo documenti, commenti o casi di riferimento non si
compila e non si eseguono i casi in locale: la verifica la fa la CI.
`scripts/controlla.sh --solo formato`, che non compila né esegue i casi, resta
permesso.
Come chiede il nucleo ("Prima di dichiarare il lavoro finito"), la pull request
dice quali passi non hai eseguito e perché. Per i casi di riferimento, se sono
validi lo dicono i controlli anticipati della CI ([riferimenti.md](riferimenti.md)).

## Aspettare la CI e leggerne gli errori

Si aspetta con un solo comando che blocca, perché ogni controllo ripetuto
consuma token.

```sh
# aspetta la CI dell'ultimo commit pubblicato, aspettando dentro lo script (non
# consuma token), al massimo 9 minuti per volta. Esce 0 se tutto è verde; 1 se
# un controllo è fallito (appena un job fallisce, senza aspettare Windows e
# macOS); 2 se l'API di GitHub non risponde; 3 se è ancora in corso (rilancialo,
# riprende da dove era); 4 se è stata annullata: non è un guasto, un push più
# recente sullo stesso ramo l'ha sostituita, quindi aspetta quella; 5 se è
# ancora in corso dopo dieci attese di fila (circa 90 minuti: la CI di Linux ne
# richiede più di 30). Accetta anche <sha> o <id>; non serve `gh`.
scripts/ci-attendi.sh
# perché è fallita: per ogni job fallito, il passo e gli errori (poche righe)
scripts/ci-errori.sh              # l'ultimo commit su cui sei; oppure <sha> o <id>
```

`scripts/ci-errori.sh` legge solo l'API REST, quindi funziona anche dove i log non
sono raggiungibili (per esempio il proxy blocca il servizio che li ospita) e dove
manca `gh`: allora usa `curl` e `jq`, senza token. Gli
errori vengono da `scripts/ci-annota.sh`, che la CI esegue su ogni passo di
compilazione, test e verifica; per i passi in Python l'annotazione contiene le
ultime righe dell'output. Se ancora non basta, e solo allora, il log intero dei
controlli falliti:

```sh
gh run view <id> --repo Matleyx78/tecnigrafo --log-failed
```

Con exit 2 o 5 non rilanciare l'attesa: scrivi nella pull request il messaggio
dello script e fermati. Dopo dieci attese con exit 3 si arriva comunque a 5.

Nel riepilogo per l'utente, tra ciò che non hai potuto verificare, ci sono per
esempio Windows e macOS.
