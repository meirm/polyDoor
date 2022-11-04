#ifndef _DOORLOCK
#define _DOORLOCK
// Definitions
#define PIN_BUTTON 12
#define PIN_RELAY 4
#define TIMEON 5 * 1000

#define RST_PIN 0
#define SS_PIN 5
#define DEBUG false

#define MISO 19
#define MOSI 23
#define SCK 18

/*
Vcc <-> 3V3 (or Vin(5V) depending on the module version)
RST (Reset) <-> D0
GND (Masse) <-> GND
MISO (Master Input Slave Output) <-> 19
MOSI (Master Output Slave Input) <-> 23
SCK (Serial Clock) <-> 18
SS/SDA (Slave select) <-> 5
*/
#define ON 1
#define OFF 0
// PROTOCOLS
#define WIFION ON
#define SSID "polyhedra"
#define PASSWD "polyhedra3d"

// COMMANDS
#define CMD_HELP '?'
#define CMD_OPEN 'v'
#define CMD_CLOSE 'x'
#define CMD_STATUS 's'
#define CMD_NOOP '\0'

#define CMD_SOURCE_BT 0
#define CMD_SOURCE_SERIAL 1
#define CMD_SOURCE_BUTTON 2
#define CMD_SOURCE_RFID 3
#define MEMBERS 20
//GLOBAL VARS
byte readCard[4];
String * MasterTag[MEMBERS] ;	// REPLACE this Tag ID with your Tag ID!!!

String tagID = "";

boolean getID();
boolean query_access(String tagID);
#endif
