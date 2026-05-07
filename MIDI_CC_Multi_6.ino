#include "MIDIUSB.h"
#include <EEPROM.h>

const byte inputPin[6] = { A0, A1, A2, A3, A4, A5 };
const byte DEFAULT_CC[6] = { 20, 21, 22, 23, 24, 25 };
const byte EEPROM_START = 0;

int reading[6] = { 0 };
int prev[6] = { 0 };
byte ccMapping[6] = { 0 }; // Store current CC mappings

void setup() {
  Serial.begin(9600);
  loadCCMappings(); // Load from EEPROM on startup
  delay(500);
  printStatus();
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    handleSerialCommand();
  }

  // Read analog inputs and send MIDI
  for (int i = 0; i < 6; i++) {
    reading[i] = analogRead(inputPin[i]);
    reading[i] = reading[i] >> 3; // Convert 10-bit to 7-bit

    if (reading[i] != prev[i]) {
      controlChange(0, ccMapping[i], reading[i]);
      MidiUSB.flush();
      prev[i] = reading[i];
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
    // Format: SET:channel:ccValue
    // Example: SET:0:50
    int colon1 = command.indexOf(':');
    int colon2 = command.lastIndexOf(':');
    
    if (colon1 > 0 && colon2 > colon1) {
      byte channel = command.substring(colon1 + 1, colon2).toInt();
      byte ccValue = command.substring(colon2 + 1).toInt();
      
      if (channel < 6 && ccValue >= 0 && ccValue <= 127) {
        ccMapping[channel] = ccValue;
        saveCCMappings();
        Serial.print("✓ Input ");
        Serial.print(channel);
        Serial.print(" now sends CC ");
        Serial.println(ccValue);
      } else {
        Serial.println("✗ Invalid: use SET:0-5:0-127");
      }
    }
  }
  else if (command == "GET" || command == "STATUS") {
    printStatus();
  }
  else if (command == "RESET") {
    for (byte i = 0; i < 6; i++) {
      ccMapping[i] = DEFAULT_CC[i];
    }
    saveCCMappings();
    Serial.println("✓ Reset to defaults");
    printStatus();
  }
  else if (command == "HELP") {
    printHelp();
  }
  else if (command.length() > 0) {
    Serial.println("? Unknown command. Type HELP for options.");
  }
}

void printStatus() {
  Serial.println("\n=== MIDI CC Configuration ===");
  for (byte i = 0; i < 6; i++) {
    Serial.print("Input ");
    Serial.print(i);
    Serial.print(" (A");
    Serial.print(i);
    Serial.print(") → CC ");
    Serial.println(ccMapping[i]);
  }
  Serial.println("=============================\n");
}

void printHelp() {
  Serial.println("\n=== Commands ===");
  Serial.println("SET:channel:ccValue  - Set input to CC (e.g., SET:0:50)");
  Serial.println("STATUS               - Show current mapping");
  Serial.println("GET                  - Show current mapping");
  Serial.println("RESET                - Restore defaults (CC 20-25)");
  Serial.println("HELP                 - Show this message");
  Serial.println("================\n");
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
    // If uninitialized (0xFF) or invalid, use default
    ccMapping[i] = (stored > 127) ? DEFAULT_CC[i] : stored;
  }
}
