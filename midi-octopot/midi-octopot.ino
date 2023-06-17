#include <USB-MIDI.h>
#include <EEPROM.h>

USBMIDI_CREATE_DEFAULT_INSTANCE();

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

#define MIDI_CHANNEL 1
#define POT_NB 8

byte pot_pin[] = {A10, A9, A8, A7,
                  A0,  A1, A2, A3};

byte default_pot_mcc[] = {MIDI_CC_SOUND_CONTROLLER_2, MIDI_CC_SOUND_CONTROLLER_3, MIDI_CC_PORTAMENTO_TIME, MIDI_CC_EFFECTS_1_DEPTH, 
                          MIDI_CC_SOUND_CONTROLLER_5, MIDI_CC_SOUND_CONTROLLER_4, MIDI_CC_EFFECTS_4_DEPTH, MIDI_CC_PAN_MSB};

byte pot_mcc[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte pot_val[] = {0, 0, 0, 0, 0, 0, 0, 0};

enum
{
  PATCH_REQ, // In:  Request for current configuration
  PATCH_STS, // Out: Send configuration
  PATCH_CMD, // In:  New patch command
  SAVE_CMD,  // In:  Save current configuration command
  RESET_CMD  // In:  Restore default configuration
};

typedef struct
{
  byte msg_idx;
  byte pot_mcc[POT_NB];
} patch_sts_t;

typedef struct
{
  byte syx_hdr; // 0xF0
  byte msg_idx;
  byte pot_idx;
  byte pot_mcc;
  byte syx_ftr; // 0xF7
} patch_cmd_t;

void sendPatchStatus()
{
  patch_sts_t sts;
  sts.msg_idx = PATCH_STS;
  memcpy(&sts.pot_mcc, &pot_mcc, POT_NB);
  MIDI.sendSysEx(sizeof(patch_sts_t), (byte*)&sts);
}

void updatePatch(byte* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->pot_idx < POT_NB) && (patch->pot_mcc <= 127))
      pot_mcc[patch->pot_idx] = patch->pot_mcc;
  }
}

void saveConfig()
{
  for (int i = 0; i < POT_NB; i++)
    EEPROM.update(i, pot_mcc[i]);
}

void restoreConfig()
{
  for (int i = 0; i < POT_NB; i++)
  {
    byte value = EEPROM.read(i);
    if (value <= 127)
      pot_mcc[i] = value;
    else
      pot_mcc[i] = default_pot_mcc[i];
  }
}

void resetConfig()
{
  for (int i = 0; i < POT_NB; i++)
    pot_mcc[i] = default_pot_mcc[i];
}

void handleSysEx(byte* array, unsigned size)
{
  if (size < 1) return;
  switch (array[1])
  {
    case PATCH_REQ:
      sendPatchStatus();
      break;
    case PATCH_CMD:
      updatePatch(array, size);
      break;
    case SAVE_CMD:
      saveConfig();
      break;
    case RESET_CMD:
      resetConfig();
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  restoreConfig();

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleSystemExclusive(handleSysEx);
}

void loop() {
  MIDI.read();

  char serial_output[32];
  bool display_update = false;

  for (byte i = 0; i < POT_NB; i++)
  {
    byte val = analogRead(pot_pin[i]) / 8;
    if (val != pot_val[i])
    {
      display_update = true;
      pot_val[i] = val;
      MIDI.sendControlChange(pot_mcc[i], pot_val[i], MIDI_CHANNEL);
    }

    sprintf(serial_output+3*i+i, "%03d ", (int)pot_val[i]);
  }

  if (display_update)
    Serial.println(serial_output);
}
