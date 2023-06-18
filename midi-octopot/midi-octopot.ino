#include <Control_Surface.h>
#include <EEPROM.h>

#define POT_NB 8

USBMIDI_Interface midi;  // Instantiate a MIDI Interface to use

uint8_t default_pot_mcc[] = {MIDI_CC::Sound_Controller_2, MIDI_CC::Sound_Controller_4, MIDI_CC::Portamento_Time,  MIDI_CC::Effects_1, 
                             MIDI_CC::Sound_Controller_5, MIDI_CC::Sound_Controller_4, MIDI_CC::Effects_4,        MIDI_CC::Pan};

CCPotentiometer pot[] = {
  { A10, 0 }, { A9, 0 }, { A8, 0 }, { A7, 0 },
  { A0,  0 }, { A1, 0 }, { A2, 0 }, { A3, 0 }
};

enum
{
  PATCH_REQ, // In:  Request for current configuration
  PATCH_STS, // Out: Send configuration
  PATCH_CMD, // In:  New patch command
  SAVE_CMD,  // In:  Save current configuration command
  RESET_CMD  // In:  Restore default configuration
};

typedef union
{
  struct msg
  {
    uint8_t syx_hdr; // 0xF0
    uint8_t msg_idx;
    uint8_t pot_mcc[POT_NB];
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
  patch_sts_u sts;
  sts.sts.syx_hdr = 0xF0;
  sts.sts.syx_ftr = 0xF7;
  sts.sts.msg_idx = PATCH_STS;
  
  for (int i = 0; i < POT_NB; i++)
    sts.sts.pot_mcc[i] = pot[i].getAddress().getAddress();

  midi.sendSysEx(sts.array);
}

void updatePatch(const uint8_t* array, unsigned size)
{
  if (size == sizeof(patch_cmd_t))
  {
    patch_cmd_t* patch = (patch_cmd_t*)array;
    if ((patch->pot_idx < POT_NB) && (patch->pot_mcc <= 127))
      pot[patch->pot_idx].setAddress(patch->pot_mcc);
  }
}

void saveConfig()
{
  for (int i = 0; i < POT_NB; i++)
    EEPROM.update(i, pot[i].getAddress().getAddress());
}

void restoreConfig()
{
  for (int i = 0; i < POT_NB; i++)
  {
    uint8_t value = EEPROM.read(i);
    if (value <= 127)
      pot[i].setAddress(value);
    else
      pot[i].setAddress(default_pot_mcc[i]);
  }
}

void resetConfig()
{
  for (int i = 0; i < POT_NB; i++)
    pot[i].setAddress(default_pot_mcc[i]);
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
      case PATCH_CMD:
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

void setup() {
  
  restoreConfig();
  
  Control_Surface.begin();  // Initialize the Control Surface
  midi.begin();
  midi.setCallbacks(callback);
}

void loop() {
  Control_Surface.loop();  // Update the Control Surface
  midi.update();
}
