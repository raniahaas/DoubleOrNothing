#include <Arduino.h>
#include <MS5611.h>
#include <Adafruit_LSM6DSO32.h>

// Function to print IMU and barometer data to the serial monitor for debugging/testing
// Code by Loring T
// Input Description:
// IMU: Sensor specific class variable for the IMU, set in the pre-setup loop code (ex. Adafruit_LSM6DSO32 dso32;) where dso32 would be passed as IMU
// BARO: Sensor specific class variable for the barometer, set in the pre-setup loop code (ex. MS5611 MS5611(0x77);) where MS5611 would be passes as BARO
// plot: integer that determines how the function formats the output data. 
//  plot=1: (default) Outputs the data in a format optimized for the "Serial Plotter" VS Code extension (see code notes for details) 
//  plot=0: Outputs the data in a format easily read in the regular serial monitor, won't work with any serial plotter extensions
void data_print_test(Adafruit_LSM6DSO32& IMU, MS5611& BARO,int plot=1){ // & after class declaration determines how to pass, where & means by reference
    if (plot==0) {
        //  /* Get a new normalized sensor event */
        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;
        IMU.getEvent(&accel, &gyro, &temp);

        Serial.print("\t\tTemperature ");
        Serial.print(temp.temperature);
        Serial.println(" deg C");

        /* Display the results (acceleration is measured in m/s^2) */
        Serial.print("\t\tAccel X: ");
        Serial.print(accel.acceleration.x);
        Serial.print(" \tY: ");
        Serial.print(accel.acceleration.y);
        Serial.print(" \tZ: ");
        Serial.print(accel.acceleration.z);
        Serial.println(" m/s^2 ");

        /* Display the results (rotation is measured in rad/s) */
        Serial.print("\t\tGyro X: ");
        Serial.print(gyro.gyro.x);
        Serial.print(" \tY: ");
        Serial.print(gyro.gyro.y);
        Serial.print(" \tZ: ");
        Serial.print(gyro.gyro.z);
        Serial.println(" radians/s ");
        Serial.println();
    } else if (plot==1) {
        // serial plotter friendly format - Works w/ VS Code extension Serial Plotter
        // Hit Control(cmd)+Shift+P and type: "Serial Plotter: Open pane" into window and hit enter to open plotter

        /* Get a new normalized sensor event */
        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;
        IMU.getEvent(&accel, &gyro, &temp);

        Serial.print('>');
        
        Serial.print("Ax:");
        Serial.print(accel.acceleration.x);
        Serial.print(',');

        Serial.print("Ay:");
        Serial.print(accel.acceleration.y);
        Serial.print(',');

        Serial.print("Az:");
        Serial.print(accel.acceleration.z);
        Serial.print(',');

        Serial.print("Gx:");
        Serial.print(gyro.gyro.x);
        Serial.print(',');

        Serial.print("Gy:");
        Serial.print(gyro.gyro.y);
        Serial.print(',');

        Serial.print("Gz:");
        Serial.print(gyro.gyro.z);
        Serial.print(',');

        BARO.read();

        Serial.print("Pressure:");
        Serial.print(BARO.getPressure(),2); // Pressure is in mBar so values near 999 are close to sea level atmospheric pressure
        Serial.print(',');

        Serial.print("TempB:");
        Serial.print(BARO.getTemperature(),2); // Temp from barometer, much more accurate than from IMU
        Serial.print(',');

        Serial.print("TempI:"); // This temperature is from the IMU, may be used to correct for barometer heating
        Serial.print(temp.temperature);
        Serial.print(',');

        Serial.print("Alt:");
        Serial.println(BARO.getAltitudeFeet(),2);

        delayMicroseconds(10000);
        }
}


// Function for varied continuity testing, NOT final function for reading continuity during flight
// Original code by Jessie, changes and conversion to function by Loring T
// NOTE: Pass GPIO pins as integer array, for example declare ig[3]={ig1,ig2,ig3}, where ig1-ig3 are GPIO pins for igniters, and pass just ig (the array) to function
// Input descriptions:
// mode: Integer that determines how the test behaves - not currently implimented 
//  mode=0: Voltage/Continuity Read Mode - See in-code notes below
//  mode=1: Port Indicator (blink) Mode - See in-code notes below
//  mode=else: Sequential Blink Mode - See in-code notes below
// ig: integer array that stores GPIO pins for igniters 
// cont: integer array that stores GPIO pins for continuity test (analog) lines
void continuity_test(int mode, int ig[3], int cont[3]){
    float ADC1 = analogRead(cont[0]);
    float Vout1 = ADC1* (3.3/4095.0);
    float Vigniter1 = Vout1*2;

    float ADC2 = analogRead(cont[1]);
    float Vout2 = ADC2* (3.3/4095.0);
    float Vigniter2 = Vout2*2;

    float ADC3 = analogRead(cont[2]);
    float Vout3 = ADC3* (3.3/4095.0);
    float Vigniter3 = Vout3*2;

    if (mode==0){
        // Voltage/Continuity Read Mode: Reads continuity and displays voltage from each pyro port, offers indicators for battery status based on voltage
        // READ BELOW WARNINGS BEFORE USING WITH IGNITERS INSTALLED!! IMPROPER USE CAN LEAD TO INJURY, YOU WON'T ENJOY HAVING LESS FINGERS!
        // OK TO USE WITH IGNITERS, BUT ENSURE THAT IGNITERS ARE IN A SAFE LOCATION AWAY FROM PEOPLE BEFORE PLUGGIN IN FLIGHT COMPUTER!
        // DO NOT INSTALL IGNITERS WITH POWER SUPPLIED TO COMPUTER! THIS CAN LEAD TO UNINTENTIONAL EARLY IGNITION AND SERIOUS BODILY HARM
        if (Vigniter1 >= 3.2) {
            Serial.print("Continuity 1 Good: ");
        }
        if (Vigniter1 >= 3.6) {
            Serial.print("Good Voltage 1: ");
        } else {
            Serial.print("Low Battery 1: ");
        }
        Serial.print(Vigniter1); Serial.print(" ");

        if (Vigniter2 >= 3.2) {
            Serial.print("Continuity 2 Good: ");
        }
        if (Vigniter2 >= 3.6) {
            Serial.print("Good Voltage 2: ");
        } else {
            Serial.print("Low Battery 2: ");
        }
        Serial.print(Vigniter2); Serial.print(" ");

        if (Vigniter3 >= 3.2) {
            Serial.print("Continuity 3 Good: ");
        }
        if (Vigniter3 >= 3.6) {
            Serial.print("Good Voltage 3: ");
        } else {
            Serial.print("Low Battery 3: ");
        }
        Serial.println(Vigniter3);
        delay(1000);
    } else if (mode==1){
        // Port Indicator Mode: Indicates what port is active by blinking all LEDs to indicate port number before illuminating the given port LED to show it's location
        // NOTE: Intended for use when testing with LEDs, will not function with fuses 
        // DO NOT USE WITH ANY LIVE IGNITERS, WILL IGNITE AS SOON AS PROGRAM IS FLASHED!
        for (int i=0; i<3; i++) {
            delay(1000);
            for (int k=0; k<i+1; k++) { // Blink indicator, blinks to indicate the ignition port (1 blink for ignition 1, 2 blinks for ignition 2, etc.)
                digitalWrite(ig[0],HIGH);
                digitalWrite(ig[1],HIGH);
                digitalWrite(ig[2],HIGH);
                delay(500); 

                // Turns all the channels off
                digitalWrite(ig[0],LOW);
                digitalWrite(ig[1],LOW);
                digitalWrite(ig[2],LOW);
                delay(500);
            }

            // Turns all the channels off
            digitalWrite(ig[0],LOW);
            digitalWrite(ig[1],LOW);
            digitalWrite(ig[2],LOW);
            delay(500); // Delay to space out blinks from port indication

            // Print current port to serial
            Serial.print("Current Port: ");
            Serial.println(i+1);

            // Turn on and off given ignition port
            digitalWrite(ig[i],HIGH);
            delay(2000);
            digitalWrite(ig[i],LOW);
        }
    } else{
    // Sequential Blink Mode: Turns each LED on and off in sequence
    // DO NOT USE WITH ANY LIVE IGNITERS, WILL IGNITE AS SOON AS PROGRAM IS FLASHED!
        for (int i=0; i<3; i++) {
            delay(1000);

            // Indicates what port is being blinked
            Serial.print("Port: ");
            Serial.println(i+1);

            digitalWrite(ig[i],HIGH);
            delay(1000);
            digitalWrite(ig[i],LOW);
        }
        // Indicates when loop completes
        Serial.println("Loop Complete");
    }
}

// Sensor initialization - Inteded to be placed in setup loop
// Notes: Starts up sensors, also defines the IMU range and rate (Hz), which is currently not defined in the main loop
// Input Descriptions:
// IMU: Sensor specific class variable for the IMU, set in the pre-setup loop code (ex. Adafruit_LSM6DSO32 dso32;) where dso32 would be passed as IMU
// BARO: Sensor specific class variable for the barometer, set in the pre-setup loop code (ex. MS5611 MS5611(0x77);) where MS5611 would be passes as BARO
void sensor_init(Adafruit_LSM6DSO32& IMU, MS5611& BARO){
    // IMU Bootup & Test ------------------------
    Serial.println("Adafruit LSM6DSO32 test!");
    // Setup SPI for accelerometer
    while (!IMU.begin_I2C()) { // Init hardware SPI
    Serial.println("Failed to find LSM6DSO32 chip");
    delay(100);
    }

    Serial.println("LSM6DSO32 Found!");

    IMU.setAccelRange(LSM6DSO32_ACCEL_RANGE_16_G);
    Serial.print("Accelerometer range set to: ");
    switch (IMU.getAccelRange()) {
    case LSM6DSO32_ACCEL_RANGE_4_G:
        Serial.println("+-4G");
        break;
    case LSM6DSO32_ACCEL_RANGE_8_G:
        Serial.println("+-8G");
        break;
    case LSM6DSO32_ACCEL_RANGE_16_G:
        Serial.println("+-16G");
        break;
    case LSM6DSO32_ACCEL_RANGE_32_G:
        Serial.println("+-32G");
        break;
    }

    // dso32.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );
    Serial.print("Gyro range set to: ");
    switch (IMU.getGyroRange()) {
    case LSM6DS_GYRO_RANGE_125_DPS:
        Serial.println("125 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_250_DPS:
        Serial.println("250 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_500_DPS:
        Serial.println("500 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_1000_DPS:
        Serial.println("1000 degrees/s");
        break;
    case LSM6DS_GYRO_RANGE_2000_DPS:
        Serial.println("2000 degrees/s");
        break;
    case ISM330DHCX_GYRO_RANGE_4000_DPS:
        break; // unsupported range for the DSO32
    }

    IMU.setAccelDataRate(LSM6DS_RATE_208_HZ);
    Serial.print("Accelerometer data rate set to: ");
    switch (IMU.getAccelDataRate()) {
    case LSM6DS_RATE_SHUTDOWN:
        Serial.println("0 Hz");
        break;
    case LSM6DS_RATE_12_5_HZ:
        Serial.println("12.5 Hz");
        break;
    case LSM6DS_RATE_26_HZ:
        Serial.println("26 Hz");
        break;
    case LSM6DS_RATE_52_HZ:
        Serial.println("52 Hz");
        break;
    case LSM6DS_RATE_104_HZ:
        Serial.println("104 Hz");
        break;
    case LSM6DS_RATE_208_HZ:
        Serial.println("208 Hz");
        break;
    case LSM6DS_RATE_416_HZ:
        Serial.println("416 Hz");
        break;
    case LSM6DS_RATE_833_HZ:
        Serial.println("833 Hz");
        break;
    case LSM6DS_RATE_1_66K_HZ:
        Serial.println("1.66 KHz");
        break;
    case LSM6DS_RATE_3_33K_HZ:
        Serial.println("3.33 KHz");
        break;
    case LSM6DS_RATE_6_66K_HZ:
        Serial.println("6.66 KHz");
        break;
    }

    IMU.setGyroDataRate(LSM6DS_RATE_208_HZ); 
    Serial.print("Gyro data rate set to: ");
    switch (IMU.getGyroDataRate()) {
    case LSM6DS_RATE_SHUTDOWN:
        Serial.println("0 Hz");
        break;
    case LSM6DS_RATE_12_5_HZ:
        Serial.println("12.5 Hz");
        break;
    case LSM6DS_RATE_26_HZ:
        Serial.println("26 Hz");
        break;
    case LSM6DS_RATE_52_HZ:
        Serial.println("52 Hz");
        break;
    case LSM6DS_RATE_104_HZ:
        Serial.println("104 Hz");
        break;
    case LSM6DS_RATE_208_HZ:
        Serial.println("208 Hz");
        break;
    case LSM6DS_RATE_416_HZ:
        Serial.println("416 Hz");
        break;
    case LSM6DS_RATE_833_HZ:
        Serial.println("833 Hz");
        break;
    case LSM6DS_RATE_1_66K_HZ:
        Serial.println("1.66 KHz");
        break;
    case LSM6DS_RATE_3_33K_HZ:
        Serial.println("3.33 KHz");
        break;
    case LSM6DS_RATE_6_66K_HZ:
        Serial.println("6.66 KHz");
        break;
    }

    // Barometer Bootup & Test ------------------
    Wire.begin();
    if (BARO.begin() == true){
        Serial.println("MS5611 found.");
    } else{
        Serial.println("MS5611 not found. halt.");
        while (1);
    }
    Serial.println();

    BARO.setOversampling(OSR_STANDARD);
}

// Barometer Test - Function to test the barometer by itself, should be called in main loop
// Note: This function is meant to be temporary and easily modified, thus it is documented slightly less than normal, sorry! - LT
void barometer_test(MS5611& BARO, int mode=0) {
    if (mode==0) {
        // Get baro data and save to variables
        BARO.read(); // Gets actual data from barometer. If this line not present, all other BARO.XYZ calls will return -9.99
        float temp = BARO.getTemperature(); // To get usable data from barometer to variables use these BARO. calls
        float baro = BARO.getPressure();
        float alt = BARO.getAltitudeFeet();

        // Print the gathered barometer data to the serial port
        Serial.print("Temp (C): ");
        Serial.print(temp);
        Serial.print("\tPressure (mbar): ");
        Serial.print(baro);
        Serial.print("\tAltitude (ft): ");
        Serial.println(alt);
        delay(100); // Delay to keep data feed somewhat readable in the serial monitor (10Hz)
    }
}

// IMU MOSFETS - This function opens the three MOSFETs depending on the orientation of the board, used with LEDs in pyro terminals ONLY!
// Note: This function is intended to be used with LED diodes placed in the pyro terminals for indication of their open/closed state
//       Using any pyrotechnic device in the ports with this function active WILL result in pyrotechnic ignition!
// Input Description: 
// IMU: Sensor specific class variable for the IMU, set in the pre-setup loop code (ex. Adafruit_LSM6DSO32 dso32;) where dso32 would be passed as IMU
// ig: Integer array that stores GPIO pins for igniters 
void mosfet_IMU_test(Adafruit_LSM6DSO32& IMU, int ig[3]){
    // Get data from IMU
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    IMU.getEvent(&accel, &gyro, &temp);

    // Set vars and get absolute value of accel. in each direction
    float abs_x = abs(accel.acceleration.x);
    float abs_y = abs(accel.acceleration.y);
    float abs_z = abs(accel.acceleration.z);

    // If statements for turning LEDs on
    if (abs_x > abs_y & abs_x > abs_z){ // Turns on APO LED
        // Turn on (open) single pyro MOSFET
        digitalWrite(ig[0],HIGH);
        // Turn off (close) other pyro MOSFETs
        digitalWrite(ig[1],LOW);
        digitalWrite(ig[2],LOW);
    } else if (abs_y > abs_x & abs_y > abs_z){ // Turns on MAIN LED
        // Turn on (open) single pyro MOSFET
        digitalWrite(ig[1],HIGH);
        // Turn off (close) other pyro MOSFETs
        digitalWrite(ig[0],LOW);
        digitalWrite(ig[2],LOW);
    } else if (abs_z > abs_x & abs_z > abs_y){ // Turns on MOTOR LED
        // Turn on (open) single pyro MOSFET
        digitalWrite(ig[2],HIGH);
        // Turn off (close) other pyro MOSFETs
        digitalWrite(ig[0],LOW);
        digitalWrite(ig[1],LOW);
    } else {
        // If all equal (very unlikely), turn on all pyro MOSFETs
        digitalWrite(ig[0],HIGH);
        digitalWrite(ig[1],HIGH);
        digitalWrite(ig[2],HIGH);
    }

    // Delay to avoid flashing LEDs too much
    delay(250);
}

// Serial Pyro Ignition - Function that allows for the control of pyro MOSFETs via commands sent to through the serial monitor
void pyro_serial(int ig[3], int cont[3]){
    // Check if serial port is open
    if(Serial){
        delay(1000);
        // Initial setup
        Serial.println("Welcome to the Serial Pyro Tester!\n");
        Serial.println("Please select which port(s) you intend to test by entering the following into the serial port:");
        Serial.println("APO Port: 1\nMAIN Port: 2\nMOTOR Port: 3");

        String in = "null"; // Variable for serial input
        long port = 10;
        long restart = 0; // Variable that determines if the function keeps asking for an input
        while (Serial.available() == 0) {} // Waits for input
        while (restart == 0){
            in = Serial.readString();
            delay(5);
            in.trim(); // Removes any potential formatting like \n from the string
            if (in == "1" or in == "2" or in == "3"){ // If one of the ports is selected correctly, don't keep asking
                delay(5);
                restart = 1;
            } else { // If no correct input, keep asking
                Serial.println("Please re-enter a correct input");
                restart=0;
                while (Serial.available() == 0) {} // Waits for input
            }

            // Readback
            delay(5);
            if (in == "1"){
                Serial.print("\nYour selected port was: ");
                Serial.println("APO");
                port = 0;
            } else if (in == "2"){
                Serial.print("\nYour selected port was: ");
                Serial.println("MAIN");
                port = 1;
            } else if (in == "3"){
                Serial.print("\nYour selected port was: ");
                Serial.println("MOTOR");
                port = 2;
            }

            if (port==0 || port==1 || port==2){
                // Display ignition information if correct input
                Serial.println("Type FIRE into the serial port to activate the chosen pyro channel or type ABORT to abort test");
            }
        }

        // Ignition
        restart = 0;
        in = "null";
        while (Serial.available() == 0) {} // Waits for input
        while (restart == 0){
            in = Serial.readString();
            in.trim();
            if (in == "FIRE"){ // If correct fire command ordered, don't keep asking
                restart = 1;
            } else if (in == "ABORT") { // If abort command ordered, end the function
                Serial.println("Aborting Test!\n");
                return;
            }else { // If no correct input, keep asking
                Serial.println("Please re-enter a correct input");
                restart=0;
                while (Serial.available() == 0) {} // Waits for input
            }
        }

        Serial.println("\nFiring Started!");
        digitalWrite(ig[port],HIGH); // Open pyro MOSFET
        delay(5000);
        digitalWrite(ig[port],LOW); // Closes pyro MOSFET after 5 seconds
        Serial.println("Firing Complete!\n");
    }
}