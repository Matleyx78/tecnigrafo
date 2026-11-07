# Analisi: più pacchetti medi al posto di pochi impegnativi (#442)

- **Pacchetto:** [0.7.5-P8](P8-analisi-pacchetti-medi.md), issue #549
- **Autore:** Claude (Claude Code), modello Claude Opus 5.5, impegno alto
- **Data:** 2026-11-07; rivisto lo stesso giorno dopo il verbale di revisione
  di Codex e le note del responsabile (sezioni 2.2, 2.5, 3.1, 3.6, 3.9, 5 e 6)
- **Dati:** API REST di GitHub, lette il 2026-11-07, e `main` al commit
  `54bcda92e3782e583bd31f95c0991a043831bff3` (solid-space al commit fissato
  `73d7e42949018e5e36eb137c3f8ceb4ab6eb1c87`)
- **Stato:** analisi per il responsabile. Nessuna regola cambia con questo
  documento: le proposte di testo della sezione 6 si applicano solo se lui le
  approva.

## In breve

**Raccomandazione: sì, con condizioni**, con due letture.

- **Per il tempo e la qualità** (la domanda della #442): dividere conviene
  quando toglie un pezzo da una catena di dipendenze, da un'attesa di
  solid-space o da file comuni, oppure quando il pacchetto supera la misura; non
  conviene per il solo fatto che il pacchetto è impegnativo.
- **Per il costo dei modelli** (la valutazione chiesta dal responsabile nella
  revisione, sezione 3.9): si può spostare in fascia media gran parte del
  codice degli impegnativi, con un costo stimato sotto il giorno per tappa, se
  la fascia forte resta dove i dati dicono che rende: i riferimenti delicati, le
  schede e la revisione delle interfacce.

I dati delle tappe 0.4–0.7 dicono cinque cose che pesano sulla decisione:

1. **Nel campione i giri di correzione in CI non crescono con la misura.** Tra
   righe cambiate e run fallite non c'è un'associazione monotona (Spearman −0,07
   su 89 pacchetti di lavoro). Sono più frequenti nei medi (65 % con almeno un
   giro) che negli impegnativi di Claude (18 %). Modello e impegno usati non
   sono registrati (sezione 1): che i giri dipendano dalla fascia e
   dall'impegno è un'ipotesi plausibile, non un risultato, e il confronto tra
   pacchetti diversi non separa la natura del lavoro, il periodo e la scelta dei
   compiti. Il campione non dimostra che dividere faccia risparmiare giri.
2. **I ritardi della 0.7 non sono venuti dalla misura dei pacchetti.** Dopo
   l'unione di P25, sul percorso critico ci sono 27 ore di lavoro su 112: il
   resto sono fermi per buchi nelle schede, casi di riferimento sbagliati e
   pacchetti di solid-space, e attese tra un pacchetto e l'altro.
3. **Nella 0.7 la catena l'hanno fatta i file comuni.** `builtin_commands.cpp`,
   `command_descriptions.cpp`, `curve_questions.cpp`, `question_table.cpp` e
   il runner mettono in fila quasi tutti i pacchetti dei comandi e delle
   facciate. Due metà di un pacchetto che toccano gli stessi file sono in
   catena per regola: la tappa si allunga di un nodo, con i suoi costi fissi.
4. **La difficoltà di un riferimento la decide la geometria.** Sui sei
   impegnativi della 0.7 i casi delicati restano 73 qualunque divisione si
   faccia; con la regola del 2026-11-07 i pacchetti di riferimenti in fascia
   forte sarebbero 4 senza divisione e 5–6 con la divisione, più piccoli.
5. **La fascia forte, nei riferimenti, si vede.** Dei 37 pacchetti di
   riferimenti scritti con Gemini 3.8 Flash, 25 hanno avuto almeno una pull
   request di correzione (40 in tutto); degli 11 scritti in fascia forte
   (Claude, Codex, Gemini 4 Argon), uno solo (0.4-R8, due correzioni). Il
   campione forte è piccolo e i pacchetti non sono gli stessi, ma la differenza
   è grande, e ogni correzione della 0.7 ha fermato un pacchetto di lavoro.

Le condizioni (sezione 5; il testo proposto per le regole è nella sezione 6)
sono: dividere solo lungo un confine che toglie un arco di dipendenza o un file
comune, o che separa le decisioni da prendere dal codice da scrivere; misura
contata sul codice, test esclusi, con una soglia minima; fascia forte per chi
prende decisioni delicate, fascia media possibile per il codice che le
traduce; riferimenti divisi per natura (delicati e ordinari) e non per
pacchetto, quelli delicati sempre in fascia forte; revisione delle interfacce
prima delle parti; modello e impegno registrati in ogni pull request; un
pacchetto iniziale di interfaccia solo dove toglie file comuni a più
consumatori, come la variante `Entity` nella 0.8.

## 1. Metodo e limiti

**Che cosa è misurato.** Per ogni issue delle tappe 0.4–0.7 (etichetta
`tappa-X.Y`, 142 issue) il documento prende la pull request principale, cioè la
prima unita tra quelle che la chiudono o ne portano il codice nel titolo, e le
pull request successive che correggono i suoi casi o la sua scheda. Per ognuna:
commit, file e righe cambiate (aggiunte più tolte, senza `third_party/`), run di
`ci.yml` dell'evento `pull_request` sul suo ramo, fallite e annullate, ore tra
apertura e unione. Dai commenti della issue prende la prima presa in carico e
l'ultima prima della pull request. Comandi e regole di abbinamento sono nella
sezione 7.

**Che cosa è stimato, e dove.** Le stime sono segnate come tali nel testo. Le
principali: il costo in token della lettura di una sessione (dalla dimensione
dei file da leggere, non dal consumo reale, che non è registrato); la durata
aggiunta da un nodo in più in una catena (dalla mediana delle attese misurate);
il confine possibile di ogni impegnativo e le righe che ne sarebbero uscite
(dai file delle pull request reali); la fascia dei riferimenti divisi (dalla
regola del 2026-11-07 applicata ai casi contati).

**Definizioni.**

- **Giro di correzione:** una run fallita prima dell'ultima run verde della pull
  request. Le run annullate (un push nuovo annulla la run della pull request)
  si contano a parte.
- **Difficoltà:** l'etichetta della issue. Le issue di riferimenti delle tappe
  0.4–0.6 non hanno etichetta di difficoltà, e compaiono come «n.d.».
- **Autore:** lo strumento dichiarato nella sezione sull'uso dell'IA della pull
  request principale; dove il lavoro è passato di mano, chi ha scritto il codice
  unito (0.4-P5: Claude; 0.4-P9, 0.7-P18, 0.7-P22: Codex; 0.7-P25: Codex, che ha
  scritto il verbale; 0.7-R9 e R16: Gemini 4 Argon).
- **Caso fine:** una domanda `.tgq` la cui risposta contiene un numero con
  esponente o con almeno quattro decimali, o che segue una riga `!tolerance`, o
  che chiede una costruzione di tangenza, raccordo, curva parallela o smusso; uno
  script `.tgs` il cui `atteso.txt` contiene un numero così. Gli altri sono
  **ordinari**. È una regola meccanica, ripetibile con lo script della sezione
  7, non un giudizio caso per caso.

**Limiti.**

- **Un solo progetto, quattro tappe, una settimana di calendario ciascuna.** Le
  medie su 5–10 pacchetti per gruppo sono fragili; le tabelle danno mediane e
  totali, e i confronti si fanno solo dove la differenza è grande.
- **Modello e impegno usati non sono registrati.** Nessuna pull request di
  lavoro dichiara l'impegno; il modello lo dichiarano 20 dei 22 impegnativi di
  Claude (Opus 5.5), 6 dei 14 medi di Claude (2 Opus, 4 Sonnet) e nessuna pull
  request di Codex. La fascia del piano è un'indicazione, e il modello lo
  sceglie il responsabile all'avvio (LAVORO-IN-PARALLELO.md, «Modello e
  impegno»): l'etichetta di difficoltà non registra l'esecuzione. Il documento
  confronta quindi autori e difficoltà, e tratta ogni attribuzione a modello e
  impegno come ipotesi.
- **Difficoltà e autore si sovrappongono.** Gli impegnativi sono quasi tutti di
  Claude, i medi in gran parte di Codex. La sezione 2.2 separa i due effetti dove
  i numeri lo permettono; un confronto tra pacchetti diversi non separa comunque
  la natura del lavoro, il periodo e la scelta dei compiti.
- **Le ore sono di calendario, non di lavoro.** Comprendono notti, attese del
  responsabile e fermi; non dicono quanto ha ragionato un agente.
- **Il consumo di token e la fatica della revisione non sono registrati.** Il
  documento non li stima oltre la dimensione dei file da leggere.
- **I difetti scoperti dopo l'unione non sono misurati.** Le correzioni di
  codice successive (per esempio #533, l'alias `J` di `JOIN`) non sono
  attribuite in modo sistematico al pacchetto che le ha causate.
- **La 0.7.5 è aperta.** Il suo piano usa già quasi solo pacchetti medi (30
  pacchetti su 33 sono medi o facili), ma i suoi dati ancora non esistono.

## 2. Dati

### 2.1 Per tappa e d'insieme

«Righe» sono aggiunte più tolte nella pull request principale, test compresi e
`third_party/` escluso. «PR con ≥1 giro» conta le pull request principali con
almeno una run fallita prima del verde. «Ore presa→unita» va dalla prima presa
in carico all'unione, e comprende i fermi. L'ultima colonna conta le pull
request successive che hanno corretto i casi di quel pacchetto di riferimenti.

#### Tappa 0.4

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 1 | 1 | 1; 1 | 0 | 0 | 0,0 | 0,1 | 2 | 350 | — |
| pacchetto | media | 10 | 3 | 3; 31 | 12 | 6 | 2,6 | 3,3 | 7 | 384 | — |
| pacchetto | impegnativa | 4 | 2 | 2; 7 | 1 | 1 | 0,8 | 1,9 | 32 | 1798 | — |
| riferimenti | media | 1 | 1 | 1; 1 | 0 | 0 | 0,7 | 1,2 | 39 | 1255 | 0 |
| riferimenti | n.d. | 9 | 1 | 1; 11 | 0 | 0 | 1,0 | 2,4 | 13 | 445 | 9 |

#### Tappa 0.5

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 2 | 1 | 1; 2 | 0 | 0 | 0,1 | 0,4 | 3,5 | 128 | — |
| pacchetto | media | 10 | 2 | 2,5; 21 | 8 | 6 | 1,4 | 2,9 | 9,5 | 494 | — |
| pacchetto | impegnativa | 7 | 2 | 1; 10 | 2 | 1 | 0,9 | 1,7 | 14 | 1427 | — |
| riferimenti | n.d. | 10 | 1,5 | 1; 16 | 3 | 2 | 0,8 | 1,0 | 8,5 | 266 | 6 |

#### Tappa 0.6

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 2 | 1 | 1; 2 | 0 | 0 | 0,3 | 0,5 | 3,5 | 532 | — |
| pacchetto | media | 18 | 3,5 | 4; 63 | 19 | 12 | 0,9 | 1,3 | 11,5 | 682 | — |
| pacchetto | impegnativa | 9 | 3 | 3; 27 | 6 | 4 | 1,0 | 1,9 | 15 | 1155 | — |
| riferimenti | media | 1 | 1 | 1; 1 | 0 | 0 | 0,7 | 0,9 | 76 | 813 | 0 |
| riferimenti | n.d. | 8 | 2 | 1,5; 12 | 2 | 1 | 1,3 | 1,5 | 9 | 466 | 8 |

#### Tappa 0.7

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 3 | 2 | 2; 9 | 0 | 0 | 3,8 | 4,1 | 3 | 668 | — |
| pacchetto | media | 16 | 3,5 | 3; 65 | 12 | 11 | 2,4 | 5,8 | 12 | 608 | — |
| pacchetto | impegnativa | 7 | 2 | 2; 11 | 0 | 0 | 1,0 | 8,1 | 10 | 1052 | — |
| riferimenti | facile | 5 | 2 | 2; 9 | 3 | 2 | 3,4 | 3,6 | 5 | 132 | 2 |
| riferimenti | media | 14 | 1 | 1,5; 23 | 2 | 2 | 1,9 | 2,0 | 8,5 | 269 | 21 |
| riferimenti | impegnativa | 3 | 2 | 2; 5 | 2 | 2 | 2,8 | 3,0 | 9 | 466 | 0 |

#### Insieme 0.4–0.7

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 8 | 1 | 1; 14 | 0 | 0 | 0,3 | 0,5 | 3 | 414 | — |
| pacchetto | media | 54 | 3 | 3; 180 | 51 | 35 | 1,2 | 2,9 | 10,5 | 554 | — |
| pacchetto | impegnativa | 27 | 2 | 2; 55 | 9 | 6 | 1,0 | 1,9 | 14 | 1372 | — |
| riferimenti | facile | 5 | 2 | 2; 9 | 3 | 2 | 3,4 | 3,6 | 5 | 132 | 2 |
| riferimenti | media | 16 | 1 | 1; 25 | 2 | 2 | 1,7 | 1,8 | 9 | 294 | 21 |
| riferimenti | impegnativa | 3 | 2 | 2; 5 | 2 | 2 | 2,8 | 3,0 | 9 | 466 | 0 |
| riferimenti | n.d. | 27 | 1 | 1; 39 | 5 | 3 | 1,0 | 1,4 | 9 | 432 | 23 |

Tra le tappe, a parità di difficoltà, i numeri cambiano poco. Due cose
crescono nella 0.7: le ore dalla presa all'unione degli impegnativi (mediana
8,1 contro 1,7–1,9), per i fermi (sezione 2.4), e le correzioni dei
riferimenti (23 pull request su 22 pacchetti di riferimenti).

### 2.2 Per difficoltà e autore, pacchetti di lavoro

| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | Righe (med.) | Run fallite per 1000 righe |
|---|---|---|---|---|---|---|---|
| facile, claude | 3 | 0 | 0 | 0 % | 1 | 478 | 0,00 |
| facile, codex | 1 | 0 | 0 | 0 % | 1 | 350 | 0,00 |
| facile, gemini | 4 | 0 | 0 | 0 % | 1,5 | 398 | 0,00 |
| media, claude | 14 | 14 | 9 | 64 % | 2 | 783,5 | 1,04 |
| media, codex | 40 | 37 | 26 | 65 % | 3 | 509,5 | 1,63 |
| impegnativa, claude | 22 | 5 | 4 | 18 % | 2 | 1474 | 0,14 |
| impegnativa, codex | 5 | 4 | 2 | 40 % | 4 | 668 | 1,05 |

A parità di autore (Claude), i medi hanno avuto un giro nel 64 % dei casi e gli
impegnativi nel 18 %, pur essendo più grandi (mediana 1474 righe contro 784,
test compresi). Codex nei medi ha la stessa quota di Claude nei medi (65 %).
Nel campione, quindi, i giri seguono la difficoltà e non la misura: la
correlazione di Spearman tra righe e run fallite prima del verde è −0,07 su
tutti i pacchetti di lavoro, 0,05 sui medi, 0,13 sui medi di Codex, 0,01 sugli
impegnativi, cioè nessuna associazione monotona osservata.

Perché i medi abbiano più giri, i dati non lo dicono. Un'ipotesi è l'impegno
(gli impegnativi dichiarano Opus 5.5 e il piano chiede impegno alto), ma
l'impegno non è registrato, e i medi di Claude hanno usato modelli diversi: dei
14, 2 dichiarano Opus (2 con almeno un giro), 4 Sonnet (4 su 4), 8 nessun
modello (3 su 8); numeri troppo piccoli per un confronto. Altre spiegazioni
possibili, non separabili qui: i medi seguono uno schema già nel codice e
scoprono più spesso in CI le differenze con quello schema; gli impegnativi sono
stati pianificati con più cura; i periodi sono diversi.

### 2.3 Costi fissi misurati di una pull request

- **CI:** una run verde dura in mediana 14,9 minuti (222 run verdi delle pull
  request principali e di correzione).
- **Vita della pull request:** dall'apertura all'ultima run verde, mediana 0,6
  ore; dal verde all'unione, mediana 0,2 ore (141 pull request principali).
- **Run per pull request:** mediana 1–3 secondo il gruppo; 392 run in tutto
  nelle quattro tappe (60, 56, 113, 163).
- **Pull request per tappa**, correzioni comprese: 34, 36, 46, 78.
- **Correzioni dei riferimenti:** 46 pull request su 51 pacchetti di
  riferimenti, 0,9 per pacchetto; nella 0.7, 14 pacchetti di riferimenti su 22
  ne hanno avuta almeno una. Le correzioni crescono un poco con la misura del
  pacchetto di riferimenti (Spearman 0,36 tra righe e correzioni).
- **Lettura di una sessione (stima dalla dimensione dei file):** per un
  pacchetto di facciata della 0.7, le regole da leggere sempre (AGENTS.md,
  `pacchetto.md`, `comandi.md`, `contratto.md`, CODING_STYLE.md, tabella della
  ROADMAP) pesano circa 31 KB; gli ADR 0035 e 0036 circa 39 KB; la scheda 9–20
  KB; poi le intestazioni di solid-space e il codice. Una sessione in più rilegge
  regole e ADR: circa 70 KB, nell'ordine di 20 000 token (stima con circa 3,5
  caratteri per token).

### 2.4 Tempi della 0.7

«Fermo» va dalla prima presa in carico all'ultima prima della pull request:
è il tempo in cui il pacchetto era stato preso e si era fermato (buchi della
scheda, casi che sembravano sbagliati, solid-space). «Ultimo lavoro» va
dall'ultima presa all'unione. Orari in UTC (in Italia un'ora in più).

| Pacchetto | Difficoltà | Ultima dipendenza unita | Prima presa | Ultima presa prima della PR | Unito | Fermo (h) | Ultimo lavoro (h) |
|---|---|---|---|---|---|---|---|
| P0 | media | — | 29/10 16:40 | 29/10 16:40 | 29/10 20:13 | 0,0 | 3,5 |
| P1 | media | — | 29/10 16:41 | 29/10 16:41 | 30/10 08:11 | 0,0 | 15,5 |
| P2 | impegnativa | R2 29/10 19:07 | 30/10 12:05 | 30/10 12:05 | 30/10 13:07 | 0,0 | 1,0 |
| P3 | media | P2 30/10 13:07 | 30/10 15:41 | 30/10 15:41 | 30/10 16:40 | 0,0 | 1,0 |
| P4 | media | R4 30/10 15:25 | 30/10 15:34 | 30/10 15:34 | 30/10 20:18 | 0,0 | 4,7 |
| P5 | media | P4 30/10 20:18 | 30/10 20:28 | 31/10 06:04 | 31/10 06:58 | 9,6 | 0,9 |
| P6 | media | P5 31/10 06:58 | 31/10 07:01 | 31/10 10:53 | 31/10 11:33 | 3,9 | 0,7 |
| P7 | media | P3 30/10 16:40 | 30/10 17:17 | 31/10 02:51 | 31/10 06:02 | 9,6 | 3,2 |
| P8 | media | P6 31/10 11:33 | 31/10 17:07 | 31/10 17:07 | 31/10 19:16 | 0,0 | 2,1 |
| P9 | media | P4 30/10 20:18 | 31/10 07:04 | 31/10 07:04 | 31/10 07:59 | 0,0 | 0,9 |
| P10 | impegnativa | P8 31/10 19:16 | 01/11 04:25 | 01/11 05:48 | 01/11 06:10 | 1,4 | 0,4 |
| P11 | media | P10 01/11 06:10 | 01/11 09:43 | 01/11 09:43 | 01/11 10:07 | 0,0 | 0,4 |
| P12 | facile | P11 01/11 10:07 | 01/11 16:40 | 01/11 16:40 | 01/11 19:54 | 0,0 | 3,2 |
| P13 | facile | P12 01/11 19:54 | 03/11 13:26 | 03/11 13:26 | 03/11 17:31 | 0,0 | 4,1 |
| P14 | impegnativa | R14 01/11 12:27 | 01/11 12:42 | 01/11 14:06 | 01/11 15:54 | 1,4 | 1,8 |
| P25 | impegnativa | — | 01/11 17:37 | 01/11 21:12 | 02/11 02:13 | 3,6 | 5,0 |
| P15 | media | P25 02/11 02:13 | 01/11 16:41 | 03/11 13:25 | 03/11 20:47 | 44,7 | 7,4 |
| P16 | impegnativa | P15 03/11 20:47 | 03/11 20:53 | 03/11 20:53 | 04/11 04:58 | 0,0 | 8,1 |
| P17 | impegnativa | R17b 03/11 17:31 | 03/11 05:38 | 04/11 20:27 | 04/11 21:17 | 38,8 | 0,8 |
| P18 | media | P17 04/11 21:17 | 03/11 09:03 | 05/11 06:17 | 05/11 08:29 | 45,2 | 2,2 |
| P19 | impegnativa | P15 03/11 20:47 | 03/11 09:03 | 05/11 11:15 | 05/11 12:32 | 50,2 | 1,3 |
| P20 | media | P19 05/11 12:32 | 05/11 18:58 | 05/11 19:47 | 06/11 04:27 | 0,8 | 8,7 |
| P21 | media | P20 06/11 04:27 | 06/11 04:29 | 06/11 06:32 | 06/11 11:20 | 2,0 | 4,8 |
| P22 | media | R22b 04/11 13:17 | 01/11 16:41 | 04/11 17:52 | 04/11 20:59 | 73,2 | 3,1 |
| P23 | media | P21 06/11 11:20 | 06/11 11:27 | 06/11 13:03 | 06/11 13:46 | 1,6 | 0,7 |
| P24 | facile | P23 06/11 13:46 | 06/11 13:48 | 06/11 13:48 | 06/11 17:52 | 0,0 | 4,1 |

**Percorso critico realizzato:** P25 → P15 → P19 → P20 → P21 → P23 → P24 (si
risale dall'ultima unione lungo la dipendenza unita per ultima). P25 esiste dal
2026-11-01, quando P15 si è fermato; prima la tappa correva sulla prima metà.
Dall'unione di P25 (02/11 02:13) all'ultima unione (06/11 17:52) passano 111,6
ore; l'ultimo lavoro dei nodi del percorso somma 27,0 ore. Il resto sono i fermi
di P15 (fino al 03/11 13:25) e di P19 (fino al 05/11 11:15, decisione B2,
Matleyx78/solid-space#184 e #190, un caso di R19) e le attese tra i nodi. I fermi
di tutti i pacchetti della tappa sommano 286 ore.

Tra un nodo pronto (l'ultima dipendenza unita) e la sua prima presa in carico
la mediana è 3,1 ore, sui 20 pacchetti presi dopo essere pronti; con le notti si
arriva a 17 ore (P2) e 41,5 (P13). Gli altri sei sono stati presi prima di
essere pronti e si sono fermati.

### 2.5 Correzioni dei riferimenti per chi li ha scritti

Il modello è quello dichiarato nella sezione sull'uso dell'IA della pull request
principale. Le pull request di correzione sono quelle della sezione 2.1.

| Chi ha scritto i riferimenti | Pacchetti | Con ≥1 correzione | PR di correzione | Righe (med.) |
|---|---|---|---|---|
| Gemini 3.8 Flash | 37 | 25 | 40 | 325 |
| Gemini, modello non dichiarato | 3 | 3 | 4 | 212 |
| Gemini 4 Argon | 5 | 1 | 2 | 619 |
| Claude | 5 | 0 | 0 | 298 |
| Codex | 1 | 0 | 0 | 1461 |

In fascia forte (Gemini 4 Argon, Claude, Codex) gli 11 pacchetti hanno avuto
una sola volta delle correzioni: 0.4-R8, con due. Dei 37 di Gemini 3.8 Flash,
25 ne hanno avute, 40 in tutto, sia su casi geometrici (0.7-R14, R17, R19) sia
su comandi (0.7-R5, R18, R20, R21). I pacchetti forti non sono un campione
scelto a caso (nella 0.7 Argon ha preso due dei più delicati, R9 e R16), ma la
differenza è grande. Nella 0.7 ogni correzione ha fermato il pacchetto di
lavoro che aveva trovato il caso sbagliato (sezione 2.4).

## 3. Le otto domande

### 3.1 Costi fissi

**Quanto aggiunge dividere un impegnativo in due medi.** Per ogni pacchetto in
più, misurato: una pull request (0,8 ore di vita in mediana), una o più run di
CI (15 minuti ciascuna, 1–3 per pull request), un merge del responsabile e una
revisione. Stimato: una lettura di regole e ADR (circa 20 000 token) e una
presa in carico che, in una catena, aspetta in mediana 3,1 ore. Se si dividono
anche i riferimenti: un'altra pull request di riferimenti, un'altra revisione
del responsabile (il punto in cui la sua attenzione rende di più,
LAVORO-IN-PARALLELO.md, sezione 3) e, al ritmo misurato, 0,5–0,9 pull request
di correzione dei casi in più (stima: il valore basso se le correzioni seguono
i casi, quello alto se seguono i pacchetti; la correlazione 0,36 dice che
seguono un poco la misura).

**Quanto si recupera in giri di correzione.** Questo campione non dimostra un
risparmio. Gli impegnativi hanno avuto 9 run fallite su 27 pull request (0,33
ciascuna), i medi 51 su 54 (0,94), e la misura non è associata ai giri (sezione
2.2). Uno scenario, non un effetto misurato: se le due metà avessero i tassi
osservati dei medi e l'insieme quello osservato degli impegnativi, due medi
darebbero circa 1,9 run fallite al posto di 0,3. In assoluto costa poco: ogni
giro è un quarto d'ora di CI e una correzione piccola.

**Dove potrebbe esserci un guadagno vero, che i dati non misurano:** la
revisione di una pull request più piccola, e il danno di un errore che tocca
meno codice. I difetti scoperti dopo l'unione non sono registrati in modo
sistematico (sezione 1), quindi il documento non dice né che ci sono né che
mancano.

**Conclusione:** dividere ha un costo fisso moderato e misurabile, e nessun
risparmio dimostrato in CI. Conviene solo se dà qualcos'altro: parallelismo,
attese più corte verso solid-space, una parte che passa a una fascia più
economica (sezione 3.9).

### 3.2 «Non troppo piccoli» e misura massima

**La misura massima, oggi, non si rispetta.** Contando le righe come le conta la
regola («righe cambiate», test compresi), superano i 15 file o le 600 righe 29
medi su 54 e 25 impegnativi su 27. Contando solo il codice, test esclusi, i medi
hanno mediana 276 righe (primo quartile 121, terzo 477) e gli impegnativi 706
(350 e 1037); superano 600 righe di codice o 15 file 16 medi e 18 impegnativi. I
test sono circa metà delle righe: nella 0.7, P2 ha 509 righe di codice e 543 di
test, P16 770 e 687, P19 1077 e 624.

**Si concilia, con due ritocchi.** L'idea della #442 non contrasta con nessuna
delle due regole: chiede di dividere, e la misura massima chiede la stessa
cosa. Va però riscritta la misura, perché oggi conta i test, che crescono con i
casi limite e sono proprio ciò che si vuole: la misura conti le righe di codice,
test e documenti esclusi.

**La soglia sotto cui un medio è troppo piccolo.** I medi più piccoli misurati
sono una specifica, infrastruttura o configurazione (0.5-P3, 0.6-P8, 0.6-P3,
0.5-P9, 0.7-P0, da 0 a 25 righe di codice), legittimi perché il loro lavoro non
è il codice. Tra i
pacchetti di codice, il primo quartile dei medi è circa 120 righe. Proposta: una
metà di un pacchetto diviso deve avere almeno **150 righe di codice** (stima dal
primo quartile, arrotondata in alto) **oppure** essere un'unità completa che si
verifica da sola (un'entità, un comando, una famiglia di domande). Sotto, si
unisce al vicino. Con questa soglia P10 e P17 della 0.7 non si dividono
(sezione 3.3).

### 3.3 Divisibilità reale dei sei impegnativi della 0.7

Righe di codice e di test della pull request principale; CI della stessa;
«ultimo lavoro» e «fermo» dalla sezione 2.4. Confine e risultato sono stime,
costruite sui file che la pull request ha davvero cambiato.

| Pacchetto | File | Codice + test | Run (fallite) | Ultimo lavoro / fermo (h) | Confine possibile | Che cosa ne sarebbe uscito |
|---|---|---|---|---|---|---|
| P2 `cmd`: richieste di oggetti, numeri e selezione (#336) | 10 | 509 + 543 | 1 (0) | 1,0 / 0 | richieste di oggetto e di numero · richiesta di selezione e selezione scritta | due medi di circa 250 righe di codice, **in catena** (tutti e due in `command_runner.*`, già in catena con P3, P7, P8, P10); R2 serve solo alla seconda. Motore dei comandi: fascia forte anche divisi. Divisibile, non conveniente |
| P10 `cmd`: grip nel runner (#352) | 9 | 269 + 458 | 1 (0) | 0,4 / 1,4 | nessuno utile: il comando del grip è 124 righe, il resto è il runner | sotto la soglia. **Non divisibile.** Impegnativo per natura (motore dei comandi), medio per misura |
| P14 `core`: facciata, intersezioni, lato, tolleranze (#357) | 14 | 1399 + 785 | 2 (0) | 1,8 / 1,4 | traduzione e regola delle tolleranze, con `side`, `coincident`, `degenerate` · intersezioni e `cutsAlong`, con `intersections` | (a) circa 350 righe, medio con impegno alto; (b) circa 1050, ancora oltre la misura e ancora geometria: impegnativo, divisibile ancora tra coppie di entità e tagli lungo un'entità. In catena (b dopo a). Guadagno possibile: P22 e P17 partono dopo (a) se `sideOf` sta in (a) (stima, da verificare sulle intestazioni) |
| P16 `cmd`: `TRIM`, `EXTEND` (#361) | 11 | 770 + 687 | 2 (0) | 8,1 / 0 | `TRIM` con i due modi, le opzioni e `TRIMEXTENDMODE` · `EXTEND` e lo scambio con `Shift` | due medi di circa 400 righe, **in catena** (stesso `trim_extend_commands.cpp`, più `builtin_commands.cpp` e `command_descriptions.cpp`). R16 si divide per natura: 3 script ordinari, 4 fini. Divisibile, nessun guadagno di tempo |
| P17 `core`: curva parallela (#363) | 8 | 288 + 362 | 2 (0) | 0,8 / 38,8 | nessuno utile | sotto la soglia. **Non divisibile.** Il fermo (due casi di R17, Matleyx78/solid-space#130) non dipendeva dalla misura |
| P19 `core`: raccordi, tangenze, snap a tangente (#367) | 10 | 1077 + 624 | 2 (0) | 1,3 / 50,2 | raccordo (`fillet`, `filletSegments`, `filletPolyline`) · tangenze (`circleTangentTangentRadius`, `tangentPoints`, snap a tangente) | circa 650 e 400 righe, in intestazioni separate ma in catena per `curve_questions.cpp` e `question_table.cpp`; tutte e due fascia forte. La metà delle tangenze non dipendeva da B2 e poteva unirsi prima, ma il suo consumatore P20 (`FILLET` e *Ttr* insieme) aspettava anche P18: guadagno al più di circa 4 ore (stima), e solo dividendo anche P20 |

**Che cosa resta impegnativo per natura.** Nei sei casi la divisione riduce la
misura, non la fascia. Restano di fascia forte, anche piccoli:

- il **motore dei comandi** (runner, richieste, grip: P2, P10);
- la **geometria con molti casi limite** dietro una facciata di solid-space
  (P14, P17, P19), dove il lavoro è tradurre senza perdere le decisioni esatte
  (#359);
- la **progettazione di un'interfaccia nuova** di `core` (nella 0.8, le entità
  composte);
- la **conservazione DXF** che deve rileggere ciò che scrive (0.5-P10, 0.6-P10).

Questa è la lettura per il tempo e la qualità. In quella per il costo dei
modelli (sezione 3.9) conta chi prende le decisioni, non chi scrive il codice:
se le decisioni sono scritte prima e coperte da riferimenti forti, il codice di
quasi tutti questi pacchetti può andare in fascia media.

Con la soglia della sezione 3.2, due dei sei (P10, P17) non si dividono; due
(P2, P16) si dividono senza guadagno; due (P14, P19) si dividono con un
guadagno possibile, piccolo e legato ad altre condizioni (`sideOf` in P14a, P20
diviso).

### 3.4 Dipendenze e parallelismo

**Più pacchetti, nella 0.7, avrebbero fatto catene più lunghe.** La regola
«due pacchetti che toccano gli stessi file sono sempre legati da Dipende da»
(LAVORO-IN-PARALLELO.md, sezione 1) mette in fila, per i file comuni elencati
nel piano della 0.7 (sezione 6):

- i comandi: P5 → P6 → P8 → P16 → P18 → P20 → P21 → P23
  (`builtin_commands.cpp`, `command_descriptions.cpp`);
- il runner: P2 → P3 → P7 → P8 → P10;
- gli snap: P7 → P15 → P19;
- le domande delle curve: P14, P17, P19, P21, P22 (`curve_questions.cpp`,
  `question_table.cpp`).

Una metà in più su una di queste catene è un nodo in più: circa 4 ore di
calendario (stima: 3,1 ore di attesa mediana prima della presa in carico, più
0,8 ore di vita della pull request), senza contare i fermi. Divise P16 e P19, la
0.7 sarebbe durata circa 4 ore in più su 198 (P19 è sul percorso critico, P16
no), o circa 4 ore in meno nel caso migliore di P19 con P20 diviso (sezione
3.3): la differenza è dentro l'incertezza della stima. In ogni caso è poco rispetto ai fermi: circa 85 ore sul percorso dopo
P25.

**Le sessioni parallele di Claude aiutano i rami, non le catene.** Nella 0.7 le
attese prima della presa in carico dei pacchetti di Claude (P2: 17 ore, P10: 9,2)
cadono di notte, quando il responsabile non avvia sessioni e non unisce: il
limite non era il numero di sessioni. Più pacchetti medi accorciano una tappa
solo se stanno su rami diversi, con file diversi.

**Che cosa accorcia davvero.** Togliere file comuni. Un registro in cui ogni
comando, domanda o codec ha il suo file (o un elenco che si genera) toglie archi
dalle catene senza dividere nulla; nella 0.8 conta di più che nella 0.7
(sezione 4). Togliere fermi: la revisione delle interfacce prima delle schede
(nella 0.7 arrivata solo con P25, il 2026-11-01) e i casi di riferimento giusti
al primo colpo (sezione 3.5).

### 3.5 Riferimenti

**I casi dei sei riferimenti, contati** (regola della sezione 1; i file come
sono oggi su `main`, correzioni comprese):

| Riferimenti | Casi | Fini | Valori da | Chi li ha scritti nella 0.7 | Fascia con la regola del 2026-11-07, senza divisione | Con la divisione |
|---|---|---|---|---|---|---|
| R2 (selezione scritta) | 6 script | 0 | propri | Gemini 3.8 Flash | leggera | 1 leggero (serve solo alla seconda metà di P2) |
| R10 (grip nel runner) | 4 script nuovi, 1 modificato | 0 | propri | Gemini 3.8 Flash | leggera | 1 leggero (P10 non si divide) |
| R14 (facciata, intersezioni) | 74 domande | 15 (12 in `lato-e-tolleranze`) | archivio di solid-space | Gemini 3.8 Flash | forte (intersezioni) | R14a: 36 domande, 12 fini, forte; R14b: 38 domande, 3 fini, forte per la regola o leggero se tutti i valori vengono dall'archivio |
| R16 (`TRIM`, `EXTEND`) | 7 script | 4 | DXF di AutoCAD e oracolo | Gemini 4 Argon | forte | per natura: 3 script ordinari (leggero) e 4 fini (forte) |
| R17 (curva parallela) | 30 domande | 30 | archivio e DXF di AutoCAD | Gemini 3.8 Flash | forte | 1 forte (P17 non si divide) |
| R19 (raccordi, tangenze) | 24 domande | 24 | archivio e DXF di AutoCAD | Gemini 3.8 Flash | forte | R19a: 15 raccordi, forte; R19b: 9 tangenze, forte |

In tutto 73 casi fini e 72 ordinari. **Pacchetti di riferimenti in fascia
forte:** nella 0.7 com'è andata, 1 dei sei (R16, ad Argon); con la regola del
2026-11-07 e senza divisione, 4 (R14, R16, R17, R19); con la divisione, 5 o 6,
più 3 o 4 leggeri.

**L'ipotesi del responsabile del 2026-10-31, punto per punto.**

- *«I pacchetti di riferimenti aumentano, ma saranno medi o leggeri»*:
  **vero solo per la parte meccanica.** I casi fini non diminuiscono (73 in
  ogni divisione) e i pacchetti forti aumentano da 4 a 5–6. Le facciate di
  `core` (R14, R17, R19) non hanno una parte meccanica da staccare: le domande,
  i messaggi e l'annulla stanno nei comandi (P16, P18, P20), che hanno già i
  loro riferimenti.
- *«Pacchetti più piccoli si adattano meglio alla quota di Argon»*: **vero.** Un
  pacchetto forte diviso ha 9–36 casi invece di 24–74; con uno o due pacchetti
  alla settimana, Argon chiude interi i pezzi che prende.
- *«Una parte dei casi di un impegnativo diviso diventa ordinaria»*: **vero dove
  la divisione separa geometria e meccanica**, come in R16 (3 script su 7). Non
  vale per le facciate.
- *«La difficoltà la decide la geometria, non la misura»*: **confermato.** R9
  (grip, lavoro medio) ha 73 domande, 36 fini.

**Un pacchetto di riferimenti per più pacchetti di lavoro: sì, ma diviso per
natura.** I dati dicono che dividere i riferimenti insieme al lavoro moltiplica
le sessioni e le revisioni del responsabile senza cambiare i casi delicati. Meglio
un pacchetto di riferimenti **forte** con i casi fini di tutte le metà, e uno
**leggero** con quelli ordinari; ciascuno scrive un file in
`tests/verifica/in-attesa/` per ogni pacchetto di lavoro servito, così ogni
metà cancella solo il suo. Il meccanismo dei blocchi con la lettera esiste già
(ADR 0011, punto 7; `riferimenti.md`); cambia la regola «un pacchetto di
riferimenti copre un solo pacchetto di lavoro». Da verificare se il testo
dell'ADR 0011 lo lega a un solo pacchetto: se sì, la correzione dell'ADR la
decide il responsabile.

### 3.6 Assegnazione

**Chi prende le metà.** I dati della sezione 2.2: nei medi Claude e Codex hanno
la stessa quota di giri (64 % e 65 %), Codex qualche run fallita in più per riga
(1,63 contro 1,04 ogni 1000 righe); negli impegnativi Claude 18 %, Codex 40 % su
soli 5 pacchetti. Con quali modelli e impegni non si sa (sezione 1). Non ci sono
dati sulla qualità dopo l'unione.

**Con la lettura per il tempo e la qualità, l'idea sposta verso Codex meno di
quanto sembri.** Le metà di un impegnativo restano in gran parte di fascia forte
(sezione 3.3). Passano alla fascia media solo le metà meccaniche (opzioni, modi,
annulla, `Esc`), che nella 0.7 sarebbero state una sola (P16). Per quelle i dati
non danno motivi di sfiducia verso Codex: la stessa quota di giri di Claude nei
medi. La lettura per il costo dei modelli sposta molto di più, a condizioni
diverse (sezione 3.9).

**Una regola utile, per la fiducia:** la scheda di una metà dice da quale
pacchetto nasce e quale fascia aveva l'insieme, e scrive le decisioni delicate
che la riguardano, prese prima (nella scheda, nel contratto di solid-space o in
un ADR), così che chi la realizza le traduce e non le prende.

### 3.7 solid-space

**Le richieste non si moltiplicano, perché seguono le capacità.** Nella 0.7 le
richieste sono state quattro (Matleyx78/solid-space#20–#23), diventate i
pacchetti della tappa 0.2 di solid-space; P14 ne aspettava 8 (P4–P11), P17 4
più P4–P10, P19 4 più P4–P10, P22 3 più tre di quelli (tabella «Che cosa sblocca
in Tecnigrafo» del piano 0.2 di solid-space). solid-space è già più fine di
Tecnigrafo. Dividere un pacchetto di Tecnigrafo lungo le intestazioni di
solid-space non apre richieste: restringe le righe «Dipende da», e una metà può
partire quando sono uniti solo i pacchetti che le servono (nel caso di P14,
la traduzione e la regola delle tolleranze prima delle intersezioni).

**Le richieste nuove della 0.7 sono nate dai buchi, non dalla misura**:
Matleyx78/solid-space#158 (ordine degli estremi delle linee allineate, dai
buchi della scheda P22), #184 (parte da tenere nel raccordo, dal buco B2 di
P19), #187 (raggio troppo grande, da un buco segnalato nella #494).
Dividere non le avrebbe evitate né moltiplicate.

**Il rischio:** due metà che usano la stessa intestazione di solid-space con
letture diverse. Condizione: il confine passa tra intestazioni diverse, e la
traduzione (`solid_space_bridge`) resta un solo pacchetto, il primo.

### 3.8 Qualità del risultato

**Il rischio esiste già, anche senza dividere.** Nella 0.7 le schede P15–P23,
scritte una per una, avevano 18 buchi (B1–B18, verbale di P25, #464) e uno tra
due funzioni (#359: `partOf` riceveva solo valori arrotondati). Ogni confine tra
pacchetti è un posto in più per un buco di quel tipo: dividere ne aggiunge.

**Le quattro difese, valutate:**

- **Scheda madre.** No: rompe l'autosufficienza delle schede, fa rileggere a
  ogni metà un testo in più e duplica il progetto di massima del piano (sezione
  4 del modello). Basta che ogni metà citi le sorelle e la scheda dell'insieme,
  se esiste, nel suo «Da leggere».
- **ADR.** Sì per i principi (ADR 0035 e 0036 nella 0.7), ma i buchi della 0.7
  erano nelle schede, non negli ADR: l'ADR non basta.
- **Revisione delle interfacce prima delle metà.** Sì, ed è già regola dal
  2026-11-01 (LAVORO-IN-PARALLELO.md, sezione 1). Va estesa: la revisione
  guarda insieme tutte le schede nate dallo stesso pacchetto, anche se non
  introducono tipi in `core`. Nella 0.7 è arrivata dopo: i fermi di P15, P19 e
  P22 (168 ore in tutto) sono il costo di non averla avuta prima; quanta parte
  ne avrebbe evitata non si può misurare.
- **Pacchetto iniziale di sola interfaccia.** Solo dove toglie file comuni a più
  consumatori o sblocca due metà in parallelo. Altrimenti è una pull request di
  intestazioni senza comportamento, che il controllo dei riferimenti non vede;
  Codex, nella #522 (punto 7), avverte che non deve contenere intestazioni
  pubbliche senza contratto né sostituire i test d'integrazione. Nella 0.8 c'è un caso in cui
  conviene (sezione 4).

**Più un criterio di integrazione:** l'ultima metà ha nel suo «Fatto quando» i
casi che vedono l'insieme (per esempio un comando che usa tutte e due le metà),
così nessun test resta solo su un pezzo.

### 3.9 Una valutazione diversa: il costo dei modelli

Nella revisione il responsabile ha chiarito un secondo motivo della #442: con
pacchetti medi si possono usare Opus 5.5 medio e Codex sol medium (o alto)
invece della fascia forte, anche al costo di un giorno in più per tappa. Nella
tabella di LAVORO-IN-PARALLELO.md, «Modello e impegno», Opus 5.5 medio sta già
sia nella fascia forte sia nella media: per Claude il risparmio è l'impegno
(alto → medio) sullo stesso modello; per Codex è il modello (astra medium → sol
medium).

**Che cosa dicono i dati, e che cosa no.**

- **Il tempo regge.** Un giorno sono circa 24 ore di calendario. Un nodo in più
  sul percorso critico costa circa 4 ore (stima, sezione 3.4); una run fallita
  in più costa un quarto d'ora di macchina, quasi sempre mentre si lavora ad
  altro. Nello scenario della sezione 3.1 (tassi dei medi al posto di quelli
  degli impegnativi) le run fallite in più per un impegnativo sono circa 0,6:
  per i sei impegnativi della 0.7, circa 4 run, un'ora di CI. Il costo vero
  sarebbero i fermi in più, se un modello medio si ferma più spesso su un
  dubbio: non è misurabile. Nella 0.7 i fermi più lunghi li hanno avuti due
  pacchetti medi di Codex (P22, 73 ore; P15, 45 ore), ma per buchi delle schede,
  non per il modello: si sono fermati quando dovevano.
- **La qualità di un modello medio su lavoro impegnativo non è misurata.**
  Nessun impegnativo dichiara un modello o un impegno di fascia media (20 su 22
  di Claude dichiarano Opus 5.5 senza impegno, quelli di Codex nessun modello),
  e i difetti dopo l'unione non sono registrati (sezione 1).
- **Dove la fascia forte rende è misurato: nei riferimenti.** Le correzioni
  dei casi stanno quasi tutte sui riferimenti in fascia leggera (sezione 2.5),
  e ogni correzione ha fermato un pacchetto di lavoro. Un errore di un
  pacchetto di lavoro contro riferimenti giusti è invece una CI rossa: un giro,
  che costa poco. La fascia forte rende di più dove un errore non lo prende
  nessun altro controllo: i casi delicati, le decisioni nelle schede e nel
  contratto, la revisione delle interfacce.

**Ne viene un modello di lavoro: la fascia forte decide e controlla, la media
realizza.** Restano in fascia forte i riferimenti delicati, la revisione delle
interfacce, i pacchetti che progettano un'interfaccia nuova e la revisione
mirata delle pull request che toccano parti delicate (Copilot o Codex). Va in
fascia media il codice degli impegnativi le cui decisioni delicate sono già
scritte (nella scheda, nel contratto di solid-space o in un ADR) e coperte da
riferimenti forti, scritti prima.

**Per il costo, dividere non è il punto: lo è spostare le decisioni prima.**
L'etichetta «impegnativa» mette insieme due cose: prendere decisioni difficili
e scrivere codice difficile. Se le decisioni stanno nella scheda e nei
riferimenti, il codice resta da scrivere con cura ma secondo regole scritte.
Dividere serve solo dove un pacchetto mescola le due cose o supera la misura.

**I sei impegnativi della 0.7, con questa lettura** (stima):

| Pacchetto | Codice | In fascia media? | A quale condizione |
|---|---|---|---|
| P2 richieste e selezione | 509 | in parte | i tipi nuovi di richiesta nel runner sono progetto di interfaccia: restano forti, o si fissano nella scheda; la selezione scritta va in media |
| P10 grip nel runner | 269 | sì, senza dividere | comportamento con annulla ed `Esc` già scritto nella scheda; R10 resta leggero |
| P14 facciata e intersezioni | 1399 | sì, diviso per misura | regola delle tolleranze e traduzione dal contratto di solid-space; R14 forte |
| P16 `TRIM`, `EXTEND` | 770 | sì | scelta del pezzo da togliere scritta (ADR 0035, punto 5); i 4 casi fini di R16 forti |
| P17 curva parallela | 288 | sì, senza dividere | lato e tolleranza dal contratto; R17 forte |
| P19 raccordi e tangenze | 1077 | sì, diviso per misura | parte da tenere decisa nel contratto (Matleyx78/solid-space#184) prima della presa; R19 forte |

In righe di codice andrebbero in fascia media circa 3800 delle 4300 dei sei
impegnativi (88 %; il 94 % con metà di P2). Restano forti P2 (o la sua parte di
interfaccia), la revisione delle interfacce (P25) e quattro pacchetti di
riferimenti (R14, R16, R17, R19) invece di uno solo come nella 0.7: sessioni più
piccole (le pull request di R14, R17 e R19 della 0.7 hanno 268, 125 e 157 righe;
R16 ne ha 6889 per i DXF di AutoCAD), ma non meno numerose. Quanto costino in token non si può dire (sezione 1). Sul percorso
critico della 0.7 si aggiungerebbe al più un nodo (P19 diviso): con questa
lettura la tappa costerebbe forse qualche ora in più, ben dentro il giorno
accettato, più i fermi in più, se ci sono.

**Nella 0.8, con questa lettura** (stima sulla tabella della sezione 4):
restano forti PT1 (interfaccia delle entità composte), PT2 (revisione), PT11
(la prima quota completa, che prova l'interfaccia) e PT33 (coda comune del
canale MCP, con accessi concorrenti); possono andare in fascia media, con
riferimenti forti e decisioni scritte prima, PT6, PT12, PT18, PT20, PT24, PT26,
PT27 e PT32: 8 dei 12 impegnativi. Per PT26, PT27 e PT32 la condizione è che il
contratto di solid-space sia scritto prima della scheda, come la scheda della
0.8 già chiede.

**Che cosa serve per saperlo davvero.** Registrare modello e impegno in ogni
pull request (oggi l'impegno non lo dichiara nessuno, il modello solo una parte:
sezione 1), per esempio con una riga in più nella sezione sull'uso dell'IA del
modello di pull request (è un file di `.github/`, quindi lo decide il
responsabile); poi, alla chiusura della 0.8, contare con gli script della
sezione 7 i giri, i fermi, le correzioni dei riferimenti per fascia e le
correzioni di codice dopo l'unione dei pacchetti fatti in fascia media.

**Conclusione di questa lettura: sì, da provare nella 0.8**, con le condizioni
del modello «la fascia forte decide e controlla». Il costo atteso in tempo sta
sotto il giorno per tappa (stima), dominato dai fermi possibili e non dalla CI;
il rischio è un errore che i riferimenti non coprono, e la difesa è la
revisione mirata. Quello che i dati sostengono con più forza è il contrario
dell'ipotesi iniziale sui riferimenti: il risparmio sta nel codice, non nei
casi delicati, che conviene scrivere sempre in fascia forte.

## 4. Prova sulla carta sulla 0.8

Parte dalla divisione proposta da Codex nella #522 (punto 7) e dalla scheda della
0.8 di oggi, che comprende anche gli attributi al livello (b) e `PEDIT`
(DECISIONI.md, 2026-11-07). Gli ADR (entità composte, blocchi, canale MCP) si
scrivono con il piano, come nella 0.7 (ADR 0031–0036 nella #332): non sono
pacchetti. È una prova, non un piano: numeri, file e dipendenze sono stime.

**Un vincolo della 0.8 che la 0.7 non aveva.** `Entity` è una `std::variant` in
`core/include/tg/core/document.hpp`, e circa 25 file in sei moduli la visitano o
ne controllano il tipo (conteggio con `grep` di `std::visit` e
`holds_alternative`, che conta anche qualche visita di altre varianti). Ogni
entità nuova (testo multiriga, quota, retino, inserimento, attributo) tocca
quei file: senza un pacchetto che li prepara tutti insieme, le cinque famiglie
finiscono in una sola catena. Lo stesso vale, in piccolo, per `findCodec` in
`io/src/dxf_entities.cpp` e per i registri di `cmd` e `script`.

| Pacchetto | Modulo | File (stima) | Difficoltà | Dipende da | Perché impegnativo, se lo è |
|---|---|---|---|---|---|
| PT1 Entità composte: le nuove alternative di `Entity`, appartenenza, geometria generata, invalidazione; visite di tutti i moduli con comportamento conservativo | `core` (più le righe delle visite) | 15 | impegnativa | ADR del piano | progettazione di interfaccia; toglie la variante dalle catene delle cinque famiglie |
| PT2 Revisione delle interfacce delle schede di `core`, `io` ed `edit` | — | verbale | impegnativa | — | come 0.7-P25 e 0.7.5-P23; Codex, forte |
| PT3 Stili di testo: tabella e proprietà | `core` | 8 | media | PT1, PT2 | |
| PT4 Stili di testo nel DXF | `io` | 6 | media | PT3 | |
| PT5 Testo multiriga: modello e codici di formato | `core` | 8 | media | PT1, PT2 | |
| PT6 Testo multiriga: impaginazione e misure | `core` | 6 | impegnativa | PT5 | a capo, larghezze e codici con molti casi limite |
| PT7 Testo multiriga nel DXF | `io` | 6 | media | PT4, PT5 | |
| PT8 Testo multiriga e stili nel disegno | `render` | 5 | media | PT6 | |
| PT9 `MTEXT`, `STYLE` | `cmd` | 8 | media | PT6 | |
| PT10 Stili di quota | `core` | 8 | media | PT1, PT2 | |
| PT11 Quota lineare: modello in tre parti e geometria generata | `core` | 10 | impegnativa | PT10 | prima sezione completa: prova l'interfaccia di PT1 (Codex, #522, punto 7) |
| PT12 Quota nel DXF: `DIMENSION`, blocco della figura, dati conservati | `io` | 8 | impegnativa | PT11 | rigenerare senza perdere override e dati sconosciuti (ADR 0006, 0018) |
| PT13 `DIMLINEAR` | `cmd` | 7 | media | PT11 | |
| PT14 Quote allineate e angolari | `core` | 6 | media | PT11 | sullo schema di PT11 |
| PT15 Quote radiali e diametrali | `core` | 6 | media | PT14 | |
| PT16 Le altre quote nel DXF | `io` | 5 | media | PT12, PT15 | |
| PT17 `DIMALIGNED`, `DIMANGULAR`, `DIMRADIUS`, `DIMDIAMETER` | `cmd` | 9 | media | PT13, PT15 | |
| PT18 Blocchi: definizioni, inserimento, annidamento, layer 0 e *ByBlock* | `core` | 10 | impegnativa | PT1, PT2 | scale non uniformi (`Similarity` non basta), cicli |
| PT19 Attributi nel modello | `core` | 6 | media | PT18 | |
| PT20 Blocchi e attributi nel DXF: `BLOCK`, `INSERT`, `ATTDEF`, `ATTRIB`, `SEQEND` | `io` | 8 | impegnativa | PT19 | ordine e conservazione delle varianti non gestite (#522, punto 4) |
| PT21 Operazioni coerenti sugli inserimenti con i loro attributi | `edit` | 6 | media | PT19 | |
| PT22 `BLOCK`, `INSERT` | `cmd` | 8 | media | PT21 | |
| PT23 `EXPLODE` di inserimenti e quote | `cmd` | 5 | media | PT17, PT22 | |
| PT24 Definizione modificata: tutti gli inserimenti aggiornati, un passo di annulla | `edit` | 6 | impegnativa | PT21 | documento, annulla e DXF insieme (criterio di chiusura) |
| PT25 Retino con contorno dato: modello e motivi | `core` | 8 | media | PT1, PT2 | |
| PT26 Riempimento del motivo dentro il contorno, con isole | `core` | 6 | impegnativa | PT25 | decisioni geometriche da solid-space; scheda dopo il contratto |
| PT27 Facciata del rilevamento del contorno | `core` | 6 | impegnativa | PT25, contratto di solid-space | topologia 2D (rischio della scheda 0.8); scheda dopo il contratto |
| PT28 Retini nel DXF | `io` | 6 | media | PT25 | |
| PT29 Riempimenti e motivi nel disegno | `render` | 5 | media | PT26 | |
| PT30 `HATCH` | `cmd` | 8 | media | PT26, PT27 | |
| PT31 `PEDIT`: conversione e opzioni (*Close*, *Open*, *Reverse*, *Ltype gen*, *Undo*) | `cmd` | 7 | media | — | |
| PT32 `PEDIT` *Join* con tolleranza ed *Extend*, *Add*, *Both* | `cmd` | 6 | impegnativa | PT31 | se *Extend* chiede a solid-space, scheda dopo il contratto |
| PT33 Canale MCP: adattatore alla sessione aperta e coda comune | `app` | 8 | impegnativa | — | stesso runner, stesso annulla, nessun accesso concorrente |
| PT34 Canale MCP: trasporto locale su tre sistemi | `app` | 6 | media | PT33 | |
| PT35 Canale MCP: accensione e osservazione nell'interfaccia | `app` | 8 | media | PT34 | |
| PT36 Icone e voci dei comandi nuovi | `app` | 15 (a coppie) | media | PT9, PT17, PT22, PT30, PT31 | |
| PT37 Manuale | `docs` | 4 | facile | PT36 | |

**Risultato della prova.** 37 pacchetti; 12 impegnativi (32 %), tutti sotto i
15 file, ciascuno con il motivo in una riga; 24 medi e 1 facile. La quota di
impegnativi è vicina a quella della 0.7 (7 su 26 di lavoro, 27 %): la 0.8 ha
molti lavori difficili per natura, e la regola li rende più piccoli, non meno
numerosi.

**Parallelismo.** Con PT1 e PT2 unite, testi, quote, blocchi, retini, `PEDIT` e
MCP sono sei rami con file diversi in `core`, `io` e `edit`; restano in comune
`builtin_commands.cpp` e `command_descriptions.cpp` in `cmd` (PT9, PT13, PT17,
PT22, PT23, PT30, PT31, PT32, in catena) e le domande di `script`. Togliere
anche quelli (un file di registrazione per famiglia) è una separazione da
decidere con il piano. `PEDIT` e il canale MCP non dipendono da PT1 e possono
partire subito.

**Riferimenti (stima).** Fascia forte per l'impaginazione del testo multiriga
(PT6), la geometria delle quote (PT11, PT14, PT15), le trasformazioni degli
inserimenti (PT18), il riempimento e il contorno dei retini (PT26, PT27; il
contorno dall'archivio di solid-space) e il *Join* con tolleranza (PT32): 6–8
pacchetti di riferimenti forti, divisi per natura come nella sezione 3.5. Gli
altri sono ordinari.

**Che cosa resta da decidere nel piano:** se PT1 sta nei 15 file (se no, si
divide tra le alternative della variante e le visite, ma le due metà restano in
catena); se i retini hanno bisogno di decisioni robuste anche nel motivo (#522,
punto 6); come si verifica il canale MCP al livello 2.

## 5. Raccomandazione

**Sì, con condizioni.** Privilegiare i pacchetti medi regge come indirizzo per
la misura, per staccare dalle catene e, se lo scopo è il costo dei modelli, per
spostare in fascia media il codice degli impegnativi le cui decisioni sono già
prese (sezione 3.9). Non regge come conversione meccanica degli impegnativi.
Le condizioni:

1. **Si divide per un motivo che si scrive.** Un pacchetto si divide quando
   supera la misura, o quando la divisione toglie una parte da una catena di
   file comuni o da un'attesa di solid-space, o separa le decisioni da
   prendere dal codice da scrivere. Il piano scrive il motivo in una riga, come
   per un impegnativo che resta tale.
2. **La misura conta il codice.** Al massimo 15 file e circa 600 righe di
   codice, test e documenti esclusi; almeno 150 righe di codice o un'unità che
   si verifica da sola.
3. **La fascia segue le decisioni, non la misura.** Resta in fascia forte chi
   prende decisioni delicate: progetta un'interfaccia nuova, sceglie un
   comportamento con casi limite che nessun documento fissa, scrive i casi
   delicati, rivede le interfacce. Il codice che traduce decisioni già scritte
   (nella scheda, nel contratto di solid-space, in un ADR) e coperte da
   riferimenti forti può andare in fascia media, anche se un tempo era
   «impegnativo».
4. **I riferimenti delicati sono sempre di fascia forte**, e si dividono per
   natura: un pacchetto forte con i casi fini delle parti, uno leggero con gli
   ordinari, un file di attesa per pacchetto di lavoro servito. È il punto in
   cui i dati dicono che la fascia forte rende di più (sezione 2.5).
5. **Revisione delle interfacce prima delle parti**, estesa a tutte le schede
   nate dallo stesso pacchetto; l'ultima parte ha i casi che vedono l'insieme. I
   pacchetti in fascia media che toccano parti delicate hanno una revisione
   mirata (Copilot o Codex).
6. **Un pacchetto iniziale di interfaccia solo se toglie file comuni** a più
   consumatori o sblocca due parti in parallelo (nella 0.8: PT1).
7. **Si registra e si misura.** Ogni pull request dichiara modello e impegno; si
   misura di nuovo alla chiusura della 0.7.5, che è già quasi tutta di medi, e
   della 0.8, con gli script della sezione 7: giri, fermi, correzioni dei
   riferimenti per fascia, correzioni di codice dopo l'unione.

**Dove non regge.** Il campione non dimostra un risparmio di giri di CI
(sezione 3.1). Non accorcia una tappa i cui ritardi vengono da buchi delle
schede, casi sbagliati e solid-space (sezione 2.4). Non sposta su Gemini 3.8
Flash i riferimenti delle facciate (sezione 3.5). La qualità del codice
impegnativo in fascia media non è misurata: è un'ipotesi da provare, con il
costo in tempo stimato sotto il giorno per tappa (sezione 3.9).

**Le alternative della #442, alla luce dei dati.** *Lasciare tutto com'è*: gli
impegnativi di Claude hanno avuto pochi giri, ma la misura resta superata quasi
sempre, i riferimenti forti restano grandi e il codice resta tutto in fascia
forte. *Dividere solo gli impegnativi sul percorso critico*: nella 0.7 avrebbe
toccato solo P19, l'unico impegnativo sul percorso critico realizzato, con un
effetto di poche ore in un senso o nell'altro; è un caso particolare della
condizione 1. *Un pacchetto di interfaccia prima di ogni impegnativo*: no in
generale (sezione 3.8), sì dove toglie file comuni (condizione 6).

## 6. Testo proposto per le regole (non applicato)

Proposte per [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), sezione 1,
e per i documenti che ne seguono. Si applicano solo dopo che il responsabile le
ha lette e approvate, nella pausa tra le tappe (DECISIONI.md, 2026-11-07).

### «Pacchetti leggeri», punto «Misura»

**Oggi:**

> **Misura.** Al massimo una quindicina di file e qualche centinaio di righe
> cambiate (indicativamente fino a 600). Un pacchetto più grande si divide, di
> solito lungo i confini naturali (un'entità, un comando, una famiglia di
> domande).

**Proposto:**

> **Misura.** Al massimo una quindicina di file e circa 600 righe di codice
> cambiate; test, documenti e casi di riferimento non contano, né per i file né
> per le righe. Un pacchetto più grande si divide, di solito lungo i confini
> naturali (un'entità, un comando, una famiglia di domande, un'intestazione di
> solid-space).

### «Pacchetti leggeri», punto «Non troppo piccoli»

**Oggi:**

> **Non troppo piccoli.** Un pacchetto che tocca una sola funzione o un solo
> file si unisce a un vicino dello stesso modulo: ogni pacchetto costa comunque
> una sessione, una CI, una revisione e un merge, e spesso una pull request di
> riferimenti.

**Proposto:**

> **Non troppo piccoli.** Un pacchetto con meno di circa 150 righe di codice si
> unisce a un vicino dello stesso modulo, salvo che sia un'unità che si verifica
> da sola (un'entità, un comando, una famiglia di domande) o infrastruttura:
> ogni pacchetto costa comunque una sessione, una CI, una revisione e un merge,
> e spesso una pull request di riferimenti.

### «Pacchetti leggeri», punto nuovo «Dividere»

**Proposto (dopo «Non troppo piccoli»):**

> **Dividere.** Si preferiscono pacchetti medi, ma un pacchetto si divide solo
> per un motivo che il piano scrive in una riga: supera la misura; oppure la
> divisione toglie una parte da una catena di file comuni o da un'attesa di
> solid-space, o separa le decisioni da prendere dal codice da scrivere (con la
> variante B di «Modello e impegno», il codice va in fascia media). Due parti che toccano gli stessi file restano in catena, e
> dividerle allunga la tappa di un pacchetto. Le schede nate dallo stesso
> pacchetto si citano a vicenda in «Da leggere», passano insieme dalla
> revisione delle interfacce, e l'ultima ha i casi che vedono l'insieme. Un
> pacchetto iniziale di sola interfaccia si fa solo se toglie file comuni a più
> pacchetti o ne fa partire due in parallelo.

### «Pacchetti leggeri», punto «Riferimenti della stessa misura»

**Oggi:**

> **Riferimenti della stessa misura.** Un pacchetto di riferimenti copre un
> solo pacchetto di lavoro; oltre una ventina di casi si divide per area.

**Proposto:**

> **Riferimenti divisi per natura.** Un pacchetto di riferimenti copre di norma
> un pacchetto di lavoro; per le parti di un pacchetto diviso, i riferimenti si
> dividono per natura e non per parte: uno di fascia forte con i casi delicati
> di tutte le parti, uno leggero con quelli ordinari. Ciascuno scrive un file
> in `tests/verifica/in-attesa/` per ogni pacchetto di lavoro che serve, con la
> lettera, e ogni pacchetto di lavoro cancella solo il suo. Oltre una ventina di
> casi si divide per area.

### «Modello e impegno», ultimo paragrafo

**Oggi:**

> Di norma: facile → leggero e basso; media → medio e medio; impegnativa →
> forte e alto. Un pacchetto impegnativo che non si riesce a dividere resta
> impegnativo; uno che si divide dà spesso due pacchetti medi.

**Proposto, variante A (lettura per il tempo e la qualità):**

> Di norma: facile → leggero e basso; media → medio e medio; impegnativa →
> forte e alto. La difficoltà viene dalla natura del lavoro, non dalla misura:
> il motore dei comandi, una facciata di solid-space con casi limite, una nuova
> interfaccia di `core` e la conservazione DXF sono di fascia forte anche in un
> pacchetto piccolo, e le parti di un pacchetto diviso che le contengono
> restano impegnative. Un pacchetto impegnativo che resta tale ha il motivo in
> una riga del piano.

**Proposto, variante B (lettura per il costo dei modelli, sezione 3.9):**

> Di norma: facile → leggero e basso; media → medio e medio; impegnativa →
> forte e alto. La fascia forte va a chi prende decisioni delicate: progettare
> un'interfaccia nuova, scegliere un comportamento con casi limite che nessun
> documento fissa, scrivere i riferimenti con calcoli delicati, rivedere le
> interfacce. Un pacchetto che traduce in codice decisioni già scritte (nella
> scheda, nel contratto di solid-space o in un ADR) e coperte da riferimenti di
> fascia forte è medio anche se il codice è delicato; la sua pull request ha una
> revisione mirata. Il piano scrive in una riga perché un pacchetto resta
> impegnativo.

### Che cosa ne segue

- [tappe/0000-modello.md](../0000-modello.md), sezione 6: sotto la tabella dei
  pacchetti, una riga di motivo per ogni impegnativo e per ogni divisione; nella
  tabella, i pacchetti di riferimenti divisi per natura con la colonna
  «Riferimenti» che li cita per tutte le parti.
- [tappe/0000-pacchetto.md](../0000-pacchetto.md): nel «Da leggere», le schede
  sorelle di un pacchetto diviso; nel commento iniziale, la misura contata sul
  codice.
- [agenti/riferimenti.md](../../agenti/riferimenti.md): un pacchetto di
  riferimenti che serve più pacchetti di lavoro scrive un file per ciascuno.
  Prima, verificare l'ADR 0011, punto 7: se lega i riferimenti a un solo
  pacchetto, serve una correzione dell'ADR, che decide il responsabile.
- [GUIDA-RESPONSABILE.md](../../GUIDA-RESPONSABILE.md), B1: nel preparare il
  piano, contare i file comuni di ogni catena e cercare prima le divisioni che
  li tolgono.
- Il modello di pull request in `.github/` (lo cambia solo il responsabile):
  nella sezione sull'uso dell'IA, oltre allo strumento e al modello, l'impegno
  scelto; senza, la misura della condizione 7 della sezione 5 non si può
  fare.

## 7. Dati grezzi e comandi

### 7.1 Per pacchetto

Una riga per issue con l'etichetta della tappa. «File» e «Righe» della pull
request principale, `third_party/` escluso, test compresi; «Run», «Fallite» e
«Annullate» della stessa; «Ore PR» dall'apertura all'unione; «Ore
presa→unione» dalla prima presa in carico. «PR di correzione»: pull request
unite dopo la principale che correggono i suoi casi; «Altre PR»: tentativi non
uniti e correzioni della scheda.

| Codice | Issue | Tipo | Diff. | Autore | PR | Commit | File | Righe | Run | Fallite | Annullate | Ore PR | Ore presa→unione | PR di correzione | Altre PR |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.4-P1 | #76 | pacchetto | impegnativa | claude | #102 | 2 | 44 | 5356 | 2 | 1 | 0 | 2,0 | 3,2 | — | — |
| 0.4-R2 | #77 | riferimenti | media | gemini | #103 | 1 | 39 | 1255 | 1 | 0 | 0 | 0,7 | 1,2 | — | — |
| 0.4-P3 | #78 | pacchetto | impegnativa | claude | #109 | 2 | 13 | 1560 | 2 | 0 | 1 | 0,7 | 1,5 | — | — |
| 0.4-P4 | #79 | pacchetto | media | codex | #111 | 1 | 7 | 306 | 1 | 0 | 0 | 0,8 | 1,3 | — | — |
| 0.4-P5 | #80 | pacchetto | media | claude | #112 | 2 | 10 | 617 | 1 | 0 | 0 | 0,5 | 1,0 | — | — |
| 0.4-R6 | #81 | riferimenti | n.d. | gemini | #113 | 1 | 7 | 386 | 1 | 0 | 0 | 0,5 | 1,0 | — | — |
| 0.4-P6 | #82 | pacchetto | impegnativa | claude | #115 | 2 | 36 | 2037 | 2 | 0 | 1 | 1,0 | 1,9 | — | — |
| 0.4-R7 | #83 | riferimenti | n.d. | gemini | #118 | 1 | 17 | 619 | 1 | 0 | 0 | 0,9 | 1,6 | — | — |
| 0.4-P7 | #84 | pacchetto | media | codex | #120 | 2 | 7 | 457 | 2 | 0 | 1 | 0,4 | 0,5 | — | — |
| 0.4-R8 | #85 | riferimenti | n.d. | gemini | #119 | 1 | 17 | 973 | 1 | 0 | 0 | 0,3 | 0,8 | #123, #125 | — |
| 0.4-P8 | #86 | pacchetto | media | codex | #121 | 3 | 7 | 416 | 3 | 2 | 0 | 3,4 | 3,5 | — | — |
| 0.4-R9 | #87 | riferimenti | n.d. | gemini | #122 | 1 | 13 | 849 | 1 | 0 | 0 | 1,0 | 7,7 | #124, #127 | — |
| 0.4-P9 | #88 | pacchetto | media | codex | #126 | 18 | 7 | 352 | 8 | 4 | 3 | 23,3 | 23,3 | — | — |
| 0.4-R10 | #89 | riferimenti | n.d. | gemini | #128 | 1 | 15 | 445 | 1 | 0 | 0 | 0,2 | 0,8 | #129, #132 | — |
| 0.4-P10 | #90 | pacchetto | media | codex | #131 | 4 | 7 | 312 | 4 | 1 | 1 | 2,2 | 3,0 | — | — |
| 0.4-R11 | #91 | riferimenti | n.d. | gemini | #116 | 2 | 2 | 78 | 2 | 0 | 0 | 1,0 | 2,4 | — | — |
| 0.4-P11 | #92 | pacchetto | media | codex | #133 | 2 | 18 | 589 | 2 | 1 | 0 | 9,3 | 9,5 | — | — |
| 0.4-P12 | #93 | pacchetto | media | codex | #135 | 3 | 7 | 307 | 3 | 0 | 2 | 0,4 | 0,6 | — | — |
| 0.4-R13 | #94 | riferimenti | n.d. | gemini | #130 | 1 | 4 | 212 | 2 | 0 | 0 | 1,8 | 2,8 | #147 | — |
| 0.4-P13 | #95 | pacchetto | impegnativa | claude | #136 | 1 | 28 | 1372 | 1 | 0 | 0 | 0,4 | 1,9 | — | — |
| 0.4-R14 | #96 | riferimenti | n.d. | gemini | #134 | 1 | 19 | 886 | 1 | 0 | 0 | 8,2 | 9,0 | #139 | — |
| 0.4-P14 | #97 | pacchetto | media | codex | #137 | 4 | 10 | 523 | 4 | 2 | 1 | 3,0 | 13,7 | — | — |
| 0.4-R15 | #98 | riferimenti | n.d. | gemini | #138 | 1 | 13 | 391 | 1 | 0 | 0 | 1,5 | 4,5 | #145 | — |
| 0.4-P15 | #99 | pacchetto | media | claude | #144 | 3 | 5 | 141 | 3 | 2 | 0 | 7,7 | 10,4 | — | — |
| 0.4-P16 | #100 | pacchetto | facile | codex | #146 | 1 | 2 | 350 | 1 | 0 | 0 | 0,0 | 0,1 | — | — |
| 0.5-P1 | #153 | pacchetto | impegnativa | claude | #180 | 1 | 10 | 671 | 1 | 0 | 0 | 0,7 | 1,2 | — | — |
| 0.5-R2 | #154 | riferimenti | n.d. | claude | #181 | 1 | 8 | 432 | 1 | 0 | 0 | 1,1 | 1,4 | — | — |
| 0.5-P2 | #155 | pacchetto | impegnativa | codex | #182 | 1 | 10 | 445 | 1 | 0 | 0 | 1,4 | 1,6 | — | — |
| 0.5-P3 | #156 | pacchetto | media | codex | #179 | 1 | 1 | 252 | 1 | 0 | 0 | 0,1 | 0,2 | — | — |
| 0.5-P4 | #157 | pacchetto | impegnativa | claude | #184 | 2 | 14 | 2173 | 1 | 0 | 0 | 0,8 | 1,6 | — | — |
| 0.5-P5 | #158 | pacchetto | media | codex | #183 | 1 | 3 | 256 | 1 | 0 | 0 | 1,0 | 1,1 | — | — |
| 0.5-P6 | #159 | pacchetto | media | codex | #185 | 3 | 8 | 215 | 3 | 2 | 0 | 8,7 | 8,8 | — | — |
| 0.5-P7 | #160 | pacchetto | media | codex | #186 | 1 | 12 | 404 | 1 | 0 | 0 | 8,7 | 8,8 | — | — |
| 0.5-P8 | #161 | pacchetto | impegnativa | claude | #187 | 3 | 21 | 1568 | 1 | 0 | 0 | 7,9 | 8,8 | — | — |
| 0.5-P9 | #162 | pacchetto | media | codex | #188 | 2 | 5 | 585 | 1 | 0 | 0 | 1,1 | 1,6 | — | — |
| 0.5-R10 | #163 | riferimenti | n.d. | gemini | #193 | 2 | 11 | 122 | 2 | 1 | 0 | 1,0 | 1,4 | #202 | — |
| 0.5-P10 | #164 | pacchetto | impegnativa | codex | #199 | 5 | 14 | 1427 | 4 | 2 | 1 | 2,6 | 5,8 | — | #200 (scheda) |
| 0.5-R11 | #165 | riferimenti | n.d. | codex | #192 | 6 | 18 | 1461 | 4 | 1 | 2 | 1,6 | 1,7 | — | — |
| 0.5-R12 | #166 | riferimenti | n.d. | gemini | #194 | 1 | 7 | 325 | 1 | 0 | 0 | 0,7 | 0,8 | #204, #205 | — |
| 0.5-P12 | #167 | pacchetto | media | codex | #203 | 1 | 13 | 692 | 3 | 1 | 1 | 3,1 | 3,3 | — | — |
| 0.5-P11 | #168 | pacchetto | impegnativa | claude | #207 | 2 | 23 | 1490 | 1 | 0 | 0 | 0,7 | 1,8 | — | — |
| 0.5-P13 | #169 | pacchetto | impegnativa | claude | #189 | 2 | 23 | 1288 | 1 | 0 | 0 | 0,9 | 1,7 | — | — |
| 0.5-R14 | #170 | riferimenti | n.d. | gemini | #196 | 2 | 9 | 18 | 2 | 0 | 0 | 0,4 | 0,4 | — | — |
| 0.5-P14 | #171 | pacchetto | media | codex | #206 | 3 | 14 | 1210 | 3 | 1 | 1 | 1,4 | 1,8 | — | — |
| 0.5-R15 | #172 | riferimenti | n.d. | gemini | #197 | 2 | 14 | 757 | 1 | 0 | 0 | 1,9 | 2,0 | #208 | — |
| 0.5-P15 | #173 | pacchetto | media | codex | #211 | 4 | 17 | 608 | 3 | 2 | 0 | 0,7 | 3,5 | — | — |
| 0.5-R16 | #174 | riferimenti | n.d. | gemini | #198 | 3 | 15 | 579 | 2 | 1 | 0 | 1,2 | 1,3 | #214, #216 | — |
| 0.5-P16 | #175 | pacchetto | media | codex | #215 | 2 | 7 | 402 | 3 | 1 | 1 | 1,8 | 2,9 | — | — |
| 0.5-R17 | #176 | riferimenti | n.d. | gemini | #201 | 1 | 5 | 206 | 1 | 0 | 0 | 0,7 | 0,7 | — | — |
| 0.5-P17 | #177 | pacchetto | media | codex | #218 | 2 | 11 | 807 | 2 | 1 | 0 | 1,4 | 2,8 | — | — |
| 0.5-P18 | #178 | pacchetto | facile | gemini | #222 | 1 | 2 | 210 | 1 | 0 | 0 | 0,0 | 0,4 | — | — |
| 0.5-P13b | #190 | pacchetto | facile | claude | #191 | 1 | 5 | 47 | 1 | 0 | 0 | 0,2 | 0,3 | — | — |
| 0.5 (#209) | #209 | riferimenti | n.d. | claude | #210 | 1 | 1 | 176 | 1 | 0 | 0 | 0,5 | 0,5 | — | — |
| 0.5 (#212) | #212 | riferimenti | n.d. | claude | #213 | 1 | 1 | 142 | 1 | 0 | 0 | 0,5 | 0,5 | — | — |
| 0.6-P1 | #227 | pacchetto | impegnativa | claude | #265 | 3 | 16 | 1155 | 3 | 1 | 1 | 1,0 | 1,4 | — | — |
| 0.6-P2 | #228 | pacchetto | media | codex | #264 | 4 | 11 | 923 | 4 | 1 | 2 | 1,3 | 1,4 | — | — |
| 0.6-P3 | #229 | pacchetto | media | codex | #266 | 1 | 3 | 53 | 3 | 1 | 1 | 3,9 | 4,0 | — | — |
| 0.6-P4 | #230 | pacchetto | impegnativa | claude | #275 | 2 | 21 | 1048 | 2 | 0 | 1 | 1,0 | 5,3 | — | — |
| 0.6-P5 | #231 | pacchetto | impegnativa | claude | #277 | 1 | 15 | 1768 | 1 | 0 | 0 | 5,8 | 6,2 | — | — |
| 0.6-P6 | #232 | pacchetto | media | codex | #276 | 3 | 10 | 1035 | 3 | 2 | 0 | 6,1 | 6,2 | — | — |
| 0.6-P7 | #233 | pacchetto | impegnativa | claude | #278 | 3 | 15 | 1018 | 3 | 2 | 0 | 3,8 | 3,9 | — | — |
| 0.6-P8 | #234 | pacchetto | media | codex | #281 | 1 | 3 | 282 | 1 | 0 | 0 | 0,9 | 5,7 | — | — |
| 0.6-R9 | #235 | riferimenti | n.d. | gemini | #283 | 2 | 4 | 150 | 2 | 0 | 0 | 1,4 | 1,6 | #287 | — |
| 0.6-P9 | #236 | pacchetto | media | codex | #286 | 4 | 14 | 318 | 5 | 3 | 1 | 2,5 | 2,6 | — | — |
| 0.6-R10 | #237 | riferimenti | n.d. | gemini | #284 | 1 | 9 | 567 | 1 | 0 | 0 | 0,7 | 0,8 | #295 | — |
| 0.6-P10 | #238 | pacchetto | impegnativa | claude | #294 | 3 | 20 | 2958 | 3 | 1 | 1 | 1,1 | 5,1 | — | — |
| 0.6-R11 | #239 | riferimenti | n.d. | claude | #282 | 1 | 6 | 298 | 1 | 0 | 0 | 0,6 | 0,7 | — | — |
| 0.6-P11 | #240 | pacchetto | impegnativa | codex | #299 | 5 | 10 | 668 | 5 | 0 | 4 | 0,6 | 0,8 | — | — |
| 0.6-R12 | #241 | riferimenti | n.d. | gemini | #285 | 2 | 9 | 465 | 1 | 0 | 0 | 1,2 | 1,3 | #296, #301 | — |
| 0.6-P12 | #242 | pacchetto | media | codex | #298 | 4 | 12 | 998 | 4 | 1 | 2 | 1,1 | 1,5 | — | — |
| 0.6-R13 | #243 | riferimenti | n.d. | gemini | #288 | 1 | 9 | 549 | 1 | 0 | 0 | 1,5 | 1,6 | #302, #303, #305 | — |
| 0.6-P13 | #244 | pacchetto | media | codex | #304 | 1 | 11 | 682 | 4 | 1 | 2 | 1,2 | 9,9 | — | — |
| 0.6-R14 | #245 | riferimenti | n.d. | gemini | #289 | 2 | 9 | 792 | 2 | 1 | 0 | 1,4 | 1,5 | #308 | — |
| 0.6-P14 | #246 | pacchetto | media | claude | #309 | 3 | 12 | 1145 | 3 | 1 | 1 | 0,6 | 1,4 | — | — |
| 0.6-P15 | #247 | pacchetto | media | claude | #306 | 2 | 8 | 950 | 1 | 0 | 0 | 0,2 | 0,5 | — | — |
| 0.6-R16 | #248 | riferimenti | n.d. | gemini | #267 | 2 | 11 | 468 | 2 | 1 | 0 | 1,0 | 1,6 | — | — |
| 0.6-P16 | #249 | pacchetto | impegnativa | codex | #297 | 8 | 12 | 559 | 7 | 2 | 4 | 1,0 | 1,2 | — | — |
| 0.6-P17 | #250 | pacchetto | media | codex | #311 | 5 | 16 | 682 | 5 | 1 | 3 | 0,9 | 1,0 | — | — |
| 0.6-P18 | #251 | pacchetto | media | claude | #313 | 1 | 18 | 550 | 1 | 0 | 0 | 0,2 | 0,3 | — | — |
| 0.6-P19 | #252 | pacchetto | media | claude | #314 | 1 | 28 | 446 | 1 | 0 | 0 | 0,3 | 0,4 | — | — |
| 0.6-P20 | #253 | pacchetto | media | codex | #316 | 4 | 10 | 678 | 4 | 0 | 3 | 0,5 | 0,6 | — | — |
| 0.6-P21 | #254 | pacchetto | media | codex | #318 | 4 | 8 | 813 | 4 | 0 | 3 | 0,6 | 0,8 | — | — |
| 0.6-P22 | #255 | pacchetto | media | claude | #319 | 7 | 16 | 2057 | 7 | 5 | 1 | 1,9 | 3,0 | — | — |
| 0.6-R23 | #256 | riferimenti | n.d. | gemini | #290 | 2 | 3 | 126 | 2 | 0 | 0 | 1,4 | 1,5 | — | — |
| 0.6-P23 | #257 | pacchetto | media | codex | #312 | 5 | 12 | 366 | 5 | 1 | 3 | 0,7 | 0,8 | — | — |
| 0.6-P24 | #258 | pacchetto | media | claude | #320 | 6 | 14 | 2202 | 6 | 1 | 4 | 0,9 | 1,1 | — | — |
| 0.6-P25 | #259 | pacchetto | impegnativa | claude | #307 | 1 | 9 | 1308 | 2 | 0 | 0 | 1,6 | 1,9 | — | — |
| 0.6-P26 | #260 | pacchetto | impegnativa | claude | #315 | 1 | 12 | 1690 | 1 | 0 | 0 | 0,2 | 0,8 | — | — |
| 0.6-P27 | #261 | pacchetto | facile | gemini | #321 | 1 | 4 | 586 | 1 | 0 | 0 | 0,5 | 0,6 | — | — |
| 0.6-P28 | #262 | pacchetto | facile | claude | #317 | 1 | 3 | 478 | 1 | 0 | 0 | 0,2 | 0,3 | — | — |
| 0.6-P3-Qt | #268 | pacchetto | media | claude | #269 | 2 | 11 | 135 | 2 | 1 | 0 | 1,2 | 1,2 | — | — |
| 0.6-R4 | #272 | riferimenti | media | gemini | #274 | 1 | 76 | 813 | 1 | 0 | 0 | 0,7 | 0,9 | — | — |
| 0.7-P0 | #333 | pacchetto | media | claude | #381 | 1 | 8 | 159 | 2 | 1 | 0 | 3,5 | 3,5 | — | — |
| 0.7-P1 | #334 | pacchetto | media | codex | #380 | 3 | 6 | 328 | 3 | 1 | 1 | 15,4 | 15,5 | — | — |
| 0.7-R2 | #335 | riferimenti | media | gemini | #383 | 1 | 13 | 318 | 1 | 0 | 0 | 2,0 | 2,1 | — | — |
| 0.7-P2 | #336 | pacchetto | impegnativa | claude | #405 | 1 | 10 | 1052 | 1 | 0 | 0 | 0,5 | 1,1 | — | — |
| 0.7-R3 | #337 | riferimenti | facile | gemini | #385 | 2 | 5 | 153 | 2 | 1 | 0 | 10,9 | 11,0 | — | — |
| 0.7-P3 | #338 | pacchetto | media | codex | #409 | 3 | 14 | 188 | 3 | 0 | 2 | 0,9 | 1,0 | — | — |
| 0.7-R4 | #339 | riferimenti | impegnativa | claude | #404 | 2 | 9 | 466 | 2 | 1 | 0 | 2,8 | 3,0 | — | — |
| 0.7-P4 | #340 | pacchetto | media | codex | #408 | 6 | 9 | 663 | 5 | 2 | 2 | 4,6 | 4,7 | — | — |
| 0.7-R5 | #341 | riferimenti | media | gemini | #386 | 2 | 13 | 270 | 2 | 0 | 0 | 1,9 | 1,9 | #416, #417 | — |
| 0.7-P5 | #342 | pacchetto | media | claude | #419 | 5 | 14 | 1475 | 3 | 1 | 1 | 0,8 | 10,5 | — | — |
| 0.7-R6 | #343 | riferimenti | media | gemini | #388 | 2 | 9 | 178 | 2 | 0 | 0 | 2,2 | 2,2 | #421 | — |
| 0.7-P6 | #344 | pacchetto | media | claude | #424 | 2 | 13 | 1955 | 1 | 0 | 0 | 0,5 | 4,5 | — | — |
| 0.7-R7 | #345 | riferimenti | media | gemini | #389 | 3 | 6 | 179 | 3 | 1 | 0 | 10,5 | 10,6 | #414 | — |
| 0.7-P7 | #346 | pacchetto | media | claude | #418 | 3 | 22 | 1022 | 3 | 1 | 1 | 2,7 | 12,7 | — | — |
| 0.7-R8 | #347 | riferimenti | facile | gemini | #390 | 3 | 7 | 236 | 3 | 2 | 0 | 10,2 | 10,2 | #439, #440 | — |
| 0.7-P8 | #348 | pacchetto | media | claude | #438 | 1 | 19 | 554 | 2 | 1 | 0 | 1,7 | 2,1 | — | — |
| 0.7-R9 | #349 | riferimenti | impegnativa | gemini-argon | #402 | 2 | 4 | 249 | 2 | 1 | 0 | 4,5 | 4,9 | — | — |
| 0.7-P9 | #350 | pacchetto | media | codex | #420 | 4 | 8 | 496 | 4 | 1 | 2 | 0,8 | 0,9 | — | — |
| 0.7-R10 | #351 | riferimenti | media | gemini | #391 | 3 | 9 | 464 | 3 | 0 | 0 | 1,5 | 1,6 | #444, #445 | — |
| 0.7-P10 | #352 | pacchetto | impegnativa | claude | #446 | 1 | 9 | 727 | 1 | 0 | 0 | 0,3 | 1,8 | — | — |
| 0.7-P11 | #353 | pacchetto | media | codex | #448 | 5 | 16 | 428 | 3 | 0 | 3 | 0,3 | 0,4 | — | — |
| 0.7-P12 | #354 | pacchetto | facile | gemini | #462 | 2 | 3 | 105 | 2 | 0 | 1 | 3,1 | 3,2 | — | — |
| 0.7-P13 | #355 | pacchetto | facile | gemini | #488 | 2 | 2 | 668 | 2 | 0 | 0 | 3,8 | 4,1 | — | — |
| 0.7-R14 | #356 | riferimenti | media | gemini | #454 | 1 | 5 | 268 | 1 | 0 | 0 | 0,4 | 0,6 | #459 | — |
| 0.7-P14 | #357 | pacchetto | impegnativa | claude | #460 | 2 | 14 | 2184 | 2 | 0 | 0 | 1,4 | 3,2 | — | — |
| 0.7-R15 | #358 | riferimenti | media | gemini | #447 | 1 | 5 | 234 | 1 | 0 | 0 | 3,4 | 3,6 | #474 | — |
| 0.7-P15 | #359 | pacchetto | media | codex | #487 | 16 | 11 | 1363 | 15 | 1 | 10 | 7,2 | 52,1 | — | — |
| 0.7-R16 | #360 | riferimenti | impegnativa | gemini-argon | #403 | 1 | 21 | 6889 | 1 | 0 | 0 | 0,6 | 1,8 | — | — |
| 0.7-P16 | #361 | pacchetto | impegnativa | claude | #491 | 2 | 11 | 1458 | 2 | 0 | 0 | 7,7 | 8,1 | — | — |
| 0.7-R17 | #362 | riferimenti | media | gemini | #455 | 1 | 4 | 125 | 1 | 0 | 0 | 0,3 | 0,4 | #498 | — |
| 0.7-P17 | #363 | pacchetto | impegnativa | claude | #502 | 3 | 8 | 650 | 2 | 0 | 0 | 0,8 | 39,6 | — | — |
| 0.7-R18 | #364 | riferimenti | media | gemini | #410 | 1 | 8 | 1495 | 2 | 1 | 0 | 1,2 | 1,4 | #503, #504, #506 | — |
| 0.7-P18 | #365 | pacchetto | media | codex | #505 | 7 | 10 | 554 | 6 | 1 | 4 | 2,2 | 47,4 | — | — |
| 0.7-R19 | #366 | riferimenti | media | gemini | #456 | 1 | 5 | 157 | 1 | 0 | 0 | 1,0 | 1,1 | #510 | — |
| 0.7-P19 | #367 | pacchetto | impegnativa | claude | #511 | 6 | 10 | 1701 | 2 | 0 | 0 | 1,0 | 51,5 | — | — |
| 0.7-R20 | #368 | riferimenti | media | gemini | #411 | 2 | 12 | 2806 | 2 | 0 | 0 | 3,5 | 3,7 | #507, #518, #520 | — |
| 0.7-P20 | #369 | pacchetto | media | codex | #519 | 5 | 13 | 729 | 4 | 1 | 2 | 8,5 | 9,5 | — | #512 (non unita), #514 (non unita), #515 (non unita), #517 (non unita) |
| 0.7-R21 | #370 | riferimenti | media | gemini | #412 | 1 | 12 | 2330 | 1 | 0 | 0 | 3,1 | 3,2 | #509, #523, #524 | — |
| 0.7-P21 | #371 | pacchetto | media | codex | #525 | 6 | 18 | 1314 | 5 | 0 | 3 | 4,8 | 6,8 | — | — |
| 0.7-R22 | #372 | riferimenti | media | gemini | #457 | 1 | 5 | 263 | 1 | 0 | 0 | 0,5 | 0,7 | #463 | — |
| 0.7-P22 | #373 | pacchetto | media | codex | #499 | 3 | 9 | 673 | 3 | 0 | 2 | 3,0 | 76,3 | — | — |
| 0.7-R23 | #374 | riferimenti | media | gemini | #413 | 2 | 10 | 2256 | 2 | 0 | 0 | 8,7 | 8,8 | #527 | — |
| 0.7-P23 | #375 | pacchetto | media | codex | #529 | 3 | 9 | 482 | 3 | 1 | 1 | 0,6 | 2,3 | — | #528 (scheda) |
| 0.7-P24 | #376 | pacchetto | facile | claude | #530 | 5 | 8 | 816 | 5 | 0 | 0 | 3,9 | 4,1 | — | — |
| 0.7 (#422) | #422 | altro | facile | claude | #423 | 2 | 1 | 55 | 2 | 0 | 1 | 0,3 | 0,4 | — | — |
| 0.7 (#433) | #433 | riferimenti | facile | gemini | #434 | 1 | 5 | 15 | 1 | 0 | 0 | 0,3 | 0,5 | — | — |
| 0.7-P25 | #464 | pacchetto | impegnativa | codex | #466 | 1 | 14 | 719 | 1 | 0 | 0 | 4,9 | 8,6 | — | — |
| 0.7 (#468) | #468 | altro | n.d. | claude | #469 | 1 | 10 | 481 | 1 | 0 | 0 | 0,6 | 0,7 | — | — |
| 0.7-R17b | #481 | riferimenti | facile | gemini | #490 | 2 | 2 | 5 | 2 | 0 | 0 | 3,4 | 3,6 | — | — |
| 0.7-R22b | #493 | riferimenti | facile | gemini | #495 | 1 | 2 | 132 | 1 | 0 | 0 | 0,8 | 1,0 | — | — |

**Abbinamenti fatti a mano**, letti dal nome del ramo e dal titolo, perché la
pull request non porta né `Closes` né il codice del pacchetto: #145 → 0.4-R15;
#204, #205 → 0.5-R12; #208 → 0.5-R15; #214, #216 → 0.5-R16; #444, #445 →
0.7-R10; #459 → 0.7-R14; #463 → 0.7-R22; #474 → 0.7-R15; #507 → 0.7-R20 (corregge
anche R21); #523, #524 → 0.7-R21; #527 → 0.7-R23. Correzioni di scheda: #200 →
0.5-P10, #528 → 0.7-P23. Esclusa #273 (numero di una issue nel piano).

### 7.2 Comandi

Il proxy delle sessioni cloud rifiuta i percorsi numerici che `gh api
--paginate` segue: le pagine si chiedono una per una con `&page=N`.

```sh
# Issue delle tappe (una pagina basta: meno di 100 per tappa)
for t in 0.4 0.5 0.6 0.7; do
  gh api "repos/Matleyx78/tecnigrafo/issues?labels=tappa-$t&state=all&per_page=100" > issues-$t.json
done
# Pull request, run di ci.yml (evento pull_request), dettagli e file, commenti
gh api "repos/Matleyx78/tecnigrafo/pulls?state=all&per_page=100&page=N"
gh api "repos/Matleyx78/tecnigrafo/actions/workflows/ci.yml/runs?event=pull_request&per_page=100&page=N"
gh api "repos/Matleyx78/tecnigrafo/pulls/<numero>"
gh api "repos/Matleyx78/tecnigrafo/pulls/<numero>/files?per_page=100&page=N"
gh api "repos/Matleyx78/tecnigrafo/issues/<numero>/comments?per_page=100&page=N"
```

Gli script, nell'ordine in cui si eseguono. La cartella di lavoro contiene una sottocartella `dati/` con i file `issues-<tappa>.json` del primo comando.

<details>
<summary><code>pulls.py</code>: Tutte le pull request, pagina per pagina: `python3 pulls.py <cartella>`</summary>

```python
import json, subprocess, sys
S = sys.argv[1]
out = []
page = 1
while True:
    r = subprocess.run(["gh", "api", f"repos/Matleyx78/tecnigrafo/pulls?state=all&per_page=100&page={page}"],
                       capture_output=True, text=True, check=True)
    d = json.loads(r.stdout)
    if not d:
        break
    for o in d:
        out.append({k: o[k] for k in ['number', 'title', 'state', 'created_at', 'merged_at', 'closed_at', 'body']}
                   | {'head': o['head']['ref'], 'head_sha': o['head']['sha'],
                      'labels': [l['name'] for l in o['labels']]})
    page += 1
json.dump(out, open(f"{S}/dati/pulls.json", "w"))
print(len(out), sum(1 for p in out if p['merged_at']))
```

</details>

<details>
<summary><code>runs.py</code>: Le run di `ci.yml` dell'evento `pull_request`: `python3 runs.py <cartella>`</summary>

```python
import json, subprocess, sys
S = sys.argv[1]
out = []
page = 1
while True:
    r = subprocess.run(["gh", "api", f"repos/Matleyx78/tecnigrafo/actions/workflows/ci.yml/runs?event=pull_request&per_page=100&page={page}"],
                       capture_output=True, text=True, check=True)
    d = json.loads(r.stdout)['workflow_runs']
    if not d:
        break
    for o in d:
        out.append({k: o[k] for k in ['id', 'head_branch', 'head_sha', 'status', 'conclusion', 'created_at', 'updated_at', 'run_started_at', 'run_attempt', 'run_number']}
                   | {'prs': [p['number'] for p in o['pull_requests']]})
    page += 1
json.dump(out, open(f"{S}/dati/runs.json", "w"))
print(len(out), min(x['created_at'] for x in out))
```

</details>

<details>
<summary><code>raccolta.py</code>: Pull request principale e correzioni di ogni issue, con commit, file, righe e run: `python3 raccolta.py <cartella>`</summary>

```python
# Builds dati/pacchetti.json: one row per stage issue with its main PR and correction PRs.
import json, re, subprocess, sys
from datetime import datetime
S = sys.argv[1]
pulls = {p['number']: p for p in json.load(open(f"{S}/dati/pulls.json"))}
runs = json.load(open(f"{S}/dati/runs.json"))
code_re = re.compile(r'\[(0\.[4-7])(?:-([PR])(\d+[a-z]?(?:-Qt)?))?\]', re.I)
def ts(s): return datetime.fromisoformat(s.replace('Z', '+00:00'))

issues = []
for t in ['0.4', '0.5', '0.6', '0.7']:
    for x in json.load(open(f"{S}/dati/issues-{t}.json")):
        if 'pull_request' in x: continue
        labs = [l['name'] for l in x['labels']]
        m = code_re.search(x['title'])
        code = f"{m.group(1)}-{m.group(2).upper()}{m.group(3)}" if m and m.group(2) else f"{t}-#{x['number']}"
        kind = 'pacchetto' if 'pacchetto' in labs else 'riferimenti' if 'riferimenti' in labs else 'altro'
        diff = next((l for l in labs if l in ('facile', 'media', 'impegnativa')), None)
        issues.append(dict(issue=x['number'], tappa=t, code=code, title=x['title'], kind=kind, diff=diff,
                           agenti=[l[7:] for l in labs if l.startswith('agente:')]))
by_num = {i['issue']: i for i in issues}
by_code = {i['code'].upper(): i for i in issues}

# Main PR: earliest merged PR that closes the issue; otherwise earliest merged PR with the code in its title.
closes = {}
for p in pulls.values():
    for n in re.findall(r'(?i)\b(?:closes|fixes|resolves)\s+#(\d+)', p['body'] or ''):
        closes.setdefault(int(n), []).append(p['number'])
title_code = {}
for p in pulls.values():
    for m in re.finditer(r'(0\.[4-7])-([PR])(\d+[a-z]?)', p['title'], re.I):
        title_code.setdefault(f"{m.group(1)}-{m.group(2).upper()}{m.group(3)}".upper(), set()).add(p['number'])

# Manual assignments, read from branch names and titles (listed in the document).
MANUAL_FIX = {  # PR -> code of the reference or work package it corrects
    145: '0.4-R15', 204: '0.5-R12', 205: '0.5-R12', 208: '0.5-R15', 214: '0.5-R16', 216: '0.5-R16',
    444: '0.7-R10', 445: '0.7-R10', 459: '0.7-R14', 463: '0.7-R22', 474: '0.7-R15',
    523: '0.7-R21', 524: '0.7-R21', 527: '0.7-R23', 507: '0.7-R20',
}
SCHEDA_FIX = {200: '0.5-P10', 528: '0.7-P23'}  # corrections of the card, not of references
EXCLUDE = {273}  # docs: issue number of 0.6-R4 in the plan
main_of = {}
for i in issues:
    cand = set(closes.get(i['issue'], [])) | title_code.get(i['code'].upper(), set())
    c = [n for n in cand if pulls[n]['merged_at'] and n not in MANUAL_FIX and n not in SCHEDA_FIX and n not in EXCLUDE]
    if c:
        main_of[i['issue']] = min(c, key=lambda n: pulls[n]['created_at'])
used = set(main_of.values())
fix_of = {i['issue']: [] for i in issues}
attempts_of = {i['issue']: [] for i in issues}
scheda_of = {i['issue']: [] for i in issues}
for n, p in pulls.items():
    if n in used or n in EXCLUDE: continue
    target = None
    if n in MANUAL_FIX: target = by_code[MANUAL_FIX[n].upper()]
    elif n in SCHEDA_FIX:
        scheda_of[by_code[SCHEDA_FIX[n].upper()]['issue']].append(n); continue
    else:
        cs = [c for c, ns in title_code.items() if n in ns]
        if cs and cs[0] in by_code: target = by_code[cs[0]]
        else:
            ci = [k for k in closes if n in closes[k] and k in by_num]
            if ci: target = by_num[ci[0]]
    if target is None: continue
    (fix_of if p['merged_at'] else attempts_of)[target['issue']].append(n)

detail_cache = f"{S}/dati/pull-details.json"
try: details = {int(k): v for k, v in json.load(open(detail_cache)).items()}
except FileNotFoundError: details = {}
def det(n):
    if n not in details:
        r = subprocess.run(["gh", "api", f"repos/Matleyx78/tecnigrafo/pulls/{n}"], capture_output=True, text=True, check=True)
        o = json.loads(r.stdout)
        details[n] = {k: o[k] for k in ['commits', 'additions', 'deletions', 'changed_files']}
    if 'files' not in details[n]:
        fs, page = [], 1
        while True:
            r = subprocess.run(["gh", "api", f"repos/Matleyx78/tecnigrafo/pulls/{n}/files?per_page=100&page={page}"], capture_output=True, text=True, check=True)
            d = json.loads(r.stdout)
            if not d: break
            fs += [[f['filename'], f['additions'] + f['deletions']] for f in d]
            page += 1
        details[n]['files'] = fs
    return details[n]

def ci(p):
    start, end = ts(p['created_at']), ts(p['merged_at'] or p['closed_at'])
    rs = [r for r in runs if r['head_branch'] == p['head'] and start.timestamp() - 120 <= ts(r['created_at']).timestamp() <= end.timestamp() + 60]
    rs.sort(key=lambda r: r['created_at'])
    concl = [r['conclusion'] for r in rs]
    last_ok = max((k for k, c in enumerate(concl) if c == 'success'), default=-1)
    return dict(runs=len(rs), failure=concl.count('failure'), cancelled=concl.count('cancelled'),
                success=concl.count('success'), fail_before_green=sum(1 for c in concl[:last_ok] if c == 'failure'))

def prrow(n):
    p = pulls[n]; d = det(n); c = ci(p)
    hours = (ts(p['merged_at'] or p['closed_at']) - ts(p['created_at'])).total_seconds() / 3600
    return dict(pr=n, head=p['head'], created=p['created_at'], merged=p['merged_at'], hours=round(hours, 2),
                commits=d['commits'], files=d['changed_files'], lines=d['additions'] + d['deletions'],
                files_own=sum(1 for f, l in d['files'] if not f.startswith('third_party/')),
                lines_own=sum(l for f, l in d['files'] if not f.startswith('third_party/')),
                lines_tests=sum(l for f, l in d['files'] if f.startswith('tests/')),
                lines_docs=sum(l for f, l in d['files'] if f.startswith('docs/') or f.endswith('.md')),
                additions=d['additions'], deletions=d['deletions'], **c)

rows = []
for i in sorted(issues, key=lambda i: i['issue']):
    r = dict(i)
    r['main'] = prrow(main_of[i['issue']]) if i['issue'] in main_of else None
    r['fixes'] = [prrow(n) for n in sorted(fix_of[i['issue']])]
    r['attempts'] = [prrow(n) for n in sorted(attempts_of[i['issue']])]
    r['scheda'] = [prrow(n) for n in sorted(scheda_of[i['issue']])]
    rows.append(r)
json.dump(details, open(detail_cache, 'w'))
json.dump(rows, open(f"{S}/dati/pacchetti.json", 'w'), indent=1)
print(len(rows), sum(1 for r in rows if r['main'] is None), [r['code'] for r in rows if r['main'] is None])
```

</details>

<details>
<summary><code>commenti.py</code>: I commenti delle issue, per le prese in carico: `python3 commenti.py <cartella>`</summary>

```python
import json, subprocess, sys
S = sys.argv[1]
rows = json.load(open(f"{S}/dati/pacchetti.json"))
out = {}
for r in rows:
    cs, page = [], 1
    while True:
        p = subprocess.run(["gh", "api", f"repos/Matleyx78/tecnigrafo/issues/{r['issue']}/comments?per_page=100&page={page}"], capture_output=True, text=True, check=True)
        d = json.loads(p.stdout)
        if not d: break
        cs += [[c['created_at'], c['body'][:400]] for c in d]
        page += 1
    out[r['issue']] = cs
json.dump(out, open(f"{S}/dati/commenti.json", "w"))
print(len(out), sum(len(v) for v in out.values()))
```

</details>

<details>
<summary><code>analisi.py</code>: Le tabelle per tappa (sezione 2.1) e la durata delle run verdi: `python3 analisi.py <cartella>`</summary>

```python
import json, sys, statistics as st
from datetime import datetime
S = sys.argv[1]
rows = json.load(open(f"{S}/dati/pacchetti.json"))
com = {int(k): v for k, v in json.load(open(f"{S}/dati/commenti.json")).items()}
runs = json.load(open(f"{S}/dati/runs.json"))
def ts(s): return datetime.fromisoformat(s.replace('Z', '+00:00'))
for r in rows:
    pc = sorted(c[0] for c in com[r['issue']] if 'resa in carico' in c[1])
    m = r['main']
    r['start'] = pc[0] if pc else None
    r['lead'] = round((ts(m['merged']) - ts(pc[0])).total_seconds() / 3600, 2) if pc else None
    r['nfix'] = len(r['fixes']); r['natt'] = len(r['attempts']); r['nsch'] = len(r['scheda'])
    r['allruns'] = m['runs'] + sum(f['runs'] for f in r['fixes'] + r['attempts'] + r['scheda'])
    r['allfail'] = m['failure'] + sum(f['failure'] for f in r['fixes'] + r['attempts'] + r['scheda'])
json.dump(rows, open(f"{S}/dati/pacchetti-analisi.json", 'w'), indent=1)

def med(xs): return st.median(xs) if xs else None
def fmt(x, d=1):
    if x is None: return '—'
    return f"{x:.{d}f}".replace('.', ',') if isinstance(x, float) and x != int(x) else f"{int(x)}"
def group(sel):
    g = {}
    for r in rows:
        if not sel(r): continue
        if r['kind'] == 'altro': continue
        k = (r['kind'], r['diff'] or 'n.d.')
        g.setdefault(k, []).append(r)
    return g
order = [('pacchetto', 'facile'), ('pacchetto', 'media'), ('pacchetto', 'impegnativa'),
         ('riferimenti', 'facile'), ('riferimenti', 'media'), ('riferimenti', 'impegnativa'), ('riferimenti', 'n.d.')]
hdr = ("| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | PR con ≥1 giro | "
       "Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | PR di correzione dei casi (tot.) |")
sep = "|---|---|---|---|---|---|---|---|---|---|---|---|"
def table(sel):
    g = group(sel); out = [hdr, sep]
    for k in order:
        if k not in g: continue
        rs = g[k]; ms = [r['main'] for r in rs]
        out.append("| " + " | ".join([
            k[0], k[1], str(len(rs)),
            fmt(med([m['commits'] for m in ms])),
            f"{fmt(med([m['runs'] for m in ms]))}; {sum(m['runs'] for m in ms)}",
            str(sum(m['failure'] for m in ms)),
            str(sum(1 for m in ms if m['fail_before_green'] > 0)),
            fmt(med([m['hours'] for m in ms])),
            fmt(med([r['lead'] for r in rs if r['lead'] is not None])),
            fmt(med([m['files_own'] for m in ms])),
            fmt(med([m['lines_own'] for m in ms]), 0),
            str(sum(r['nfix'] for r in rs)) if k[0] == 'riferimenti' else '—',
        ]) + " |")
    return "\n".join(out)
res = []
for t in ['0.4', '0.5', '0.6', '0.7']:
    res.append(f"### Tappa {t}\n\n" + table(lambda r, t=t: r['tappa'] == t))
res.append("### Insieme 0.4–0.7\n\n" + table(lambda r: True))
open(f"{S}/dati/tabelle.md", 'w').write("\n\n".join(res) + "\n")
print("\n\n".join(res))

# CI wall time
own = {}
for r in rows:
    for p in [r['main']] + r['fixes'] + r['attempts'] + r['scheda']:
        own[p['head']] = r['kind']
d = [(ts(x['updated_at']) - ts(x['run_started_at'])).total_seconds() / 60 for x in runs
     if x['conclusion'] == 'success' and x['head_branch'] in own and x['run_started_at']]
dw = [(ts(x['updated_at']) - ts(x['run_started_at'])).total_seconds() / 60 for x in runs
      if x['conclusion'] == 'success' and own.get(x['head_branch']) == 'pacchetto' and x['run_started_at']]
dr = [(ts(x['updated_at']) - ts(x['run_started_at'])).total_seconds() / 60 for x in runs
      if x['conclusion'] == 'success' and own.get(x['head_branch']) == 'riferimenti' and x['run_started_at']]
print("\nCI success minutes: all med", round(med(d), 1), "work med", round(med(dw), 1), "refs med", round(med(dr), 1), len(d), len(dw), len(dr))
```

</details>

<details>
<summary><code>aggiunte.py</code>: Autore, tabella per autore (2.2), correlazioni, vita delle pull request (2.3), fermi della 0.7 (2.4): `python3 aggiunte.py <cartella>`</summary>

```python
# Second pass on dati/pacchetti-analisi.json (written by analisi.py): author of the
# main PR, table by difficulty and author, rank correlations, life of a PR,
# stopped time and last work in stage 0.7.
import json, re, sys, statistics as st
from datetime import datetime
S = sys.argv[1]
rows = json.load(open(f"{S}/dati/pacchetti-analisi.json"))
pulls = {p['number']: p for p in json.load(open(f"{S}/dati/pulls.json"))}
runs = json.load(open(f"{S}/dati/runs.json"))
com = {int(k): v for k, v in json.load(open(f"{S}/dati/commenti.json")).items()}
def ts(s): return datetime.fromisoformat(s.replace('Z', '+00:00'))

# Author: the tool named in the AI-use section of the main PR; by hand where the
# work changed hands (read in the issue comments).
def tool(n):
    sec = re.search(r'(?is)(uso dell.?ia|intelligenza artificiale|ai use|assisted-by)(.{0,600})', pulls[n]['body'] or '')
    if sec:
        for k, v in [('codex', 'codex'), ('antigravity', 'gemini'), ('gemini', 'gemini'), ('copilot', 'copilot'), ('claude', 'claude')]:
            if k in sec.group(2).lower(): return v
    b = (pulls[n]['body'] or '').lower()
    hits = [(b.find(k), v) for k, v in [('claude code', 'claude'), ('codex', 'codex'), ('antigravity', 'gemini'), ('gemini', 'gemini'), ('claude', 'claude')] if k in b]
    return min(hits)[1] if hits else '?'
OVR = {'0.4-P5': 'claude', '0.4-P9': 'codex', '0.7-P18': 'codex', '0.7-P22': 'codex', '0.7-P25': 'codex',
       '0.5-R10': 'gemini', '0.7-R9': 'gemini-argon', '0.7-R16': 'gemini-argon'}
for r in rows: r['autore_finale'] = OVR.get(r['code'], tool(r['main']['pr']))

out = ["| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | Righe (med.) | Run fallite per 1000 righe |",
       "|---|---|---|---|---|---|---|---|"]
for d in ['facile', 'media', 'impegnativa']:
    for a in ['claude', 'codex', 'gemini']:
        rs = [r for r in rows if r['kind'] == 'pacchetto' and r['diff'] == d and r['autore_finale'] == a]
        if not rs: continue
        ms = [r['main'] for r in rs]; g = sum(1 for m in ms if m['fail_before_green'] > 0)
        line = f"| {d}, {a} | {len(rs)} | {sum(m['failure'] for m in ms)} | {g} | {round(100*g/len(rs))} % | {st.median([m['runs'] for m in ms]):g} | {st.median([m['lines_own'] for m in ms]):g} | {1000*sum(m['failure'] for m in ms)/sum(m['lines_own'] for m in ms):.2f} |"
        out.append(line.replace('.', ','))
print("\n".join(out))

def rank(xs):
    o = sorted(range(len(xs)), key=lambda i: xs[i]); r = [0] * len(xs); i = 0
    while i < len(o):
        j = i
        while j + 1 < len(o) and xs[o[j + 1]] == xs[o[i]]: j += 1
        for k in range(i, j + 1): r[o[k]] = (i + j) / 2 + 1
        i = j + 1
    return r
def spearman(a, b):
    ra, rb = rank(a), rank(b); ma, mb = st.mean(ra), st.mean(rb)
    num = sum((x - ma) * (y - mb) for x, y in zip(ra, rb))
    return num / (sum((x - ma) ** 2 for x in ra) * sum((y - mb) ** 2 for y in rb)) ** .5
W = [r for r in rows if r['kind'] == 'pacchetto']
for lbl, sel in [('lavoro', W), ('medi', [r for r in W if r['diff'] == 'media']),
                 ('medi codex', [r for r in W if r['diff'] == 'media' and r['autore_finale'] == 'codex']),
                 ('impegnativi', [r for r in W if r['diff'] == 'impegnativa'])]:
    print('rho', lbl, len(sel), round(spearman([r['main']['lines_own'] for r in sel], [r['main']['fail_before_green'] for r in sel]), 2))
R = [r for r in rows if r['kind'] == 'riferimenti']
print('rho riferimenti', len(R), round(spearman([r['main']['lines_own'] for r in R], [r['nfix'] for r in R]), 2))

# Life of a PR: opening -> last green run before the merge -> merge.
ci_h, merge_h = [], []
for r in rows:
    p = pulls[r['main']['pr']]; c0, me = ts(p['created_at']), ts(p['merged_at'])
    ok = [ts(x['updated_at']) for x in runs if x['head_branch'] == p['head'] and x['conclusion'] == 'success'
          and c0.timestamp() - 120 <= ts(x['created_at']).timestamp() <= me.timestamp() + 60 and ts(x['updated_at']) <= me]
    if ok:
        g = max(ok); ci_h.append((g - c0).total_seconds() / 3600); merge_h.append((me - g).total_seconds() / 3600)
print('PR', len(ci_h), 'apertura→verde', round(st.median(ci_h), 2), 'verde→unione', round(st.median(merge_h), 2))

# Stage 0.7: stopped time (first claim -> last claim before the PR) and last work.
for r in rows:
    if r['tappa'] != '0.7' or r['kind'] != 'pacchetto': continue
    cl = sorted(ts(c[0]) for c in com[r['issue']] if re.search(r'[Pp]resa in carico', c[1]))
    pr = ts(r['main']['created']); last = [t for t in cl if t <= pr][-1]
    print(r['code'], 'fermo', round((last - cl[0]).total_seconds() / 3600, 1), 'ultimo lavoro', round((ts(r['main']['merged']) - last).total_seconds() / 3600, 1))
json.dump(rows, open(f"{S}/dati/pacchetti-analisi.json", 'w'), indent=1)
```

</details>

<details>
<summary><code>percorso.py</code>: Attese e percorso critico della 0.7 (2.4): `python3 percorso.py <cartella>`</summary>

```python
# Realised critical path of stage 0.7 from the dependency table of its plan (section 6).
import json, sys
from datetime import datetime
S = sys.argv[1]
rows = {r['code']: r for r in json.load(open(f"{S}/dati/pacchetti-analisi.json")) if r['tappa'] == '0.7'}
def ts(s): return datetime.fromisoformat(s.replace('Z', '+00:00'))
DEPS = {  # docs/tappe/0.7-modifiche.md, section 6, "Dipende da" (Tecnigrafo issues only)
 'P0': [], 'P1': [], 'P2': ['R2'], 'P3': ['P2', 'R3'], 'P4': ['R4'], 'P5': ['P1', 'P3', 'P4', 'R5'],
 'P6': ['P5', 'R6'], 'P7': ['P3', 'R7'], 'P8': ['P6', 'P7', 'R8'], 'P9': ['P4', 'R9'],
 'P10': ['P1', 'P8', 'P9', 'R10'], 'P11': ['P10'], 'P12': ['P11'], 'P13': ['P6', 'P10', 'P12'],
 'R14': [], 'P14': ['P0', 'R14'], 'P25': [], 'R15': ['P7'], 'P15': ['P7', 'P14', 'P25', 'R15'],
 'P16': ['P1', 'P8', 'P15', 'P25', 'R16'], 'P17': ['P14', 'P25', 'R17', 'R17b'],
 'P18': ['P16', 'P17', 'P25', 'R18', 'R17b'], 'P19': ['P15', 'P25', 'R19'], 'P20': ['P18', 'P19', 'P25', 'R20'],
 'P21': ['P20', 'P25', 'R21'], 'P22': ['P14', 'P25', 'R22', 'R22b'], 'P23': ['P21', 'P22', 'P25', 'R23'],
 'P24': ['P15', 'P23'],
}
def end(c): return ts(rows['0.7-' + c]['main']['merged'])
def start(c): return ts(rows['0.7-' + c]['start'])
open_t = ts('2026-10-29T12:10:00Z')
print("| Pacchetto | Difficoltà | Ultima dipendenza | Pronto | Preso | Unito | Attesa (h) | Lavoro (h) |")
print("|---|---|---|---|---|---|---|---|")
crit = {}
for c in DEPS:
    deps = DEPS[c]
    last = max(deps, key=end) if deps else None
    ready = max(end(last), open_t) if last else open_t
    wait = (start(c) - ready).total_seconds() / 3600
    work = (end(c) - start(c)).total_seconds() / 3600
    crit[c] = last
    if c.startswith('P'):
        print(f"| {c} | {rows['0.7-' + c]['diff']} | {last or '—'} | {ready:%d/%m %H:%M} | {start(c):%d/%m %H:%M} | {end(c):%d/%m %H:%M} | {wait:.1f} | {work:.1f} |".replace('.', ','))
last = max((c for c in DEPS if c.startswith('P')), key=end)
path = [last]
while crit[path[-1]]: path.append(crit[path[-1]])
print("\nPercorso critico realizzato:", " ← ".join(path))
tot = (end(last) - open_t).total_seconds() / 3600
print("durata apertura→ultimo merge (h)", round(tot, 1))
w = sum((end(c) - start(c)).total_seconds() / 3600 for c in path)
print("lavoro sui nodi del percorso (h)", round(w, 1), "attese", round(tot - w, 1))
for c in path: print(c, rows['0.7-' + c]['diff'], round((end(c) - start(c)).total_seconds() / 3600, 1))
```

</details>

<details>
<summary><code>casi.py</code>: Casi fini e ordinari dei riferimenti (3.5), sui file di `main`: `python3 casi.py tests/verifica/casi`</summary>

```python
# Counts reference cases of the six reference packages of the 0.7 heavy packages and classifies them.
# A question (.tgq "?" line) or a script case (.tgs) is "fine" when its expected values contain a
# number with an exponent or at least four decimals, or it follows a "!tolerance" line, or it asks
# a tangency/fillet/offset construction; otherwise it is "ordinary". The source of the values
# (solid-space corpus, ezdxf oracle/AutoCAD DXF, own calculation) is read from the file headers.
import re, sys, os, glob
fine_num = re.compile(r'(\d\.\d{4,}|\de[-+]?\d)', re.I)
fine_q = {'tangents', 'fillet', 'circle-ttr', 'offset', 'chamfer'}
GROUPS = {
    'R2':  ['erase/selezione-scritta-*'],
    'R10': ['grip/cerchio-e-modifica-non-valida', 'grip/due-entita-stesso-punto', 'grip/layer-bloccato-e-deselezione', 'grip/modifica-linea-annulla-esc'],
    'R14': ['intersezioni/*'],
    'R16': ['trim/*', 'extend/*'],
    'R17': ['curva-parallela/*'],
    'R19': ['raccordi/*'],
    'R9':  ['grip/degeneri', 'grip/modifica', 'grip/punti'],
}
base = sys.argv[1]
for g, pats in GROUPS.items():
    dirs = sorted({d for p in pats for d in glob.glob(os.path.join(base, p)) if os.path.isdir(d)})
    q_tot = q_fine = 0; s_tot = s_fine = 0; src = set(); per_dir = []
    for d in dirs:
        qd = qf = sd = sf = 0
        for f in sorted(os.listdir(d)):
            path = os.path.join(d, f); txt = open(path, encoding='utf-8', errors='replace').read()
            low = txt.lower()
            if 'corpus' in low or 'archivio' in low: src.add('archivio solid-space')
            if 'oracolo' in low or 'ezdxf' in low or 'autocad' in low: src.add('oracolo/AutoCAD')
            if f.endswith('.tgq'):
                tol = False
                for line in txt.splitlines():
                    s = line.strip()
                    if s.startswith('!tolerance'): tol = True
                    if s.startswith('?'):
                        qd += 1
                        name = s[1:].split()[0]
                        ans = s.split('->', 1)[1] if '->' in s else ''
                        if tol or name in fine_q or fine_num.search(ans) or fine_num.search(s.split('->')[0]): qf += 1
            if f.endswith('.tgs'):
                sd += 1
                att = os.path.join(d, 'atteso.txt')
                a = open(att, encoding='utf-8', errors='replace').read() if os.path.exists(att) else ''
                if fine_num.search(a) or '!tolerance' in txt: sf += 1
        q_tot += qd; q_fine += qf; s_tot += sd; s_fine += sf
        per_dir.append(f"{os.path.relpath(d, base)}: domande {qd} (fini {qf}), script {sd} (fini {sf})")
    print(f"{g}: domande {q_tot}, fini {q_fine}; script {s_tot}, fini {s_fine}; fonti {sorted(src)}")
    for p in per_dir: print('   ', p)
```

</details>

<details>
<summary><code>riferimenti-fascia.py</code>: correzioni dei riferimenti per modello dichiarato (2.5) e modelli e impegni dichiarati dai pacchetti di lavoro (sezione 1): `python3 riferimenti-fascia.py <cartella>`</summary>

```python
# Reference corrections by the model declared in the AI-use section of the main PR,
# and the models (and efforts) declared by the work packages.
import json, sys, re
from collections import defaultdict
S = sys.argv[1]
rows = json.load(open(f"{S}/dati/pacchetti-analisi.json"))
pulls = {p['number']: p for p in json.load(open(f"{S}/dati/pulls.json"))}
def declared(r):
    b = pulls[r['main']['pr']]['body'] or ''
    m = re.search(r'(?is)##\s*uso dell.?ia(.{0,400})', b)
    s = (m.group(1) if m else '').lower()
    if 'argon' in s: return 'Gemini 4 Argon'
    if 'flash' in s: return 'Gemini 3.8 Flash'
    if 'gemini' in s or 'antigravity' in s: return 'Gemini, modello non dichiarato'
    if r['autore_finale'] == 'gemini-argon': return 'Gemini 4 Argon'
    if r['autore_finale'] == 'gemini': return 'Gemini, modello non dichiarato'
    return {'claude': 'Claude', 'codex': 'Codex'}.get(r['autore_finale'], r['autore_finale'])
g = defaultdict(list)
for r in rows:
    if r['kind'] == 'riferimenti':
        g[declared(r)].append(r)
print("| Chi ha scritto i riferimenti | Pacchetti | Con ≥1 correzione | PR di correzione | Righe (med.) |")
print("|---|---|---|---|---|")
import statistics as st
for k in ['Gemini 3.8 Flash', 'Gemini, modello non dichiarato', 'Gemini 4 Argon', 'Claude', 'Codex']:
    rs = g.get(k, [])
    if not rs: continue
    print(f"| {k} | {len(rs)} | {sum(1 for r in rs if r['nfix'])} | {sum(r['nfix'] for r in rs)} | {st.median(r['main']['lines_own'] for r in rs):g} |")
for k in g:
    print(k, sorted(f"{r['code']}:{r['nfix']}" for r in g[k]), file=sys.stderr)

# Work packages: declared model and whether any effort is declared.
def section(r):
    m = re.search(r'(?is)##\s*uso dell.?ia(.{0,500})', pulls[r['main']['pr']]['body'] or '')
    return (m.group(1) if m else '').lower()
w = defaultdict(lambda: [0, 0])
effort = 0
for r in rows:
    if r['kind'] != 'pacchetto': continue
    s = section(r)
    model = 'Opus' if 'opus' in s else 'Sonnet' if 'sonnet' in s else 'Gemini' if ('gemini' in s or 'antigravity' in s) else 'non dichiarato'
    if re.search(r'impegno|effort|\bhigh\b|\bmedium\b|\bxhigh\b', s): effort += 1
    k = (r['diff'], r['autore_finale'], model)
    w[k][0] += 1; w[k][1] += 1 if r['main']['fail_before_green'] > 0 else 0
print()
print("pacchetti di lavoro che dichiarano un impegno:", effort)
for k, v in sorted(w.items()): print(k, 'pacchetti', v[0], 'con almeno un giro', v[1])
```

</details>
