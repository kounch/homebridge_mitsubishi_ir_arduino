/*
homebridge mitsubishi vac-ir control from Arduino
version: 0.0.1
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

Waits for commands (via serial):
  'G' - Get temperature value from LM35
  'S,m,t,f,v,o' - Send Mitsubishi VAC command with IR LED where:
    m - Mode (0-auto, 1-hot, 2-cold, 3-dry, 4-fan)
    t - Desired temperature (celsius, integer value)
    f - Fan (0-auto, 1,2,3,4,5-speed, 6-silent)
    v - Vane (0-auto, 1,2,3,4,5-speed, 6-moving auto)
    o - On/Off (1-On, 0-Off)

Returns (via serial):
  - BOOT - On startup
  - OK,value - When OK, value is the temperature (obtained or desired)
  - KO,text - When an error happens. Text is a description of the error
*/

#include "IRremote2.h"

const int tmpPin = 0; // analog pin (temperature)
const int LINE_BUFFER_SIZE = 80; // max line length is one less than this
IRsend irsend; //To send infrared commands

void setup() {
  Serial.begin(9600);
  Serial.println("BOOT");
}

void loop() {
  char line[LINE_BUFFER_SIZE];
  if (read_line(line, sizeof(line)) < 0) {
    Serial.println("KO,Line too long");
    return; // skip command processing and try again on next iteration of loop
  }
  parseCommand(line, sizeof(line)); //Parse an execute
}

//Split command using "," extract values and execute command
void parseCommand(char* commandLine, int bufsize) {
  int mode = -1;
  int temp = -1;
  int fan = -1;
  int vane  = -1;
  bool off = false;

  int i = 0; //Index of command in command line
  char* command = strtok(commandLine, ","); //Tokenize
  while (command != 0)
  {
    switch (i++) {
      case 0:
        if (strcmp(command, "G")  == 0) {
          // Get Temp
          float fTemp = getTemp();
          Serial.write("OK,");
          Serial.println(fTemp);
          return;
        } else if (strcmp(command, "S")  == 0) {
          //Set Temp
        } else {
          //Unknown
          Serial.println("KO,Unknown Command");
          return;
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

  if (vane < 0 ) {
    Serial.println("KO,Not enough parameters");
    return;
  }

  setTemp (mode, temp, fan, vane, off);
  Serial.print("OK,");
  Serial.println(temp);
  return;
}

//Get Temperature
float getTemp() {
  int value = analogRead(tmpPin);
  float celsius = ( value / 1024.0 ) * 500;
  return celsius;
}

//Set temperature
void setTemp (int mode, int temp, int fan, int vane, bool off) {
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

//Read line from Serial
int read_line(char* buffer, int bufsize)
{
  for (int index = 0; index < bufsize; index++) {
    // Wait until characters are available
    while (Serial.available() == 0) {
    }

    char ch = Serial.read(); // read next character
    //Serial.print(ch); // echo it back: useful with the serial monitor (optional)

    if (ch == '\n') {
      buffer[index] = 0; // end of line reached: null terminate string
      return index; // success: return length of string (zero if string is empty)
    }

    buffer[index] = ch; // Append character to buffer
  }

  // Reached end of buffer, but have not seen the end-of-line yet.
  // Discard the rest of the line (safer than returning a partial line).
  char ch;
  do {
    // Wait until characters are available
    while (Serial.available() == 0) {
    }
    ch = Serial.read(); // read next character (and discard it)
    //Serial.print(ch); // echo it back
  } while (ch != '\n');

  buffer[0] = 0; // set buffer to empty string even though it should not be used
  return -1; // error: return negative one to indicate the input was too long
}
