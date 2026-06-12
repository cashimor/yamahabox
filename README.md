# yamahabox
Code for the Yamaha Reface CS patch storage box

Uses the PIC16F18115.
RA5 is the potentiometer in.
RA3 is the MIDI IN through a 1k resistor.
RA0 is the button in, connected to ground.
RA1 is the MIDI out, connected through a 47 ohm resistor.
An LED is attached to RA2 with a 10k resistor.

Yamaha MIDI manual: https://data.yamaha.com/files/download/other_assets/7/794817/reface_en_dl_b0.pdf
Yamaha MIDI pinout: https://sandsoftwaresound.net/reface-midi-pin-out/
Matthew's patch box code: https://github.com/matthewcieplak/reface_midi/blob/main/reface_midi_firmware/reface_midi_firmware.ino
Short video of the box: https://www.youtube.com/shorts/nZQOHJhSrHA

Do not have the Yamaha Reface CS connected while programming the PICmicro, because there is no optocoupler keeping separation.
