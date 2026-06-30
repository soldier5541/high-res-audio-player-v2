#include <SPI.h>
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// ───── S3 wiring ─────
#define SD_CS      10
#define SD_SCK     18
#define SD_MISO     8
#define SD_MOSI     9
#define I2S_BCLK   16
#define I2S_LRC    15
#define I2S_DOUT   17

#define RXD2       44
#define TXD2       43

#define STATUS_LED 13   // optional external LED (or use 21 for onboard)

AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSD *file = nullptr;
AudioOutputI2S *out = nullptr;

String songs[50];
int totalSongs = 0, currentTrack = -1;
float volume = 0.4;
bool playing = false, paused = false, shuffleMode = false;

// ───── SD init with retries ─────
bool initSD() {
    for (int i = 0; i < 5; i++) {
        if (SD.begin(SD_CS, SPI, 4000000)) return true;
        digitalWrite(STATUS_LED, HIGH); delay(300);
        digitalWrite(STATUS_LED, LOW);  delay(300);
    }
    return false;
}

// ───── Scan /music folder ─────
int scanSongs() {
    int count = 0;
    File dir = SD.open("/music");
    if (!dir) return 0;
    while (File f = dir.openNextFile()) {
        if (!f.isDirectory()) {
            String name = f.name();
            if ((name.endsWith(".wav") || name.endsWith(".WAV")) && count < 50) {
                songs[count++] = "/music/" + name;
            }
        }
        f.close();
    }
    dir.close();
    return count;
}

void sendFullList() {
    Serial2.println("LISTSTART");
    for (int i = 0; i < totalSongs; i++)
        Serial2.printf("SONG,%d,%s\n", i, songs[i].c_str());
    Serial2.println("LISTEND");
}

void playTrack(int idx) {
    if (idx < 0 || idx >= totalSongs) return;
    if (wav) { wav->stop(); delete wav; wav = nullptr; }
    if (file) { delete file; file = nullptr; }

    file = new AudioFileSourceSD(songs[idx].c_str());
    if (!file || !file->isOpen()) {
        Serial2.println("ERROR,FILE");
        playing = false; paused = false;
        digitalWrite(STATUS_LED, LOW);
        if (!initSD()) Serial2.println("ERROR,SD");
        else { totalSongs = scanSongs(); sendFullList(); }
        return;
    }

    wav = new AudioGeneratorWAV();
    if (!wav->begin(file, out)) {
        delete wav; wav = nullptr; delete file; file = nullptr;
        Serial2.println("ERROR,FILE");
        playing = false; paused = false;
        digitalWrite(STATUS_LED, LOW);
        return;
    }

    out->SetGain(volume);
    currentTrack = idx;
    playing = true; paused = false;
    digitalWrite(STATUS_LED, HIGH);
    Serial2.printf("STATE,playing\n");
    Serial2.printf("TRACK,%d,%s\n", idx, songs[idx].c_str());
}

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(3000);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);

    if (!initSD()) {
        Serial2.println("ERROR,SD");
    } else {
        totalSongs = scanSongs();
    }

    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    out->SetGain(volume);
    randomSeed(analogRead(1));
}

void loop() {
    // 1. Audio processing
    if (playing && !paused && wav && wav->isRunning()) {
        for (int i = 0; i < 8 && wav->isRunning(); i++) wav->loop();
        if (!wav->isRunning()) {
            digitalWrite(STATUS_LED, LOW);
            if (totalSongs > 0) {
                int next = shuffleMode ? random(0, totalSongs) : (currentTrack + 1) % totalSongs;
                playTrack(next);
            } else {
                playing = false; paused = false;
                Serial2.println("STATE,stopped");
            }
        }
    } else if (out) {
        int16_t sil[2] = {0,0};
        out->ConsumeSample(sil);
    }

    // 2. UART commands
    while (Serial2.available()) {
        String cmd = Serial2.readStringUntil('\n'); cmd.trim();
        if (cmd.startsWith("PLAY,")) {
            int idx = cmd.substring(5).toInt();
            if (idx >= 0 && idx < totalSongs) playTrack(idx);
        } else if (cmd == "PAUSE") {
            if (playing) { paused = true; Serial2.println("STATE,paused"); }
        } else if (cmd == "RESUME") {
            if (playing && paused) { paused = false; Serial2.println("STATE,playing"); }
        } else if (cmd.startsWith("VOL,")) {
            volume = cmd.substring(4).toFloat() / 100.0;
            if (out) out->SetGain(volume);
            Serial2.println("OK");
        } else if (cmd == "NEXT") {
            if (totalSongs) playTrack((currentTrack + 1) % totalSongs);
        } else if (cmd == "PREV") {
            if (totalSongs) playTrack((currentTrack - 1 + totalSongs) % totalSongs);
        } else if (cmd.startsWith("SHUFFLE,")) {
            shuffleMode = (cmd.substring(8) == "1");
            Serial2.println("OK");
        } else if (cmd == "STOP") {
            if (wav) wav->stop();
            playing = false; paused = false;
            digitalWrite(STATUS_LED, LOW);
            Serial2.println("STATE,stopped");
        } else if (cmd == "LIST") sendFullList();
    }
}
