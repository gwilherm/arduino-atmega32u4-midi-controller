#include <Control_Surface.h>
#include <EEPROM.h>

#define POT_NB 8
#define BTN_NB 4

USBMIDI_Interface midi;  // Instantiate a MIDI Interface to use

uint8_t default_pot_mcc[] = {MIDI_CC::Sound_Controller_2, MIDI_CC::Sound_Controller_4, MIDI_CC::Portamento_Time, MIDI_CC::Effects_1, 
                             MIDI_CC::Sound_Controller_5, MIDI_CC::Sound_Controller_4, MIDI_CC::Effects_4,       MIDI_CC::Pan};
uint8_t default_btn_mcc[] = {MIDI_CC::Sound_Controller_2, MIDI_CC::Sound_Controller_5, MIDI_CC::Portamento,      MIDI_CC::All_Notes_Off};
bool    default_btn_tog[] = {false,                       false,                       true,                     false};

uint8_t pot_pin[] = {A10, A9, A8, A7,
                     A0,  A1, A2, A3};
uint8_t btn_pin[] = {2, 3, 4, 5};

CCPotentiometer*   pot[POT_NB];
MIDIOutputElement* btn[BTN_NB];
bool               btn_tog[BTN_NB];

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
  uint8_t mcc; // MIDI CC
  bool    tog; // Is toggle button
} btn_t;

typedef union
{
  struct msg
  {
    uint8_t syx_hdr; // 0xF0
    uint8_t msg_idx;
    uint8_t pot_mcc[POT_NB];
    btn_t   btn_cfg[BTN_NB];
    uint8_t syx_ftr; // 0xF7
  } sts;
  uint8_t array[sizeof(struct msg)];
} patch_sts_u;


typedef struct
{
  uint8_t syx_hdr; // 0xF0
  uint8_t msg_idx;
  uint8_t pot_idx;
  uint8_t pot_mcc;
  uint8_t syx_ftr; // 0xF7
} patch_cmd_t;

void sendPatchStatus()
{
  // TODO: To be completed
  patch_sts_u sts;
  sts.sts.syx_hdr = 0xF0;
  sts.sts.syx_ftr = 0xF7;
  sts.sts.msg_idx = PATCH_STS;
  
  for (int i = 0; i < POT_NB; i++)
    if (pot[i])
      sts.sts.pot_mcc[i] = pot[i]->getAddress().getAddress();

  for (int i = 0; i < BTN_NB; i++)
  {
    if (btn[i])
    {
      if (btn_tog[i])
        sts.sts.btn_cfg[i].mcc = static_cast<CCButtonLatched*>(btn[i])->getAddress().getAddress();
      else
        sts.sts.btn_cfg[i].mcc = static_cast<CCButton*>(btn[i])->getAddress().getAddress();

      sts.sts.btn_cfg[i].tog = btn_tog[i];
    }
  }

  midi.sendSysEx(sts.array);
}

void updatePatch(const uint8_t* array, unsigned size)
{
  // TODO: To be completed
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->pot_idx < POT_NB) && (patch->pot_mcc <= 127))
    if (pot[patch->pot_idx])
      pot[patch->pot_idx]->setAddress(patch->pot_mcc);
  }
}

void saveConfig()
{
  // TODO: To be completed
  for (int i = 0; i < POT_NB; i++)
    if (pot[i])
      EEPROM.update(i, pot[i]->getAddress().getAddress());
}

void restoreConfig()
{
  for (int i = 0; i < POT_NB; i++)
  {
    uint8_t mcc = EEPROM.read(i);
    if (mcc <= 127)
      pot[i] = new CCPotentiometer(pot_pin[i], mcc);
    else
      pot[i] = new CCPotentiometer(pot_pin[i], default_pot_mcc[i]);
  }

  for (int i = 0; i < BTN_NB; i++)
  {
    uint8_t mcc = EEPROM.read(POT_NB+i);
    uint8_t tog = EEPROM.read(POT_NB+i+1);
    if (mcc > 127)
      mcc = default_btn_mcc[i];
    if (tog > 127)
      tog = default_btn_tog[i];

    if (tog)
      btn[i] = new CCButtonLatched(btn_pin[i], mcc);
    else
      btn[i] = new CCButton(btn_pin[i], mcc);
    
    btn_tog[i] = tog;
  }
}

void resetConfig()
{
  // TODO: To be completed
  for (int i = 0; i < POT_NB; i++)
    if (pot[i])
      pot[i]->setAddress(default_pot_mcc[i]);
}

// Custom MIDI callback that prints incoming SysEx messages.
struct MyMIDI_Callbacks : MIDI_Callbacks {
 
  // TODO: To be completed
  // This callback function is called when a SysEx message is received.
  void onSysExMessage(MIDI_Interface &, SysExMessage sysex) override {
#ifdef DEBUG
    // Print the message
    Serial << F("Received SysEx message: ")         //
           << AH::HexDump(sysex.data, sysex.length) //
           << F(" on cable ") << sysex.cable.getOneBased() << endl;
#endif

    if (sysex.length < 1) return;
    switch (sysex.data[1])
    {
      case PATCH_REQ:
        sendPatchStatus();
        break;
      case PATCH_POT_CMD:
        updatePatch(sysex.data, sysex.length);
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
} callback {};

void setup()
{
  restoreConfig();
  
  Control_Surface.begin();  // Initialize the Control Surface
  midi.begin();
  midi.setCallbacks(callback);
}

void loop()
{
  Control_Surface.loop();  // Update the Control Surface
  midi.update();
}
