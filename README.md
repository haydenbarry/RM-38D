# RM-38D

## Description

RM-38D is a sample based hardware drum machine with 16 voices.

The hardware is designed around a [Daisy Seed](https://electro-smith.com/products/daisy-seed) development board and uses only through hole components to allow for easy hand assembly. 
 
### Hardware
Sixteen Step Sequencer with LEDs
Play/Pause, Stop/Start and Shift tactile switches
Four rotary encoders with tactile switches
Master volume potentiometer
Micro-SD card
128x64 Bit OLED display
3.5mm Audio Output

### Software
The current software version has sixteen sample voices that can be selected from the micro-sd card. Currently supports 16bit PCM 48kHz mono wav files. 
Currently a single bar can be looped and the pattern for each voice edited. The volume and panning for each voice can be modified using the encoders.

Basic Step sequencer 
 - 16 Voices
 - 16 step pattern represented by the step LED's. Each step can be turned on or off with an LED displaying the current status
 - The first 16 samples on the SD Card are loaded into the 16 voices
 - Sample parameter adjustment
   - volume
   - Panning
 - Play/Pause and Stop/Start
 - Shift button for Voice selection
 - Master Settings page with tempo adjustment 

## Demo Video
[![Watch the video](https://img.youtube.com/vi/ZyIz7Wy38NY/hqdefault.jpg)](https://www.youtube.com/embed/ZyIz7Wy38NY)




## Features to add 
### Sample browsing 
Add navigation of directories
Support for differing file types

### Presets
Allow for saving of sets to the SD Card with ability to load sets
Add predefined kits with certain user set samples

### ADSR
Add envelope to sample voices

### Filter 
Add filtering to sample voice allowing for common filter types high/low pass ect.

### Sequencer 
Allow for editing of multiple sequences
Add ability to sequence patterns into a larger sequence
Add ability to change velocity of each step

### Enclosure
Design enclosure
