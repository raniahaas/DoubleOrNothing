#include <Arduino.h>
#include <MS5611.h>
#include <Adafruit_LSM6DSO32.h>


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

        Serial.print("Temp:");
        Serial.println(BARO.getTemperature(),2);

        delayMicroseconds(10000);
        }
}


// Function for varied continuity testing, NOT final function for reading continuity during flight
// Original code by Jessie, changes and conversion to function by Loring T
// Input descriptions:
// NOTE: Pass GPIO pins as integer array, for example declare ig[3]={ig1,ig2,ig3}, where ig1-ig3 are GPIO pins for igniters, and pass just ig (the array) to function
// mode: integer that determines how the test behaves - not currently implimented 
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

    if(Serial){
        if (mode==0){
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
        } else{
        // Sequential Blink Mode: Turns each LED on and off in sequence
        // DO NOT USE WITH ANY LIVE IGNITERS, WILL IGNITE AS SOON AS PROGRAM IS FLASHED!
            for (int i=0; i<3; i++) {
                delay(1000);
                digitalWrite(ig[i],HIGH);
                delay(1000);
                digitalWrite(ig[i],LOW);
            }
        }
    }
}

// Sensor initialization - Inteded to be placed in setup loop
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