#!/bin/env python3

import tkinter as tk
from PIL import Image, ImageTk
from enum import IntEnum

import mido
import sys

POT_NB      = 8
REQUEST_REC = 2000

class SysExMsg(IntEnum):
  PATCH_REQ = 0, # In:  Request for patch status
  PATCH_STS = 1, # Out: Send patch array
  PATCH_CMD = 2  # In:  Change a patch

class Root:
    def __init__(self):
        """ Initialization """
        self.root = tk.Tk()
        self.root.title('Octopot Configuration')
        self.root.resizable(0, 0)

        im = Image.open("octopot.png")
        octopot = ImageTk.PhotoImage(im)
        
        label = tk.Label(self.root, image=octopot)
        label.image = octopot
        label.pack()

        self.input_conn = tk.StringVar(self.root)
        self.input_conn.set(mido.get_input_names()[0])
        self.output_conn = tk.StringVar(self.root)
        self.output_conn.set(mido.get_output_names()[0])

        self.input_conn.trace('w', self.on_change_input_conn)
        self.output_conn.trace('w', self.on_change_output_conn)

        ddin = tk.OptionMenu(self.root, self.input_conn, *mido.get_input_names())
        ddin.place(y=10, w=200)
        ddout = tk.OptionMenu(self.root, self.output_conn, *mido.get_output_names())
        ddout.place(relx=1, y=10, w=200, anchor='ne')

        self.pot = []

        # Right side text boxes
        pady=300
        for i in range(int(POT_NB / 2)):
            self.pot += [tk.Entry()]
            self.pot[i].place(y=pady, w=50)
            self.pot[i].bind('<Return>', lambda event, idx=i: self.on_change_cc(event,idx))
            pady += 105

        # Left side text boxes
        pady=300
        for i in range(int(POT_NB / 2), POT_NB):
            self.pot += [tk.Entry()]
            self.pot[i].place(relx=1, y=pady, w=50, anchor='ne')
            self.pot[i].bind('<Return>',  lambda event, idx=i: self.on_change_cc(event,idx))
            pady += 105

        # Initialize MIDI ports
        self.midi_in   = mido.open_input(self.input_conn.get(), callback=self.on_midi_receive)
        self.midi_out  = mido.open_output(self.output_conn.get())

        # Initialize patch request timer
        self.update_pots_cc()

        self.root.protocol('WM_DELETE_WINDOW', self.on_close)
        self.root.mainloop()

    def on_change_input_conn(self, *args):
        self.midi_in.close()
        self.midi_in = mido.open_input(self.input_conn.get(), callback=self.on_midi_receive)

    def on_change_output_conn(self, *args):
        self.midi_out.close()
        self.midi_out = mido.open_output(self.input_conn.get())

    def on_change_cc(self, e, idx):
        """
        Calback to validate Entry input
        Sends the new patch for a knob.

        @param e:   event
        @param idx: index of the Entry
        """

        # Get the Entry value
        midi_cc = e.widget.get()
        print(str(idx) + ' => ' + midi_cc)

        # Send the SysEx message
        self.midi_out.send(mido.Message('sysex', data=[SysExMsg.PATCH_CMD, idx, int(midi_cc)]))

        # Lose focus to be refreshed by the timer
        self.root.focus()

    def on_midi_receive(self, midi_msg):
        """
        Callback to be called on MIDI in event.

        @param midi_msg: The incoming MIDI message
        """

        if midi_msg.type == 'sysex' and midi_msg.data[0] == SysExMsg.PATCH_STS:
            midi_cc = midi_msg.data[1:9]
            for i in range(POT_NB):
                # Do not update an Entry that being edited
                if self.root.focus_get() != self.pot[i]:
                    self.pot[i].delete(0,"end")
                    self.pot[i].insert(0, midi_cc[i])

    def on_close(self):
        """
        Properly terminate at window closing.
        """

        self.midi_in.close()
        self.midi_out.close()
        self.root.destroy()

    def update_pots_cc(self):
        """
        Patch request timer.
        Sends a SysEx message to ask for the current patch
        """

        self.midi_out.send(mido.Message('sysex', data=[SysExMsg.PATCH_REQ]))
        self.root.after(REQUEST_REC, self.update_pots_cc)

if __name__ == '__main__':
#    mido.set_backend('mido.backends.rtmidi/UNIX_JACK')
    mido.set_backend('mido.backends.rtmidi/LINUX_ALSA')
    root = Root()