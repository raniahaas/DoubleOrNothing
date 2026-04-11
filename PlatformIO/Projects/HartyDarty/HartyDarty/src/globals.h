#ifndef GLOBALS_H
#define GLOBALS_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ── Angular position globals ──────────────────────────────────────────────────
extern float ang_x;
extern float ang_y;
extern float ang_z;
extern float tilt;

// ── Data structures ───────────────────────────────────────────────────────────
typedef struct {
    int64_t timestamp_us;           // Microsecond timestamp (lasts ~292,000 years)
    float gx, gy, gz;               // Gyro (rad/s)
    float ax, ay, az;               // Accelerometer (m/s^2)
    float ang_x, ang_y, ang_z;      // Integrated angular position (degrees)
    float tilt;                     // Total tilt from vertical (degrees)
} IMUdata;

typedef struct {
    int64_t timestamp_us;
    float pressure;                 // mBar
    float temp;                     // Degrees C
    float altitude;                 // Feet
} BAROdata;

typedef struct {
    int64_t timestamp_us;
    char event[64];
} EVENTdata;

// ── Current sensor readings ───────────────────────────────────────────────────
extern IMUdata   currentIMU;
extern BAROdata  currentBARO;
extern EVENTdata currentEVENT;

// ── Queues (defined in main.cpp) ──────────────────────────────────────────────
extern QueueHandle_t imuQueue;
extern QueueHandle_t baroQueue;
extern QueueHandle_t eventQueue;

// ── Flight state flags ────────────────────────────────────────────────────────
extern bool  apogeeDetected;
extern bool  firstApogeeSample;
extern float gyroPrev;
extern float pyroPrev;
extern float apogee_gx, apogee_gy, apogee_gz;
extern float apogee_gyroMag;
extern float apogee_alt_raw;
extern float apogee_alt_rel;

#endif