#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <EEPROM.h>

// ───── C3 wiring (4 buttons) ─────
#define OLED_SCK   4
#define OLED_MOSI  3
#define OLED_CS    5
#define OLED_DC    8
#define OLED_RST   9

#define BTN_UP      0   // boot pin – avoid holding at power-on
#define BTN_DOWN    1
#define BTN_LEFT    2
#define BTN_RIGHT  10

#define RXD2  6
#define TXD2  7

Adafruit_SH1106G display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);

enum View { VIEW_MENU, VIEW_SONGS, VIEW_VOL, VIEW_EQ, VIEW_SETTINGS };
View currentView = VIEW_MENU;
enum PlayerState { STATE_STOPPED, STATE_PLAYING, STATE_PAUSED };
PlayerState audioState = STATE_STOPPED;

float eqValues[] = {4.0,3.5,4.5,3.7,4.2,5.3,5.7,5.0,3.0};
const char* eqFreqs[] = {"62","125","250","500","1k","2k","4k","8k","16k"};
int eqCursor = 0;

String songNames[50], songPaths[50];
int totalSongs = 0, cursor = 0, playingIdx = -1;
float volume = 0.4;
unsigned long lastDisplayUpdate = 0;
bool shuffleMode = false, refreshNeeded = true;

// ───── Button debounce ─────
struct Button {
    uint8_t pin;
    bool stable, last, pressed;
    unsigned long lastChange;
    Button(uint8_t p) : pin(p), stable(HIGH), last(HIGH), lastChange(0), pressed(false) {}
    void begin() {
        pinMode(pin, INPUT_PULLUP);
        stable = last = digitalRead(pin);
    }
    void update() {
        bool reading = digitalRead(pin);
        if (reading != last) { last = reading; lastChange = millis(); }
        if ((millis() - lastChange) > 40) {
            if (reading != stable) {
                stable = reading;
                if (stable == LOW) pressed = true;
            }
        }
    }
    bool wasPressed() {
        if (pressed) { pressed = false; return true; }
        return false;
    }
};

Button btnUp(BTN_UP), btnDown(BTN_DOWN), btnLeft(BTN_LEFT), btnRight(BTN_RIGHT);

// ───── Text wrap ─────
int drawWrappedName(String name, int x, int y, bool sel, int maxChars) {
    if (name.length() == 0) return 0;
    if (sel) { display.setCursor(0, y); display.print(">"); }
    if (name.length() <= maxChars) { display.setCursor(x, y); display.print(name); return 8; }
    int br = -1;
    for (int i = maxChars; i >= 0; i--) if (name.charAt(i) == ' ') { br = i; break; }
    if (br <= 0) br = maxChars;
    display.setCursor(x, y); display.print(name.substring(0, br));
    display.setCursor(x, y + 8);
    String l2 = name.substring(br + 1); l2.trim();
    if (l2.length() > maxChars) l2 = l2.substring(0, maxChars-2) + "..";
    display.print(l2);
    return 16;
}

String getCleanName(String full) {
    String n = full.substring(full.lastIndexOf('/') + 1);
    n.replace(".wav", ""); n.replace(".WAV", "");
    return n;
}

void sendCmd(String cmd) {
    Serial1.println(cmd);
}

void requestSongList() {
    totalSongs = 0;
    sendCmd("LIST");
}

void drawUI() {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(audioState == STATE_PLAYING ? "PLAYING" :
                 (audioState == STATE_PAUSED ? "PAUSED" : "STOPPED"));
    display.setCursor(100, 0); display.print((int)(volume * 100)); display.print("%");
    display.drawFastHLine(0, 10, 128, SH110X_WHITE);

    if (currentView == VIEW_MENU) {
        const char* menu[] = {"Songs List", "Volume Set", "EQ Settings", "Settings"};
        for (int i = 0; i < 4; i++) {
            display.setCursor(15, 18 + i * 11);
            display.print(cursor == i ? "> " : "  "); display.print(menu[i]);
        }
    }
    else if (currentView == VIEW_SONGS) {
        if (totalSongs == 0) {
            display.setCursor(20, 28); display.print("No songs!");
        } else {
            const int SP = 2, MC = 19, EH = 20;
            int start = (cursor / SP) * SP;
            int yPos = 12;
            for (int i = 0; i < SP; i++) {
                int idx = start + i;
                if (idx >= totalSongs) break;
                drawWrappedName(songNames[idx], 10, yPos, idx == cursor, MC);
                yPos += EH;
            }
        }
    }
    else if (currentView == VIEW_EQ) {
        for (int i = 0; i < 9; i++) {
            int x = 5 + i * 13;
            int dotY = map(eqValues[i] * 10, -75, 75, 50, 15);
            display.drawFastVLine(x + 5, 15, 35, SH110X_WHITE);
            if (eqCursor == i) {
                display.fillCircle(x + 5, dotY, 3, SH110X_WHITE);
                display.setCursor(x, 55); display.print(eqFreqs[i]);
            } else display.drawCircle(x + 5, dotY, 2, SH110X_WHITE);
        }
    }
    else if (currentView == VIEW_VOL) {
        display.setCursor(30, 20); display.print("- VOLUME -");
        display.drawRect(12, 35, 104, 12, 1);
        display.fillRect(14, 37, (int)(volume * 100), 8, 1);
    }
    else if (currentView == VIEW_SETTINGS) {
        display.setCursor(15, 20);
        display.print(cursor == 0 ? "> Shuffle: " : "  Shuffle: ");
        display.print(shuffleMode ? "ON" : "OFF");
        display.setCursor(15, 32);
        display.print(cursor == 1 ? "> Back" : "  Back");
    }
    display.display();
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(3000);

    EEPROM.begin(10);
    volume = (EEPROM.read(0) > 100) ? 0.4 : (float)EEPROM.read(0) / 100.0;
    shuffleMode = (EEPROM.read(1) == 1);

    btnUp.begin(); btnDown.begin(); btnLeft.begin(); btnRight.begin();

    SPI.begin(OLED_SCK, -1, OLED_MOSI, -1);
    if (!display.begin(0, true)) {
        Serial.println("OLED init failed!");
    }
    display.clearDisplay(); display.display();
    delay(2000);
    requestSongList();
}

void loop() {
    // 1. Read responses from S3
    while (Serial1.available()) {
        String line = Serial1.readStringUntil('\n'); line.trim();
        if (line.startsWith("STATE,")) {
            String st = line.substring(6);
            if (st == "playing") audioState = STATE_PLAYING;
            else if (st == "paused") audioState = STATE_PAUSED;
            else audioState = STATE_STOPPED;
            refreshNeeded = true;
        }
        else if (line.startsWith("TRACK,")) {
            int c1 = line.indexOf(','), c2 = line.indexOf(',', c1 + 1);
            playingIdx = line.substring(c1 + 1, c2).toInt();
        }
        else if (line == "LISTSTART") { totalSongs = 0; }
        else if (line.startsWith("SONG,")) {
            int c1 = line.indexOf(','), c2 = line.indexOf(',', c1 + 1);
            int idx = line.substring(c1 + 1, c2).toInt();
            String full = line.substring(c2 + 1);
            if (idx < 50) {
                songPaths[idx] = full;
                songNames[idx] = getCleanName(full);
                if (idx >= totalSongs) totalSongs = idx + 1;
            }
        }
        else if (line == "LISTEND") { refreshNeeded = true; }
    }

    // 2. Update buttons
    btnUp.update(); btnDown.update(); btnLeft.update(); btnRight.update();

    // 3. LEFT button (back)
    if (btnLeft.wasPressed()) {
        if (currentView == VIEW_EQ && eqCursor > 0) eqCursor--;
        else { currentView = VIEW_MENU; cursor = 0; }
        refreshNeeded = true;
    }

    // 4. RIGHT button (select / play-pause)
    if (btnRight.wasPressed()) {
        if (currentView == VIEW_MENU) {
            if (cursor == 0) { currentView = VIEW_SONGS; cursor = (playingIdx >= 0) ? playingIdx : 0; }
            else if (cursor == 1) currentView = VIEW_VOL;
            else if (cursor == 2) { currentView = VIEW_EQ; eqCursor = 0; }
            else if (cursor == 3) { currentView = VIEW_SETTINGS; cursor = 0; }
        }
        else if (currentView == VIEW_SONGS && totalSongs > 0) {
            // If this song is already playing/paused → toggle
            if (playingIdx == cursor && audioState != STATE_STOPPED) {
                sendCmd((audioState == STATE_PLAYING) ? "PAUSE" : "RESUME");
            } else {
                sendCmd("PLAY," + String(cursor));
            }
        }
        else if (currentView == VIEW_EQ && eqCursor < 8) eqCursor++;
        else if (currentView == VIEW_SETTINGS && cursor == 0) {
            shuffleMode = !shuffleMode;
            sendCmd("SHUFFLE," + String(shuffleMode ? 1 : 0));
            EEPROM.write(1, shuffleMode); EEPROM.commit();
        }
        refreshNeeded = true;
    }

    // 5. UP button
    if (btnUp.wasPressed()) {
        if (currentView == VIEW_EQ) eqValues[eqCursor] = min(7.5f, eqValues[eqCursor] + 0.5f);
        else if (currentView == VIEW_VOL) {
            volume = min(1.0f, volume + 0.05f);
            sendCmd("VOL," + String((int)(volume * 100)));
            EEPROM.write(0, (int)(volume * 100)); EEPROM.commit();
        } else {
            int lim = (currentView == VIEW_MENU) ? 4 :
                      (currentView == VIEW_SETTINGS) ? 2 :
                      (currentView == VIEW_SONGS && totalSongs > 0) ? totalSongs : 1;
            if (lim > 0) cursor = (cursor - 1 + lim) % lim;
        }
        refreshNeeded = true;
    }

    // 6. DOWN button
    if (btnDown.wasPressed()) {
        if (currentView == VIEW_EQ) eqValues[eqCursor] = max(-7.5f, eqValues[eqCursor] - 0.5f);
        else if (currentView == VIEW_VOL) {
            volume = max(0.0f, volume - 0.05f);
            sendCmd("VOL," + String((int)(volume * 100)));
            EEPROM.write(0, (int)(volume * 100)); EEPROM.commit();
        } else {
            int lim = (currentView == VIEW_MENU) ? 4 :
                      (currentView == VIEW_SETTINGS) ? 2 :
                      (currentView == VIEW_SONGS && totalSongs > 0) ? totalSongs : 1;
            if (lim > 0) cursor = (cursor + 1) % lim;
        }
        refreshNeeded = true;
    }

    // 7. Display refresh
    if (refreshNeeded || audioState == STATE_PLAYING) {
        if (millis() - lastDisplayUpdate > 100) {
            drawUI();
            lastDisplayUpdate = millis();
            refreshNeeded = false;
        }
    }
}
