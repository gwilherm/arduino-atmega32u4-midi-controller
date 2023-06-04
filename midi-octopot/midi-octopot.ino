#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>

enum {
    MIDI_CC_PORTAMENTO_TIME             = 0x05,
    MIDI_CC_VOLUME                      = 0x07,
    MIDI_CC_PAN_MSB                     = 0x0A,

    MIDI_CC_SOUND_CONTROLLER_2          = 0x47, // Timbre/Harmonic Intensity
    MIDI_CC_SOUND_CONTROLLER_3          = 0x48, // Release Time
    MIDI_CC_SOUND_CONTROLLER_4          = 0x49, // Attack Time
    MIDI_CC_SOUND_CONTROLLER_5          = 0x4A, // Brightness
    MIDI_CC_EFFECTS_1_DEPTH             = 0x5B, // Reverb amount
    MIDI_CC_EFFECTS_4_DEPTH             = 0x5F, // Detune amount
};

#define MIDI_CHANNEL 0
#define POT_NB 8

byte pot_pin[] = {A0,  A1, A2, A3,
                  A10, A9, A8, A7};

byte pot_mcc[] = {MIDI_CC_SOUND_CONTROLLER_2, MIDI_CC_SOUND_CONTROLLER_3, MIDI_CC_PORTAMENTO_TIME, MIDI_CC_EFFECTS_1_DEPTH, 
                  MIDI_CC_SOUND_CONTROLLER_5, MIDI_CC_SOUND_CONTROLLER_4, MIDI_CC_EFFECTS_4_DEPTH, MIDI_CC_PAN_MSB};

byte pot_val[] = {0, 0, 0, 0, 0, 0, 0, 0};

void controlChange(byte channel, byte control, byte value) {

  midiEventPacket_t event = {0x0B, (byte)(0xB0 | channel), control, value};

  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void setup() {
  // put your setup code here, to run once:
   Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (byte i = 0; i < POT_NB; i++)
  {
    byte val = analogRead(pot_pin[i]) / 8;
    if (val != pot_val[i])
    {
      pot_val[i] = val;
      controlChange(MIDI_CHANNEL, pot_mcc[i], pot_val[i]);
    }
    char buf[5];
    sprintf(buf, "%3d ", (int)pot_val[i]);
    Serial.print(buf);
  }
  Serial.println();
}