
#include <ArduinoBLE.h>
#include "Sodaq_wdt.h"

int countAcq = 1;  // compteur d'acquisition de résultats pour validation
int cycle = 1;
int captureCycle = 0;

const int CLOCK = 7;  // Clock transmission: D7
const int TX = 8;     // Transmission résultat du scan: D8

const int LED = 13;  // Led internes

String scanInit[10] = { "none", "none", "none", "none", "none", "none", "none", "none", "none", "none" };
String scanFinal[10] = { "none", "none", "none", "none", "none", "none", "none", "none", "none", "none" };

int phase = 0;
int NBRE_SCAN = 200;

//*********************************************************************************************************
void setup() {

  pinMode(CLOCK, INPUT);
  pinMode(TX, OUTPUT);
  pinMode(LED, OUTPUT);


  Serial.begin(9600);
  while (!Serial)
    ;


  if (!BLE.begin()) {
    Serial.println("Lancement Bluetooth Low Energy echec");
    digitalWrite(LED, LOW);
    while (1)
      ;
  }

  Serial.println("Scan bluetooth démarré!");
  digitalWrite(LED, HIGH);
  digitalWrite(CLOCK, HIGH);  // D7
  digitalWrite(TX, LOW);      // D8
}

//*********************************************************************************************************
// START LOOP
void loop() {

  // attente de démarrage
  /*
  int h = 0;
  while (phase == 0) {

    if (h == 0) {
      Serial.println("Test sendAcq");
      sendAcq();
    }
    h = 1;
  }
  */

  sodaq_wdt_enable(WDT_PERIOD_8X);

  while (phase == 0) {

    //Serial.println("phase 0: attente");
    phase = getPhase(0);
    if (sodaq_wdt_flag) {
      sodaq_wdt_flag = false;
      sodaq_wdt_reset();
    }
  }


  sodaq_wdt_disable();
  sodaq_wdt_flag = false;

  Serial.print("phase détectée1: ");
  Serial.println(phase);
  for (int i = 0; i < NBRE_SCAN; i++) {

    processingScanBLE();  // scan initial
  }

  sendAcq();
  // affichage tableau initial
  if (phase == 1) {

    for (int i = 0; i < 10; i++) {

      Serial.print("phase initiale - rang: ");
      Serial.print(i);
      Serial.print(" - ");
      Serial.print(scanInit[i]);
      Serial.println();
    }
  }

  // attente fin de la phase 1
  while (phase == 1) {

    //Serial.println("phase 0: attente");
    phase = getPhase(1);
  }

  Serial.print("phase détectée2: ");
  Serial.println(phase);


  for (int i = 0; i < NBRE_SCAN; i++) {

    processingScanBLE();  // scan final
  }


  // affichage tableau final
  if (phase == 2) {

    for (int i = 0; i < 10; i++) {


      Serial.print("phase finale   - rang: ");
      Serial.print(i);
      Serial.print(" - ");
      Serial.print(scanFinal[i]);
      Serial.println();
    }
  }

  sendAcq();  // acquitement de la phase 2

  // Attente fin de la phase 2
  while (phase == 2) {

    //Serial.println("phase 0: attente");
    phase = getPhase(2);
  }

  // Retour aux conditions initiales

  Serial.print("phase détectée3: ");
  Serial.println(phase);


  if (phase >= 3) {

    phase = 0;
    cycle = 1;
    captureCycle = 0;

    sendAcq();  // acquitement de la phase 3
    transmissionAcqBle();
    Serial.println("---------------  Fin transmission BLE  ------------------");
    waitForClock(false);
    resetPeripheraAllLLists();
    Serial.println(' ');
    Serial.println("---------------  Fin de test ------------------");
  }

  delay(100);
}

// END LOOP
//***************************************************************************************************


// ------------------------  Définition fonctions --------------------------------------------------

// Constitue les tableaux d'acquisition des codes BLE scannés

void savePeripheral(String peripheralName, int phase) {

  int j = 0;
  bool saved = false;
  while (!saved) {

    if (phase == 1) {

      if (scanInit[j].equals("none")) {

        scanInit[j] = peripheralName;
        saved = true;


      } else {

        if (scanInit[j].equals(peripheralName)) {

          saved = true;

        } else {
          j++;
          if (j > 9) {

            saved = true;
          }
        }
      }
    }

    if (phase == 2) {

      if (scanFinal[j].equals("none")) {

        scanFinal[j] = peripheralName;
        saved = true;


      } else {

        if (scanFinal[j].equals(peripheralName)) {

          saved = true;

        } else {
          j++;
          if (j > 9) {

            saved = true;
          }
        }
      }
    }
  }
}
//***************************************************************************************************

// Réinitialise le tableau initial des acquisitions de codes BLE scannés

void resetPeripheralList() {

  for (int i = 0; i < 10; i++) {

    scanInit[i] = "none";
  }
}

//***************************************************************************************************

// Réinitialise les tableaux des acquisitions de code BLE scannés

void resetPeripheraAllLLists() {

  for (int i = 0; i < 10; i++) {

    scanInit[i] = "none";
    scanFinal[i] = "none";
  }
}

//***************************************************************************************************
// Retourne la phase en cours
// phase 1: scan BLE initial
// phase 2: scan BLE: final

int getPhase(int phase) {

  int phaseNew = phase;
  bool getNewPhase = false;

  while (!getNewPhase) {

    phaseNew = waitForStartCLK(phase);
    if (phaseNew != 0) {
      return phaseNew;
    }
  }
}

//***************************************************************************************************
// Compare les tableaux de codes BLE scannés pour restituer le code BLE de la carte en test

String getPeripheralName() {

  String foundName = "none";
  bool found = false;
  String nameFinal = "none";
  int j = 0;

  int presentList[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  for (int i = 0; i < 10; i++) {

    nameFinal = scanFinal[i];
    j = 0;
    found = false;

    while (!found) {


      if (nameFinal.equals("none")) {

        presentList[i] = 0;  // égale à un none
        found = true;

      } else {

        if (nameFinal.substring(5, 16).equals(scanInit[j].substring(5, 16))) {

          presentList[i] = 1;  // présent
          found = true;

        } else {

          if (j == 9) {

            presentList[i] = 2;  // non présent
            found = true;
          }
        }
      }

      j++;
    }
  }

  found = false;
  int i = 0;

  while (!found) {

    Serial.println(presentList[i]);
    if (presentList[i] == 2) {

      foundName = scanFinal[i];
      found = true;
    }
    i++;
  }

  return foundName;
}


//***************************************************************************************************

void waitForClock(bool start) {

  if (start) {
    while (digitalRead(7) == 1) {}
  } else {
    while (digitalRead(7) == 0) {}
  }
}

//**************************************************************************************************
// Analyse les activations de l'entrée horloge

int waitForStartCLK(int phase) {

  int pulse = 0;
  bool endCheckForCLK = false;

  long startTime;
  long currentTime;
  startTime = millis();

  while (!endCheckForCLK) {

    while (digitalRead(7) == 1) {

      currentTime = millis() - startTime;
      if (currentTime > 100) {
        endCheckForCLK = true;
        return pulse;
      }
    }

    pulse++;

    while (digitalRead(7) == 0) {
    }
    startTime = millis();
  }
}

//**************************************************************************************************
// envoi un caractère sur la ligne de transmission

void sendChar(char car) {

  Serial.print(' ');
  int cInt = car;
  int c;
  for (int i = 0; i < 8; i++) {

    c = cInt << i;
    waitForClock(true);
    int bit = c & 128;
    if (bit == 128) {

      digitalWrite(TX, LOW);
      Serial.print("1");
    }
    if (bit == 0) {

      digitalWrite(TX, HIGH);
      Serial.print("0");
    }
    waitForClock(false);
  }
}

//***************************************************************************************************
// transmet le code BLE de la carte en test

void transmitPeriphericalName(String name) {

  sendChar('#');  // entete
  for (int i = 0; i < name.length(); i++) {

    char caracter = name.charAt(i);

    sendChar(caracter);
  }
  sendChar('!');  // terminaison
}
//***************************************************************************************************

void transmissionAcqBle() {


  String periphericalName = getPeripheralName();
  displayScanBLE();
  Serial.print("BLE à transmettre: ");
  Serial.println(periphericalName);
  Serial.println("---------------  Fin de la phase d'acquisition BLE ------------------");
  Serial.println("---------------  Début de transmission code BLE  ------------------");

  transmitPeriphericalName(periphericalName);
  Serial.println("---------------  Fin de Transmission code BLE  ------------------");
}


//***************************************************************************************************
// execute le scan BLE

void processingScanBLE() {


  BLE.scan();
  delay(10);
  BLEDevice peripheral = BLE.available();

  if (peripheral) {

    if (peripheral.hasLocalName()) {

      if (peripheral.localName().startsWith("BX")) {


        savePeripheral(peripheral.localName(), phase);


      } else {

        countAcq++;
        if (countAcq > 50) {

          //resetPeripheralList();
          countAcq = 0;
        }
      }
    }
  } else {

    countAcq++;
    if (countAcq > 50) {

      //resetPeripheralList();
      countAcq = 0;
    }
  }
}

//***************************************************************************************************
// Affiche les tableaux d'acquisition des codes BLE.
// Utilisée en debug

void displayScanBLE() {

  Serial.println("Scan init");
  for (int i = 0; i < 10; i++) {

    Serial.println(scanInit[i]);
  }

  Serial.println("--------------------------------");
  Serial.println("Scan final");
  for (int i = 0; i < 10; i++) {

    Serial.println(scanFinal[i]);
  }
}

//***************************************************************************************************

void sendAcq() {

  digitalWrite(TX, HIGH);
  delay(10);
  digitalWrite(TX, LOW);
  delay(2);
}
