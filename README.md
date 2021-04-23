# Fngrs
An all-in-one, vr hand glove with finger tracking for less than £50

The SteamVR driver Lucas and I have been developing for both our projects can be located here: https://github.com/LucidVR/opengloves-driver

## An example of what this project can do:
[![Fngrs Demo](https://img.youtube.com/vi/FPaCHbXl4mU/0.jpg)](https://www.youtube.com/watch?v=FPaCHbXl4mU)
## Credit
I would like to thank [@spayne](https://github.com/spayne) for his soft_knuckles example repo, which was a great help in setting up this project. 
## Status: Proof Of Concept
This was a weekend project that gained some interest, so I decided to publish this for people to view and perhaps contribute towards. I don't recommend people to actually build one for themselves yet, particularly as this project is no where near the level of polish that other vr controllers have, there's not many other controllers like this and that this is more of a **proof of concept** to perhaps gain the attention of companies that something like this is possible. Attempting to build one (at the moment) will most certainly be a waste of money. However, this project does "work".
## What is this?
This project is intended to provide affordable hand and finger tracking for as many headsets and games as possible. As such, the project is developed on OpenVR, making it compatible with SteamVR and all the headsets and controllers it supports. The OpenVR driver is compatible with all controllers that support OpenVR/SteamVR, as well as all games that support skeletal hand tracking through OpenVR.

## How does this work?
OpenVR provides a layer for drivers to interact with applications. The driver for this project passes hand tracking data from the glove into a new pair of controllers which can then be used to track finger movement from the arduinos and positioning and rotation from the controllers, without needing extra equipment for tracking.

## Current limitations of the project
* There is only a design for the Oculus Touch controllers (the ones which ship with the Rift S and Quest 1), but if someone creates a design of their own which suports other controllers, the driver and rest of the project should work fine with them. If you do create one, please submit a pull request with the design and I'll happily merge it in!

### Requirements
* This project requires a 3D printer in order to make the mount for the components
* This project requires some knowledge in c++ for building the driver
* Soldering knowledge and electronic knowledge is required for building the glove
* <b>Again,</b>  I would like to re-iterate that this project is not something that I would recommend people to spend money on - this is more of a proof of concept device to show that cheap finger tracking is poosible. However, if you are willing to or have the components already available I would appreciate feedback on the project.

## Equipment needed
| Name      | Amount needed | Purpose |
| ----------- | ----------- | ----------- | 
| 60mm 10K slide potentiometer | 8 |  Measuring the finger position |
| Fishing Wire | > 2m | Connects the potentiometer to the tips of the finger |
| Rubber/Plaiting Bands | 8 | (typically come in batches of 100 or more) For getting the potentiometer to return to the default position. I've found that horse plaiting bands are the best option for this, for the cheapest price, but it is important to get high quality ones, as cheaper ones often have a low elastic limit |
| Arduino nano | 2 | Clones are available for around £10 for a pack of 3 |
| Buttons | (~4) | It is possible to add as many buttons as the arduino supports, or none at all - it's up to you how many you want |
| Joystick | (2) | It's not essential to use them, but if you are going to buy some, it's a good idea to get ones with buttons in-built |
| A - Mini B Cable | 2 (~5m) | For connecting the arduinos to a computer. The length you choose is up to you and your playspace, but ~5m is a good length |
| Mini Breadboard | 2 | For wiring the components together. You could also use a prototype board for a more permanent solution |
| PTFE Tubing | ~10cm | I cut them up to use on the fingers as guides for the wire, but more permanent solutions like threading the wires through the glove could also be used. PTFE tubing is a common thing to have in 3D printing |
| Elastic chord | ~ 1m | This is used to secure the 3d printed part to the hand. Button hole elastic works well. |

## How can I get started?
The GitHub Wiki contains instructions needed to get started with this project

## Contact me
If you run into any issues, please contact me on discord: `danwillm#8254`
