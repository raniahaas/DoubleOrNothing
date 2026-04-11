#include "localFunctions.h"
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <LittleFS.h>
#include "FS.h"
#include <globals.h>
#define LAUNCH_ACCEL 4 // The amount of G that detects launch


// Apogee detection constants
const float noiseThreshold = 2.0;
const int decentCount = 5;
const float gyroMax = 20.0;

float apogeeTime = 0;

// NOTE: IMU_update, BARO_update, gravity_cal, vector_disp, simple_position
// are all defined in orientation.cpp — do NOT redefine them here.
// Make sure localFunctions.h does NOT redeclare them either (include orientation.h instead).

void logToCSV(const String &row) {
    File file = LittleFS.open("/data.csv", "a");
    if (file) {
        file.println(row);
        file.close();
    }
}



void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro) {

    if (apogeeDetected)
        return;

    sensors_event_t accel, gyro, temp2;
    imu.getEvent(&accel, &gyro, &temp2);
    baro.read();

    float pressure = baro.getPressure();
    float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
    float pyrolog = seaLevelAltitude - 226.2;

    float gx = gyro.gyro.x;
    float gy = gyro.gyro.y;
    float gz = gyro.gyro.z;
    float gyrolog = sqrt(gx*gx + gy*gy + gz*gz);

    if (pyrolog < 20) return;

    if (firstApogeeSample) {
        gyroPrev = gyrolog;
        pyroPrev = pyrolog;
        firstApogeeSample = false;
        Serial.println("Apogee sampling started.\n");
        return;
    }

    static bool gyroMaxed = false;
    static int descentCount = 0;

    if (!gyroMaxed && gyroPrev > gyrolog) {
        gyroMaxed = true;
        Serial.println("Gyro peak detected.\n");
    }

    if (gyroMaxed && !apogeeDetected) {
        if (pyrolog < pyroPrev - noiseThreshold) {
            descentCount++;
            Serial.printf("Descending sample %d\n", descentCount);
        } else {
            descentCount = 0;
        }
    }

    if (descentCount >= 3 && !apogeeDetected) {
        apogeeDetected = true;
        apogeeTime = millis();

        apogee_gx = gx;
        apogee_gy = gy;
        apogee_gz = gz;
        apogee_gyroMag = gyrolog;
        apogee_alt_raw = seaLevelAltitude;
        apogee_alt_rel = pyrolog;

        File file = LittleFS.open("/data.csv", "a");
        if (file) {
            file.printf(",,,,,,,,,,APOGEE,%lu\n", apogeeTime);
            file.close();
        }
        Serial.println("Apogee logged.");
    }

    gyroPrev = gyrolog;
    pyroPrev = pyrolog;

    if (descentCount >= 3 && !apogeeDetected) {
        apogeeDetected = true;
        apogeeTime     = millis();
        apogee_gx      = gx;
        apogee_gy      = gy;
        apogee_gz      = gz;
        apogee_gyroMag = gyrolog;
        apogee_alt_raw = seaLevelAltitude;
        apogee_alt_rel = pyrolog;

        if (eventQueue != NULL) {
            EVENTdata ev;
            ev.timestamp_us = micros();
            strncpy(ev.event, "APOGEE", sizeof(ev.event));
            xQueueSend(eventQueue, &ev, 0);
        }
        Serial.println("Apogee detected.");
    }
}

// Staging detection state
bool staged = false;
float stagingTime = 0;
float burnoutTime = 0;
bool launch = false;
bool burnout = false;

float burnout_acc_threshold = 4.0f * 9.80665f;
float burnout_delta_threshold = -3.0f;
float staging_buffer[25];
float prevAccel = 0;
float launchTime = 0;
int i = 0;
int j = 0;

void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32) {
    static bool launched = false;

    if (!launched && currentIMU.az >= (LAUNCH_ACCEL * 9.8)) {
        launched = true;
        Serial.println("Launch Detected");

        if (eventQueue != NULL) {
            EVENTdata ev;
            ev.timestamp_us = micros();
            strncpy(ev.event, "LAUNCH", sizeof(ev.event));
            xQueueSend(eventQueue, &ev, 0);
        }
    }
}
