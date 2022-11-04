//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include <SPI.h>
#include <MFRC522.h>

#include "BluetoothSerial.h"
#include "config.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Create instances
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool door_locked = true;
unsigned long last_open = 0;
BluetoothSerial SerialBT;
class Terminal {
public:
  Terminal() {
    source = 0;
  }
  int println(char *buff);
  int source;
};

int Terminal::println(char *buff) {
  if (source == CMD_SOURCE_BT) {
    SerialBT.println(buff);
  } else if (source == CMD_SOURCE_SERIAL) {
    Serial.println(buff);
  }
  return 1;
}
Terminal *terminal;

void setup() {
  MasterTag[0] = new String("C1834E24");
  terminal = new Terminal();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  Serial.begin(115200);
  SerialBT.begin("Polyhedra Door");  //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  // Initiating
  SPI.begin();         // SPI bus
  mfrc522.PCD_Init();  // MFRC522
}

void loop() {
  static char cmd = CMD_NOOP;
  uint8_t source = CMD_SOURCE_SERIAL;
  if (Serial.available()) {
    cmd = Serial.read();
    source = CMD_SOURCE_SERIAL;
  } else if (SerialBT.available()) {
    cmd = SerialBT.read();
    source = CMD_SOURCE_BT;
  } else if (
    digitalRead(PIN_BUTTON) == LOW) {
    cmd = CMD_OPEN;
    source = CMD_SOURCE_BUTTON;
  }
  terminal->source = CMD_SOURCE_SERIAL;
  if (source == CMD_SOURCE_BT) {
    terminal->source = CMD_SOURCE_BT;
  }
  if ((cmd == '\r') || (cmd == '\n')) {
    cmd = CMD_NOOP;
  }
  if (((getID()) && (query_access(tagID))) || ((DEBUG == true) && (!tagID.equals(String(""))))) {
    if (door_locked == false) {
      source = CMD_SOURCE_RFID;
      cmd = CMD_OPEN;
      //terminal->println(" Access Granted!  ");
      // You can write any code here like opening doors, switching on a relay, lighting up an LED, or anything else you can think of.
    } else {
      //terminal->println(" Access Denied!  ");
    }
  }
    //Serial.print(tagID);
    tagID = "";
    if (cmd != CMD_NOOP) {
      terminal->println(&cmd);
      switch (tolower(cmd)) {
        case CMD_STATUS:
          if (door_locked == false) {
            terminal->println("Open");
          } else {
            terminal->println("Closed");
          }
          break;
        case CMD_OPEN:
          digitalWrite(PIN_RELAY, HIGH);
          last_open = millis();
          door_locked = false;
          terminal->println("Opened");
          break;
        case CMD_CLOSE:
          digitalWrite(PIN_RELAY, LOW);
          terminal->println("Closed");
          door_locked = true;
          break;
        case CMD_HELP:
          terminal->println("?\tHELP");
          terminal->println("x\tCLOSE");
          terminal->println("v\tOPEN");
          terminal->println("s\tSTATUS");
          break;
      }
      cmd = CMD_NOOP;
    }
    if (door_locked == false) {
      if (millis() - last_open > TIMEON) {
        digitalWrite(PIN_RELAY, LOW);
        door_locked = true;
      }
    }
    delay(20);
  }


  boolean query_access(String tagID) {
    for (int i = 0; i < MEMBERS; i++) {
    if (tagID.equals(*MasterTag[i])) {return true;}
    }
    return false;
  }

  //Read new tag if available
  boolean getID() {
    // Getting ready for Reading PICCs
    if (!mfrc522.PICC_IsNewCardPresent()) {  //If a new PICC placed to RFID reader continue
      return false;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {  //Since a PICC placed get Serial and continue
      return false;
    }
    tagID = "";
    for (uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
      //readCard[i] = mfrc522.uid.uidByte[i];
      tagID.concat(String(mfrc522.uid.uidByte[i], HEX));  // Adds the 4 bytes in a single String variable
    }
    //byte readbackblock[18];
    tagID.toUpperCase();
    mfrc522.PICC_HaltA();  // Stop reading
    return true;
  }