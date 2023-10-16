#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include "peaches.h"
#define SENSOR_TX_PIN 4
#define SENSOR_RX_PIN 3
#define SENSOR_TOUCH_PIN 2
#define RED_LED_PIN 9
#define GREEN_LED_PIN 8
#define BUZZ_PIN 10
#define BUZZ_PIN_2 11
#define MOTOR_PIN 7
#define SERIAL_SPEED 9600
 
SoftwareSerial sensorSerial(SENSOR_TX_PIN, SENSOR_RX_PIN);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&sensorSerial);
 
int greenLed = LOW;
Servo motor;

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void setup() {
  pinMode(SENSOR_TOUCH_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(BUZZ_PIN_2, OUTPUT);
  motor.attach(MOTOR_PIN);
  Serial.begin(SERIAL_SPEED);
  delay(1000);
  Serial.begin(9600);
  finger.begin(57600);
  delay(100);
  if (!finger.verifyPassword()) {
    Serial.println("Errore: sensore non trovato!");
    digitalWrite(RED_LED_PIN, HIGH);
    while (true);
  }
}

void loop() {

  Serial.println("Seleziona l'azione da eseguire:");
  Serial.println(" -> 1: Inserisci o elimina impronta");
  Serial.println(" -> 2: Verifica impronta");

  Serial.println();
  while (!Serial.available());
  String command = Serial.readStringUntil('\n');

  int choose = command.toInt();

  if (choose == 1) {
    printStoredFingerprints();

    Serial.println("Seleziona l'azione da eseguire:");
    Serial.println(" -> numero da 1 a 127 per inserire");
    Serial.println(" -> numero da -1 a -127 per rimuovere");
    Serial.println(" -> x per svuotare tutto");
    Serial.println();
    while (!Serial.available());
    String input = Serial.readStringUntil('\n');

    if (input == "x") {
      removeAllFingerprints();
      return;
    }
  
    int location = input.toInt();
    if (location > 0 && location < 128) {
      saveFingerprint(location);
    } else if (location < 0 && location > -128) {
      removeFingerprint(-location);
    } else {
      Serial.println("Errore: comando non valido!");
      Serial.println();
    }

  }

  if (choose == 2) {
    Serial.println("Posiziona il dito sul sensore");
    while (!digitalRead(SENSOR_TOUCH_PIN));
    if (scanFingerprint(1) != FINGERPRINT_OK) {
      error();
      return;
    }
  
    if (finger.fingerSearch() != FINGERPRINT_OK) {
      error();
      return;
    }
  
    greenLed = !greenLed;
    digitalWrite(GREEN_LED_PIN, HIGH);
    // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(8, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
    delay(100);
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(100);
    analogWrite(MOTOR_PIN, 128);
  }
}

void printStoredFingerprints() {
  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.println("Nessuna impronta registrata");
    Serial.println();
  } else {
    if (finger.templateCount == 1) {
      Serial.println("Un’impronta registrata");
    } else {
      Serial.println(String(finger.templateCount) + " impronte registrate");
    }
    
    String locations = "";
    for (uint16_t i = 1; i <= 127; i++) {
      if (finger.loadModel(i) == FINGERPRINT_OK) {
        locations += String(i) + ", ";
      }
    }
    locations = locations.substring(0, locations.length() - 2);
    Serial.println("Posizioni occupate: " + locations);
    Serial.println();
  }
}

void saveFingerprint(uint16_t location) {
  Serial.println("Posiziona il dito");
  while (!digitalRead(SENSOR_TOUCH_PIN));
  
  if (scanFingerprint(1) != FINGERPRINT_OK) {
    Serial.println("Errore: scansione non riuscita");
    Serial.println();
    return;
  }
 
  Serial.println("Rimuovi il dito");
  while (digitalRead(SENSOR_TOUCH_PIN));
  delay(200);
 
  Serial.println("Posiziona di nuovo lo stesso dito");
  while (!digitalRead(SENSOR_TOUCH_PIN));
  if (scanFingerprint(2) != FINGERPRINT_OK) {
    Serial.println("Errore: scansione non riuscita");
    Serial.println();
    return;
  }
 
  if (finger.createModel() != FINGERPRINT_OK) {
    Serial.println("Errore: impossibile creare il modello");
    Serial.println();
    return;
  }
 
  if (finger.storeModel(location) != FINGERPRINT_OK) {
    Serial.println("Errore: impossibile salvare il modello");
    Serial.println();
    return;
  }
 
  Serial.println("L'impronta (#" + String(location) + ") è stata salvata");
  Serial.println();
}
 
void removeFingerprint(uint16_t location) {
  if (finger.deleteModel(location) != FINGERPRINT_OK) {
    Serial.print("Errore: impossibile svuotare la posizione #");
    Serial.println(location);
    Serial.println();
    return;
  }
  
  Serial.println("L'impronta (#" + String(location) + ") è stata eliminata");
  Serial.println();
}
 
void removeAllFingerprints() {
  if (finger.emptyDatabase() != FINGERPRINT_OK) {
    Serial.println("Errore: impossibile svuotare le impronte");
    Serial.println();
    return;
  }
  Serial.println("Tutte le impronte sono state cancellate");
  Serial.println();
}
 
uint8_t scanFingerprint(uint8_t slot) {
  uint8_t status;
 
  do {
    status = finger.getImage();
  } while (status == FINGERPRINT_NOFINGER);
 
  if (status != FINGERPRINT_OK) {
    return status;
  }
 
  return finger.image2Tz(slot);
}

void error() {
  Serial.println("Impronta non riconosciuta");
  tone(BUZZ_PIN_2, 1000); // Suono con freq. 1KHz
  delay(1000); // Pausa (1 sec.)
  noTone(BUZZ_PIN_2); // Stop
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(100);
    digitalWrite(RED_LED_PIN, LOW);
    delay(100);
  }
  while (digitalRead(SENSOR_TOUCH_PIN));
  delay(200);
}
  
