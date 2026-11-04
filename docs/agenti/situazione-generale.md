# Situazione generale

Si legge quando il messaggio del responsabile è **«SITUAZIONE GENERALE»**, in una
chat di Tecnigrafo o di solid-space. Serve al responsabile quando non sa più a che
punto sono i due progetti: chi lavora, chi aspetta che cosa e perché, che cosa deve
fare lui adesso.

## Regole

- **Sola lettura, in tutti e due i repository.** Non prendi in carico niente, non
  scrivi file, commenti, etichette o relazioni, non unisci. Fai qualcosa solo se il
  responsabile te lo chiede dopo aver letto la situazione, e allora segui le regole
  normali del repository della chat.
- Colleghi l'altro repository in sola lettura. Non serve compilare.
- Dici solo ciò che hai letto su GitHub o nei file: niente supposizioni sullo stato
  di una sessione che non si vede.

## Che cosa raccogliere

1. **Le dipendenze della tappa aperta di Tecnigrafo:** `scripts/dipendenze.py <tappa>
   --senza-project`, con `TG_SS_DIR` puntato a un clone di `main` di solid-space.
   Ready, Blocked, To fix, Information.
2. **Le pull request aperte nei due repository.** Per ciascuna: pacchetto o lavoro,
   ultimo commit, CI (verde, rossa, in corso; se è rossa, quale job e il primo
   errore), ultimo verbale di revisione e che cosa chiede, se aspetta una decisione
   del responsabile o una correzione dell'autore, se ha conflitti.
3. **Le issue con `AA-Presa in carico`:** chi ci lavora e da quando. Un'etichetta
   rimasta senza attività da ore è probabilmente di una sessione chiusa.
4. **Le issue `contratto` aperte in solid-space** e le richieste aperte: decise o
   no, e quale PR le chiude.
5. **Gli agenti fermi:** gli ultimi commenti delle issue dei pacchetti, per trovare
   chi si è fermato e perché (buco, riferimento contestato, commit del sottomodulo
   mancante, CI).
6. **Le issue pronte senza la riga «Sottomodulo di solid-space»** quando ne hanno
   bisogno: il commit è quello di merge della PR che ha chiuso la loro ultima
   dipendenza in solid-space.

## Come valutare

- **Una decisione chiesta al responsabile** si guarda prima con le regole che
  esistono: dati fuori dominio (ADR 0035, punto 23), pezzi degeneri (ADR 0036,
  punto 5), fasce (ADR 0013 di solid-space, punto 4), buchi visibili e no
  ([pacchetto.md](pacchetto.md)). Se una regola risponde, lo dici: non è una
  decisione nuova.
- **Una revisione** blocca solo per un buco visibile o una regola persa
  ([revisione.md](revisione.md)). Una garanzia si verifica con i test, non con
  dimostrazioni.
- **Un secondo giro** serve solo se la correzione cambia il contratto o una regola;
  se fa esattamente ciò che il verbale chiedeva, basta il controllo del
  responsabile.
- Se una decisione serve davvero, la proponi con la scelta che consigli e il motivo
  (AutoCAD come riferimento quando c'entra).

## Che cosa rispondere

1. **Una tabella**, una riga per lavoro aperto nei due repository: che cos'è, stato,
   che cosa aspetta (un agente, la CI, una decisione, un merge, un'altra issue).
2. **«Che cosa fai adesso»**, in ordine: prima ciò che sblocca più lavoro. Per ogni
   passo, il messaggio pronto da incollare (all'agente, nella PR come decisione, o
   il comando), e se una PR si può unire.
3. **Che cosa può partire** appena il passo è fatto, con la frase per l'agente.
4. In fondo, in una riga, le cose da tenere d'occhio che non chiedono niente adesso.

Breve e senza gergo: il responsabile legge questa risposta per uscire dalla
confusione, non per entrarci.
