# Lavoro in parallelo

Come far lavorare più persone o più agenti IA sulla stessa tappa senza che si
intralcino. Il principio è uno: **GitHub è l'unico registro di chi fa cosa**, e
solo il responsabile assegna i pacchetti e unisce le pull request.

## Come leggere i riferimenti

GitHub usa **una sola numerazione** per issue e pull request: ogni elemento nuovo,
qualunque sia il tipo, prende il numero successivo. Per questo un numero può
indicare una issue o una pull request.

| Scrittura | Significato | Dove si trova |
|---|---|---|
| `[0.2-P4]` | tappa 0.2, pacchetto P4 | titolo della issue del pacchetto |
| `#5` | issue o pull request numero 5 | scheda **Issues** o **Pull requests** |
| `Closes #5` | la pull request, quando viene unita, chiude la issue 5 | descrizione della pull request |
| `p0.2-04-…` | ramo su cui si lavora al pacchetto P4 della tappa 0.2 | scheda **Branches** |

Il documento della tappa (`docs/tappe/…`) riporta per ogni pacchetto il numero
della sua issue; ogni pull request riporta in alto la issue che chiude.

## 1. Scegliere i pacchetti da far partire

Lo fa il responsabile. Un pacchetto si può far partire solo se:

1. **è libero**: la sua issue è aperta e non ha un'etichetta `agente:…`;
2. **le sue dipendenze sono unite** in `main` (riga "Dipende da" nel piano della
   tappa);
3. **non tocca gli stessi file** di un pacchetto già in corso (riga "File" nel
   piano della tappa).

Due o tre pacchetti in parallelo sono il massimo sensato: ogni pull request va
letta dal responsabile.

## 2. Assegnare

Per ogni pacchetto scelto, nella sua issue:

1. si aggiunge l'etichetta dell'agente: `agente:claude`, `agente:claude-2`,
   `agente:codex` (o il nome GitHub della persona, come *Assignee*);
2. si scrive un commento: "Assegnato a …".

Le etichette servono perché gli agenti lavorano tutti attraverso l'account
GitHub del responsabile, e non si possono distinguere in altro modo. La lista
delle issue filtrata per etichetta mostra in ogni momento chi sta facendo cosa.

## 3. Avviare l'agente

A ogni agente si dà sempre lo stesso messaggio, cambiando solo pacchetto, issue e
ramo:

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Realizza solo il pacchetto P4 della tappa 0.2 (issue #5), che è
assegnato a te. Per prima cosa prendi in carico la issue #5 con l'etichetta
AA-Presa in carico. Lavora sul ramo p0.2-04-coordinate-parser, partendo da main
aggiornato. Non toccare file fuori da quelli elencati nel pacchetto. Alla fine
apri una pull request che chiuda la issue #5 e compila il modello della pull
request, compresa la sezione sull'uso dell'IA.
```

Mai dare compiti generici ("continua il progetto", "sistema quello che serve"):
un agente senza pacchetto preciso sconfina.

## 4. Mentre lavorano

- Ogni agente lavora **solo sul suo ramo** e apre **una sola pull request**.
- Nessuno scrive su `main` o sui rami degli altri.
- Se un agente scopre che gli serve un file non previsto, si ferma e lo scrive
  nella issue: decide il responsabile.
- Chi prende in carico una issue (per realizzarla) o una pull request già aperta
  (per rivederla, correggerla o risolvere un conflitto) le mette **per prima
  cosa** l'etichetta `AA-Presa in carico` con un commento che dice chi è, e la
  toglie quando ha finito. Una issue o una pull request con quell'etichetta non
  si tocca: ci sta già lavorando qualcun altro. I comandi sono in
  [AGENTS.md](../AGENTS.md).

## 5. Unire

- Il responsabile unisce **una pull request alla volta**, con *Squash and merge*,
  solo con la CI verde.
- Dopo ogni merge, le altre pull request aperte mostrano il pulsante **Update
  branch**: si preme, la pull request si allinea a `main` e la CI riparte.
- Se due pacchetti toccano lo stesso file comune (per esempio
  `tests/CMakeLists.txt`), vale una regola fissa: **chi arriva dopo si aggiorna**.
  Si chiede all'agente di quella pull request di risolvere il conflitto.
- Quando possibile, una pull request scritta da un modello viene rivista da un
  modello diverso prima del merge (per esempio Codex rivede Claude e viceversa).
  La decisione resta del responsabile.

## 6. Chiudere

Con il merge la issue si chiude da sola (`Closes #…`). Il responsabile toglie
l'etichetta `agente:…` solo se il pacchetto viene abbandonato senza pull request:
dopo 14 giorni senza notizie il pacchetto torna libero (CONTRIBUTING.md).
