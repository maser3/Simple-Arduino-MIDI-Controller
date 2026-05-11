#include "MIDIUSB.h"
#include <EEPROM.h>

const byte inputPin[6] = { A1, A0, A3, A2, A5, A4 };
const byte DEFAULT_CC[6] = { 20, 21, 22, 23, 24, 25 };
const byte EEPROM_START = 0;

int reading[6] = { 0 };
int prev[6] = { -1, -1, -1, -1, -1, -1 };
byte ccMapping[6] = { 0 };

void setup() {
  Serial.begin(9600);
  loadCCMappings();

  delay(500);
  printStatus();
  printValues();
}

void loop() {
  if (Serial.available()) {
    handleSerialCommand();
  }

  for (int i = 0; i < 6; i++) {
    int rawValue = analogRead(inputPin[i]);

    // Convert 10-bit analogue value, 0-1023, to MIDI value, 0-127
    reading[i] = rawValue >> 3;

    if (reading[i] != prev[i]) {
      controlChange(0, ccMapping[i], reading[i]);
      MidiUSB.flush();

      prev[i] = reading[i];

      // Send live MIDI value back to the browser UI
      Serial.print("A");
      Serial.print(i);
      Serial.print(":");
      Serial.println(reading[i]);
    }

    delay(1);
  }
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}

// ============ SERIAL CONFIGURATION ============

void handleSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();

  if (command.startsWith("SET:")) {
    // Format from browser:
    // SET:inputIndex:ccValue
    // Example:
    // SET:0:74

    int colon1 = command.indexOf(':');
    int colon2 = command.lastIndexOf(':');

    if (colon1 > 0 && colon2 > colon1) {
      int inputIndex = command.substring(colon1 + 1, colon2).toInt();
      int ccValue = command.substring(colon2 + 1).toInt();

      if (inputIndex >= 0 && inputIndex < 6 && ccValue >= 0 && ccValue <= 127) {
        ccMapping[inputIndex] = ccValue;
        saveCCMappings();

        // Machine-readable response for the browser UI
        Serial.print("MAP:A");
        Serial.print(inputIndex);
        Serial.print(":");
        Serial.println(ccValue);

        // Human-readable confirmation
        Serial.print("Input ");
        Serial.print(inputIndex);
        Serial.print(" now sends CC ");
        Serial.println(ccValue);
      } else {
        Serial.println("ERR: Invalid SET command. Use SET:0-5:0-127");
      }
    }
  }

  else if (command == "GET" || command == "STATUS") {
    printStatus();
  }

  else if (command == "VALUES") {
    printValues();
  }

  else if (command == "RESET") {
    for (byte i = 0; i < 6; i++) {
      ccMapping[i] = DEFAULT_CC[i];
    }

    saveCCMappings();

    Serial.println("RESET:OK");
    printStatus();
  }

  else if (command == "HELP") {
    printHelp();
  }

  else if (command.length() > 0) {
    Serial.println("ERR: Unknown command. Type HELP for options.");
  }
}

void printStatus() {
  Serial.println("STATUS:BEGIN");

  for (byte i = 0; i < 6; i++) {
    // Machine-readable mapping for browser UI
    Serial.print("MAP:A");
    Serial.print(i);
    Serial.print(":");
    Serial.println(ccMapping[i]);
  }

  Serial.println("STATUS:END");
}

void printValues() {
  // Compact format supported by the browser UI:
  // VALUES:64,32,0,127,80,90

  Serial.print("VALUES:");

  for (byte i = 0; i < 6; i++) {
    int rawValue = analogRead(inputPin[i]);
    int midiValue = rawValue >> 3;

    if (i > 0) {
      Serial.print(",");
    }

    Serial.print(midiValue);
  }

  Serial.println();
}

void printHelp() {
  Serial.println("=== Commands ===");
  Serial.println("SET:input:ccValue    - Set input to CC, e.g. SET:0:50");
  Serial.println("STATUS               - Show current CC mapping");
  Serial.println("GET                  - Show current CC mapping");
  Serial.println("VALUES               - Show current MIDI values");
  Serial.println("RESET                - Restore defaults, CC 20-25");
  Serial.println("HELP                 - Show this message");
  Serial.println("================");
}

// ============ EEPROM PERSISTENCE ============

void saveCCMappings() {
  for (byte i = 0; i < 6; i++) {
    EEPROM.write(EEPROM_START + i, ccMapping[i]);
  }
}

void loadCCMappings() {
  for (byte i = 0; i < 6; i++) {
    byte stored = EEPROM.read(EEPROM_START + i);

    // If uninitialized, 0xFF, or invalid, use default
    ccMapping[i] = (stored > 127) ? DEFAULT_CC[i] : stored;
  }
}