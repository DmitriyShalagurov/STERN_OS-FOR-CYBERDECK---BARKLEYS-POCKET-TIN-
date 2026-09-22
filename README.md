# STERN OS — Pocket Cyberdeck (v2.0 Extended)

### A DIY Pocket Cyberdeck built inside a Barkleys tin
**Multi-Chip • Multi-Display • Linux POSIX Shell • Game Boy & NES & CHIP-8 Emulators • Cyberdeck Desktop**

* **Schematic & PCB (EasyEDA):** https://oshwlab.com/dmitriyshalagurov/project_itafwezn
* **Build Video:** https://youtu.be/7yzCkS4Cg1M
* **Build Video (FULL VERSION):** https://youtu.be/xNvaoZqZ404

---

## 🚀 Was ist neu? (Updates & Erweiterungen v2.0)

Dieses Release erweitert das ursprüngliche STERN OS um umfassende Multi-Hardware-Kompatibilität, ein Linux-artiges Betriebssystem-Gefühl, zusätzliche Emulatoren und eine moderne Desktop-Oberfläche:

| Feature-Bereich | Neuerungen & Updates |
| :--- | :--- |
| **Multi-Chip Support** | Native Unterstützung für **ESP32-WROOM**, **ESP32-WROVER** (mit PSRAM-Erkennung), **ESP32-S3** (Xtensa LX7 & USB-CDC) und **ESP32-C3** (RISC-V). |
| **Multi-Display Support** | Dynamisches Display-Layout für **ST7789** (320×240 / 240×240), **ST7735** (128×160 & 128×128), **ILI9341** (320×240) und **SSD1306** (128×64 OLED). Keine starren Hardcoded-Koordinaten mehr! |
| **Cyberdeck Desktop UI** | Grafischer Desktop mit 3×3 App-Grid und Pixel-Icons (**startet automatisch beim Booten**). Nahtlose Rückkehr aus dem Terminal über `exit`, `desktop`, `startx` oder `ESC`. |
| **Linux POSIX Shell** | Vollwertiges Terminal mit Verzeichnis-Navigation (`cd`, `pwd`, `ls`), Dateimanipulation (`cat`, `touch`, `mkdir`, `rm`, `cp`, `mv`), System-Monitoring (`uname`, `uptime`, `free`, `ps`), Stream-Redirection (`echo "..." > file.txt`), Shell-Skripten (`sh`) und Netzwerk-Tools (`ifconfig`, `scan`, `wifi connect`, `ping`, `wget`, `curl`). |
| **Multi-Emulator Framework** | Modulares Emulator-Subsystem mit einheitlicher Steuerung:<br>• **NES** (Nintendo Entertainment System)<br>• **Game Boy (DMG)** via optimiertem Peanut-GB Core mit Scanline-Rendering<br>• **CHIP-8 / SuperChip** mit vorinstallierten ROMs (Pong, Brix) direkt im Flash. |
| **DRY & Flash-Optimierung** | Stark reduzierte Code-Duplizierung durch zentrale Komponenten (`ui_list`, `storage_hal`, `emulator_interface`). Eigene **No-OTA Partitionstabellen** (4MB, 8MB, 16MB) für maximalen ROM- und App-Speicherplatz sowie `-Os`-Kompilierung. |

---

## 🖥️ Cyberdeck Desktop Environment

Nach der Boot-Animation startet STERN OS automatisch in den grafischen **Cyberdeck Desktop**.

```
+---------------------------------------------------------+
| [STERN OS]  WiFi: [OK]  BAT: 3.9V  RAM: 142KB  12:00    | <- Status Bar
+---------------------------------------------------------+
|                                                         |
|   [ >_ ]          [ [DIR] ]         [ [TXT] ]           |
|  Terminal           Files            Editor             |
|                                                         |
|   [ [PAD] ]       [ [DINO] ]        [ [LIFE] ]          |
|  Emulators          Dino              Life              |
|                                                         |
|   [ ((o)) ]       [ [SET] ]         [ [INFO] ]          |
|    Wi-Fi          Settings          Sys Info            |
|                                                         |
+---------------------------------------------------------+
| [NAV] Pfeiltasten / TAB   [OK] Enter   [T] Terminal     | <- Bottom Bar
+---------------------------------------------------------+
```

### Desktop-Navigation:
* **Pfeiltasten / TAB:** Zwischen den Apps navigieren.
* **ENTER:** Gewählte App starten.
* **T:** Direkt in das Linux-Terminal springen.
* **ESC / `exit` / `desktop` / `startx`:** Aus dem Terminal oder Apps jederzeit sofort zurück auf den Desktop.

---

## 🐚 Linux POSIX Terminal & Shell

Das Terminal bietet eine interaktive Shell mit dem Prompt `root@stern:~# ` und automatischer Pfadanzeige.

### Unterstützte Befehle:

#### Dateisystem & Navigation
* `pwd` — Zeigt das aktuelle Arbeitsverzeichnis an.
* `cd <dir>` — Verzeichnis wechseln (unterstützt `..`, `/` und relative Pfade).
* `ls [-l]` — Verzeichnisinhalt auflisten (Dateigrößen und Typen).
* `cat <file>` — Dateiinhalt auf dem Bildschirm ausgeben.
* `touch <file>` — Leere Datei erstellen oder Zeitstempel aktualisieren.
* `mkdir <dir>` — Neues Verzeichnis anlegen.
* `rm <file>` — Datei löschen.
* `rmdir <dir>` — Verzeichnis löschen.
* `cp <src> <dst>` — Datei kopieren.
* `mv <src> <dst>` — Datei verschieben oder umbenennen.
* `grep <muster> <file>` — Nach Zeichenketten in Dateien suchen.
* `head [-n N] <file>` — Erste Zeilen einer Datei anzeigen.
* `df [-h]` — Speicherplatzbelegung von SD-Karte und internem SPIFFS-Flash anzeigen.

#### System & Status
* `uname [-a]` — Systemname, Kernel-Version, Prozessor-Architektur und CPU-Frequenz.
* `uptime` — Betriebsdauer seit dem letzten Einschalten.
* `free [-h]` — Verfügbarer und belegter Heap-RAM sowie PSRAM.
* `ps` / `top` — Liste aktiver Tasks, Stack-High-Water-Mark und CPU-Laufzeit.
* `whoami` — Aktueller Benutzer (`root`).
* `date` — Aktuelle Uhrzeit und Datum.
* `clear` — Bildschirm leeren.
* `reboot` — ESP32 neu starten.

#### Netzwerk & Web
* `ifconfig` — WLAN-Status, IP-Adresse, Subnetzmaske, Gateway und MAC-Adresse.
* `scan` — Verfügbare WLAN-Netzwerke in der Umgebung auflisten.
* `wifi connect <ssid> <pass>` — Mit einem WLAN-Netzwerk verbinden.
* `ping <host>` — Erreichbarkeit eines Hosts im Netzwerk prüfen.
* `wget <url> [ziel]` — Datei aus dem Web herunterladen und speichern.
* `curl <url>` — HTTP(S)-GET-Anfrage senden und Antwort im Terminal anzeigen.

#### Skripte, Redirection & Hilfen
* `echo "text" > datei.txt` — Text direkt in eine Datei umleiten/schreiben.
* `sh <skript.sh>` — Ausführen von Shell-Skripten Zeile für Zeile.
* `help` oder `man <befehl>` — Hilfe und Dokumentation zu Befehlen anzeigen.
* `desktop`, `startx`, `exit` — Zurück zur grafischen Desktop-Oberfläche.

---

## 🎮 Multi-Emulator Framework

STERN OS besitzt eine modulare Emulator-Architektur (`IEmulator`), die minimale RAM- und Flash-Ressourcen verbraucht:

### Verfügbare Emulatoren:
1. **NES (Nintendo Entertainment System)**:
   * Volle CPU- und PPU-Emulation mit Sound.
   * Lädt `.nes`-ROMs von SD-Karte oder SPIFFS.
   * Aufruf: Dateibrowser auswählen oder im Terminal: `nes /sd/roms/mario.nes`
2. **Game Boy (DMG / Peanut-GB)**:
   * Hohe Performance mit Scanline-basiertem Rendern.
   * Unterstützt ROM-Banking (MBC1, MBC2, MBC3, MBC5) und nutzt PSRAM oder optimierten Heap.
   * Aufruf: Dateibrowser auswählen oder im Terminal: `gb /sd/roms/tetris.gb`
3. **CHIP-8 / SuperChip**:
   * Eigene extrem speichereffiziente Implementierung (< 8 KB Code).
   * **Integrierte Flash-ROMs:** *Pong* und *Brix* sind fest in der Firmware integriert und laufen auch ohne SD-Karte!
   * Unterstützt externe `.ch8`-ROMs.
   * Aufruf: `chip8` (öffnet Auswahl der integrierten Spiele) oder `chip8 <datei.ch8>`
4. **Universelles Spiele-Menü**:
   * Befehl: `emu` oder `games` listet alle installierten Spiele auf und startet sie.

### Einheitliche Steuerung (Keymap):
| Taste am Cyberdeck | Funktion im Spiel |
| :--- | :--- |
| **S** | D-Pad Oben |
| **X** | D-Pad Unten |
| **Z** | D-Pad Links |
| **C** | D-Pad Rechts |
| **;** | Button A |
| **?** | Button B |
| **Space (Leertaste)** | Start |
| **Enter** | Select |
| **ESC** | Spiel beenden & zurück zum Menü |

---

## ⚡ Hardware & Multi-Chip Pinout

Die gesamte Hardware- und Pin-Konfiguration ist in [`include/hardware_config.h`](file:///include/hardware_config.h) und [`include/display_config.h`](file:///include/display_config.h) zentralisiert.

### Unterstützte Chips:
| Controller | Architektur | Takt | PSRAM | Typischer Einsatz |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32-WROOM** | Xtensa LX6 Dual-Core | 240 MHz | Nein | Standard Barkleys Tin Cyberdeck |
| **ESP32-WROVER** | Xtensa LX6 Dual-Core | 240 MHz | 4–8 MB SPI | Cyberdeck mit großen Game Boy / NES ROMs |
| **ESP32-S3** | Xtensa LX7 Dual-Core | 240 MHz | 2–8 MB OPI | LilyGO T-Deck, Cardputer, moderne Custom Builds |
| **ESP32-C3** | RISC-V Single-Core | 160 MHz | Nein | Ultra-kompakte Miniatur-Geräte |

### Pin-Mapping Übersicht:

| Signal | ESP32-WROOM / WROVER | ESP32-S3 | ESP32-C3 |
| :--- | :--- | :--- | :--- |
| **TFT CS** | GPIO 15 | GPIO 10 | GPIO 7 |
| **TFT DC** | GPIO 2 | GPIO 11 | GPIO 2 |
| **TFT RST** | GPIO 4 | GPIO 12 | GPIO 3 |
| **TFT MOSI** | GPIO 23 | GPIO 13 | GPIO 6 |
| **TFT SCLK** | GPIO 18 | GPIO 14 | GPIO 4 |
| **TFT BL** | GPIO 32 | GPIO 48 | GPIO 1 |
| **SD CS** | GPIO 5 | GPIO 4 | GPIO 10 |
| **I2C SDA (TCA8418)** | GPIO 21 | GPIO 8 | GPIO 8 |
| **I2C SCL (TCA8418)** | GPIO 22 | GPIO 9 | GPIO 9 |
| **KBD INT** | GPIO 34 | GPIO 3 | GPIO 5 |
| **Speaker / Audio** | GPIO 25 (DAC) | GPIO 1 (I2S) | GPIO 0 (PWM) |

---

## 💾 Partitionen & Speicheroptimierung

Um Platz für große ROMs und Anwendungen zu schaffen, wurden benutzerdefinierte **No-OTA Partitionstabellen** erstellt (`partitions/`):

* **4 MB Flash (`partitions/no_ota_4mb.csv`):**
  * App: **3.14 MB** (statt 1.3 MB bei Standard-OTA)
  * SPIFFS / Data: **896 KB**
* **8 MB Flash (`partitions/no_ota_8mb.csv`):**
  * App: **3.5 MB**
  * SPIFFS / ROMs: **4.4 MB**
* **16 MB Flash (`partitions/no_ota_16mb.csv`):**
  * App: **4.0 MB**
  * SPIFFS / ROMs: **11.9 MB** (Platz für hunderte NES- und Game Boy-Spiele im internen Flash!)

Zusätzlich ist in allen Profilen der Compiler-Flag `-Os` aktiv, um minimale Code-Größe bei maximaler Performance zu garantieren.

---

## 🛠️ Kompilieren & Flashen (PlatformIO)

In [`platformio.ini`](file:///platformio.ini) sind vorkonfigurierte Umgebungen für alle Hardware-Kombinationen hinterlegt:

### Für das Standard-Board (ESP32-WROOM):
```bash
pio run -e esp32-wroom -t upload
```

### Für Boards mit PSRAM (ESP32-WROVER):
```bash
pio run -e esp32-wrover -t upload
```

### Für ESP32-S3 (z.B. T-Deck):
```bash
pio run -e esp32-s3 -t upload
```

### Für kompakte Displays:
```bash
# Für ST7735 (128x160):
pio run -e esp32-st7735 -t upload

# Für ILI9341 (320x240):
pio run -e esp32-ili9341 -t upload
```

---

## 📦 Stückliste (Bill of Materials)

| Komponente | Bezeichnung / Spezifikation |
| :--- | :--- |
| MCU | ESP32-WROOM / WROVER / S3 / C3 |
| USB-UART | CH340C / CP2102 |
| Keyboard Controller | TCA8418RTWR (QFN-24, I²C) |
| Tastatur-Dioden | 1N4148 SOD-323 (1 Diode pro Taste) |
| Display | 2.4" TFT SPI (ST7789, ST7735 oder ILI9341) |
| Spannungsregler | TPS63020 DC/DC Buck-Boost (3.3 V) |
| Netzschalter | SS12D11 Schiebeschalter |
| Kondensatoren | 470 µF / 10 V Elko, 10 µF 0603, 100 nF 1206 |
| FPC-Verbindung | 14-Pin FPC (0.5 mm Pitch, 10 cm Kabel) |
| Gehäuse | Barkleys Metalldose (Pocket Tin) |
| PCB-Fertigung | JLCPCB |

---

## 👥 Mitwirken & Danksagung

* **Ursprüngliches Hardware- & PCB-Design:** [Dmitriy Shalagurov](https://github.com/DmitriyShalagurov/STERN_OS-FOR-CYBERDECK---BARKLEYS-POCKET-TIN-)
* **v2.0 Multi-Chip, POSIX Shell, Emulatoren & Desktop Erweiterungen:** Custom Community Build
* Feedback, Issues und Pull Requests sind jederzeit herzlich willkommen!
