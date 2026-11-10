# Misura dei pacchetti della 0.7.5 (#442, condizione 7)

- **Pacchetto:** [0.7.5-P39](P39-misura-pacchetti.md), issue #642
- **Autore:** Claude (Claude Code), modello Claude Opus 5.5, impegno non noto
- **Data:** 2026-11-10
- **Dati:** API REST di GitHub, lette il 2026-11-10, e `main` al commit
  `14eb938fe6c39ecdd6dac976d24b03c988e7461e` (solid-space al commit fissato
  `73d7e42949018e5e36eb137c3f8ceb4ab6eb1c87`)
- **Stato:** misura per il responsabile. Nessuna regola cambia con questo
  documento: le proposte della sezione 6 si applicano solo se lui le approva.

È la prima delle due misure chieste dal responsabile il 2026-11-10
([DECISIONI.md](../../DECISIONI.md)) con la condizione 7 dell'[analisi della
#442](analisi-pacchetti-medi.md), sezione 5; la seconda si fa alla fine della
0.8 con lo stesso comando (sezione 7). I numeri vengono tutti da
`scripts/misura_pacchetti.py`; dove il testo stima, lo dice.

## In breve

La 0.7.5 ha 39 pull request principali unite: 31 pacchetti medi, 4 facili, 3
impegnativi (due dei quali sono documenti: l'analisi P8 e le decisioni di
P23) e un pacchetto di riferimenti, R28b. Cinque cose:

1. **I fermi sono quasi spariti.** La somma dei fermi (dalla prima presa in
   carico all'ultima prima della pull request) è 5 ore, tutte di P23, che è
   passato da Codex a Claude; nella 0.7 era 286. Nessun altro pacchetto è stato
   ripreso in carico prima della sua pull request. È la differenza più grande,
   ed è misurata; il perché no (sezione 3).
2. **I giri di correzione dei medi sono un po' meno frequenti, non
   abbastanza per dirlo, e si concentrano nell'interfaccia.** Il 48 % dei medi
   ha avuto almeno una run fallita prima del verde (15 su 31), contro il 65 %
   delle tappe 0.4–0.7 (35 su 54): circa una volta e mezza l'errore tipico di
   due campioni così (stima), quindi può essere il caso. Dei pacchetti di
   lavoro che toccano `app` (Qt) ne ha avuto uno il 59 % (13 su 22), degli
   altri il 19 % (3 su 16); nelle tappe 0.4–0.7 erano 60 % e 41 %. E per la
   prima volta i giri crescono con le righe di codice (Spearman 0,51, contro
   0,11 nelle tappe 0.4–0.7), ma i pacchetti più grandi sono tutti in `app`.
3. **Correzioni dopo l'unione: nessuna con la regola della scheda.** Nessuna
   pull request unita dopo una principale ne cita il codice per correggerne
   casi o codice. Una correzione di codice c'è (#628, che chiude la #627, sulle
   icone dei pannelli), ma non cita un pacchetto e la regola non la attribuisce.
4. **La tappa è andata in fila e di corsa.** 38 pacchetti di lavoro e uno di
   riferimenti in 71 ore di calendario dalla prima issue all'ultima unione; il
   percorso critico realizzato ha 16 nodi, con 39,5 ore di ultimo lavoro su
   64,7 dall'unione del primo. Le attese tra un nodo pronto e la sua presa
   hanno mediana 0,3 ore.
5. **Modello sì, impegno quasi mai.** Tutte le 38 pull request di lavoro
   dichiarano il modello; l'impegno lo scrivono solo P8 e P33 (P31 scrive
   «impegno non noto»). Per il resto vale la dichiarazione del responsabile
   del 2026-11-10, segnata come tale. Per 19 medi (17 di Claude, 2 di Codex) il
   piano prevedeva impegno alto, e la dichiarazione dice medio.

Che cosa questa misura **non** prova: l'ipotesi della sezione 3.9
dell'analisi, cioè codice impegnativo scritto in fascia media. Nella 0.7.5 i
tre impegnativi sono stati fatti in fascia forte e i medi erano medi anche per
natura; si prova nella 0.8 (sezione 5).

## 1. Metodo e limiti

Il metodo è quello della sezione 1 dell'analisi, con le stesse definizioni
(pull request principale, giro di correzione, autore, caso fine), riunito in
`scripts/misura_pacchetti.py`. Le differenze, tutte scritte in testa allo
script:

- **Correzioni.** Sono le pull request unite dopo la principale che ne citano
  il codice (`0.7.5-P24`) nel titolo o, se il titolo non cita codici, nella
  descrizione, o chiudono la sua issue, e non sono la principale di un'altra
  issue. Il titolo vince perché una pull request che nel titolo porta il
  codice di un altro pacchetto è di quel pacchetto, anche se nella descrizione
  ne nomina altri (per esempio la #582, di 0.7.5-P7, cita 0.7-P15). Toccano solo
  `tests/verifica/`: correzione dei riferimenti; solo `docs/` o file Markdown:
  di documenti; il resto: di codice. L'analisi le abbinava dal titolo e a mano;
  gli abbinamenti a mano delle tappe 0.4–0.7 sono nello script.
- **Pull request principale.** Deve essere aperta dopo la issue: una pull
  request del piano che crea un pacchetto e ne porta il codice nel titolo (per
  P39, la #641) non è la sua principale.
- **Righe di codice.** Oltre alle righe totali (test compresi), lo script
  conta le righe senza test (`tests/`, `*_test.*`), documenti (`docs/`,
  `*.md`) e risorse (`resources/`, cioè le icone), per la misura della
  condizione 2. Script, file di CMake e della CI contano come codice.
- **Autore.** Lo strumento dichiarato, cioè il primo nominato nella sezione
  «Uso dell'IA»; «nessuna» se la sezione spunta «Nessuna IA usata». Solo per
  le pull request vecchie che la sezione non l'hanno (0.6-P16), il primo
  strumento nominato nella descrizione, come nell'analisi: vale come autore,
  mai come dato dichiarato, e non fa scattare la dichiarazione del
  responsabile, che si applica allo strumento dichiarato.
- **Previsti e dichiarati.** Difficoltà, agente, fascia e impegno previsti
  vengono dalla tabella della sezione 6 del piano; strumento, modello e impegno
  dichiarati dalla sezione «Uso dell'IA» della pull request principale, senza i
  commenti del modello. Dove mancano: «non dichiarato».
- **Impegno del responsabile.** Per le pull request della 0.7.5 senza impegno
  vale la dichiarazione del responsabile del 2026-11-10 (scheda di P39): medi
  con impegno medio (Opus 5.5 o Codex sol), impegnativi con impegno alto. Le
  tabelle la segnano come «responsabile», distinta da «pull request». Non
  copre i facili e i riferimenti, che restano «non dichiarato». Nessuna pull
  request ha un modello che la contraddice: i modelli di Codex scritti (GPT-6,
  GPT-6.1, GPT-6.1 Sol) non nominano astra.

Questa prima misura serve anche da prova dello script sulle tappe dell'analisi
(sezione 3, ultimo paragrafo).

**Limiti** (oltre a quelli della sezione 1 dell'analisi, che valgono tutti):

- **Una tappa sola, di tipo diverso.** La 0.7.5 è una correzione di rotta:
  CI, strumenti, interfaccia, separazioni di codice; quasi niente geometria.
  Un solo pacchetto ha riferimenti (R28b), che cambia quasi solo la riga
  `documents:` di casi esistenti.
- **Quasi solo pacchetti medi**, e medi per natura: nessun impegnativo è
  stato spostato in fascia media. Per i medi la misura dà una base, non un
  confronto tra fasce.
- **Impegno dichiarato a posteriori.** La riga dell'impegno nel modello di
  pull request esiste dalla #641 (2026-11-10): l'impegno delle pull request
  della 0.7.5 è quasi tutto della dichiarazione del responsabile, non letto.
  Che 19 medi pianificati con impegno alto siano stati fatti con impegno medio
  viene da quella dichiarazione.
- **P23 e P3 sono passati di mano.** Il verbale di P23 l'ha scritto Codex
  (GPT-6) nella issue; la pull request unita (#588, le decisioni sui buchi
  trascritte nelle schede) è di Claude, e lo script attribuisce P23 a Claude.
  P3 l'ha cominciato Codex e finito Claude; lo script lo attribuisce a Codex,
  primo strumento nominato.
- **P34 non c'è.** La sua issue è chiusa senza pull request: il piano della
  0.8 è uscito dalla tappa (#578).
- **I difetti dopo l'unione restano quasi invisibili.** Una correzione che non
  cita il pacchetto (come la #628) non si attribuisce.

## 2. Dati

### 2.1 Tappa 0.7.5

Colonne come nella sezione 2.1 dell'analisi, più le run annullate, le righe di
codice e le correzioni divise tra riferimenti e codice.

| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | Run annullate (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | File (med.) | Righe (med.) | Righe di codice (med.) | Correzioni dei riferimenti (tot.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| pacchetto | facile | 4 | 2 | 2; 7 | 0 | 0 | 0 | 0,9 | 1,0 | 2 | 78 | 6 | 0 | 0 |
| pacchetto | media | 31 | 3 | 4; 127 | 18 | 56 | 15 | 2,2 | 2,4 | 11 | 723 | 263 | 0 | 0 |
| pacchetto | impegnativa | 3 | 2 | 2; 9 | 1 | 2 | 1 | 1,6 | 3,7 | 12 | 1628 | 0 | 0 | 0 |
| riferimenti | media | 1 | 2 | 3; 3 | 1 | 0 | 1 | 15,0 | 15,4 | 55 | 543 | 0 | 0 | 0 |

### 2.2 Per difficoltà e autore, pacchetti di lavoro

| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | Righe (med.) | Run fallite per 1000 righe |
|---|---|---|---|---|---|---|---|
| facile, claude | 1 | 0 | 0 | 0 % | 2 | 21 | 0 |
| facile, codex | 1 | 0 | 0 | 0 % | 2 | 87 | 0 |
| facile, gemini | 2 | 0 | 0 | 0 % | 1,5 | 512 | 0 |
| media, claude | 19 | 11 | 9 | 47 % | 3 | 917 | 0,60 |
| media, codex | 12 | 7 | 6 | 50 % | 4 | 446 | 1,24 |
| impegnativa, claude | 3 | 1 | 1 | 33 % | 2 | 1628 | 0,24 |

Pacchetti di lavoro che toccano `app/` o `tests/app/` (Qt): 22, con almeno un giro 13 (0.7.5-P3, 0.7.5-P10, 0.7.5-P14, 0.7.5-P16, 0.7.5-P18, 0.7.5-P19, 0.7.5-P20, 0.7.5-P24, 0.7.5-P28, 0.7.5-P29, 0.7.5-P30, 0.7.5-P31, 0.7.5-P30b).
Pacchetti di lavoro che non toccano `app/` o `tests/app/` (Qt): 16, con almeno un giro 3 (0.7.5-P6, 0.7.5-P7, 0.7.5-P26).
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro tutti (38): 0,27; con le righe di codice: 0,51.
Spearman tra righe e run fallite prima del verde, pacchetti di lavoro medi (31): 0,22; con le righe di codice: 0,38.

### 2.3 Per fascia e impegno, previsti e dichiarati

«Fascia prevista» e «Impegno previsto» sono le colonne «Modello» e «Impegno»
della tabella del piano; «Modello dichiarato» e «Impegno» vengono dalla pull
request o, dove segnato, dalla dichiarazione del responsabile.

**Previsti dal piano**

| Difficoltà | Fascia prevista | Impegno previsto | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|
| facile | leggero | basso | 2 | 0 | 0 | 512 | 0 |
| facile | medio | medio | 2 | 0 | 0 | 54 | 0 |
| impegnativa | forte | alto | 3 | 1 | 1 | 1628 | 0 |
| media | medio | alto | 19 | 10 | 13 | 846 | 0 |
| media | medio | medio | 12 | 5 | 5 | 430 | 0 |

**Dichiarati**

| Autore | Modello dichiarato | Impegno | Fonte dell'impegno | PR | PR con ≥1 giro | Run fallite (tot.) | Righe (med.) | Correzioni del codice (tot.) |
|---|---|---|---|---|---|---|---|---|
| claude | Claude Opus 5.5 | alto | pull request | 1 | 0 | 0 | 1628 | 0 |
| claude | Claude Opus 5.5 | alto | responsabile | 2 | 1 | 1 | 1229 | 0 |
| claude | Claude Opus 5.5 | medio | responsabile | 18 | 8 | 10 | 930 | 0 |
| claude | Claude Opus 5.5 | non dichiarato | non dichiarato | 1 | 0 | 0 | 21 | 0 |
| claude | Claude Opus 5.5 + GPT-6.1 | medio | responsabile | 1 | 1 | 1 | 846 | 0 |
| codex | GPT-6 | medio | responsabile | 6 | 3 | 3 | 431 | 0 |
| codex | GPT-6 | non dichiarato | non dichiarato | 1 | 0 | 0 | 87 | 0 |
| codex | GPT-6 + Claude Opus 5.5 | medio | responsabile | 1 | 1 | 1 | 326 | 0 |
| codex | GPT-6.1 | medio | responsabile | 2 | 1 | 1 | 516 | 0 |
| codex | GPT-6.1 sol | medio | responsabile | 3 | 1 | 2 | 471 | 0 |
| gemini | Gemini 3.8 Flash | alto | pull request | 1 | 0 | 0 | 954 | 0 |
| gemini | Gemini 3.8 Flash | non dichiarato | non dichiarato | 1 | 0 | 0 | 69 | 0 |

Dei 19 medi pianificati con impegno alto, 17 sono di Claude e 2 di Codex (P24
e P25). Secondo la dichiarazione del responsabile sono stati fatti tutti con
impegno medio: le due righe dei medi della prima tabella non confrontano due
impegni usati, ma due insiemi di pacchetti (quelli pianificati alto sono più
grandi, mediana 846 righe contro 430, e quasi tutti in `app`).

### 2.4 Misura dei pacchetti (condizione 2)

Pacchetti di lavoro uniti: 38; con più di 15 file (test e documenti compresi): 12; con più di 600 righe di codice: 6; con meno di 150 righe di codice: 17; righe di codice, mediana 216.

Oltre i 15 file (test e documenti compresi): P1, P4, P19, P20, P21, P22, P24,
P25, P28, P28b, P29 e P30; quelli delle icone (P19–P22, 29–73 file) sono
soprattutto SVG, che non contano come codice. Oltre le 600 righe di codice:
P11, P14, P18, P28, P29 e P30, tutti in `app`; cinque di loro hanno avuto
almeno un giro (non P11). Sotto le 150 righe di codice: documenti e immagini (P8,
P9, P23, P32, P33), icone (P20–P22, P35), CI e strumenti (P1, P2, P36–P38) e tre
pacchetti di codice (P7, P27, P31).

### 2.5 Costi fissi

- Run verde, durata mediana: 13,3 minuti (68 run verdi delle pull request principali, di correzione e dei tentativi).
- Vita della pull request principale: apertura→ultimo verde 1,33 ore, verde→unione 0,14 ore (mediane su 39).
- Pull request principali unite: 39; correzioni dei riferimenti 0, del codice 0, di documenti 0; tentativi non uniti 0.
- Run delle pull request principali: 146, di cui 20 fallite e 58 annullate.

### 2.6 Tempi

«Fermo», «ultimo lavoro» e percorso critico come nella sezione 2.4
dell'analisi; «Attesa» va da quando il pacchetto è pronto (l'ultima
dipendenza del piano unita, o l'apertura della tappa, cioè la creazione della
prima issue) alla prima presa in carico.

Orari UTC.

| Pacchetto | Difficoltà | Ultima dipendenza unita | Prima presa | Ultima presa prima della PR | Unito | Fermo (h) | Ultimo lavoro (h) | Attesa da pronto a presa (h) |
|---|---|---|---|---|---|---|---|---|
| P1 | media | — | 07/11 12:21 | 07/11 12:21 | 07/11 16:38 | 0 | 4,3 | 0,3 |
| P2 | media | — | 07/11 12:22 | 07/11 12:22 | 07/11 13:05 | 0 | 0,7 | 0,3 |
| P3 | media | P1 07/11 16:38 | 07/11 18:01 | 07/11 18:01 | 08/11 05:27 | 0 | 11,4 | 1,4 |
| P35 | facile | P3 08/11 05:27 | 08/11 05:37 | 08/11 05:37 | 08/11 06:21 | 0 | 0,7 | 0,2 |
| P36 | media | P3 08/11 05:27 | 08/11 05:38 | 08/11 05:38 | 08/11 08:31 | 0 | 2,9 | 0,2 |
| P37 | facile | P36 08/11 08:31 | 08/11 08:50 | 08/11 08:50 | 08/11 11:07 | 0 | 2,3 | 0,3 |
| P38 | media | P37 08/11 11:07 | 08/11 11:22 | 08/11 11:22 | 08/11 14:39 | 0 | 3,3 | 0,3 |
| P4 | media | P38 08/11 14:39 | 08/11 14:52 | 08/11 14:52 | 08/11 17:01 | 0 | 2,1 | 0,2 |
| P5 | media | P4 08/11 17:01 | 08/11 17:10 | 08/11 17:10 | 08/11 19:38 | 0 | 2,5 | 0,2 |
| P6 | media | P5 08/11 19:38 | 08/11 19:51 | 08/11 19:51 | 09/11 07:03 | 0 | 11,2 | 0,2 |
| P7 | media | — | 07/11 13:09 | 07/11 13:09 | 07/11 18:01 | 0 | 4,9 | 1,1 |
| P8 | impegnativa | — | 07/11 13:19 | 07/11 13:19 | 07/11 16:59 | 0 | 3,7 | 1,2 |
| P9 | media | — | 07/11 13:20 | 07/11 13:20 | 07/11 16:40 | 0 | 3,3 | 1,3 |
| P10 | media | — | 07/11 13:20 | 07/11 13:20 | 07/11 18:28 | 0 | 5,1 | 1,3 |
| P11 | media | P10 07/11 18:28 | 07/11 19:30 | 07/11 19:30 | 07/11 20:06 | 0 | 0,6 | 1 |
| P12 | media | P11 07/11 20:06 | 08/11 09:39 | 08/11 09:39 | 08/11 14:40 | 0 | 5,0 | 13,6 |
| P13 | media | P12 08/11 14:40 | 08/11 14:52 | 08/11 14:52 | 08/11 16:13 | 0 | 1,4 | 0,2 |
| P14 | media | P13 08/11 16:13 | 08/11 16:16 | 08/11 16:16 | 08/11 18:40 | 0 | 2,4 | 0 |
| P15 | media | — | 07/11 13:21 | 07/11 13:21 | 07/11 17:57 | 0 | 4,6 | 1,3 |
| P16 | media | P12 08/11 14:40 | 08/11 14:52 | 08/11 14:52 | 08/11 15:56 | 0 | 1,1 | 0,2 |
| P18 | media | P14 08/11 18:40 | 08/11 19:18 | 08/11 19:18 | 08/11 21:02 | 0 | 1,7 | 0,6 |
| P19 | media | P16 08/11 15:56 | 08/11 16:13 | 08/11 16:13 | 08/11 16:53 | 0 | 0,7 | 0,3 |
| P20 | media | P19 08/11 16:53 | 08/11 17:11 | 08/11 17:11 | 08/11 18:40 | 0 | 1,5 | 0,3 |
| P21 | media | P20 08/11 18:40 | 08/11 19:17 | 08/11 19:17 | 08/11 20:03 | 0 | 0,8 | 0,6 |
| P22 | media | P21 08/11 20:03 | 08/11 20:06 | 08/11 20:06 | 08/11 21:03 | 0 | 0,9 | 0,1 |
| P23 | impegnativa | — | 07/11 13:23 | 07/11 18:25 | 07/11 19:14 | 5,0 | 0,8 | 1,3 |
| P24 | media | P18 08/11 21:02 | 09/11 05:04 | 09/11 05:04 | 09/11 07:52 | 0 | 2,8 | 8 |
| P25 | media | P24 09/11 07:52 | 09/11 08:38 | 09/11 08:38 | 09/11 10:44 | 0 | 2,1 | 0,8 |
| P26 | media | P25 09/11 10:44 | 09/11 10:47 | 09/11 10:47 | 09/11 12:43 | 0 | 1,9 | 0,1 |
| P27 | media | P26 09/11 12:43 | 09/11 12:49 | 09/11 12:49 | 09/11 14:07 | 0 | 1,3 | 0,1 |
| P28 | impegnativa | P26 09/11 12:43 | 09/11 12:48 | 09/11 12:48 | 09/11 14:45 | 0 | 1,9 | 0,1 |
| P28b | media | P28 09/11 14:45 | 09/11 15:16 | 09/11 15:16 | 09/11 16:59 | 0 | 1,7 | 0,5 |
| P29 | media | P28b 09/11 16:59 | 09/11 17:02 | 09/11 17:02 | 09/11 21:24 | 0 | 4,4 | 0,1 |
| P30 | media | P29 09/11 21:24 | 09/11 21:25 | 09/11 21:25 | 10/11 04:29 | 0 | 7,1 | 0 |
| P30b | media | P30 10/11 04:29 | 10/11 04:30 | 10/11 04:30 | 10/11 08:17 | 0 | 3,8 | 0 |
| P31 | media | P30b 10/11 08:17 | 10/11 08:31 | 10/11 08:31 | 10/11 10:49 | 0 | 2,3 | 0,2 |
| P32 | facile | — | 08/11 09:39 | 08/11 09:39 | 08/11 10:53 | 0 | 1,2 | 21,6 |
| P33 | facile | P31 10/11 10:49 | 10/11 10:52 | 10/11 10:52 | 10/11 11:12 | 0 | 0,3 | 0 |

Percorso critico realizzato: P10 → P11 → P12 → P13 → P14 → P18 → P24 → P25 → P26 → P28 → P28b → P29 → P30 → P30b → P31 → P33; dall'unione di P10 all'ultima unione 64,7 ore, ultimo lavoro dei nodi dopo il primo 39,5 ore; fermi di tutti i pacchetti 5 ore; attesa da pronto a presa, mediana 0,3 ore su 38 pacchetti di lavoro; dalla prima issue all'ultima unione 71,1 ore.

### 2.7 Riferimenti

Casi fini e ordinari contati con la regola meccanica della sezione 1
dell'analisi su **tutte** le cartelle di casi toccate da R28b, come sono oggi
su `main`: la maggior parte dei casi fini (raccordi, smussi, curve parallele,
tagli) non li ha scritti R28b, che vi ha cambiato la riga `documents:`.

| Riferimenti | Chi li ha scritti (modello dichiarato) | Domande (fini) | Script (fini) | Correzioni dei riferimenti | Righe |
|---|---|---|---|---|---|
| 0.7.5-R28b | gemini (Gemini 3.8 Flash + Gemini 4 Argon) | 0 (0) | 27 (13) | — | 543 |

## 3. Confronto con le tappe 0.4–0.7

Solo dove la differenza è grande, con la prudenza della sezione 1
dell'analisi sui campioni piccoli. I numeri delle tappe 0.4–0.7 sono quelli
delle sezioni 2.1–2.5 dell'analisi; quelli che l'analisi non aveva (run
annullate, `app`, righe di codice) vengono dallo script sulle stesse tappe.

| Che cosa | 0.4–0.7 | 0.7 | 0.7.5 | Giudizio |
|---|---|---|---|---|
| Fermi di tutti i pacchetti | — | 286 h | 5 h | grande, misurata |
| PR di correzione dei riferimenti per pacchetto di riferimenti | 0,9 | 1,0 (23 su 22) | 0 (su 1) | grande, ma su un solo pacchetto |
| Durata della tappa (prima issue o apertura → ultima unione) | — | circa 198 h | 71 h | grande, con più pacchetti di lavoro (38 contro 26) |
| Ore presa→unita, medi (mediana) | 2,9 | 5,8 | 2,4 | grande rispetto alla 0.7, non rispetto all'insieme |
| Pacchetti di lavoro con almeno un giro, che toccano `app` / gli altri | 60 % (15 su 25) / 41 % (26 su 64) | — | 59 % (13 su 22) / 19 % (3 su 16) | grande fuori da `app`, nessuna in `app` |
| Spearman tra righe di codice e giri, pacchetti di lavoro | 0,11 (89) | — | 0,51 (38) | grande, ma confusa con `app` |
| Medi con almeno un giro | 65 % (35 su 54) | 69 % (11 su 16) | 48 % (15 su 31) | moderata: entro il caso (stima) |
| Run fallite per 1000 righe, medi di Claude / di Codex | 1,04 / 1,63 | — | 0,60 / 1,24 | moderata, stesse riserve |
| Run annullate sulle run dei medi | 39 % (71 su 180) | 52 % (34 su 65) | 44 % (56 su 127) | nessuna |
| Durata mediana di una run verde | 14,9 min | — | 13,0 min | piccola |
| Apertura→ultimo verde, PR principali (mediana) | 0,6 h | — | 1,3 h | raddoppiata, in assoluto un'ora |

**Fermi.** Nella 0.7 i fermi venivano da buchi delle schede, casi di
riferimento sbagliati e pacchetti di solid-space (analisi, sezione 2.4). Nella
0.7.5 nessun pacchetto si è fermato in attesa di solid-space, uno solo aveva
riferimenti, e i buchi delle schede P24–P31 li ha trovati prima la revisione
delle interfacce di P23, con le decisioni scritte nelle schede (#588) prima
della presa dei pacchetti. Quale dei tre motivi pesi di più, i dati non lo
separano.

**Giri.** La quota dei medi con almeno un giro scende da 65 % a 48 %; per
Claude da 64 % a 47 %, per Codex da 65 % a 50 %. Con 31 e 54 pacchetti la
differenza (17 punti) è circa 1,5 volte l'errore tipico della differenza tra
due proporzioni (stima con l'approssimazione normale): non basta per dire che
sia reale. I medi della 0.7.5 sono anche di natura diversa (CI e interfaccia,
non comandi e geometria), e le run annullate restano tante quante prima.

**Dove stanno i giri.** Il calo dei giri viene tutto dai pacchetti fuori da
`app` (da 41 % a 19 %); in `app` la quota è la stessa delle tappe precedenti
(60 % e 59 %). Un'ipotesi, non verificata qui: le sessioni cloud non
compilano Qt (non si installa), quindi il codice di `app` si prova per la
prima volta in CI e il primo giro è il modo normale di trovarne gli errori. La
correlazione tra righe di codice e giri (0,51) ha la stessa ombra: i sei
pacchetti oltre le 600 righe sono tutti in `app`. Contro la lettura
dell'analisi (i giri non crescono con la misura) la 0.7.5 non basta: dice solo
che, nell'interfaccia, i pacchetti grandi hanno avuto più giri.

**Vita della pull request.** Dall'apertura all'ultimo verde la mediana passa
da 0,6 a 1,3 ore, con run verdi un poco più corte: le pull request della 0.7.5
hanno avuto più run (mediana 4 contro 3 nei medi). Il costo resta piccolo in
assoluto.

**Lo script sulle tappe dell'analisi.** Con `--tappe 0.4 0.5 0.6 0.7` lo
script rifà le tabelle delle sezioni 2.1 e 2.2 dell'analisi con gli stessi
numeri (cambia solo l'arrotondamento di due mediane a metà, 784 invece di
783,5), le stesse 46 correzioni dei riferimenti, la stessa correlazione tra
righe e giri (−0,07) e lo stesso percorso critico della 0.7 (111,6 ore dopo
l'unione di P25, fermi 286 ore). Per arrivarci servono la regola del titolo
sopra e due esclusioni scritte nello script: la #441 e la #443, strumenti fuori
tappa che nominano i file di attesa di alcuni riferimenti.

## 4. Le condizioni della sezione 5

| Condizione | Che cosa dice la 0.7.5 | Perché |
|---|---|---|
| 1. Si divide per un motivo che si scrive | non misurabile | Il piano della 0.7.5 è precedente alla decisione; i dati non registrano il motivo di una divisione. |
| 2. La misura conta il codice | non misurabile come regola; misurata la distribuzione | Il piano non usava la soglia. Con la soglia, 6 dei 38 pacchetti di lavoro la superano e 17 stanno sotto il minimo di 150 righe di codice (sezione 2.4). Dei 6 sopra la soglia, 5 hanno avuto almeno un giro: va nel verso della condizione, ma sono tutti in `app`, dove i giri sono frequenti a ogni misura (sezione 3). |
| 3. La fascia segue le decisioni | non misurabile | Nessun impegnativo è stato fatto in fascia media; i medi erano medi per natura. È l'ipotesi della sezione 3.9, da provare nella 0.8. |
| 4. Riferimenti delicati in fascia forte | non misurabile | Un solo pacchetto di riferimenti, scritto da Gemini 3.8 Flash e Gemini 4 Argon, che ha cambiato quasi solo una riga di casi esistenti; nessuna correzione. |
| 5. Revisione delle interfacce prima delle parti | confermata in questo campione, debolmente | P23 ha rivisto le schede P24–P31 prima della loro presa, e il responsabile ha deciso i nove buchi trovati (B1–B9) prima dell'inizio; i dieci pacchetti che ne discendono (P24–P31, P28b, P30b) non hanno avuto fermi né correzioni. Un caso solo, senza un confronto; la revisione mirata delle pull request (Copilot o Codex) non è registrata. |
| 6. Pacchetto iniziale di interfaccia solo se toglie file comuni | non misurabile | Nessun pacchetto di questo tipo nella 0.7.5. |
| 7. Si registra e si misura | in corso | Il modello è dichiarato da tutte le pull request di lavoro; l'impegno da due. La registrazione dell'impegno parte dalla #641: la misura della 0.8 sarà la prima con l'impegno letto dalle pull request. |

## 5. Che cosa resta per la misura della 0.8

- **L'ipotesi della sezione 3.9** (codice degli impegnativi in fascia media,
  con decisioni scritte prima e riferimenti forti): servono pacchetti
  dichiarati in fascia media con difficoltà alta per natura, e il confronto con
  i loro riferimenti e le correzioni dopo l'unione.
- **I riferimenti per fascia:** con i riferimenti divisi per natura (decisione
  del 2026-11-10) la 0.8 avrà blocchi delicati e ordinari; la sezione 2.7 li
  conterà per blocco.
- **Le correzioni di codice dopo l'unione:** la regola meccanica le trova solo
  se citano il pacchetto (proposta 1 della sezione 6).

## 6. Proposte per il responsabile

Nessuna regola cambia con questo documento.

1. **Chi corregge un difetto dopo l'unione cita il pacchetto.** Una riga nella
   pull request di correzione («Corregge 0.8-PT12»), anche quando chiude una
   issue di bug: è l'unico modo perché la misura della 0.8 conti le correzioni
   di codice per fascia (condizione 7). Oggi la #628 non si attribuisce.
2. **Impegno previsto e impegno usato.** Il piano della 0.7.5 prevedeva impegno
   alto per 19 medi; la dichiarazione del responsabile dice medio. Dalla 0.8 la
   pull request scrive l'impegno usato: se resta diverso da quello del piano,
   vale la pena correggere la colonna del piano o la scelta all'avvio, perché
   la misura confronta le due cose.
3. **I giri si contano separando `app`.** Nella misura della 0.8 conviene
   leggere i giri dei pacchetti di `app` a parte (lo script lo fa già): se il
   primo giro di `app` è il costo normale di non poter compilare Qt nelle
   sessioni cloud, confrontarlo con gli altri confonde fascia e modulo.
4. **Un pacchetto di revisione che passa di mano** (P23: verbale di Codex,
   pull request di Claude) andrebbe registrato con chi ha fatto che cosa nella
   pull request, come il modello: oggi lo script vede solo lo strumento
   della pull request unita.

## 7. Dati grezzi e comando

### 7.1 Per pacchetto

«Previsti» dalla tabella del piano; «Impegno (fonte)»: «pull request» se
scritto nella pull request, «responsabile» se dalla dichiarazione del
2026-11-10. «Altre PR»: correzioni di documenti e tentativi non uniti.

| Codice | Issue | Tipo | Diff. | Previsti (agente, fascia, impegno) | Autore | Modello dichiarato | Impegno (fonte) | PR | Commit | File | Righe | Righe di codice | Run | Fallite | Annullate | Giri | Ore PR | Ore presa→unione | Correzioni riferimenti | Correzioni codice | Altre PR |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.7.5-P1 | #542 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 | medio (responsabile) | #579 | 3 | 20 | 251 | 99 | 3 | 0 | 2 | 0 | 4,2 | 4,3 | — | — | — |
| 0.7.5-P2 | #543 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #580 | 1 | 10 | 388 | 99 | 1 | 0 | 0 | 0 | 0,6 | 0,7 | — | — | — |
| 0.7.5-P3 | #544 | pacchetto | media | codex, medio, medio | codex | GPT-6 + Claude Opus 5.5 | medio (responsabile) | #587 | 7 | 6 | 326 | 199 | 7 | 1 | 5 | 1 | 11,4 | 11,4 | — | — | — |
| 0.7.5-P4 | #545 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #615 | 2 | 27 | 568 | 298 | 2 | 0 | 1 | 0 | 1,6 | 2,1 | — | — | — |
| 0.7.5-P5 | #546 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #618 | 7 | 6 | 746 | 425 | 7 | 0 | 6 | 0 | 2,4 | 2,5 | — | — | — |
| 0.7.5-P6 | #547 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #623 | 3 | 6 | 943 | 477 | 3 | 1 | 0 | 1 | 11,1 | 11,2 | — | — | — |
| 0.7.5-P7 | #548 | pacchetto | media | codex, medio, medio | codex | GPT-6.1 | medio (responsabile) | #582 | 6 | 8 | 272 | 79 | 6 | 1 | 3 | 1 | 4,8 | 4,9 | — | — | — |
| 0.7.5-P8 | #549 | pacchetto | impegnativa | claude, forte, alto | claude | Claude Opus 5.5 | alto (pull request) | #586 | 2 | 1 | 1628 | 0 | 2 | 0 | 0 | 0 | 3,3 | 3,7 | — | — | — |
| 0.7.5-P9 | #550 | pacchetto | media | claude, medio, medio | claude | Claude Opus 5.5 | medio (responsabile) | #583 | 2 | 9 | 1760 | 0 | 2 | 0 | 0 | 0 | 3,2 | 3,3 | — | — | — |
| 0.7.5-P10 | #551 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 + GPT-6.1 | medio (responsabile) | #584 | 8 | 8 | 846 | 587 | 8 | 1 | 5 | 1 | 5 | 5,1 | — | — | — |
| 0.7.5-P11 | #552 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #591 | 1 | 13 | 1690 | 888 | 1 | 0 | 0 | 0 | 0,4 | 0,6 | — | — | — |
| 0.7.5-P12 | #553 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #608 | 5 | 10 | 817 | 480 | 5 | 0 | 3 | 0 | 4,8 | 5,0 | — | — | — |
| 0.7.5-P13 | #554 | pacchetto | media | codex, medio, medio | codex | GPT-6.1 sol | medio (responsabile) | #613 | 6 | 9 | 471 | 263 | 6 | 0 | 3 | 0 | 1,2 | 1,4 | — | — | — |
| 0.7.5-P14 | #555 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #617 | 6 | 11 | 1900 | 1287 | 6 | 1 | 3 | 1 | 2,1 | 2,4 | — | — | — |
| 0.7.5-P15 | #556 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #585 | 2 | 7 | 555 | 245 | 2 | 0 | 0 | 0 | 4,4 | 4,6 | — | — | — |
| 0.7.5-P16 | #557 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #614 | 4 | 14 | 577 | 293 | 4 | 1 | 2 | 1 | 0,9 | 1,1 | — | — | — |
| 0.7.5-P18 | #558 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #622 | 7 | 12 | 1309 | 993 | 7 | 2 | 4 | 2 | 1,5 | 1,7 | — | — | — |
| 0.7.5-P19 | #559 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #616 | 2 | 33 | 917 | 279 | 2 | 1 | 0 | 1 | 0,4 | 0,7 | — | — | — |
| 0.7.5-P20 | #560 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #619 | 6 | 29 | 474 | 56 | 6 | 1 | 3 | 1 | 1,4 | 1,5 | — | — | — |
| 0.7.5-P21 | #561 | pacchetto | media | codex, medio, medio | codex | GPT-6.1 | medio (responsabile) | #621 | 2 | 73 | 759 | 0 | 2 | 0 | 0 | 0 | 0,7 | 0,8 | — | — | — |
| 0.7.5-P22 | #562 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #624 | 3 | 51 | 728 | 44 | 3 | 0 | 1 | 0 | 0,9 | 0,9 | — | — | — |
| 0.7.5-P23 | #563 | pacchetto | impegnativa | codex, forte, alto | claude | Claude Opus 5.5 | alto (responsabile) | #588 | 2 | 12 | 583 | 0 | 2 | 0 | 0 | 0 | 0,7 | 5,8 | — | — | — |
| 0.7.5-P24 | #564 | pacchetto | media | codex, medio, alto | codex | GPT-6.1 sol | medio (responsabile) | #629 | 4 | 21 | 420 | 210 | 4 | 2 | 0 | 2 | 2,7 | 2,8 | — | — | — |
| 0.7.5-P25 | #565 | pacchetto | media | codex, medio, alto | codex | GPT-6.1 sol | medio (responsabile) | #632 | 3 | 18 | 723 | 528 | 3 | 0 | 2 | 0 | 2,1 | 2,1 | — | — | — |
| 0.7.5-P26 | #566 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #633 | 3 | 12 | 379 | 222 | 4 | 1 | 1 | 1 | 1,8 | 1,9 | — | — | — |
| 0.7.5-P27 | #567 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #635 | 2 | 7 | 174 | 47 | 2 | 0 | 0 | 0 | 1,2 | 1,3 | — | — | — |
| 0.7.5-P28 | #568 | pacchetto | impegnativa | claude, forte, alto | claude | Claude Opus 5.5 | alto (responsabile) | #636 | 5 | 21 | 1875 | 1294 | 5 | 1 | 2 | 1 | 1,6 | 1,9 | — | — | — |
| 0.7.5-R28b | #569 | riferimenti | media | gemini, leggero, medio | gemini | Gemini 3.8 Flash + Gemini 4 Argon | non dichiarato | #592 | 2 | 55 | 543 | 0 | 3 | 1 | 0 | 1 | 15,0 | 15,4 | — | — | — |
| 0.7.5-P28b | #570 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #637 | 2 | 18 | 1101 | 501 | 2 | 0 | 0 | 0 | 1,3 | 1,7 | — | — | — |
| 0.7.5-P29 | #571 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #638 | 5 | 22 | 1721 | 1042 | 8 | 2 | 2 | 2 | 4,1 | 4,4 | — | — | — |
| 0.7.5-P30 | #572 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #639 | 6 | 22 | 1512 | 843 | 7 | 1 | 3 | 1 | 6,8 | 7,1 | — | — | — |
| 0.7.5-P31 | #573 | pacchetto | media | codex, medio, medio | codex | GPT-6 | medio (responsabile) | #644 | 4 | 5 | 335 | 110 | 4 | 1 | 2 | 1 | 2,2 | 2,3 | — | — | — |
| 0.7.5-P32 | #574 | pacchetto | facile | gemini, leggero, basso | gemini | Gemini 3.8 Flash | non dichiarato | #606 | 2 | 1 | 69 | 0 | 2 | 0 | 0 | 0 | 1,2 | 1,2 | — | — | — |
| 0.7.5-P33 | #575 | pacchetto | facile | gemini, leggero, basso | gemini | Gemini 3.8 Flash | alto (pull request) | #645 | 1 | 2 | 954 | 0 | 1 | 0 | 0 | 0 | 0,1 | 0,3 | — | — | — |
| 0.7.5-P34 | #576 | pacchetto | impegnativa | non dichiarato | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| 0.7.5-P30b | #589 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #640 | 4 | 9 | 1088 | 594 | 7 | 1 | 4 | 1 | 3,6 | 3,8 | — | — | — |
| 0.7.5-P35 | #594 | pacchetto | facile | codex, medio, medio | codex | GPT-6 | non dichiarato | #597 | 2 | 4 | 87 | 24 | 2 | 0 | 0 | 0 | 0,7 | 0,7 | — | — | — |
| 0.7.5-P36 | #595 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #598 | 1 | 1 | 104 | 104 | 1 | 0 | 0 | 0 | 2,8 | 2,9 | — | — | — |
| 0.7.5-P37 | #602 | pacchetto | facile | claude, medio, medio | claude | Claude Opus 5.5 | non dichiarato | #605 | 2 | 2 | 21 | 12 | 2 | 0 | 0 | 0 | 2,3 | 2,3 | — | — | — |
| 0.7.5-P38 | #610 | pacchetto | media | claude, medio, alto | claude | Claude Opus 5.5 | medio (responsabile) | #612 | 1 | 3 | 37 | 29 | 2 | 0 | 1 | 0 | 3,1 | 3,3 | — | — | — |
| 0.7.5-P39 | #642 | pacchetto | media | claude, medio, medio | ? | non dichiarato | non dichiarato | — | — | — | — | — | — | — | — | — | — | — | — | — | — |

Issue senza pull request principale unita: 0.7.5-P34 (chiusa), 0.7.5-P39 (aperta).

### 7.2 Comando

Il comando che ha prodotto i dati, dalla radice del repository, e che la
misura della 0.8 rifà con `--tappe 0.8`:

```sh
scripts/misura_pacchetti.py --tappe 0.7.5 --dati misura-0.7.5 --scarica > misura-0.7.5.md
```

`--scarica` legge da GitHub (solo API REST, una pagina alla volta) e salva
tutto nella cartella `misura-0.7.5/`; senza `--scarica` lo script rifà i
calcoli da quella cartella, senza rete. Più tappe insieme si misurano con
`--tappe 0.7 0.7.5`.
