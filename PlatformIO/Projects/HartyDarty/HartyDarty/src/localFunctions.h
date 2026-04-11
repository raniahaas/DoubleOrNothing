#ifndef LOCALFUNCTIONS_H
#define LOCALFUNCTIONS_H

#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <FS.h>

// ===== IMPORT GLOBAL VARIABLES FROM main.cpp / globals.h =====

// Sensor objects
extern Adafruit_LSM6DSO32 dso32;
extern MS5611 baro;

// Shared sensor event structs
extern sensors_event_t accel;
extern sensors_event_t gyro;
extern sensors_event_t temp2;


// Staging + burnout globals
extern bool staged;
extern bool burnout;
extern bool launch;

extern float stagingTime;
extern float burnoutTime;
extern float launchTime;


//RH BEGIN
extern bool firstApogeeSample;
extern bool apogeeDetected;

extern float gyroPrev;
extern float pyroPrev;

extern float apogee_gx;
extern float apogee_gy;
extern float apogee_gz;

extern float apogee_gyroMag;
extern float apogee_alt_raw;
extern float apogee_alt_rel;
//RH END

//AJ BEGIN
extern bool staged;
extern float stagingTime;

extern float burnout_acc_threshold;

extern float staging_buffer[25];

extern float prevAccel;
extern float launchTime;
//AJ END

void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro); //RH
void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32); // AJ

#endif