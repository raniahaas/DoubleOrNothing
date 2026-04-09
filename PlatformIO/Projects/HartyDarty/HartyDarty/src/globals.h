#ifndef GLOBALS_H
#define GLOBALS_H

// Declare global variables for angular position
extern float ang_x; 
extern float ang_y;
extern float ang_z;
extern float tilt; // Variable for describing the total tilt from the positive z axis for angle lockout

// Define data structures for datalogging ------------------------------------------------------------------
typedef struct{ // Structure for IMU data that will be sent to datalogging file
  unsigned long timestamp_us; // Timestamp in microseconds, will last up to 72 minutes before rolling over!
  float gx, gy, gz; // Floats containing gyro values (deg/s)
  float ax, ay, az; // Floats containing accelerometer values (m/s^2)
} IMUdata;

typedef struct{
  unsigned long timestamp_us; // Timestamp in microseconds
  float pressure; // Float containing pressure (in mbar)
  float temp; // Float containing temperature (in C)
  float altitude; // Float containing calculated altitude (in ft)
} BAROdata;

typedef struct{
  unsigned long timestamp_us; // Timestamp in microseconds
  char event[64]; // Actual event 
} EVENTdata; 

// Declare global variables based on above structures
extern IMUdata currentIMU;
extern BAROdata currentBARO;
extern EVENTdata currentEVENT;

#endif