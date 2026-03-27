#include <Arduino.h>
#include <MS5611.h>
#include <Adafruit_LSM6DSO32.h>
#include <MadgwickAHRS.h> // Madgwick filter library
#include <globals.h>

// Simple Position Integration
long simple_position(Adafruit_LSM6DSO32& IMU, long prev_time, unsigned long debug = 0){
    // Get data from IMU
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    IMU.getEvent(&accel, &gyro, &temp);

    // Save angular velocity
    float wx = gyro.gyro.x;
    float wy = gyro.gyro.y;
    float wz = gyro.gyro.z;

    // Get current time
    long current_time = micros();

    // Calculate change in time between last read (delta t)
    float dt = (current_time - prev_time) / 1000000.0; // Divide by 1 million to convert into seconds

    // Update global position variables
    ang_x += wx*dt;
    ang_y += wy*dt;
    ang_z += wz*dt;

    if (debug == 1){ // Only print rate to serial monitor if debut is set to 1 (set to 0 by default)
        // Calculate the rate
        Serial.println(1.0/dt);
    }
    // Return current time for looping
    return current_time;
}   

// Madgwick position funciton
long madgwick_position(Adafruit_LSM6DSO32& IMU, Madgwick& filter, long prevMicros, unsigned long rate = 500, unsigned long debug = 0){
    // Initial Vars
    float microsPerRead = 1000000.0/rate; // Calculates the time (in microseconds) between each read given the inputted data rate

    // Get current time (in microseconds)
    long currentMicros = micros();

    // If time to read sensor, read sensor
    if ((currentMicros - prevMicros) >= microsPerRead){
        // Get data from IMU
        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;
        IMU.getEvent(&accel, &gyro, &temp);

        // Save angular velocity & convert to deg/s
        float wx = gyro.gyro.x*(180/PI);
        float wy = gyro.gyro.y*(180/PI);
        float wz = gyro.gyro.z*(180/PI);
        // Save acceleration & convert to g
        float ax = accel.acceleration.x/9.80665;
        float ay = accel.acceleration.y/9.80665;
        float az = accel.acceleration.z/9.80665;

        // Update filter
        filter.updateIMU(wx,wy,wz,ax,ay,az);

        // Update global variables
        ang_x = filter.getRoll();
        ang_y = filter.getPitch();
        ang_z = filter.getYaw();

        if (debug == 1){ // Print angles if debuging is desired
            Serial.print("X: ");
            Serial.print(ang_x);
            Serial.print(" Y: ");
            Serial.print(ang_y);
            Serial.print(" Z: ");
            Serial.println(ang_z);
        }

        // Update time
        return currentMicros;
    }else{
        // Return inputted time if not ready to read (keeps value the same)
        return prevMicros;
    }
}

// Vector math test
void vector_disp (Adafruit_LSM6DSO32& IMU, unsigned long mode = 1){
    // Get data from IMU
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    IMU.getEvent(&accel, &gyro, &temp);

    // Get float values for acceleration
    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;

    // Calculate offset (angle determined by using accelerometer vectors only)
    float xOff = atan2(ay,az); // Roll (rad)
    float yOff = atan2((-ax),sqrt((ay*ay) + (az*az))); // Pitch (rad)

    // Calculate the overall tilt angle from the +Z axis
    float tilt = acos(cos(xOff)*cos(yOff)); // Don't still fully understand why this works, but it seems to be accurate

    // Print
    Serial.print("X: ");
    Serial.print(xOff*(180/PI),5);
    Serial.print(" Y: ");
    Serial.print(yOff*(180/PI),5);
    Serial.print(" Tilt: ");
    Serial.println(tilt*(180/PI),5);
}

// Calibrate Gravity - Calibrates the global variables for angle by using a gravity vector
// Note: This function will not work during flight, it only works while stationary on the ground, like when sitting on the pad
// 