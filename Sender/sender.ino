#include <SPI.h>
#include <MFRC522.h>
#include <math.h>

// ---------------- RFID ----------------
#define SS_PIN 5
#define RST_PIN 22

// ---------------- THERMISTOR ----------------
#define THERM_PIN 34
#define SERIES_R 10000.0
#define NOMINAL_R 10000.0
#define NOMINAL_TEMP 25.0
#define BETA 3950.0
#define ADC_MAX 4095.0

// ---------------- TILT ----------------
#define TILT_PIN 27

// ---------------- BUZZER ----------------
#define BUZZER_PIN 26

// ---------------- MATRIX ----------------
int latchPin = 2;
int clockPin = 4;
int dataPin = 15;

MFRC522 rfid(SS_PIN, RST_PIN);

bool caregiverAuthorized = false;
bool unauthorizedCard = false;

// ---------------- FACES ----------------

const int smile[] = {
0x00,0x00,
0x1C,0x22,0x51,0x45,
0x45,0x51,0x22,0x14,
0x00,0x00
};

const int sad[] = {
0x00,0x00,
0x1C,0x22,0x41,0x45,
0x45,0x41,0x22,0x1C,
0x00,0x00
};

// ---------------- WORDS ----------------

const int bad[] = {
0x00,0x00,
0x7F,0x49,0x49,0x36,
0x00,
0x3F,0x48,0x48,0x3F,
0x00,
0x7F,0x41,0x22,0x1C,
0x00,0x00
};

const int aha[] = {
0x00,0x00,
0x3F,0x48,0x48,0x3F,
0x00,
0x7F,0x08,0x08,0x7F,
0x00,
0x3F,0x48,0x48,0x3F,
0x00,0x00
};

const int hot[] = {
0x00,0x00,
0x7F,0x08,0x08,0x7F,
0x00,
0x3E,0x41,0x41,0x3E,
0x00,
0x40,0x7F,0x40,0x40,
0x00,0x00
};

const int cold[] = {
0x00,0x00,
0x3E,0x41,0x41,0x22,
0x00,
0x3E,0x41,0x41,0x3E,
0x00,
0x7F,0x01,0x01,0x01,
0x00,
0x7F,0x41,0x41,0x3E,
0x00,0x00
};

void setup() {

  Serial.begin(115200);

  pinMode(TILT_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Digital Tantrum System Started");
}

void loop() {

  // NORMAL POSITION
  if (digitalRead(TILT_PIN) == HIGH) {

    caregiverAuthorized = false;
    unauthorizedCard = false;

    digitalWrite(BUZZER_PIN, LOW);

    clearMatrix();

    delay(100);
    return;
  }

  // RFID CHECK
  if (!caregiverAuthorized && !unauthorizedCard) {

    if (rfid.PICC_IsNewCardPresent() &&
        rfid.PICC_ReadCardSerial()) {

      String uid = "";

      for (byte i = 0; i < rfid.uid.size; i++) {

        if (rfid.uid.uidByte[i] < 0x10)
          uid += "0";

        uid += String(rfid.uid.uidByte[i], HEX);
      }

      uid.toUpperCase();

      Serial.print("RFID UID: ");
      Serial.println(uid);

      if (uid == "ED07FF06") {

        caregiverAuthorized = true;
        Serial.println("CAREGIVER VERIFIED");

      } else if (uid == "5D361F07") {

        unauthorizedCard = true;
        Serial.println("NOT CAREGIVER");
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  // UNAUTHORIZED RFID
  if (unauthorizedCard) {

    digitalWrite(BUZZER_PIN, LOW);

    scrollMessage(bad, sizeof(bad)/sizeof(int));
    scrollMessage(sad, sizeof(sad)/sizeof(int));

    return;
  }

  // CAREGIVER RFID
  if (caregiverAuthorized) {

    int sum = 0;

    for (int i = 0; i < 5; i++) {
      sum += analogRead(THERM_PIN);
      delay(10);
    }

    int raw = sum / 5;

    if (raw < 10) raw = 10;
    if (raw > 4085) raw = 4085;

    float resistance =
      SERIES_R / (ADC_MAX / (float)raw - 1.0);

    float logVal =
      log(resistance / NOMINAL_R);

    float steinhart =
      (logVal / BETA) +
      (1.0 / (NOMINAL_TEMP + 273.15));

    float celsius =
      (1.0 / steinhart) - 273.15;

    Serial.print("Temp = ");
    Serial.print(celsius);
    Serial.println(" C");

    if (celsius > 50.0) {

      digitalWrite(BUZZER_PIN, HIGH);

      scrollMessage(hot, sizeof(hot)/sizeof(int));
      scrollMessage(sad, sizeof(sad)/sizeof(int));
    }

    else if (celsius >= 10.0) {

      digitalWrite(BUZZER_PIN, LOW);

      scrollMessage(aha, sizeof(aha)/sizeof(int));
      scrollMessage(smile, sizeof(smile)/sizeof(int));
    }

    else {

      digitalWrite(BUZZER_PIN, LOW);

      scrollMessage(cold, sizeof(cold)/sizeof(int));
      scrollMessage(sad, sizeof(sad)/sizeof(int));
    }
  }
}

void scrollMessage(const int msg[], int len) {

  for (int i = 0; i < len - 7; i++) {

    // stop immediately if returned to NORMAL
    if (digitalRead(TILT_PIN) == HIGH) {
      clearMatrix();
      return;
    }

    for (int repeat = 0; repeat < 15; repeat++) {

      int cols = 0x01;

      for (int j = i; j < i + 8; j++) {

        matrixRowsVal(msg[j]);
        matrixColsVal(~cols);

        delay(1);

        matrixRowsVal(0x00);

        cols <<= 1;
      }
    }
  }
}

void clearMatrix() {

  matrixRowsVal(0x00);
  matrixColsVal(0xFF);
}

void matrixRowsVal(int value) {

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, value);
  digitalWrite(latchPin, HIGH);
}

void matrixColsVal(int value) {

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, value);
  digitalWrite(latchPin, HIGH);
}
