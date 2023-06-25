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
  uint8_t ctl_idx;
  uint8_t ctl_val;
  uint8_t syx_ftr; // 0xF7
} patch_cmd_t;

void sendPatchStatus()
{
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

void updatePotPatch(const uint8_t* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->ctl_idx < POT_NB) && (patch->ctl_val <= 127))
      if (pot[patch->ctl_idx])
        pot[patch->ctl_idx]->setAddress(patch->ctl_val);
  }
}

void updateBtnPatch(const uint8_t* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->ctl_idx < BTN_NB) && (patch->ctl_val <= 127))
    {
      if (btn[patch->ctl_idx])
      {
        if (btn_tog[patch->ctl_idx])
          static_cast<CCButtonLatched*>(btn[patch->ctl_idx])->setAddressUnsafe(patch->ctl_val);
        else
          static_cast<CCButton*>(btn[patch->ctl_idx])->setAddressUnsafe(patch->ctl_val);
      }
    }
  }
}

void updateBtnToggle(byte* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->ctl_idx < BTN_NB) && (patch->ctl_val <= 1))
    {
      uint8_t mcc = default_btn_mcc[patch->ctl_idx];
      if (btn[patch->ctl_idx])
      {
        if (btn_tog[patch->ctl_idx])
          mcc = static_cast<CCButtonLatched*>(btn[patch->ctl_idx])->getAddress().getAddress();
        else
          mcc = static_cast<CCButton*>(btn[patch->ctl_idx])->getAddress().getAddress();
        delete btn[patch->ctl_idx];
      }
      
    if ((bool)patch->ctl_val)
      btn[patch->ctl_idx] = new CCButtonLatched(btn_pin[patch->ctl_idx], mcc);
    else
      btn[patch->ctl_idx] = new CCButton(btn_pin[patch->ctl_idx], mcc);
    }
  }
}

void saveConfig()
{
  for (int i = 0; i < POT_NB; i++)
  {
    if (pot[i])
      EEPROM.update(i, pot[i]->getAddress().getAddress());
  }

  for (int i = 0; i < BTN_NB; i++)
  {
    uint8_t mcc = default_btn_mcc[i];
    if (btn[i])
    {
      if (btn_tog[i])
        mcc = static_cast<CCButtonLatched*>(btn[i])->getAddress().getAddress();
      else
        mcc = static_cast<CCButton*>(btn[i])->getAddress().getAddress();
    }
    EEPROM.update(POT_NB+i,   mcc);
    EEPROM.update(POT_NB+i+1, btn_tog[i]);
  }
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
  for (int i = 0; i < POT_NB; i++)
    if (pot[i])
      pot[i]->setAddress(default_pot_mcc[i]);

  for (int i = 0; i < BTN_NB; i++)
  {
    uint8_t mcc = default_btn_mcc[i];
    if (default_btn_tog[i])
      btn[i] = new CCButtonLatched(btn_pin[i], mcc);
    else
      btn[i] = new CCButton(btn_pin[i], mcc);
  }
}

// Custom MIDI callback that prints incoming SysEx messages.
struct MyMIDI_Callbacks : MIDI_Callbacks {
 
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
        updatePotPatch(sysex.data, sysex.length);
        break;
      case PATCH_BTN_CMD:
        updateBtnPatch(sysex.data, sysex.length);
        break;
      case TOGGLE_BTN_CMD:
        updateBtnToggle(sysex.data, sysex.length);
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
