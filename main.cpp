#include <Wire.h>
#include "RTClib.h" // Requires Adafruit RTClib

// --- RTC CONFIGURATION ---
RTC_DS3231 rtc; // Assumes DS3231 module

// ESP32 default I2C pins for DS3231:
// SDA -> GPIO 21
// SCL -> GPIO 22

// --- BUTTON HARDWARE DEFINITIONS ---
#define BUTTON_15M_PIN 13 // Button 1: Start 10 seconds (Test Time)
#define BUTTON_30M_PIN 12 // Button 2: Start 20 seconds (Test Time)
#define BUTTON_60M_PIN 14 // Button 3: Start 30 seconds (Test Time)
#define BUTTON_STOP_PIN 27 // Button 4: Stop timer and reset trigger

// --- OUTPUT TRIGGER DEFINITION ---
#define TRIGGER_PIN 4 // GPIO 4: Goes HIGH when the countdown reaches zero

// --- TIMING VARIABLES ---
// Variables for non-blocking time management
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 1000; // Update time every 1000ms (1 second)

// Variables for button debouncing and edge detection
unsigned long lastButtonCheck = 0;
const unsigned long DEBOUNCE_DELAY = 50; // 50ms for stable reads

// --- TIMER STATE MANAGEMENT ---
unsigned long startTimeMillis = 0; // The millis() time when the timer started
long targetDurationSeconds = 0;    // The total duration in seconds 

// Last states for edge detection (HIGH = unpressed)
int lastState15M = HIGH;
int lastState30M = HIGH;
int lastState60M = HIGH;
int lastStateSTOP = HIGH;

// --- FUNCTIONS ---

// Function to print the time from the RTC module
void printRTCTime() {
  DateTime now = rtc.now();

  // Print RTC Time
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d | ", 
                now.month(), now.day(), now.year(), 
                now.hour(), now.minute(), now.second());

  // Print Timer Status
  if (targetDurationSeconds > 0) {
    long elapsedSeconds = (millis() - startTimeMillis) / 1000;
    long remainingSeconds = targetDurationSeconds - elapsedSeconds;

    if (remainingSeconds <= 0) {
      // Timer finished!
      Serial.println("TIMER FINISHED!");
      targetDurationSeconds = 0; // Stop the timer

      // *** ACTION: Activate Trigger Pin ***
      digitalWrite(TRIGGER_PIN, HIGH); 
      
    } else {
      // Timer is running
      long remainingMinutes = remainingSeconds / 60;
      long displaySeconds = remainingSeconds % 60;
      
      Serial.printf("Timer Running: %02ld:%02ld remaining.\n", remainingMinutes, displaySeconds);
    }
  } else {
    // Timer is stopped
    Serial.println("Timer Stopped/Idle.");
  }
}

// Function to check all buttons and handle state changes (edge detection)
void checkButtons() {
    unsigned long currentMillis = millis();
    
    // Debouncing check
    if (currentMillis - lastButtonCheck < DEBOUNCE_DELAY) {
        return;
    }
    lastButtonCheck = currentMillis;

    // Read current button states
    int current15M = digitalRead(BUTTON_15M_PIN);
    int current30M = digitalRead(BUTTON_30M_PIN);
    int current60M = digitalRead(BUTTON_60M_PIN);
    int currentSTOP = digitalRead(BUTTON_STOP_PIN);

    // --- Button 1 (10 Seconds Test) ---
    if (current15M == LOW && lastState15M == HIGH) {
        targetDurationSeconds = 10; // Set to 10 seconds
        startTimeMillis = currentMillis;
        digitalWrite(TRIGGER_PIN, LOW); // Reset trigger on new timer start
        Serial.println(">> Timer Set: 10 Seconds (Test) <<");
    }

    // --- Button 2 (20 Seconds Test) ---
    if (current30M == LOW && lastState30M == HIGH) {
        targetDurationSeconds = 20; // Set to 20 seconds
        startTimeMillis = currentMillis;
        digitalWrite(TRIGGER_PIN, LOW); // Reset trigger on new timer start
        Serial.println(">> Timer Set: 20 Seconds (Test) <<");
    }

    // --- Button 3 (30 Seconds Test) ---
    if (current60M == LOW && lastState60M == HIGH) {
        targetDurationSeconds = 30; // Set to 30 seconds
        startTimeMillis = currentMillis;
        digitalWrite(TRIGGER_PIN, LOW); // Reset trigger on new timer start
        Serial.println(">> Timer Set: 30 Seconds (Test) <<");
    }

    // --- Button 4 (STOP) ---
    if (currentSTOP == LOW && lastStateSTOP == HIGH) {
        targetDurationSeconds = 0;
        startTimeMillis = 0;
        // The TRIGGER_PIN must be set LOW to stop the LED/buzzer
        digitalWrite(TRIGGER_PIN, LOW); 
        Serial.println(">> Timer STOPPED <<");
    }

    // Update Last States for next iteration
    lastState15M = current15M;
    lastState30M = current30M;
    lastState60M = current60M;
    lastStateSTOP = currentSTOP;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // Initialize I2C (SDA=21, SCL=22 for ESP32)

  // --- RTC Setup ---
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10); // Halt if no RTC found
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // --- Pin Setup ---
  pinMode(BUTTON_15M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_30M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_60M_PIN, INPUT_PULLUP);
  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);

  // *** Trigger Pin Setup ***
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW); // Start with trigger off

  // Initialize lastState variables
  delay(100); 
  lastState15M = digitalRead(BUTTON_15M_PIN);
  lastState30M = digitalRead(BUTTON_30M_PIN);
  lastState60M = digitalRead(BUTTON_60M_PIN);
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
