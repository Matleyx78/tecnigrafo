# [0.6-R4] Riferimenti: identificativi di un disegno nuovo

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #272
- **Tipo:** pacchetto di **riferimenti** (AGENTS.md, "In un pacchetto di
  riferimenti")
- **Difficoltà:** media
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno medio
- **Lavoro collegato:** [P4](P4-tabelle-layer.md), issue #230 (claude)
- **Dipende da:** —
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md), punto 1,
  "Handle di un disegno nuovo"; `docs/verifica/rapporto.md` (righe
  `entities:` e `selection:`); `docs/verifica/casi.md`

## Perché

Fino a oggi un disegno nuovo dà alla prima entità l'identificativo 1. Con
l'ADR 0023 corretto gli handle da 1 a 256 sono riservati agli oggetti di
partenza (tipi di linea, layer `0`, scheletro DXF) e **la prima entità di un
disegno nuovo è 257**. Il rapporto scrive gli identificativi, quindi i
rapporti attesi dei casi che disegnano in un disegno nuovo cambiano. È un
cambio **di numerazione e basta**: geometrie, messaggi, annullamento e tutto
il resto restano uguali.

## File

- `tests/verifica/casi/<area>/<nome>/atteso.txt` dei 75 casi elencati sotto,
  e nessun altro file dei casi (`script.tgs`, `domande.tgq`, file di dati e
  immagini `atteso-*.png` restano come sono);
- `tests/verifica/in-attesa.txt`.

La CI rifiuta una pull request che mescola questi file con altri.

## La regola

In ogni `atteso.txt` elencato, ogni identificativo di un'entità **nata in un
disegno nuovo** cresce di **256**: `1` diventa `257`, `2` diventa `258`, `12`
diventa `268`.

Un identificativo si trova **solo** in due posti:

1. le righe degli elenchi `entities:`: il primo numero della riga, rientrata
   di due spazi (`  1 LINE start=0,0 end=10,0` diventa
   `  257 LINE start=0,0 end=10,0`);
2. la riga `selection:`, quando non è `none`: ogni numero
   (`selection: 1 3` diventa `selection: 257 259`).

**Non** si cambia nient'altro, nemmeno le righe che sembrano simili:

- le righe di `messages:` che iniziano con un numero sono messaggi, non
  entità: `  1 found, 1 total`, `  1 entity not supported: 3DFACE 1`
  restano uguali;
- `statistics:` (conteggi per tipo), `preview:` (senza identificativo),
  `undo:`, `redo:`, coordinate, `pointer:`, `active snap:`, `unhandled:`;
- la riga d'intestazione `tecnigrafo-report 1` e i titoli `[report N]`.

L'ordine delle righe non cambia: aggiungere lo stesso numero a tutti lascia
gli elenchi in ordine di identificativo.

### Che cos'è un disegno nuovo

- il disegno all'inizio dello script;
- il disegno dopo un `NEW` (o `Ctrl+N`) andato a buon fine: la numerazione
  riparte da 257, come oggi ripartiva da 1;
- un file **che lo script stesso ha salvato** da un disegno nuovo e poi
  riapre (`SAVEAS primo.cadx` … `OPEN primo.cadx`): le entità hanno gli
  identificativi con cui sono state salvate, già cresciuti di 256.

Restano **uguali** gli identificativi che vengono da un file aperto con `OPEN`
(o `Ctrl+O`) e non salvato dallo script: un DXF pubblico (`@sources/…`) o un
file del caso (`disegno.cadx`), e le entità disegnate dopo averlo aperto,
che continuano dalla numerazione del file. Un `OPEN` o un `NEW` annullato o
fallito lascia il disegno corrente com'è.

## Casi da cambiare (75)

Tutti i casi con almeno un identificativo nel rapporto, tranne i tre del
paragrafo successivo:

- `annulla/`: `u-undo-redo`
- `apertura/`: `new-modifiche`, `open-errori-e-limiti`, `open-modifiche-e-annulla`, `open-reset-stato`
- `arc/`: `errori-e-limiti`, `esc-annulla`, `inizio-centro-fine`, `selezione-ed-erase`, `snap`, `tre-punti`
- `circle/`: `due-punti`, `esc-annulla`, `raggio-e-diametro`, `selezione-ed-erase`, `snap`, `tre-punti`, `valore-non-positivo`, `valori-predefiniti`
- `erase/`: `esc-in-selezione`, `invio-senza-selezione`, `selezione-dopo`, `selezione-prima`
- `interfaccia/`: `file-scorciatoie-salva`, `ribbon-disegno-continuo`, `ribbon-disegno-dopo-cambio`, `ribbon-scorciatoie`, `snap-inserimento-disattivato`, `snap-inserimento`, `spazio-riga-di-comando`, `testo-caratteri-speciali`, `testo-con-spazi`, `testo-esc-a-meta`
- `line/`: `annulla-ripeti`, `close`, `esc-conserva`, `punto-coincidente`, `segmenti-e-alias`, `selezione-ed-erase`, `snap-e-orto`, `undo-a-meta`
- `orto/`: `primo-punto-non-vincolato`, `snap-prevale`, `vincoli`
- `pline/`: `archi-e-linee`, `arco-iniziale`, `chiusura-arco-e-linea`, `close-undo`, `coordinate`, `direzioni-limite`, `esc-a-meta`, `esc-conserva`, `punto-coincidente`, `ripeti-invio`, `undo-modi`
- `salvataggio/`: `file-esistente-e-annulla`, `nomi-non-validi`, `qsave-nuovo-e-ripetuto`, `save-con-nome`, `saveas-estensioni`
- `selezione/`: `clic-scala`, `esc-svuota`, `finestra-intersezione`, `rimuovi-shift`
- `snap/`: `modi-base`, `priorita-estremo-medio`
- `text/`: `altezza-e-rotazione`, `base-e-alias`, `contenuto-speciale`, `errori-e-limiti`, `esc-annulla-ripeti`, `multiriga-interlinea`, `selezione-ed-erase`, `snap`, `valori-predefiniti`

I casi `interfaccia/` sono di livello 2: stessa regola.

## Casi da **non** cambiare

- `apertura/open-dxf-pubblici`: apre due DXF pubblici; gli identificativi
  (`48 LWPOLYLINE`) sono gli handle del file.
- `apertura/open-cadx-valido`: apre `disegno.cadx` come prima cosa; la `LINE`
  disegnata dopo (`256`) continua la numerazione del file.
- `interfaccia/file-scorciatoie-apri-nuovo`: la linea disegnata nel disegno
  nuovo non compare in nessun rapporto (`Ctrl+N` la scarta prima del primo
  `!report`); `48` e `64` vengono dal file aperto con `Ctrl+O`.
- I casi senza identificativi (`interfaccia/ribbon-apri-chiudi`,
  `interfaccia/ribbon-chiudi-apri`, `salvataggio/insunits`) e tutti i casi di
  domande (`domande.tgq`): le domande rispondono con i nomi delle entità, e
  gli handle che citano (`D:A2`) vengono da file.

## `in-attesa.txt`

Alla fine del file, un blocco nuovo con una riga per caso cambiato, issue
#230 (P4):

```
# 0.6-R4: identificativi di un disegno nuovo da 257 (#230)
annulla/u-undo-redo #230
apertura/new-modifiche #230
apertura/open-errori-e-limiti #230
apertura/open-modifiche-e-annulla #230
apertura/open-reset-stato #230
arc/errori-e-limiti #230
arc/esc-annulla #230
arc/inizio-centro-fine #230
arc/selezione-ed-erase #230
arc/snap #230
arc/tre-punti #230
circle/due-punti #230
circle/esc-annulla #230
circle/raggio-e-diametro #230
circle/selezione-ed-erase #230
circle/snap #230
circle/tre-punti #230
circle/valore-non-positivo #230
circle/valori-predefiniti #230
erase/esc-in-selezione #230
erase/invio-senza-selezione #230
erase/selezione-dopo #230
erase/selezione-prima #230
interfaccia/file-scorciatoie-salva #230
interfaccia/snap-inserimento-disattivato #230
interfaccia/snap-inserimento #230
interfaccia/spazio-riga-di-comando #230
interfaccia/testo-caratteri-speciali #230
interfaccia/testo-con-spazi #230
interfaccia/testo-esc-a-meta #230
line/annulla-ripeti #230
line/close #230
line/esc-conserva #230
line/punto-coincidente #230
line/segmenti-e-alias #230
line/selezione-ed-erase #230
line/snap-e-orto #230
line/undo-a-meta #230
orto/primo-punto-non-vincolato #230
orto/snap-prevale #230
orto/vincoli #230
pline/archi-e-linee #230
pline/arco-iniziale #230
pline/chiusura-arco-e-linea #230
pline/close-undo #230
pline/coordinate #230
pline/direzioni-limite #230
pline/esc-a-meta #230
pline/esc-conserva #230
pline/punto-coincidente #230
pline/ripeti-invio #230
pline/undo-modi #230
salvataggio/file-esistente-e-annulla #230
salvataggio/nomi-non-validi #230
salvataggio/qsave-nuovo-e-ripetuto #230
salvataggio/save-con-nome #230
salvataggio/saveas-estensioni #230
selezione/clic-scala #230
selezione/esc-svuota #230
selezione/finestra-intersezione #230
selezione/rimuovi-shift #230
snap/modi-base #230
snap/priorita-estremo-medio #230
text/altezza-e-rotazione #230
text/base-e-alias #230
text/contenuto-speciale #230
text/errori-e-limiti #230
text/esc-annulla-ripeti #230
text/multiriga-interlinea #230
text/selezione-ed-erase #230
text/snap #230
text/valori-predefiniti #230
```

Tre casi cambiati sono **già** in attesa per #249 (P16):
`interfaccia/ribbon-scorciatoie`, `interfaccia/ribbon-disegno-dopo-cambio`,
`interfaccia/ribbon-disegno-continuo`. Si cambiano come gli altri, ma la loro
riga resta quella di #249 e non se ne aggiunge un'altra: li renderà
obbligatori P16, che per questo dipende anche da P4.

## Come controllare il lavoro

Senza eseguire Tecnigrafo (AGENTS.md): i valori si ottengono solo
aggiungendo 256.

- `git diff --stat` tocca 75 `atteso.txt` e `in-attesa.txt`, nient'altro.
- In ogni `atteso.txt` le righe cambiate sono solo righe di `entities:` e
  `selection:`, e in ognuna cambia solo il numero; il numero di righe del
  file non cambia.
- In nessun caso cambiato resta un identificativo minore di 257 nato in un
  disegno nuovo.
- Nella CI i 72 casi aggiunti a `in-attesa.txt` falliscono, come previsto
  (Tecnigrafo dà ancora 1), quindi il passo `Verifica` è verde. Se uno di
  loro **passa**, la CI fallisce: vuol dire che quel caso non andava cambiato
  o non è cambiato; ricontrollalo con la regola.

## Fatto quando

- I 75 `atteso.txt` seguono la regola; gli altri file dei casi non sono
  toccati.
- I 72 casi nuovi sono in `in-attesa.txt` con #230, i 3 di #249 restano
  com'erano.
- La CI è verde.
