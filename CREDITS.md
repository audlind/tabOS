# Anerkjennelser og Kreditt (Credits & Attributions)

**tabOS** er bygget på prinsipper om åpenhet, fri programvare og deling av kunnskap. 
Vi retter en stor takk til følgende prosjekter, fellesskap og standarder som har gjort prosjektet mulig:

---

## 1. Skrifttyper og Tegnsett
* **IBM Code Page 437 & Code Page 865 (Nordic):**
  * Det klassiske 8x16 bitmap-skriftsettet fra IBM PC / VGA-arkitekturen.
  * Anses som en historisk standard i Public Domain (fri til allmenn bruk).
  * Tilpasset og utvidet i [font_cp437_8x16.h](file:///c:/AG-prosjekter/tabOS/include/font_cp437_8x16.h) med nordiske glyfer for `Æ`, `Ø`, og `Å`.

## 2. Allwinner & linux-sunxi Fellesskapet
* **linux-sunxi.org:**
  * En uvurderlig åpen kunnskapsbase for Allwinner sun4i/sun5i/sun7i-plattformene.
  * Deres dokumentasjon av Allwinner A13 registerkart, Display Engine (DE-BE / DE-FE), I2C-buss, AXP209 PMIC og `fex`/`script.bin` binærformat har vært essensiell for å forstå maskinvaren direkte uten proprietær dokumentasjon.

## 3. Telehack og Retro Computing
* **Telehack (`telehack.com`):**
  * Skapt og vedlikeholdt av **forby** som en åpen, historisk simulering av ARPANET og tidlige UNIX-/BBS-systemer.
  * Takk for at de holder arven fra 1970- og 80-tallets datanettverk levende og fritt tilgjengelig for alle over telnet!

## 4. Åpen Kildekode og Verktøy
* **Zig Software Foundation:**
  * Utviklerne av [Zig](https://ziglang.org/), en fantastisk moderne C/C++ kompilator og toolchain som gjør krysskompilering til ARMv7 (`arm-linux-musleabihf`) lekende lett uten behov for kompliserte sysrooter eller tunge VM-oppsett.
* **Musl libc:**
  * Et rent, lettvekts og uavhengig standard C-bibliotek for Linux, brukt for å generere tabOS' statiske, selvstendige binærfiler.
* **Linux Input Subsystem (evdev):**
  * Standarden for `/dev/input/event*` hendelseshåndtering i Linux-kjernen, utviklet under GPLv2 av Linux-fellesskapet.

---

## 5. Egenutviklet Kode og Innhold
* **Kjerne og applikasjoner:**
  * Hele operativsystemkjernen ([src/main.c](file:///c:/AG-prosjekter/tabOS/src/main.c), [src/os.c](file:///c:/AG-prosjekter/tabOS/src/os.c), [src/display.c](file:///c:/AG-prosjekter/tabOS/src/display.c), [src/input.c](file:///c:/AG-prosjekter/tabOS/src/input.c), [src/audio.c](file:///c:/AG-prosjekter/tabOS/src/audio.c)) og alle apper ([app_bbs.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_bbs.c), [app_snake.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_snake.c), [app_keyboard.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_keyboard.c), [app_touchtest.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_touchtest.c), [app_colortest.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_colortest.c), [app_launcher.c](file:///c:/AG-prosjekter/tabOS/src/apps/app_launcher.c)) er skrevet 100% fra bunnen av i C99.
* **Lydeffekter:**
  * Alle retro-lydeffekter (`eat.wav`, `crash.wav`, `start.wav` og [sounds.h](file:///c:/AG-prosjekter/tabOS/include/sounds.h)) er matematisk syntetisert fra bunnen av ved hjelp av enkle firkantbølger, støy og frekvensmodulering.
* **ASCII-grafikk:**
  * Logo og grensesnittdesign er håndlaget for tabOS.
