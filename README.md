# 🎵 ESP32 Dual-Core Digital Audio Player V2

Version 2 of my custom **Digital Audio Player (DAP)** built using **two ESP32 microcontrollers** for improved performance and modularity.

Unlike the first version where a single ESP32 handled both the user interface and audio playback, this version separates responsibilities across two dedicated processors.

- **ESP32-C3 SuperMini** handles the OLED display, button inputs, menus, and user interaction.
- **ESP32-S3 Zero** manages SD card access, audio decoding, WAV playback, and I2S audio output.

The two controllers communicate through a custom UART command protocol, creating a responsive and modular embedded music player.

---

# ✨ Features

- 🎵 WAV playback from SD Card
- 🖥 OLED-based graphical interface
- 🎮 Four-button navigation
- 📂 Automatic music library scanning
- ⏯ Play / Pause
- ⏭ Next / Previous Track
- 🔀 Shuffle Mode
- 🔊 Adjustable Volume
- 🎚 9-Band Equalizer Interface
- 💾 EEPROM settings storage
- 📃 Automatic playlist generation
- 📡 UART communication between two ESP32 boards
- ⚡ Smooth audio playback independent of UI updates
- 🔄 Automatic next-song playback
- 🟢 Playback status indication using LED

---

# 🛠 Hardware Used

## Controller Board

- ESP32-C3 SuperMini

Responsible for:

- OLED display
- Menu system
- Button handling
- User settings
- UART communication

---

## Audio Board

- ESP32-S3 Zero

Responsible for:

- SD card management
- WAV decoding
- Audio streaming
- I2S output
- Track management
- Playback state

---

## Additional Hardware

- SH1106 SPI OLED Display
- MicroSD Card Module
- MAX98357A / PCM5102 I2S DAC
- Speaker / Headphones
- 4 Push Buttons
- Status LED

---

# 🏗 System Architecture

```
          ┌────────────────────────┐
          │ ESP32-C3 SuperMini     │
          │------------------------│
          │ OLED Display           │
          │ Menu System            │
          │ Button Inputs          │
          │ EEPROM Settings        │
          └──────────┬─────────────┘
                     │ UART
                     │
          ┌──────────▼─────────────┐
          │ ESP32-S3 Zero          │
          │------------------------│
          │ SD Card                │
          │ WAV Decoder            │
          │ Audio Playback         │
          │ I2S Output             │
          └──────────┬─────────────┘
                     │
               I2S Audio DAC
                     │
                  Speaker
```

---

# 📡 UART Communication

The two ESP32 boards communicate using a custom serial protocol.

## Commands Sent from ESP32-C3

```
PLAY,<track>
PAUSE
RESUME
STOP
NEXT
PREV
VOL,<0-100>
SHUFFLE,<0/1>
LIST
```

## Responses from ESP32-S3

```
STATE,playing
STATE,paused
STATE,stopped

TRACK,index,path

LISTSTART
SONG,index,path
LISTEND

ERROR,SD
ERROR,FILE
```

This architecture allows the UI to remain responsive while audio playback continues uninterrupted.

---

# 📂 Music Library

The player automatically scans

```
/music
```

on the SD card.

Example:

```
SD Card

music/
    Song1.wav
    Song2.wav
    Song3.wav
```

Supported format:

- WAV

---

# 🖥 User Interface

## Main Menu

- Songs List
- Volume
- Equalizer
- Settings

---

## Song Browser

- Scroll through songs
- Wrapped filenames
- Highlights currently playing song

---

## Volume

- Adjustable from 0–100%
- Saved automatically

---

## Equalizer Interface

Includes controls for:

- 62 Hz
- 125 Hz
- 250 Hz
- 500 Hz
- 1 kHz
- 2 kHz
- 4 kHz
- 8 kHz
- 16 kHz

---

## Settings

- Shuffle Mode
- Stored in EEPROM

---

# 🎮 Controls

| Button | Function |
|---------|----------|
| UP | Navigate Up / Increase Volume / Increase EQ |
| DOWN | Navigate Down / Decrease Volume / Decrease EQ |
| RIGHT | Select / Play / Pause |
| LEFT | Back |

---

# 📌 Hardware Responsibilities

## ESP32-C3 SuperMini

- OLED Graphics
- Button Debouncing
- Menu Navigation
- Volume Control
- EEPROM
- UART Master

---

## ESP32-S3 Zero

- SD Card Access
- Song Library
- Audio Streaming
- WAV Decoding
- I2S Audio Output
- UART Audio Engine

---

# 📚 Libraries Used

- SPI
- SD
- EEPROM
- Adafruit GFX
- Adafruit SH110X
- ESP8266Audio

---

# 🚀 Improvements over Version 1

- Dual ESP32 architecture
- Better task separation
- Dedicated audio processor
- Faster UI response
- Modular firmware
- UART command protocol
- Improved SD card handling
- Automatic playlist synchronization
- More reliable playback
- Easier future expansion

---

# 🔮 Future Improvements

- MP3 support
- FLAC support
- Bluetooth Audio
- USB-C File Transfer
- Real DSP Equalizer
- Album Artwork
- Battery Management System
- Repeat Modes
- Sleep Timer
- Playlist Support
- Metadata (Artist, Album)
- Battery Indicator
- Touch Interface

---

# 📸 Project Images

Include images of:

- Front View
- Internal Wiring
- ESP32-C3 Board
- ESP32-S3 Board
- OLED Menu
- Audio Playback Screen
- PCB
- Finished Enclosure

---

# 🤝 Contributions

Contributions and suggestions are always welcome.

Feel free to fork the project and submit pull requests.

---

# 📜 License

This project is released under the MIT License.

---

# 👨‍💻 Author

Designed and developed as the second generation of a custom embedded Digital Audio Player (DAP), featuring a dual-microcontroller architecture that separates the user interface from the audio engine for improved performance, responsiveness, and modularity.
