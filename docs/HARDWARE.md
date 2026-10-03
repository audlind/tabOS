# Allwinner A13 (sun5i) Maskinvarespesifikasjoner og Teardown

Dette dokumentet inneholder all teknisk informasjon, registeradresser, maskinvarekomponenter og konfigurasjonsdetaljer avdekket under utviklingen av **tabOS** på et 7-tommers Allwinner A13-nettbrett (Q88 / For-Fun D70A hovedkort).

---

## 1. System on Chip (SoC) & Prosessor

* **SoC:** Allwinner A13 (sun5i arkitekturfamilie)
* **Kjerne:** 1x ARM Cortex-A8 @ 1008 MHz (1.008 GHz)
* **Instruksjonssett:** ARMv7-A, Thumb-2, VFPv3-D32 (Hardware flyttall, 32 registre), NEON SIMD
* **Hurtigbuffer (Cache):**
  * L1 Instruksjonscache: 32 KB
  * L1 Datacache: 32 KB
  * L2 Cache: 256 KB
* **BogoMIPS:** 1001.88
* **Prosess:** 55nm CMOS

---

## 2. Minne og Lagring

* **RAM:** 512 MB DDR3 SDRAM
  * Frekvens: 408 MHz
  * Minnekanal: 16-bit bussbredde
* **NAND Flash:** 4 GB Hynix / Micron NAND Flash
  * Blokkenheter: `/dev/block/nand*`
  * `/dev/block/nandc`: Bootloader / Kjerne (boot.img)
  * `/dev/block/nandd`: Systempartisjon (ext4, ~512 MB)
  * `/dev/block/nande`: Datapartisjon (ext4, ~2 GB)
  * `/dev/block/nandh`: Cachepartisjon (ext4, ~256 MB)

---

## 3. Skjerm og Grafikk-kontroller

* **LCD Panel:** 7.0" TFT LCD
* **Oppløsning:** 800 x 480 piksler (WVGA, 5:3 / 16:9 bredskjerm)
* **Fargedybde:** 32-bit ARGB8888 (4 bytes per piksel)
* **Skjermkontroller:** Allwinner Display Engine (DE-BE / Back-End og DE-FE / Front-End)
* **Framebuffer Node:** `/dev/graphics/fb0`
* **Bufferstørrelse:**
  * Enkelt ramme: $800 \times 480 \times 4 = 1\,536\,000$ bytes (~1.46 MB)
  * Dobbelt-buffer allokert i VRAM: $3\,072\,000$ bytes (~2.93 MB)
* **Minnekartlegging (mmap):**
  * Direkte `mmap()` med `PROT_READ | PROT_WRITE` og `MAP_SHARED`
  * 0 kopier, 0 latency, direkte skriving til LCD-driverbrikkens minne
* **GPU:** ARM Mali-400 MP1 (støtter OpenGL ES 2.0 / 1.1)

---

## 4. Berøringsskjerm (Touch Digitizer)

* **Kontrollerbrikke:** Zet6221 Kapasitiv Multi-Touch IC (I2C-buss)
* **Kjernemodul:** `zet6221.ko`
* **Linux Input Node:** `/dev/input/event2`
* **Maskinvareoppløsning (Digitizer Range):**
  * Horisontal akse (X): $0 \dots 960$
  * Vertikal akse (Y): $0 \dots 640$
* **Protokoll:** Linux Input Event Subsystem (Multi-Touch Type B):
  * `EV_ABS (0x03)`, Kode `0x35` (`ABS_MT_POSITION_X`): X-posisjon ($0 \dots 960$)
  * `EV_ABS (0x03)`, Kode `0x36` (`ABS_MT_POSITION_Y`): Y-posisjon ($0 \dots 640$)
  * `EV_ABS (0x03)`, Kode `0x30` (`ABS_MT_TOUCH_MAJOR`): Berøringstrykk/fingerflate ($> 0$ betyr finger nede, $0$ betyr sluppet)
  * `EV_KEY (0x01)`, Kode `0x14A` (`BTN_TOUCH`): Digital berøringsflagg ($1 / 0$)
  * `EV_SYN (0x00)`, Kode `SYN_REPORT (0x00)`: Atomisk oppdatering av berøringsramme
* **Pikseltransformasjon:**
  $$\text{LCD}_X = \frac{\text{HW}_X \times 800}{960}$$
  $$\text{LCD}_Y = \frac{\text{HW}_Y \times 480}{640}$$

---

## 5. Lydmaskinvare og Høyttaler

* **Integrert Audio Codec:** Allwinner `sun5i-CODEC`
  * Innebygd 24-bit stereo DAC og ADC
  * Støtter samplingsfrekvenser: 8 kHz til 192 kHz
* **Høyttalerforsterker (PA):** Ekstern Klasse-D forsterkerchip styrt via GPIO `PG10`
  * Kontrollnode: `/dev/pa_dev`
  * Setter logisk `1` for å aktivere høyttaleren og unngå strømforbruk i dvale
* **ALSA Enheter:**
  * Lydkort 0: `sun5i-CODEC` (`/dev/snd/pcmC0D0p` for avspilling, `pcmC0D0c` for mikrofon)
* **tabOS Lydmotor:**
  * Genererer 8-bit retro synthesizer-lyder (44.1 kHz WAV)
  * Kjøres asynkront via lettvekts native underprosess uten å blokkere touch eller framerate

---

## 6. Trådløst Nettverk (Wi-Fi)

* **Wi-Fi Brikke:** Realtek RTL8188eu (USB 2.0 internt grensesnitt)
* **Kjernemodul:** `8188eu.ko`
* **Nettverksgrensesnitt:** `wlan0`
* **Protokoller:** 802.11b/g/n (opptil 150 Mbps, 2.4 GHz)
* **Kryptering:** WPA / WPA2-PSK (CCMP/AES)
* **Nettverksdaemoner:**
  * `wpa_supplicant` for autentisering og tilkobling mot aksesspunkt
  * `dhcpcd` for automatisk tildeling av IP-adresse, ruting og gateway
* **DNS:** Google Public DNS (`8.8.8.8`) og Cloudflare (`1.1.1.1`) i `/etc/resolv.conf`

---

## 7. Strømstyring (PMIC) og Batteri

* **PMIC (Power Management IC):** X-Powers AXP209
  * Koblet over I2C-buss til SoC
  * Styrer DC-DC konvertere, LDO-spenninger til CPU-kjerne, VDD-SYS, VDD-DLL og skjerm
  * Innebygd 12-bit ADC for spenning, strømtrekk og temperatur
* **Batteri:** 3.7V Li-Po (2500–3000 mAh)
* **Telemetrinoder i Linux:**
  * `/sys/class/power_supply/battery/capacity`: Prosentandel (0–100%)
  * `/sys/class/power_supply/battery/voltage_now`: Millivolt
  * `/sys/class/power_supply/battery/status`: Lader / Utlader / Full

---

## 8. FEX / Script.bin Maskinvarekonfigurasjon

Allwinner sunxi-plattformen bruker en spesiell binær maskinvarebeskrivelse kalt `script.bin` (tilsvarer Device Tree / DTS i nyere kjerner). Vi har dekompilert denne til `sys_config.fex` i prosjektet:

* **TFT LCD Pinner:** `PD0` til `PD27` (24-bit parallell RGB-grensesnitt)
* **Touch I2C Pinner:** `TWI2` (SCL: `PB0`, SDA: `PB1`)
* **Touch Interrupt Pinne:** `EINT11` (GPIO `PG11`)
* **Audio PA Enable:** `PG10`
* **Wi-Fi Power Enable:** `PG12`
