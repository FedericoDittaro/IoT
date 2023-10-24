#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include "pitches.h"
#define SENSOR_TX_PIN 4
#define SENSOR_RX_PIN 3
#define SENSOR_TOUCH_PIN 2
#define RED_LED_PIN 9
#define GREEN_LED_PIN 8
#define BUZZ_PIN 10
#define MOTOR_PIN 7
#define SERIAL_SPEED 9600
 
SoftwareSerial sensorSerial(SENSOR_TX_PIN, SENSOR_RX_PIN);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&sensorSerial);
 
Servo myservo;

unsigned long startTime;  // Tempo di inizio
unsigned long elapsedTime; // Tempo trascorso in millisecondi
const unsigned long duration = 20000; // Durata desiderata in millisecondi (20 secondi)
unsigned long lastPrintTime;  // Ultimo momento in cui è stato stampato il tempo

void setup() {
  pinMode(SENSOR_TOUCH_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  myservo.attach(MOTOR_PIN);
  Serial.begin(SERIAL_SPEED);
  finger.begin(57600);
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
    return;
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
  
    digitalWrite(GREEN_LED_PIN, HIGH);
    playSuccessSound();
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("Hai a disposizione 20 secondi");
    startTime = millis(); // Imposta il tempo di inizio
    lastPrintTime = millis();   // Imposta l'ultimo momento di stampa
    myservo.write(140);
    while(elapsedTime < duration) {
      elapsedTime = millis() - startTime; // Calcola il tempo trascorso
      unsigned long currentTime = millis(); // Ottieni il tempo corrente
      if (currentTime - lastPrintTime >= 1000) {
        Serial.print("Tempo trascorso: ");
        Serial.print(elapsedTime / 1000); // Converte in secondi
        Serial.println(" secondi");
        lastPrintTime = currentTime; // Aggiorna l'ultimo momento di stampa
      }    
    }
    myservo.write(89);
    playSuccessSound();
    Serial.println("Tempo scaduto!");
    elapsedTime = 0;
    return;
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

void playErrorSound() {
  // Genera il suono di errore
  tone(BUZZ_PIN, NOTE_A1, 200); // Genera un suono a 1000Hz per 200ms (0,2 secondi)
  delay(250); // Pausa di 500ms (0,5 secondi) tra i suoni
  tone(BUZZ_PIN, NOTE_A1, 200);
}

void playSuccessSound() {
  // Genera il suono di errore
  tone(BUZZ_PIN, NOTE_G4, 500); // Genera un suono a 1000Hz per 200ms (0,2 secondi)
  delay(250); // Pausa di 500ms (0,5 secondi) tra i suoni
}

void error() {
  Serial.println("Impronta non riconosciuta");
  digitalWrite(RED_LED_PIN, HIGH);
  playErrorSound();
  delay(100);
  digitalWrite(RED_LED_PIN, LOW);
}

  
