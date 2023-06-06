#!/bin/env python3
 
import tkinter as tk
from PIL import Image, ImageTk

import mido

class Root:
    def __init__(self):
        self.root = tk.Tk()
        self.root.resizable(0, 0)

        im = Image.open("octopot.png")
        octopot = ImageTk.PhotoImage(im)
        
        label = tk.Label(self.root, image=octopot)
        label.image = octopot  # keep a reference!
        label.pack()

        self.pot = []
        pady=300
        for i in range(4):
            self.pot += [tk.Entry()]
            self.pot[i].place(y=pady, w=50)
            self.pot[i].bind('<Return>', lambda event, idx=i: self.on_change(event,idx))
            pady += 105
        pady=300
        for i in range(4, 8):
            self.pot += [tk.Entry()]
            self.pot[i].place(relx=1, y=pady, w=50, anchor='ne')
            self.pot[i].bind('<Return>',  lambda event, idx=i: self.on_change(event,idx))
            pady += 105

        client_name='Octopot Conf'
        self.midi_in   = mido.open_input('input', client_name=client_name)
        self.midi_out  = mido.open_output('output', client_name=client_name)

        self.update_pots_cc()

        self.root.protocol('WM_DELETE_WINDOW', self.on_close)
        self.root.mainloop()

    def on_change(self, e, idx):
        midi_cc = e.widget.get()
        print(str(idx) + ' => ' + midi_cc)
        self.midi_out.send(mido.Message('sysex', data=[2, idx, int(midi_cc)]))
        self.root.focus()

    def on_close(self):
        self.midi_in.close()
        self.midi_out.close()
        self.root.destroy()

    def update_pots_cc(self):
        self.midi_out.send(mido.Message('sysex', data=[0]))
        resp = self.midi_in.poll()
        while resp:
            print(resp)
            if resp.type == 'sysex' and resp.data[0] == 1:
                midi_cc = resp.data[1:9]
                for i in range(8):
                    if self.root.focus_get() != self.pot[i]:
                        self.pot[i].delete(0,"end")
                        self.pot[i].insert(0, midi_cc[i])
            resp = self.midi_in.poll()
        self.root.after(2000, self.update_pots_cc)

mido.set_backend('mido.backends.rtmidi/UNIX_JACK')

root = Root()