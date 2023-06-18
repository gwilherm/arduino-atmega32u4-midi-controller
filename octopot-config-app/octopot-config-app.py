#!/bin/env python3

import tkinter as tk
from PIL import Image, ImageTk
from enum import IntEnum

import argparse
import mido
import sys
import os

POT_NB      = 8
BTN_NB      = 4
REQUEST_REC = 2000

class SysExMsg(IntEnum):
  PATCH_REQ = 0 # Out: Request for current config
  PATCH_STS = 1 # In:  Send the current config
  PATCH_POT_CMD = 2 # Out: Change a pot patch
  PATCH_BTN_CMD = 3 # Out: Change a button patch
  TOGGLE_BTN_CMD = 4 # Out: Change a button toggle
  SAVE_CMD  = 5 # Out: Save the current config
  RESET_CMD = 6 # Out: Save the current config

class Root:
    midi_in = None
    midi_out = None
    def __init__(self):
        """ Initialization """
        self.root = tk.Tk()
        self.root.title('Octopot Configuration')
        self.root.resizable(0, 0)

        im = Image.open(os.path.join(sys.path[0], 'octopot.png'))
        octopot = ImageTk.PhotoImage(im)
        
        label = tk.Label(self.root, image=octopot)
        label.image = octopot
        label.pack()

        input_list  = list(set(mido.get_input_names()))
        output_list = list(set(mido.get_output_names()))
        self.input_conn = tk.StringVar(self.root)
        if len(input_list) > 0:
            self.input_conn.set(input_list[0])
        self.output_conn = tk.StringVar(self.root)
        if len(output_list) > 0:
            self.output_conn.set(output_list[0])

        self.input_conn.trace('w', self.on_change_input_conn)
        self.output_conn.trace('w', self.on_change_output_conn)

        self.ddin = tk.OptionMenu(self.root, self.input_conn, value='')
        self.ddin.place(y=10, w=200)
        self.ddout = tk.OptionMenu(self.root, self.output_conn, value='')
        self.ddout.place(relx=1, y=10, w=200, anchor='ne')

        self.pot = []

        # Right side text boxes
        pady=300
        for i in range(int(POT_NB / 2)):
            self.pot += [tk.Entry()]
            self.pot[i].place(y=pady, w=30)
            self.pot[i].bind('<Return>', lambda event, idx=i: self.on_change_cc(event, SysExMsg.PATCH_POT_CMD, idx))
            pady += 105

        # Left side text boxes
        pady=300
        for i in range(int(POT_NB / 2), POT_NB):
            self.pot += [tk.Entry()]
            self.pot[i].place(relx=1, y=pady, w=30, anchor='ne')
            self.pot[i].bind('<Return>', lambda event, idx=i: self.on_change_cc(event, SysExMsg.PATCH_POT_CMD, idx))
            pady += 105

        self.btn_mcc = []
        self.var_tog = []

        # Buttons
        pady=360
        padx=265
        for i in range(BTN_NB):
            self.btn_mcc += [tk.Entry()]
            self.btn_mcc[i].place(x=padx, y=pady, w=30)
            self.btn_mcc[i].bind('<Return>', lambda event, idx=i: self.on_change_cc(event, SysExMsg.PATCH_BTN_CMD, idx))
            self.var_tog += [tk.IntVar()]
            self.var_tog[i].trace('w', lambda *args,idx=i: self.on_change_btn_tog(idx))
            btn_tog = tk.Checkbutton(text='Tog', variable=self.var_tog[i])
            btn_tog.place(x=padx+35, y=pady, w=50)
            pady += 105

        save_btn = tk.Button(text='Save patch into EEPROM', command = self.on_save)
        save_btn.pack()
        
        save_btn = tk.Button(text='Reset to default config', command = self.on_reset)
        save_btn.pack()
        
        # Initialize MIDI ports
        if self.input_conn.get():
            self.midi_in   = mido.open_input(self.input_conn.get(), callback=self.on_midi_receive)
        if self.output_conn.get():
            self.midi_out  = mido.open_output(self.output_conn.get())

        # Initialize timer
        self.update()

        self.root.protocol('WM_DELETE_WINDOW', self.on_close)
        self.root.mainloop()

    def on_save(self):
        # Send the SysEx message
        if self.midi_out:
           self.midi_out.send(mido.Message('sysex', data=[SysExMsg.SAVE_CMD]))

    def on_reset(self):
        # Send the SysEx message
        if self.midi_out:
           self.midi_out.send(mido.Message('sysex', data=[SysExMsg.RESET_CMD]))

    def on_change_input_conn(self, *args):
        if self.midi_in:
            self.midi_in.close()
        self.midi_in = mido.open_input(self.input_conn.get(), callback=self.on_midi_receive)

    def on_change_output_conn(self, *args):
        if self.midi_out:
            self.midi_out.close()
        self.midi_out = mido.open_output(self.output_conn.get())

    def on_change_cc(self, e, msg, idx):
        """
        Calback to validate Entry input
        Sends the new patch for a controller.

        @param e:   event
        @param msg: sysex message id (PATCH_POT_CMD or PATCH_BTN_CMD)
        @param idx: index of the Entry
        """

        if self.midi_out:
            # Get the Entry value
            midi_cc = e.widget.get()
            print(str(idx) + ' => ' + midi_cc)

            # Send the SysEx message
            self.midi_out.send(mido.Message('sysex', data=[msg, idx, int(midi_cc)]))

        # Lose focus to be refreshed by the timer
        self.root.focus()

    def on_change_btn_tog(self, idx):
        tog = self.var_tog[idx].get()
        print(str(idx) + ' => ' + str(tog))

        # Send the SysEx message
        self.midi_out.send(mido.Message('sysex', data=[SysExMsg.TOGGLE_BTN_CMD, idx, tog]))

    def on_midi_receive(self, midi_msg):
        """
        Callback to be called on MIDI in event.

        @param midi_msg: The incoming MIDI message
        """

        if midi_msg.type == 'sysex' and midi_msg.data[0] == SysExMsg.PATCH_STS:
            pot_mcc = midi_msg.data[1:POT_NB+1]
            for i in range(POT_NB):
                # Do not update an Entry that being edited
                if self.root.focus_get() != self.pot[i]:
                    self.pot[i].delete(0,"end")
                    self.pot[i].insert(0, pot_mcc[i])
            btn_cfg = midi_msg.data[POT_NB+1:POT_NB+1+BTN_NB*2]
            for i in range(BTN_NB):
                btn_mcc = btn_cfg[i*2]
                btn_tog = btn_cfg[i*2+1]
                # Do not update an Entry that being edited
                if self.root.focus_get() != self.btn_mcc[i]:
                    self.btn_mcc[i].delete(0,"end")
                    self.btn_mcc[i].insert(0, btn_mcc)
                # Remove callback on toggle
                self.var_tog[i].trace_remove(*self.var_tog[i].trace_info()[0])
                self.var_tog[i].set(btn_tog)
                # Set back the callback
                self.var_tog[i].trace('w', lambda *args,idx=i: self.on_change_btn_tog(idx))

    def on_close(self):
        """
        Properly terminate at window closing.
        """

        if self.midi_in:
            self.midi_in.close()
        if self.midi_out:
            self.midi_out.close()

        self.root.destroy()

    def update(self):
        """
        Timer.
        Sends a SysEx message to request for the current patch.
        Update midi input and output lists
        """

        # Send patch request
        if self.midi_out:
            self.midi_out.send(mido.Message('sysex', data=[SysExMsg.PATCH_REQ]))
        
        # Update MIDI ports
        inmenu = self.ddin['menu']
        inmenu.delete(0, 'end')
        for input in set(mido.get_input_names()):
            inmenu.add_command(label=input,
                command=lambda value=input: self.input_conn.set(value))
        outmenu = self.ddout['menu']
        outmenu.delete(0, 'end')
        for output in set(mido.get_output_names()):
            outmenu.add_command(label=output,
                command=lambda value=output: self.output_conn.set(value))

        self.root.after(REQUEST_REC, self.update)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog=sys.argv[0],
        description='This application configures your Octopot device')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-a', '--alsa', action='store_true', help='Use ALSA backend.')
    group.add_argument('-j', '--jack', action='store_true', help='Use JACK backend (Default if available).')
    args = parser.parse_args()
    
    is_jack_available = 'UNIX_JACK' in mido.backend.module.get_api_names()
    if not args.alsa and is_jack_available:
        mido.set_backend('mido.backends.rtmidi/UNIX_JACK')
    else:
        mido.set_backend('mido.backends.rtmidi/LINUX_ALSA')
    
    print('Using: ' + mido.backend.api)

    root = Root()
