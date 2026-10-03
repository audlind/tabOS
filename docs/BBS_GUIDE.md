# tabOS Telehack BBS Brukerveiledning

**Telehack** (`telehack.com:23`) er en nettbasert simulering av ARPANET, tidlige BBS-systemer og UNIX-systemer fra slutten av 1970-tallet og 80-tallet. 

Med **tabOS** kan du koble deg direkte opp til denne retroverdenen fra ditt Allwinner A13-nettbrett over Wi-Fi!

---

## 1. Trenger jeg en brukerkonto?

**Nei!** 
Du trenger ikke å opprette en bruker på forhånd. Når du åpner **`[1] TELEHACK BBS`** i hovedmenyen på nettbrettet, slipper systemet deg rett inn som gjest i et åpent kommandolinje-skall.

Dersom du senere vil lagre egne innstillinger, motta e-post eller ha et eget filområde, kan du opprette en konto når som helst ved å skrive:
```text
newuser
```
i terminalen og følge instruksjonene.

---

## 2. Populære kommandoer å prøve

tabOS har lagt til egne **hurtigknapper** på skjermen for de mest populære kommandoene, men du kan også skrive dem med touch-tastaturet:

| Kommando | Beskrivelse | Hurtigknapp |
|---|---|---|
| `help` | Viser en detaljert liste over alle tilgjengelige programmer | **`[HELP]`** |
| `starwars` | Viser hele filmen *Star Wars: Episode IV* animert i levende ASCII! | **`[STARWARS]`** |
| `zork` | Start det legendariske teksteventyret *Zork: The Great Underground Empire* | **`[ZORK]`** |
| `weather oslo` | Viser værmelding for Oslo (eller hvilken som helst annen by) i ASCII-grafikk | **`[WEATHER]`** |
| `eliza` | Snakk med den legendariske AI-psykoterapeuten fra 1966 | **`[ELIZA]`** |
| `cowsay [tekst]` | Lar en ASCII-ku uttale teksten din | Tastaturet |
| `figlet [tekst]` | Lager store ASCII-bokstaver av teksten din | Tastaturet |
| `cal` | Viser en retro kalender for inneværende måned | Tastaturet |
| `clock` | Viser en sanntids retro ASCII-klokke | Tastaturet |
| `minesweeper` | Spill minesveiper i terminalen | Tastaturet |
| `pong` | Spill retro Pong | Tastaturet |
| `finger` | Se hvem andre i verden som er logget på Telehack akkurat nå | Tastaturet |

---

## 3. Touch-kontroller og Spesialtaster

* **`^C` (Ctrl-C):** Avbryter et kjørende program (f.eks. Star Wars-filmen eller en loop).
* **`SLETT`:** Fungerer som Backspace for å rette opp skrivefeil.
* **`ENTER`:** Sender kommandoen til Telehack-serveren.
* **`KOBL-TIL`:** Kobler opp TCP-socketen på nytt dersom du mistet forbindelsen.
* **`[X RETUR]`:** Returnerer umiddelbart til tabOS hovedmeny.
