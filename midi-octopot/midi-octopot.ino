#include <USB-MIDI.h>
#include <EEPROM.h>

USBMIDI_CREATE_DEFAULT_INSTANCE();

enum {
    MIDI_CC_PORTAMENTO_TIME             = 0x05,
    MIDI_CC_VOLUME                      = 0x07,
    MIDI_CC_PAN_MSB                     = 0x0A,

    MIDI_CC_PORTAMENTO_ON_OFF           = 0x41,
    MIDI_CC_SOUND_CONTROLLER_2          = 0x47, // Timbre/Harmonic Intensity
    MIDI_CC_SOUND_CONTROLLER_3          = 0x48, // Release Time
    MIDI_CC_SOUND_CONTROLLER_4          = 0x49, // Attack Time
    MIDI_CC_SOUND_CONTROLLER_5          = 0x4A, // Brightness
    MIDI_CC_EFFECTS_1_DEPTH             = 0x5B, // Reverb amount
    MIDI_CC_EFFECTS_4_DEPTH             = 0x5F, // Detune amount
    MIDI_CC_ALL_NOTES_OFF               = 0x7B, // Midi Panic
};

#define MIDI_CHANNEL 1
#define POT_NB 8
#define BTN_NB 4

enum
{
  PATCH_REQ,      // In:  Request for current configuration
  PATCH_STS,      // Out: Send configuration
  PATCH_POT_CMD,  // In:  Change a pot patch
  PATCH_BTN_CMD,  // In:  Change a button patch
  TOGGLE_BTN_CMD, // In:  Change a button toggle
  SAVE_CMD,       // In:  Save current configuration command
  RESET_CMD       // In:  Restore default configuration
};

typedef struct
{
  byte mcc; // MIDI CC
  bool tog; // Is toggle button
} btn_t;

typedef struct
{
  byte msg_idx;
  byte  pot_mcc[POT_NB];
  btn_t btn_cfg[BTN_NB];
} patch_sts_t;

typedef struct
{
  byte syx_hdr; // 0xF0
  byte msg_idx;
  byte idx;
  byte val;
  byte syx_ftr; // 0xF7
} patch_cmd_t;

// Pots
byte pot_pin[] = {A10, A9, A8, A7,
                  A0,  A1, A2, A3};

byte default_pot_mcc[] = {MIDI_CC_SOUND_CONTROLLER_2, MIDI_CC_SOUND_CONTROLLER_3, MIDI_CC_PORTAMENTO_TIME, MIDI_CC_EFFECTS_1_DEPTH, 
                          MIDI_CC_SOUND_CONTROLLER_5, MIDI_CC_SOUND_CONTROLLER_4, MIDI_CC_EFFECTS_4_DEPTH, MIDI_CC_PAN_MSB};

byte pot_mcc[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte pot_val[] = {0, 0, 0, 0, 0, 0, 0, 0};

// Buttons
byte btn_pin[] = {2, 3, 4, 5};

byte default_btn_mcc[] = {MIDI_CC_SOUND_CONTROLLER_2, MIDI_CC_SOUND_CONTROLLER_5, MIDI_CC_PORTAMENTO_ON_OFF, MIDI_CC_ALL_NOTES_OFF};
byte default_btn_tog[] = {                     false,                      false,                      true,                 false};

btn_t btn_cfg[BTN_NB];
int  btn_state[] = {0, 0, 0, 0};
bool btn_value[] = {0, 0, 0, 0};

void sendPatchStatus()
{
  patch_sts_t sts;
  sts.msg_idx = PATCH_STS;
  memcpy(&sts.pot_mcc, &pot_mcc, POT_NB);
  memcpy(&sts.btn_cfg, &btn_cfg, BTN_NB*sizeof(btn_t));
  MIDI.sendSysEx(sizeof(patch_sts_t), (byte*)&sts);
}

void updatePotPatch(byte* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if (patch->idx < POT_NB)
      pot_mcc[patch->idx] = patch->val;
  }
}

void updateBtnPatch(byte* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if (patch->idx < BTN_NB)
      btn_cfg[patch->idx].mcc = patch->val;
  }
}

void updateBtnToggle(byte* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if (patch->idx < BTN_NB)
      btn_cfg[patch->idx].tog = patch->val;
  }
}

void saveConfig()
{
  for (int i = 0; i < POT_NB; i++)
    EEPROM.update(i, pot_mcc[i]);
  for (int i = 0; i < BTN_NB; i++)
  {
    EEPROM.update(POT_NB+i,   btn_cfg[i].mcc);
    EEPROM.update(POT_NB+i+1, btn_cfg[i].tog);
  }
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

  for (int i = 0; i < BTN_NB; i++)
  {
    byte value = EEPROM.read(POT_NB+i);
    if (value <= 127)
      btn_cfg[i].mcc = value;
    else
      btn_cfg[i].mcc = default_btn_mcc[i];

    value = EEPROM.read(POT_NB+i+1);
    if (value <= 127)
      btn_cfg[i].tog = (bool)value;
    else
      btn_cfg[i].tog = default_btn_tog[i];
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
    case PATCH_POT_CMD:
      updatePotPatch(array, size);
      break;
    case PATCH_BTN_CMD:
      updateBtnPatch(array, size);
      break;
    case TOGGLE_BTN_CMD:
      updateBtnToggle(array, size);
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

  memset(&btn_cfg, 0, sizeof(btn_t)*BTN_NB);

  restoreConfig();

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleSystemExclusive(handleSysEx);

  for (byte i = 0; i < BTN_NB; i++)
    pinMode(btn_pin[i], INPUT);
}

void loop() {
  MIDI.read();

  char serial_output[POT_NB*4 + BTN_NB*4 + 1];
  bool display_update = false;

  // Pots
  for (byte i = 0; i < POT_NB; i++)
  {
    byte val = analogRead(pot_pin[i]) / 8;
    if (val != pot_val[i])
    {
      display_update = true;
      pot_val[i] = val;
      MIDI.sendControlChange(pot_mcc[i], pot_val[i], MIDI_CHANNEL);
    }

    sprintf(serial_output+4*i, "%03d ", (int)pot_val[i]);
  }

  // Buttons
  for (byte i = 0; i < BTN_NB; i++)
  {
    bool update = false;
    int state = digitalRead(btn_pin[i]);
    if (state != btn_state[i])
    {
      btn_state[i] = state;
      if (btn_cfg[i].tog)
      {
        if (state == HIGH)
        {
          update = true;
          btn_value[i] = !btn_value[i];
        }
      }
      else
      {
        update = true;
        btn_value[i] = btn_state[i];
      }

      if (update)
      {
        display_update = true;
        MIDI.sendControlChange(btn_cfg[i].mcc, btn_value[i]*127, MIDI_CHANNEL);
      }
    }
    sprintf(serial_output+POT_NB*4+2*i, "%d ", btn_value[i]);
  }
  
  if (display_update)
    Serial.println(serial_output);
}
