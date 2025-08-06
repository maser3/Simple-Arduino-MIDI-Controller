#include "MIDIUSB.h"

const byte inputPin[6] = { A0, A1, A2, A3, A4, A5 };

int reading[6] = { 0 };
int prev[6] = { 0 };   

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (int i = 0; i <= 5; i++) {

    reading[i] = analogRead(inputPin[i]);
    reading[i] = reading[i] >> 3;

    if (reading[i] != prev[i]) {
      controlChange(0, 20+i, reading[i]);
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

