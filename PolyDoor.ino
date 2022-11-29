//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include "config.h"
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>

// Create instances
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool door_locked = true;
bool door_closed = false;
unsigned long door_open = 0;
unsigned long last_open = 0;
const char *ssid = "Polyhedra";
const char *password = "polyhedra3d";

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 10000;


char cmd = CMD_NOOP;

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long bot_lasttime;
char cmd_telegram = CMD_NOOP;
String urlFinal = "";
String nullstring = "";
String logTrigger = "";
String output4State = "off";
int rfidLastUpdate = RFIDUPDATETIME;
bool rfidinit = false;
TaskHandle_t taskhandle_1;
WiFiServer server(80);
String header;  //holds HTTP request

uint8_t source = CMD_SOURCE_SERIAL;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  pinMode(DOOR_C, OUTPUT);
  //PinMode(DOOR_NO, INPUT);
  pinMode(DOOR_NC, INPUT_PULLUP);
  Serial.begin(115200);

  Serial.println("The device started, now you can pair it with bluetooth!");

  // Initiating
  SPI.begin();         // SPI bus
  mfrc522.PCD_Init();  // MFRC522

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

void loop() {
  get_serial();
  getDoorStatus();
  process_rfid();
  process_telegram();
  process_http_client();

  //---COMMANDS--------
  if (cmd != CMD_NOOP) {
    process_cmd();
  }

  closeDoorOnTimeout();

  if (!(logTrigger == "")) {
    reportActivity();
  }
  tagID = "";
  delay(20);
}


void reportActivity() {
  urlFinal = "https://script.google.com/macros/s/" + String(GOOGLE_SCRIPT_ID) + "/exec?sheet=rfidlog";
  if (logTrigger == "button") {
    urlFinal += "&logline=button%20pressed";
    bot.sendMessage(CHAT_ID_1, "The door was opened by switch");
  } else if (logTrigger == "rfid") {
    urlFinal += "&logline=Access%20granted%20to%20rfid%3A%20" + tagID + "%20%28" + tagName + "%29";
    bot.sendMessage(CHAT_ID_1, "The door was opened with RFID tag: " + tagID + "(" + tagName + ")", "");
  } else if (logTrigger == "alert") {
    urlFinal += "&logline=Access%20denied%20to%20rfid%3A%20" + tagID + "%20%28" + tagName + "%29";
    bot.sendMessage(CHAT_ID_1, "Access denied to RFID tag: " + tagID + "(" + tagName + ")", "");
  } else if (logTrigger == "heartbit") {
    urlFinal += "&logline=heartbit";
    bot.sendMessage(CHAT_ID_1, "heartbit");
  }
  Serial.print("POST data to spreadsheet:");
  Serial.println(urlFinal);
  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Payload: " + payload);
  }
  http.end();
  logTrigger = "";
  urlFinal = "";
}


void closeDoorOnTimeout() {
  if (door_locked == false) {
    if (millis() - last_open > TIMEON) {
      digitalWrite(PIN_RELAY, LOW);
      door_locked = true;
    }
  }
}

void get_serial() {
  if (Serial.available()) {
    cmd = Serial.read();
    source = CMD_SOURCE_SERIAL;
  } else if (
    digitalRead(PIN_BUTTON) == LOW) {
    cmd = CMD_OPEN;
    source = CMD_SOURCE_BUTTON;
    logTrigger = "button";
  }
  if ((cmd == '\r') || (cmd == '\n')) {
    cmd = CMD_NOOP;
  }
}

void process_telegram() {
  //    ----READ
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      cmd = cmd_telegram;
    }
    bot_lasttime = millis();
  }
}

void getDoorStatus() {
  digitalWrite(DOOR_C, HIGH);
  delay(5);
  if (door_locked == true) {
    if (digitalRead(DOOR_NC) == LOW) {
      door_closed = true;
    } else  //if (digitalRead(DOOR_NO) == LOW)
    {
      door_closed = false;
    }
  }
  digitalWrite(DOOR_C, LOW);
}

void process_rfid() {
  if (getID()) {
    if ((query_access(tagID)) || ((DEBUG == true) && (!tagID.equals(String(""))))) {
      if (door_locked == true) {
        source = CMD_SOURCE_RFID;
        cmd = CMD_OPEN;
        Serial.println(" Access Granted!  ");
        logTrigger = "rfid";
        // You can write any code here like opening doors, switching on a relay, lighting up an LED, or anything else you can think of.
      }
    } else {
      Serial.println(" Access Denied!  ");
      bot.sendMessage(CHAT_ID_1, "Unauthorized RFID: " + tagID, "");
      logTrigger = "alert";
    }
  }
}
void process_cmd() {
  Serial.println(&cmd);
  switch (tolower(cmd)) {
    case CMD_STATUS:
      if (door_locked == false) {
        Serial.println("Open");
      } else {
        Serial.println("Closed");
      }
      break;
    case CMD_OPEN:
      digitalWrite(PIN_RELAY, HIGH);
      last_open = millis();
      door_locked = false;
      Serial.println("Opened");
      break;
    case CMD_CLOSE:
      digitalWrite(PIN_RELAY, LOW);
      Serial.println("Closed");
      door_locked = true;
      break;
    case CMD_HELP:
      Serial.println("?\tHELP");
      Serial.println("x\tCLOSE");
      Serial.println("v\tOPEN");
      Serial.println("s\tSTATUS");
      break;
  }
  cmd = CMD_NOOP;
}

void process_http_client() {
  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {                            // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");                                             // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              cmd = CMD_OPEN;
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              cmd = CMD_CLOSE;
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button {background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".errorbar {background-color:#ff000; width:100%;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            if (door_locked == true) {
              output4State = "on";
            } else {
              output4State = "off";
            }
            // Display current state, and ON/OFF buttons for GPIO 26
            client.println("<p>GPIO 4 - State " + output4State + "</p>");
            // If the output26State is off, it displays the ON button
            if (door_locked == true) {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">OPEN DOOR</button></a></p>");
              if (door_closed == false) {
                client.println("<div class=\"errorbar\">THE DOOR IS NOT CLOSED!</div>");
              }
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">CLOSE DOOR</button></a></p>");
            }
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

boolean query_access(String tagID) {
  return authorize(tagID);
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
  Serial.println(tagID);
  mfrc522.PICC_HaltA();  // Stop reading
  return true;
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, "");
    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    String chat_id = bot.messages[i].chat_id;
    if ((chat_id != CHAT_ID_1) && (chat_id != CHAT_ID_2)) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    if ((text == "/start") || (text == "/help")) {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/open \n";
      welcome += "/close \n";
      welcome += "/state \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/open") {
      bot.sendMessage(chat_id, "DOOR OPEN", "");
      //ledState = HIGH;
      digitalWrite(PIN_RELAY, HIGH);
      cmd_telegram = CMD_OPEN;
    }
    if (text == "/close") {
      bot.sendMessage(chat_id, "DOOR CLOSE", "");
      //ledState = LOW;
      digitalWrite(PIN_RELAY, LOW);
      door_locked = false;
    }
    if (text == "/state") {
      if (door_locked == true) {
        if (door_closed == true) bot.sendMessage(chat_id, "The door is locked and closed.", "");
        else bot.sendMessage(chat_id, "The door is not closed", "");
      } else bot.sendMessage(chat_id, "The door is unlocked", "");
    }
  }
}


bool authorize(String _tagID) {
  rfidinit = true;
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  HTTPClient http;
  urlFinal = "https://script.google.com/macros/s/" + String(GOOGLE_SCRIPT_ID) + "/exec?sheet=rfidlist&authorize&value=" + _tagID;
  Serial.println("Getting:");
  Serial.println(urlFinal);
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  //Serial.print("HTTP Status Code: ");
  //Serial.println(httpCode);
  String payload1 = String(http.getString());
  Serial.println("String value:");
  Serial.println(payload1);
  JSONVar myArray = JSON.parse(payload1);
  http.end();
  tagID = _tagID;
  tagName = "Nishta";
  logTrigger = "authorization";
  if (JSON.typeof(myArray["name"]) == "undefined") {
    Serial.println("Parsing input failed!");

    Serial.println(JSON.stringify(myArray));
    urlFinal = "";
    rfidinit = false;
    vTaskDelete(taskhandle_1);
    return false;
  } else {
    if (String((const char *)myArray["name"]) == "Nishta") {
      tagName = "Nishta";
      return false;
    } else {
      tagName = (const char *)myArray["name"];
      if (String((const char *)myArray["authorization"]) == "success") return true;
      return false;
    }
  }
}