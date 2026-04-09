#ifndef LOCALFUNCTIONS_H
#define LOCALFUNCTIONS_H

#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <FS.h>

// State variables
extern bool apogeeDetected;

extern float apogee_gx;
extern float apogee_gy;
extern float apogee_gz;

extern float apogee_gyroMag;

extern float apogee_alt_raw;
extern float apogee_alt_rel;

// Function to call from loop()
void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro, bool launch);
void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32);


//AJ BEGIN
extern bool staged;
extern float stagingTime;

extern float burnout_acc_threshold;

extern float staging_buffer[25];

extern float prevAccel;
extern float launchTime;
//AJ END

#endif