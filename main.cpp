#include <Wire.h>
#include "RTClib.h" // Requires Adafruit RTClib

// --- RTC CONFIGURATION ---
RTC_DS3231 rtc; // Assumes DS3231 module

// ESP32 default I2C pins for DS3231:
// SDA -> GPIO 21
// SCL -> GPIO 22

// --- BUTTON HARDWARE DEFINITIONS ---
#define BUTTON_15S_PIN 26 // NEW BUTTON: Start 15 seconds
#define BUTTON_15M_PIN 13 // Button: Start 15 minutes
#define BUTTON_30M_PIN 12 // Button: Start 30 minutes
#define BUTTON_60M_PIN 14 // Button: Start 1 hour
#define BUTTON_STOP_PIN 27 // Button: Stop timer and reset trigger

// --- OUTPUT TRIGGER DEFINITION ---
#define TRIGGER_PIN 4 // GPIO 4: Goes HIGH when countdown reaches zero

// --- TIMING VARIABLES ---
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 1000; // 1-second updates

unsigned long lastButtonCheck = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// --- TIMER STATE MANAGEMENT ---
unsigned long startTimeMillis = 0;
long targetDurationSeconds = 0; // Duration in seconds

// Last states for edge detection
int lastState15S = HIGH;
int lastState15M = HIGH;
int lastState30M = HIGH;
int lastState60M = HIGH;
int lastStateSTOP = HIGH;

void printRTCTime() {
  DateTime now = rtc.now();

  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d | ",
                now.month(), now.day(), now.year(),
                now.hour(), now.minute(), now.second());

  if (targetDurationSeconds > 0) {
    long elapsedSeconds = (millis() - startTimeMillis) / 1000;
    long remainingSeconds = targetDurationSeconds - elapsedSeconds;

    if (remainingSeconds <= 0) {
      Serial.println("TIMER FINISHED!");
      targetDurationSeconds = 0;
      digitalWrite(TRIGGER_PIN, HIGH); // Activate trigger
    } else {
      long minutes = remainingSeconds / 60;
      long seconds = remainingSeconds % 60;
      Serial.printf("Timer Running: %02ld:%02ld remaining.\n", minutes, seconds);
    }

  } else {
    Serial.println("Timer Stopped/Idle.");
  }
}

void checkButtons() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastButtonCheck < DEBOUNCE_DELAY) return;
  lastButtonCheck = currentMillis;

  int cur15S  = digitalRead(BUTTON_15S_PIN);
  int cur15M  = digitalRead(BUTTON_15M_PIN);
  int cur30M  = digitalRead(BUTTON_30M_PIN);
  int cur60M  = digitalRead(BUTTON_60M_PIN);
  int curSTOP = digitalRead(BUTTON_STOP_PIN);

  // --- Button 1: 15 Seconds ---
  if (cur15S == LOW && lastState15S == HIGH) {
    targetDurationSeconds = 15;
    startTimeMillis = currentMillis;
    digitalWrite(TRIGGER_PIN, LOW);
    Serial.println(">> Timer Set: 15 Seconds <<");
  }

  // --- Button 2: 15 Minutes ---
  if (cur15M == LOW && lastState15M == HIGH) {
    targetDurationSeconds = 15 * 60;
    startTimeMillis = currentMillis;
    digitalWrite(TRIGGER_PIN, LOW);
    Serial.println(">> Timer Set: 15 Minutes <<");
  }

  // --- Button 3: 30 Minutes ---
  if (cur30M == LOW && lastState30M == HIGH) {
    targetDurationSeconds = 30 * 60;
    startTimeMillis = currentMillis;
    digitalWrite(TRIGGER_PIN, LOW);
    Serial.println(">> Timer Set: 30 Minutes <<");
  }

  // --- Button 4: 1 Hour ---
  if (cur60M == LOW && lastState60M == HIGH) {
    targetDurationSeconds = 60 * 60;
    startTimeMillis = currentMillis;
    digitalWrite(TRIGGER_PIN, LOW);
    Serial.println(">> Timer Set: 1 Hour <<");
  }

  // --- STOP Button ---
  if (curSTOP == LOW && lastStateSTOP == HIGH) {
    targetDurationSeconds = 0;
    startTimeMillis = 0;
    digitalWrite(TRIGGER_PIN, LOW);
    Serial.println(">> Timer STOPPED <<");
  }

  // Update last states
  lastState15S = cur15S;
  lastState15M = cur15M;
  lastState30M = cur30M;
  lastState60M = cur60M;
  lastStateSTOP = curSTOP;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!rtc.begin()) {
    Serial.println("RTC ERROR!");
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(BUTTON_15S_PIN, INPUT_PULLUP);
  pinMode(BUTTON_15M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_30M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_60M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);

  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);

  delay(100);
  lastState15S  = digitalRead(BUTTON_15S_PIN);
  lastState15M  = digitalRead(BUTTON_15M_PIN);
  lastState30M  = digitalRead(BUTTON_30M_PIN);
  lastState60M  = digitalRead(BUTTON_60M_PIN);
  lastStateSTOP = digitalRead(BUTTON_STOP_PIN);

  Serial.println("System Ready. RTC Time | Timer Status");
}

void loop() {
  checkButtons();

  unsigned long currentMillis = millis();
  if (currentMillis - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = currentMillis;
    printRTCTime();
  }
}
