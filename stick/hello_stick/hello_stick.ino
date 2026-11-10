// hello_stick.ino - M5StickS3 hello world.
// Shows "Hello Jim" and beeps when the front button (BtnA) is pressed.
#include <M5Unified.h>

static int pressCount = 0;

static void drawScreen() {
    M5.Display.fillScreen(TFT_NAVY);
    M5.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.drawString("Hello Jim", M5.Display.width() / 2,
                          M5.Display.height() / 2 - 12);

    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(TFT_YELLOW, TFT_NAVY);
    char line[40];
    snprintf(line, sizeof(line), "Front button presses: %d", pressCount);
    M5.Display.drawString(line, M5.Display.width() / 2,
                          M5.Display.height() / 2 + 30);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setRotation(1);   // landscape: 240 wide x 135 tall
    M5.Speaker.setVolume(128);   // 0-255

    Serial.begin(115200);
    Serial.printf("hello_stick: board id %d, display %dx%d\n",
                  (int)M5.getBoard(), M5.Display.width(), M5.Display.height());

    drawScreen();
}

void loop() {
    M5.update();   // reads the buttons

    if (M5.BtnA.wasPressed()) {
        M5.Speaker.tone(2000, 100);   // 2 kHz for 100 ms
        ++pressCount;
        Serial.printf("BtnA pressed (%d)\n", pressCount);
        drawScreen();
    }
    delay(10);
}
