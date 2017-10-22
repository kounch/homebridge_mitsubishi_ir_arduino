/*
  homebridge mitsubishi vac-ir control from Arduino
  version: 1.0.0
  description: Arduino Sketch for Mitsubihishi VAC IR Remote plugin for homebridge
             https://github.com/kounch/homebridge-mitsubishi-vac-ir

  Copyright (c) 2017 @Kounch

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
  granted, provided that the above copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH
  THE USE OR PERFORMANCE OF THIS SOFTWARE.


  Requires:
  - Temperature sensor (LM35) -> analog pin 0
  - 940nm IR LED (VS1838B) -> PWM pin 3 (required by IRremote2)
  Optional:
  - ESP8266 (v1.3 SDK 2.0 or later) -> pin 4 (TX) & 5 (RX)


  Waits for commands (via serial or TCP/IP on port 80):
  'G' - Get temperature value from LM35
  'S,m,t,f,v,o' - Send Mitsubishi VAC command with IR LED where:
    m - Mode (0-auto, 1-hot, 2-cold, 3-dry, 4-fan)
    t - Desired temperature (celsius, integer value)
    f - Fan (0-auto, 1,2,3,4,5-speed, 6-silent)
    v - Vane (0-auto, 1,2,3,4,5-speed, 6-moving auto)
    o - On/Off (1-On, 0-Off)


  Returns (via serial or TCP/80):
  - BOOT - On startup
  - OK,value - When OK, value is the temperature (obtained or desired)
  - KO,text - When an error happens. Text is a description of the error


  IMPORTANT:
  If using ESP8266, the size of data is too big for arduino sometimes, so the library can't receive the whole buffer
  because the size of the serial buffer which is defined in SoftwareSerial.h is too small. Open the file from
  (..)\avr\libraries\SoftwareSerial\SoftwareSerial.h.
  Look up the following line:
  #define _SS_MAX_RX_BUFF 64
  The default size of the buffer is 64. Change it into a bigger number, like 128 or more.
*/

//#define MITSUBISHI_WIFI

#include "IRremote2.h"

#ifdef MITSUBISHI_WIFI
#include "ESP8266.h"
#include <SoftwareSerial.h>
#endif

const int tmpPin = 0;             // analog pin (temperature)
IRsend irsend;                    //To send infrared commands
const int baudRate = 19200;       //Serial connection speed (USB)
const int LINE_BUFFER_SIZE = 128; // max line length is one less than this

//LED OK delays and count
const int LED_OK_H = 300;
const int LED_OK_L = 300;
const int LED_OK_N = 5;

//LED KO delays and count
const int LED_KO_H = 100;
const int LED_KO_L = 250;
const int LED_KO_N = 12;

#ifdef MITSUBISHI_WIFI
const int rxPin = 4; //Serial RX pin - ESP8266
const int txPin = 5; //Serial TX pin - ESP8266

const char *SSID = "WifiName";
const char *PASSWORD = "WifiPassword";
const char *hostname = "ArduinoHostname";

SoftwareSerial ESPserial(rxPin, txPin); //Wifi
ESP8266 wifi(ESPserial);
#else
char buffer[LINE_BUFFER_SIZE] = {0}; // a String to hold serial incoming data
boolean stringComplete = false;         // whether the string is complete
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(baudRate); //Debug
  digitalWrite(LED_BUILTIN, LOW);

#ifdef MITSUBISHI_WIFI
  wifi.restart();
  if (!wifi.setOprToStation()) {
    Serial.println("ERR: setOprToStation");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  if (!wifi.joinAP(SSID, PASSWORD)) {
    Serial.println("ERR: JoinAP failure");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  if (!wifi.enableMUX()) {
    Serial.println("ERR: enableMU");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  if (!wifi.startTCPServer(80)) {
    Serial.println("ERR: startTCPServer");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  if (!wifi.setTCPServerTimeout(10)) {
    Serial.print("ERR: setTCPServerTimeout");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  ledBlink(LED_OK_H, LED_OK_L, LED_OK_N);
  if (!wifi.enableMDNS(hostname, hostname, 80)) {
    Serial.print("ERR: enableMDNS");
    ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
  }
  ledBlink(LED_OK_H, LED_OK_L, LED_OK_N);
#endif

  Serial.println("BOOT");
}

void loop() {
#ifdef MITSUBISHI_WIFI
  processWifi();
#else
  char answer[20] = {0};

  if (stringComplete) {
    parseCommand(buffer, sizeof(buffer), answer); //Parse and execute
    Serial.print(answer);
    // clear the string:
    buffer[0] = 0;
    stringComplete = false;
  }
#endif
}

#ifdef MITSUBISHI_WIFI
void processWifi() {
  char buffer[LINE_BUFFER_SIZE] = {0};
  char answer[20] = {0};
  char message[20];
  uint8_t mux_id;

  uint32_t len = wifi.recv(&mux_id, (uint8_t *)buffer, sizeof(buffer), 100);
  if (len > 0) {
    sprintf(message, "Received from: %i", mux_id);
    Serial.println(message);
    Serial.print((char *)buffer);
    parseCommand(buffer, sizeof(buffer), answer); //Parse and execute
    wifi.send(mux_id, (const uint8_t *)answer, strlen(answer));
    Serial.println(answer);
    delay(100);

    if (!wifi.releaseTCP(mux_id)) {
      sprintf(message, "ERR: releaseTCP: %i", mux_id);
      Serial.println(message);
      ledBlink(LED_KO_H, LED_KO_L, LED_KO_N);
    }
  }
}
#else
/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    int index = strlen(buffer);
    if (index < LINE_BUFFER_SIZE) {
      char ch = Serial.read(); // read next character
      //Serial.print(ch); // echo it back
      if (ch == '\n') {
        buffer[index] = 0;
        stringComplete = true;
      } else {
        buffer[index] = ch;
      }
    } else {
      char ch;
      do {
        // Wait until characters are available
        while (Serial.available() == 0)
        {
        }
        ch = Serial.read(); // read next character (and discard it)
        //Serial.print(ch); // echo it back
      } while (ch != '\n');

      buffer[0] = 0; // set buffer to empty string even though it should not be used
      stringComplete = false;
    }
  }
}
#endif

//Split command using "," extract values and execute command
char *parseCommand(char *commandLine, int bufsize, char *resultado) {
  int mode = -1;
  int temp = -1;
  int fan = -1;
  int vane = -1;
  bool off = false;
  char str_temp[6];
  //char resultado[30];

  int i = 0;                                //Index of command in command line
  char *command = strtok(commandLine, ","); //Tokenize
  while (command != 0) {
    switch (i++) {
      case 0:
        if (strcmp(command, "G") == 0) {
          // Get Temp
          float fTemp = getTemp();
          dtostrf(fTemp, 4, 2, str_temp);
          sprintf(resultado, "OK,%s\n", str_temp);
          return (resultado);
        } else if (strcmp(command, "S") == 0) {
          //Set Temp
        } else {
          //Unknown
          sprintf(resultado, "KO,Unknown Command\n");
          return (resultado);
        }
        break;
      case 1:
        mode = atoi(command);
        break;
      case 2:
        temp = atoi(command);
        break;
      case 3:
        fan = atoi(command);
        break;
      case 4:
        vane = atoi(command);
        break;
      case 5:
        off = 1 - atoi(command);
      default:
        //Default
        break;
    }
    command = strtok(0, ","); //Get next command (token)
  }

  if (vane < 0) {
    sprintf(resultado, "KO,Not enough parameters\n");
    return (resultado);
  }

  setTemp(mode, temp, fan, vane, off);
  sprintf(resultado, "OK,%i\n", temp);
  return (resultado);
}

//Get Temperature
float getTemp() {
  int value = analogRead(tmpPin);
  float celsius = (value / 1024.0) * 500;
  return celsius;
}

//Set temperature
void setTemp(int mode, int temp, int fan, int vane, bool off) {
  int modes[] = {HVAC_AUTO, HVAC_HOT, HVAC_COLD, HVAC_DRY, HVAC_FAN};
  int fans[] = {FAN_SPEED_AUTO, FAN_SPEED_1, FAN_SPEED_2, FAN_SPEED_3, FAN_SPEED_4, FAN_SPEED_5, FAN_SPEED_SILENT};
  int vanes[] = {VANNE_AUTO, VANNE_H1, VANNE_H2, VANNE_H3, VANNE_H4, VANNE_H5, VANNE_AUTO_MOVE};

  //HVAC Mode
  if (mode >= 0 && mode < sizeof(modes)) {
    mode = modes[mode];
  } else {
    mode = modes[0];
  }

  //HVAC Fan
  if (fan >= 0 && fan < sizeof(fans)) {
    fan = fans[fan];
  } else {
    fan = fans[0];
  }

  //HVAC Vane
  if (vane >= 0 && vane < sizeof(vanes)) {
    vane = vanes[vane];
  } else {
    vane = vanes[sizeof(vanes) - 1];
  }

  //Send Command
  irsend.sendHvacMitsubishi(mode, temp, fan, vane, off);
  return;
}

//LED blinking for notifications
void ledBlink(int pauseH, int pauseL, int number) {
  digitalWrite(LED_BUILTIN, LOW);
  for (int i = 0; i < number; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(pauseH);
    digitalWrite(LED_BUILTIN, LOW);
    delay(pauseL);
  }
}
