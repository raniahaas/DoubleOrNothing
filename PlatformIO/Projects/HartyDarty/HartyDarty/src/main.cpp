/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File management/logging
03/13/26 - LT - Forked from "Code" branch to create this simplified branch for easy testing of functions
03/15/26 - LT - Updated the continuity_test function to add a port indicator mode
03/16/26 - LT - Re-added standalone barometer read function to test I2C functionality with PCB
03/21/26 - LT - Added function to test IMU without use of serial monitor
03/21/26 - LT - Added function to ignite pyro channels via the serial monitor
04/07/26 - AJ - Changed a bunch of functions and fixed a header error
04/08/26 - RH - Changes to launch detection
04/08/26 - LT - Created new branch to test datalogging and multithreading
04/09/26 - LT - Added function to update global structures for IMU and barometer data
04/10/26 - LT - Added tasks for file writing
04/10/26 - LT - Added angular position and tilt to IMU log
04/11/26 - RH - Fixed global scope errors, duplicate fsHasSpace, wipe code in wrong place
**/

#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>
#include <localFunctions.h>
#include <globals.h>
#include <orientation.h>
#include <MadgwickAHRS.h>
#include <wifiSetup.h>
#include <LittleFS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ── Sensors ───────────────────────────────────────────────────────────────────
Adafruit_LSM6DSO32 dso32;
MS5611 baro(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

// ── Pyro pin definitions ──────────────────────────────────────────────────────
#define ig1   4
#define cont1 3
#define ig2   8
#define cont2 9
#define ig3   2
#define cont3 1

// ── Datalogging config ────────────────────────────────────────────────────────
#define WRITE_CORE         0
#define IMU_QUEUE_LENGTH   64
#define BARO_QUEUE_LENGTH  32
#define EVENT_QUEUE_LENGTH 16

#define IMU_RATE  500
#define BARO_RATE 100

// ── Queues (extern'd in globals.h) ────────────────────────────────────────────
QueueHandle_t imuQueue;
QueueHandle_t baroQueue;
QueueHandle_t eventQueue;

// ── Task + semaphore handles ──────────────────────────────────────────────────
static TaskHandle_t    imuWriterHandle   = NULL;
static TaskHandle_t    baroWriterHandle  = NULL;
static TaskHandle_t    eventWriterHandle = NULL;
static SemaphoreHandle_t fsReadySemaphore = NULL;

// ── Global sensor state (extern'd in globals.h) ───────────────────────────────
float ang_x = 0;
float ang_y = 0;
float ang_z = 0;
float tilt  = 0;

// ── Add these near the other globals ─────────────────────────────────────────
static bool loggingActive = true;
static unsigned long loggingStartMs = 0;
#define LOGGING_TIMEOUT_MS 150000UL  // 150 seconds

IMUdata   currentIMU;
BAROdata  currentBARO;
EVENTdata currentEVENT;

long prevIMUTime  = 0;
long prevBAROTime = 0;

// ── Timing helpers ────────────────────────────────────────────────────────────
long prev_micros;
long print_delay;
int  pos_ind = 0;

// ── Filesystem space check ────────────────────────────────────────────────────
bool fsHasSpace() {
    return (LittleFS.totalBytes() - LittleFS.usedBytes()) > 4096;
}

// ── IMU writer task ───────────────────────────────────────────────────────────
void imuWriterTask(void* pvParameters) {
    xSemaphoreTake(fsReadySemaphore, portMAX_DELAY);

    File file;
    for (int attempt = 0; attempt < 10; attempt++) {
        file = LittleFS.open(IMU_FILE, "a");
        if (file) break;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!file) {
        Serial.println("IMUWriter: FATAL - could not open file.");
        vTaskDelete(NULL);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (file.size() == 0)
        file.println("timestamp_us,gx,gy,gz,ax,ay,az,ang_x,ang_y,ang_z,tilt");

    file.flush();
    Serial.println("IMUWriter: ready.");

    IMUdata  row;
    uint32_t count = 0;
    char     buffer[192];

    while (1) {
        if (xQueueReceive(imuQueue, &row, portMAX_DELAY) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                (long long)row.timestamp_us,
                row.gx,    row.gy,    row.gz,
                row.ax,    row.ay,    row.az,
                row.ang_x, row.ang_y, row.ang_z,
                row.tilt);
            if (len > 0 && len < (int)sizeof(buffer)) {
                file.write((uint8_t*)buffer, len);
                if (++count % 50 == 0) file.flush();
            }
        }
    }

}

// ── Barometer writer task ─────────────────────────────────────────────────────
void baroWriterTask(void* pvParameters) {
    xSemaphoreTake(fsReadySemaphore, portMAX_DELAY);

    File file;
    for (int attempt = 0; attempt < 10; attempt++) {
        file = LittleFS.open(BARO_FILE, "a");
        if (file) break;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!file) {
        Serial.println("BaroWriter: FATAL - could not open file.");
        vTaskDelete(NULL);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (file.size() == 0)
        file.println("timestamp_us,pressure_mbar,temp_c,altitude_ft");

    file.flush();
    Serial.println("BaroWriter: ready.");

    BAROdata row;
    uint32_t count = 0;
    char     buffer[96];

    while (1) {
        if (xQueueReceive(baroQueue, &row, portMAX_DELAY) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%.2f,%.2f,%.2f\n",
                (long long)row.timestamp_us,
                row.pressure, row.temp, row.altitude);
            if (len > 0 && len < (int)sizeof(buffer)) {
                file.write((uint8_t*)buffer, len);
                if (++count % 20 == 0) file.flush();
            }
        }
    }
}

// ── Event writer task ─────────────────────────────────────────────────────────
void eventWriterTask(void* pvParameters) {
    xSemaphoreTake(fsReadySemaphore, portMAX_DELAY);

    File file;
    for (int attempt = 0; attempt < 10; attempt++) {
        file = LittleFS.open(EVENT_FILE, "a");
        if (file) break;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!file) {
        Serial.println("EventWriter: FATAL - could not open file.");
        vTaskDelete(NULL);
        while(1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (file.size() == 0)
        file.println("timestamp_us,event");

    file.flush();
    Serial.println("EventWriter: ready.");

    EVENTdata row;
    char      buffer[96];

    while (1) {
        if (xQueueReceive(eventQueue, &row, portMAX_DELAY) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%s\n",
                (long long)row.timestamp_us,
                row.event);
            if (len > 0 && len < (int)sizeof(buffer)) {
                file.write((uint8_t*)buffer, len);
                file.flush();
            }
        }
    }
}

// ── Logging shutdown ──────────────────────────────────────────────────────────
void stopLogging() {
    if (imuWriterHandle)   { vTaskDelete(imuWriterHandle);   imuWriterHandle   = NULL; }
    if (baroWriterHandle)  { vTaskDelete(baroWriterHandle);  baroWriterHandle  = NULL; }
    if (eventWriterHandle) { vTaskDelete(eventWriterHandle); eventWriterHandle = NULL; }
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("Datalogging tasks stopped.");
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    firstApogeeSample = true;

    // Create semaphore BEFORE creating tasks
    fsReadySemaphore = xSemaphoreCreateBinary();
    if (!fsReadySemaphore) {
        Serial.println("FATAL: Could not create semaphore.");
        while(1) delay(1000);
    }

    // Mount LittleFS — format on first boot or if corrupt
    if (!LittleFS.begin(true)) {
        Serial.println("FATAL: LittleFS mount failed.");
        while(1) delay(1000);
    }

    // ── Wipe filesystem if nearly full (>90% used) ────────────────────────────
    if (LittleFS.usedBytes() > LittleFS.totalBytes() * 0.9) {
        Serial.println("Filesystem >90% full — formatting...");
        LittleFS.format();
        Serial.println("Format complete.");
    }

    Serial.println("LittleFS mounted OK.");
    Serial.printf("  Total: %u bytes\n", LittleFS.totalBytes());
    Serial.printf("  Used:  %u bytes\n",  LittleFS.usedBytes());

    startWiFi();
    sensor_init(dso32, baro);

    // Pyro pin setup
    analogSetPinAttenuation(cont1, ADC_11db);
    pinMode(ig1, OUTPUT);  pinMode(cont1, INPUT);  digitalWrite(ig1, LOW);

    analogSetPinAttenuation(cont2, ADC_11db);
    pinMode(ig2, OUTPUT);  pinMode(cont2, INPUT);  digitalWrite(ig2, LOW);

    analogSetPinAttenuation(cont3, ADC_11db);
    pinMode(ig3, OUTPUT);  pinMode(cont3, INPUT);  digitalWrite(ig3, LOW);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Create queues
    imuQueue   = xQueueCreate(IMU_QUEUE_LENGTH,   sizeof(IMUdata));
    baroQueue  = xQueueCreate(BARO_QUEUE_LENGTH,  sizeof(BAROdata));
    eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(EVENTdata));

    if (!imuQueue || !baroQueue || !eventQueue) {
        Serial.println("FATAL: Queue creation failed.");
        while(1) delay(1000);
    }

    // Pin writer tasks to core 0
    xTaskCreatePinnedToCore(imuWriterTask,   "IMUWriter",   12288, NULL, 2, &imuWriterHandle,   WRITE_CORE);
    xTaskCreatePinnedToCore(baroWriterTask,  "BaroWriter",  12288, NULL, 2, &baroWriterHandle,  WRITE_CORE);
    xTaskCreatePinnedToCore(eventWriterTask, "EventWriter", 12288, NULL, 2, &eventWriterHandle, WRITE_CORE);

    // Calibrate orientation
    delay(1000);
    gravity_cal(dso32);
    Serial.print("Offset X: "); Serial.print(ang_x);
    Serial.print("  Offset Y: "); Serial.println(ang_y);

    // Seed timestamps
    prevIMUTime  = micros();
    prevBAROTime = micros();
    currentIMU.timestamp_us  = prevIMUTime;
    currentBARO.timestamp_us = prevBAROTime;
    prev_micros = micros();
    print_delay = millis();

    // Signal all three tasks that filesystem is ready
    xSemaphoreGive(fsReadySemaphore);
    xSemaphoreGive(fsReadySemaphore);
    xSemaphoreGive(fsReadySemaphore);
    loggingStartMs = millis();

    // Signal all three tasks that filesystem is ready
    xSemaphoreGive(fsReadySemaphore);
    xSemaphoreGive(fsReadySemaphore);
    xSemaphoreGive(fsReadySemaphore);

    Serial.println("Setup complete.");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = micros();

    // ── Logging timeout ───────────────────────────────────────────────────────
    if (loggingActive && (millis() - loggingStartMs) >= LOGGING_TIMEOUT_MS) {
        loggingActive = false;
        stopLogging();
        Serial.println("Logging timeout reached — WiFi server active, connect to download.");

        // Log the timeout as a final event before stopping
        if (eventQueue != NULL) {
            EVENTdata ev;
            ev.timestamp_us = micros();
            strncpy(ev.event, "LOGGING_STOPPED", sizeof(ev.event));
            xQueueSend(eventQueue, &ev, 0);
            delay(200); // Give event writer a moment to flush before task is killed
        }
    }

    // ── IMU update + queue push ───────────────────────────────────────────────
    if (loggingActive && (now - prevIMUTime) >= (1000000UL / (unsigned long)IMU_RATE)) {
        IMU_update(dso32, prevIMUTime, IMU_RATE);

        currentIMU.ang_x = ang_x;
        currentIMU.ang_y = ang_y;
        currentIMU.ang_z = ang_z;
        currentIMU.tilt  = tilt;

        if (imuQueue != NULL && fsHasSpace())
            xQueueSend(imuQueue, &currentIMU, 0);

        prevIMUTime = currentIMU.timestamp_us;
    }

    // ── Barometer update + queue push ─────────────────────────────────────────
    if (loggingActive && (now - prevBAROTime) >= (1000000UL / (unsigned long)BARO_RATE)) {
        BARO_update(baro, prevBAROTime, BARO_RATE);

        if (baroQueue != NULL && fsHasSpace())
            xQueueSend(baroQueue, &currentBARO, 0);

        prevBAROTime = currentBARO.timestamp_us;
    }

    // ── WiFi always runs regardless of logging state ──────────────────────────
    handleWiFiServer();

    // ── Flight logic only runs while logging ──────────────────────────────────
    if (loggingActive) {
        checkStaging(baro, dso32);
        checkApogee(dso32, baro);
    }

    // ── Debug print every 20 cycles ───────────────────────────────────────────
    if (pos_ind >= 20) {
        Serial.print("ax:"); Serial.print(currentIMU.ax);
        Serial.print(" ay:"); Serial.print(currentIMU.ay);
        Serial.print(" az:"); Serial.println(currentIMU.az);
        Serial.print("posX:"); Serial.print(ang_x);
        Serial.print(" posY:"); Serial.print(ang_y);
        Serial.print(" tilt:"); Serial.println(tilt);
        if (!loggingActive) Serial.println("*** LOGGING STOPPED — connect to WiFi to download ***");
        pos_ind = 0;
    } else {
        pos_ind++;
    }
}