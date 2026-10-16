Closes #<!-- numero della issue del pacchetto o del bug -->

**Tappa e pacchetto:** <!-- es. 0.2 / P4, oppure "correzione bug" -->

## Cosa cambia

<!-- Due o tre frasi. -->

## Verifiche

- [ ] La issue collegata mi è stata assegnata
- [ ] La PR realizza un solo pacchetto, senza modifiche estranee
- [ ] Ho toccato solo i file elencati nel pacchetto (o ho spiegato qui perché no)
- [ ] Ogni condizione "Fatto quando" del pacchetto è verificata
- [ ] Rispetta gli ADR (nessuna nuova dipendenza, nessun Qt fuori da `app`)
- [ ] Test richiesti presenti e funzionanti
- [ ] `clang-format` applicato, nessun avviso del compilatore né di `clang-tidy`
- [ ] Commit firmati (`git commit -s`) e in formato Conventional Commits

## Uso dell'IA

- [ ] Nessuna IA usata
- [ ] IA usata: <!-- strumento --> — ho letto e capito ogni riga, ne rispondo io,
      e i commit riportano `Assisted-by:`
