# TwoStageAvionicsBay
University of Cincinnati's Rocketry Club Spring 2026 Project, nicknamed Double or Nothing. This is in succession to the previous semester's Boosted Dart, [Harty Darty](https://github.com/raniamaaraba/HardyDarty), for the club's overall goal of building skills for a space shot within the next decade. The project is a semester long project beginning January 2nd until April 12th, 2026. Repo is a current WIP, last edit: February 20th, 2026.

# Goals
| Team  | Achievement Desired |
| ------------- | ------------- |
| Overall  | Have an apogee greater than 8,000 ft  |
| Overall  | Safely recover both parts of the rocket within a mile radius of launch  |
| Structures | Learn how to use Open Rocket to optimise a two-stage rocket efficiently |
| Structures | Learn how to build a Mach-1 capabile rocket|
| Structures | Refine construction methods while enhancing simulation techniques|
| Avionics | Learn how and when to ignite a second stage |
| Avionics | Learn how to recover both stages |
| Avionics | Work on integration of PCBs for further development |


# Initial Structures Design
Initially simulated in Open Rocket, Double or Nothing is primarily composed of G10 Fiberglass, except for the 2nd stage motor mount and the transition. The first stage will run an I-435, the second an H-130, with the whole rocket weighing approximately 6.9 pounds. Similarly, stage separation will also occur due to drag. The Transition stage will be a 3D-printed PETG part to continue seeking more advanced technologies required for further development. 

### 3D Simulated Design
<img width="181" height="386" alt="Screenshot 2026-02-19 at 18 23 49" src="https://github.com/user-attachments/assets/b4009730-5a83-4050-ad7b-4b7491ee532a" />
<img width="911" height="226" alt="Screenshot 2026-02-19 at 18 26 58" src="https://github.com/user-attachments/assets/d1148d67-15af-4dcd-a62b-391ea14f02c0" />

# Avionics
Similar to HD, there is both a custom avionics bay and off-the-shelf components. Currently, we are using three off-the-shelf components across the two stages, with the customs in the 2nd stage. One of the bigger notes for avionics development as well is that the custom electronics **_are not_** connected to the pyrotechnic charges. Due to the timeline and in-experience overall of members programming with custom electronics for flight control, this is someone we chose to bypass this semester. As an overall, the customs will be simulating with charges during events (such as apogee, parachute deployments and such) to log this data and our accuracy against the off-the-shelf components. 

| Off-the-Shelf Components  | Location | Use |
| ------------- | ------------- | ------------- |
| Blue Jay  | 1st stage  | An additional flight computer to use as a comparison alongside our custom avionics-- deploys the 1st stage parachute |
| Blue Raven  | 2nd stage  | Also an additional flight computer, but for data in the second stage |
| EasyMini Altimeter | 2nd stage | Dual redundant recovery system |
| Grey Designs V1 GPS Tracker | 1st and 2nd| Real-time GPS data via data link |
|

| Custom Avionics | Use |
| ------------- | ------------- |
| ESP32-S3 Microcontroller | Our data-logging device of choice; re-using from HD since we have legacy code and a baseline for non-coders | MS5611 Barometer | Easy C++ integration with library as well as accurate up to 20,000 feet in altitude |
| LSM6DSO32 IMU | Has high range accelerometer data and sampling rates, particularly useful for our current purpose as it is accurate at high and low Gs |
| SOT23-3 MOSTFET and 2.4GHz flex PCB WiFi Antenna | small additions; possibly considerations for modifying either based on further testing and development |

## Initial PCB Design
Not our current finalised version, but an early implementation. The overall goal for the PCB is not only to set us up for further usage down the road, but also to help keep the custom avionics clean and efficient. To keep the system together and connections close, we opted for a double sided board mainly with the ESP, capacitors, resistors, and IMU on top with the rest of the smaller components like the MOSFETs, barometer, and battery/scew terminals on the bottom. To allow wires to pass through, there are wholes according on each side of the board. 
<img width="300" height="208" alt="Top of the PCB Design" src="https://github.com/user-attachments/assets/ce6188b7-2aad-43b8-a97d-c942e26d96ab" />
<img width="261" height="213" alt="Bottom of the PCB Design" src="https://github.com/user-attachments/assets/9d0705ae-6fd4-4734-bfe6-b5faf558d963" />
 <img width="976" height="587" alt="Overall schematics for the devices in the PCB" src="https://github.com/user-attachments/assets/e920335f-56cd-445d-aeec-7d39c1d388cb" />




