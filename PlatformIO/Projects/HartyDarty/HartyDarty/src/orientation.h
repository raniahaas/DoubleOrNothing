#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <Arduino.h>
#include <MadgwickAHRS.h>

long simple_position(Adafruit_LSM6DSO32& IMU, long prev_time, unsigned long debug = 0);
long madgwick_position(Adafruit_LSM6DSO32& IMU, Madgwick& filter, long prevMicros, unsigned long rate = 500, unsigned long debug = 0);
void vector_disp (Adafruit_LSM6DSO32& IMU, unsigned long mode = 1);
void gravity_cal(Adafruit_LSM6DSO32& IMU, int mode = 0);
void IMU_update(Adafruit_LSM6DSO32& IMU, unsigned long prev_time, int rate=300);
void BARO_update(MS5611& BARO, unsigned long prev_time, int rate = 100);

#endif