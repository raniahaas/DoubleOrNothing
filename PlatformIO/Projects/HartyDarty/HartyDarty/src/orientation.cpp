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
    float tilt = acos(cos(xOff)*cos(yOff)); // Don't still fully understand why this works, has something to do with rotation matrixes, but it seems to be accurate

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
void gravity_cal(Adafruit_LSM6DSO32& IMU, int mode = 0){
    // Get IMU data
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

    // Convert rad to degrees
    xOff = xOff*(180.0/PI);
    yOff = yOff*(180.0/PI);

    // Update globals for angular position
    ang_x = xOff;
    ang_y = yOff;
}

// IMU Update Function - Updates global variables for IMU read while also updating global position and tilt via integration
//
void IMU_update(Adafruit_LSM6DSO32& IMU, long prev_time){
    // Get new data from sensors
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    IMU.getEvent(&accel, &gyro, &temp);

    // Save IMU readings
    // Gyro readings are in rad/s
    currentIMU.gx = gyro.gyro.x;
    currentIMU.gy = gyro.gyro.y;
    currentIMU.gz = gyro.gyro.z;
    // Accel readings are in m/s^2
    currentIMU.ax = accel.acceleration.x;
    currentIMU.ay = accel.acceleration.y;
    currentIMU.az = accel.acceleration.z;

    // Save timestamp
    currentIMU.timestamp_us = micros();

    // Calculate change in time between last read (delta t)
    float dt = (currentIMU.timestamp_us - prev_time) / 1000000.0; // Divide by 1 million to convert into seconds

    // Update global position variables - Convert to degrees from radians
    ang_x += (currentIMU.gx*dt)*(180.0/PI);
    ang_y += (currentIMU.gy*dt)*(180.0/PI);
    ang_z += (currentIMU.gz*dt)*(180.0/PI);

    // Update global tilt variable
    tilt = acos(cos(ang_x*(PI/180.0))*cos(ang_y*(PI/180.0)))*(180.0/PI); // Don't still fully understand why this works, has something to do with rotation matrixes, but it seems to be accurate
}