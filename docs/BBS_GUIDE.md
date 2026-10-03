# tabOS Online BBS Terminal Brukerveiledning

tabOS forvandler Allwinner A13-nettbrettet til et ekte, bærbart **Cyberdeck** med direkte tilkobling til tre legendariske retro-BBS-systemer over Wi-Fi.

---

## 1. De Tre Innebygde BBS-Nodene

Du kan starte dem direkte fra tabOS hovedmeny, eller bytte sømløst mellom dem ved å trykke på den lilla knappen **`[BYTT BBS]`** i terminalens topplinje.

### Node 1: TELEHACK ARPANET BBS (`telehack.com:23`)
* **Hva er det?** En autentisk, levende simulering av ARPANET og 1970-/80-tallets UNIX-systemer med over 26 000 noder.
* **Innlogging:** Ingen konto nødvendig! Alle slipper direkte inn som gjester i et interaktivt shell.
* **Populære programmer:**
  * `help`: Viser liste over alle verktøy og spill.
  * `starwars`: Viser hele Star Wars: Episode IV animert i full ASCII-film!
  * `zork`: Det klassiske teksteventyret fra Infocom.
  * `eliza`: Den originale AI-psykologen fra MIT (1966).
  * `weather [by]`: Værmelding i retro-grafikk.
  * `newuser`: Opprett en permanent konto med egen e-postkasse.

### Node 2: VERTRAUEN SYNCHRONET BBS (`vert.synchro.net:23`)
* **Hva er det?** Det offisielle "moderskipet" for **Synchronet BBS**-programvaren, opprinnelig skapt av Rob Swindell (Digital Man) tidlig på 1990-tallet og kontinuerlig i drift siden!
* **Innhold:**
  * Fulle ANSI-fargemenyer og CP437 grafikk.
  * Legendariske Door-spill som *LORD (Legend of the Red Dragon)* og *TradeWars 2002*.
  * Internasjonale meldingsnettverk som **DOVE-Net** og **FidoNet**.
* **Innlogging:** Trykk på hurtigknappen **`[GUEST]`** for å logge inn som gjest, eller **`[NEW]`** for å opprette din egen BBS-bruker.

### Node 3: THE TITANTIC RETRO BBS (`ttb.rgbbs.info:23`)
* **Hva er det?** En klassisk, stemningsfull dial-up style BBS dedikert til 90-tallets BBS-kultur, retro-filer, ASCII/ANSI-art og diskusjonsfora.
* **Innlogging:** Trykk **`[GUEST]`** eller skriv inn et kallenavn/alias ved velkomstskjermen.

---

## 2. Touch-kontroller og Hurtigmakroer

Terminalen har en smart makrolinje rett over tastaturet som automatisk tilpasser seg hvilken BBS du er koblet til:

### Når du er på Telehack:
* **`[HELP]`**: Sender `help` + Enter.
* **`[ZORK]`**: Starter Zork-teksteventyret.
* **`[STARWARS]`**: Starter ASCII Star Wars-filmen.
* **`[WEATHER]`**: Henter værmelding.
* **`[ELIZA]`**: Starter samtalen med Eliza.
* **`[USER]`**: Kaller opp brukersystemet.

### Når du er på Vertrauen eller Titantic:
* **`[GUEST]`**: Sender `guest` + Enter for umiddelbar gjesteinnlogging.
* **`[NEW]`**: Starter veiviseren for å opprette ny brukerkonto.
* **`[YES / JA]`**: Svarer `Y` på spørsmål (f.eks. "Vil du ha fargegrafikk?").
* **`[NO / NEI]`**: Svarer `N` på spørsmål.
* **`[LOGOFF]`**: Logger ut ryddig fra noden.
* **`[ENTER]`**: Sender vognretur/bekreftelse.

---

## 3. Spesialfunksjoner

* **`[BYTT BBS]`:** Bytter umiddelbart mellom Telehack $\to$ Vertrauen $\to$ Titantic uten at du trenger å gå tilbake til hovedmenyen.
* **`^C` (Ctrl-C):** Sender ASCII ETX (`0x03`) for å avbryte kommandoer eller animasjoner.
* **`KOBL-TIL`:** Gjenoppretter forbindelsen hvis serveren kobler fra eller oppkoblingen får tidsavbrudd.
* **Automatisk Telnet NAWS (Window Size):** tabOS forhandler automatisk vindusstørrelse med serverne ($60 \times 32$ i portrett, $100 \times 20$ i landskap), slik at menyene tilpasses skjermen perfekt!
