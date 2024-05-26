# Arduino Euclidean Rhythm Generator
Euclidean Rhythm Generator module based on Arduino Nano, compatible with Eurorack and Kosmo modular synthesizers. This is a DIY project, suitable for beginners, although a general knowledge of electronics, basic tools and skills are required to build this module. The schematics presented here feature:

- 4 channels
- max. 16 steps per channel
- individual encoders for each channel
- adjustable sequence length
- external clock input
- reset input/button

A [short demonstration](https://youtu.be/4AOBJ-tzH3Y) is available on YouTube. View the [schematics](schematic+BOM/arduino_euclidean_rhythm_generator.pdf) and [bill of materials](https://htmlpreview.github.io/?https://github.com/TimMJN/Arduino-Euclidean-Rhythm-Generator/blob/main/schematic%2BBOM/arduino_euclidean_rhythm_generator_BOM.html) on this repository.

An optional CV expander module is available, which adds 4 CV inputs. By default, they are configured to affect the number of hits and rotation of the first two channels. 

# Frequently Asked Questions
## The build
#### Do you have PCBs / panels available?
A PCB design for Kosmo format is available, along with a panel drill guide. I might have some PCBs available, please get in touch. However, it is also possible to build this design on perfboard/stripboard. I want to encourage any builder to come up with their own panel layout, make whatever feels natural to them. If you document your work, I'll be happy to add it here for future reference.

## The code
#### Submodules
This repository uses submodules to include libraries in the firmware. Please make sure, when cloning this repository, to also include the submodules in the [`src`](https://github.com/TimMJN/Arduino-Euclidean-Rhythm-Generator/tree/main/arduino_euclidean_rhythm_generator_firmware/src) directory.

#### What settings are available to me in the firmware?
- The `XXX_BRIGHTNESS` defines set the LED brightness for different function. Setting excessively high brightness might draw more current than the 5v regulator can comfortable supply.
- `TIMEOUT` sets the time in ms after which adjustment of the sequence length gets cancelled.
- `n_hits` and `offset` get be used to set a pattern upon startup of the module.
- Enable `#define CV_EXPANDER` to enable the CV expander module, if you have it installed.
- Edit `N_HITS_CV_CHAN` and `OFFSET_CV_CHAN` in [CV_expander.h](arduino_euclidean_rhythm_generator_firmware/src/CV_expander/CV_expander.h) to change how the CV inputs affect the patterns.

## The patch
#### How do I use this module?
[Euclidean rhythms](https://en.wikipedia.org/wiki/Euclidean_rhythm) are generator by distributing a number of hits or notes along the length of a sequence as evenly as possible. The number of hits on each channel can be changed by turning the corresponding encoder. Additionally, the entire pattern can be rotated by pressing down on the encoder, and then twisting it. Lastly, the length of the sequence can be changed by shortly pressing down on the encoder and releasing it. The length can then be set by twisting it, and confirmed by pressing shortly again. With these 3 settings combined, all Euclidean rhythms upto a length of 16 steps can be generated.

The signal applied to the clock input determines the rate or tempo at which the sequences are played, as well as the length of each trigger. The length of each trigger is equal to the length of the clock pulse.

By setting different lengths for each channel, polyrhythms can be generated. The reset input allows for re-syncing of all channels, as it will set each sequence back to its first step on the next clock pulse.

# License
All designs in this repository are released under GPL-3.0 licence. Feel free to use and adapt them to your heart's desire. The only ask to refrain from mass-production and commercialisation of PCBs/modules, as I rely on PCB sales for funding new module designs. If you use (parts of) my designs in your own modules, please credit me on your schematics and PCBs to help users find the original creator. I call upon your good conscience to make fair use of my work shared here.

# Support
If you like the resources I have made available here, and wish to support the development of new modules, feel free to buy me a few components through a small donation. I mainly design modules for fun, but you probably already know it can be a costly endeavour. All small contributions help me, thank you very much!

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/donate?hosted_button_id=FZJELWSAH4UKU)
