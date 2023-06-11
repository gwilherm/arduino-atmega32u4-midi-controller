# Arduino ATmega32u4 MIDI Octopot

Arduino SysEx programmable 8 Knobs MIDI Controller inspired by [Crius Octapot Midi Controller](https://www.instructables.com/Crius-OctaPot-Midi-Controller) but using ATmega32u4 microcontroller in order to use MIDI USB.

## Photo
![](doc/photo.jpg)

## Breadboard
![](doc/schematics/midi-octopot_bb.png)

## Schematics
![](doc/schematics/midi-octopot_schema.png)

## Flash Arduino ProMicro
### Arduino IDE
* Open midi-octopot/midi-octopot.ino
* Sketch menu
  * Card type: `Arduino Micro`
  * Port: `/dev/ttyACM0`
  * Upload

### Command line (Ubuntu/Debian)
```shell
sudo apt install arduino-mk
cd midi-octopot
make
make upload
```

## Octopot Configuration App (Ubuntu/Debian)
```shell
sudo apt install python3-rtmidi
cd octopot-config-app
python3 -m venv venv
source venv/bin/activate
python3 -m pip install -r requirements.txt
# Launch using Jack
./octopot-config-app.py
# Launch using Alsa
./octopot-config-app.py -a
```

![](doc/octopot-config-app.png)
