/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File mangement/logging
03/13/26 - LT - Forked from "Code" branch to create this simplified branch for easy testing of functions
03/15/26 - LT - Updated the continuity_test function to add a port indicator mode, commented out sensor calls and un-commented continuity test calls
03/16/26 - LT - Re-added standalone barometer read function to test I2C functionality with PCB, created a function to exclusively read the barometer and not the IMU for this testing
03/21/26 - LT - Added function to test IMU wihout use of serial monitor by activating pyro ports depending on board orientation
03/21/26 - LT - Added function to ignite pyro channels via the serial monitor, currently only supports a single port at a time and doesn't test for continuity
04/07/26 - AJ - Changed a bunch of functions and fixed a header error.
04/08/26 - RH - Changes to launch detection
04/08/26 - LT - Created new branch to test out datalogging and multithreading
04/09/26 - LT - Added function to update global structures for IMU and barometer data, IMU function also keeps track of angular position and tilt automatically! 
04/10/26 - LT - Added the tasks for file writing, still not finished adding the actual task declarations in setup tho


**/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>
#include <localFunctions.h>
#include <globals.h> // Header file for the global variables 
#include <orientation.h> 
#include <MadgwickAHRS.h>
#include <wifiSetup.h>
#include <LittleFS.h> // File system library
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Check that all components are up and running

// Init IMU
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name" or "not a name type"
MS5611 baro(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

// Define pins for continuity testing
// NOTE! Reflects ports on final flight computer, not breadboard computer!
#define ig1 4
#define cont1 3
#define ig2 8
#define cont2 9
#define ig3 2
#define cont3 1

// Define queue config ------------------------------------------------------------------
#define WRITE_CORE 0 // Sets the core that datalogging will use. 0 is the non-primary core, regular loop runs on core 1
#define IMU_QUEUE_LENGTH 64 // Adjust as needed
#define BARO_QUEUE_LENGTH 32 // Adjust as needed
#define EVENT_QUEUE_LENGTH 16 // Adjust as needed, 16 may not be needed

// Define files for datalogging ------------------------------------------------------------------
#define IMU_FILE "/IMU.csv" // File for saving IMU data, at a higher rate than baro data
#define BARO_FILE "/BARO.csv" // File for saving barometer data
#define EVENT_FILE "/EVENT.csv" // File for event log

// Define data rates for sensors
#define IMU_RATE 500
#define BARO_RATE 100

// Define launch parameters
#define LAUNCH_ACCEL 4 // The amount of G that detects launch

// Define queus for each datalogging function
static QueueHandle_t imuQueue; 
static QueueHandle_t baroQueue;
static QueueHandle_t eventQueue;

// Define task handles used in datalogging function shutdown - ADD BETTER DEFINITION LATER
static TaskHandle_t imuWriterHandle = NULL;
static TaskHandle_t baroWriterHandle = NULL;
static TaskHandle_t eventWriterHandle = NULL;

// Define file handles as global (in this file), used in shutdown of datalogging
static File imuFile;
static File baroFile;
static File eventFile;

// Define global variables for angular position - ONLY DONE IN THIS FILE!
float ang_x = 0;
float ang_y = 0;
float ang_z = 0;
float tilt = 0;

// Define datalogging parameters 
bool launched = false;
bool stage = false;
int max_datalog = 30; // Max time for datalogging in seconds
unsigned long launch_us; // Launch time in us
volatile bool loggingActive = false; // 

// Define global variables for current sensor readings - ONLY DONE IN THIS FILE!
IMUdata currentIMU;
BAROdata currentBARO;
EVENTdata currentEVENT;

long prevIMUTime = 0;
long prevBAROTime = 0;

// Calculate time per read for each sensor (in microseconds) ------------------------------------------------------------------
float IMUmicrosPerRead = 1000000.0/IMU_RATE; // Microseconds per IMU read
float BAROmicrosPerRead = 1000000.0/BARO_RATE; // Microseconds per barometer read

// Debug Zone: Delete all code below (until the setup loop) this line for final code
int pos_ind = 0; // Counter variable for position detection testing

// Define time variables
long prev_micros;
long print_delay;

// Set rate for IMU (in Hz)
unsigned long rate = 500;
float microsPerRead = 1000000.0/rate; // Calculated the number of mircoseconds per reading of the IMU


// ─── Each writer task cleans up after itself ───────────────────────────────────
void imuWriterTask(void* pvParameters) {
    imuFile = LittleFS.open(IMU_FILE, "a");
    if (!imuFile) {
        imuWriterHandle = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (imuFile.size() == 0)
        imuFile.println("timestamp_us,gyro_x,gyro_y,gyro_z,accel_x,accel_y,accel_z");

    IMUdata row;
    uint32_t count = 0;
    char buffer[128];

    while (loggingActive) {
        if (xQueueReceive(imuQueue, &row, pdMS_TO_TICKS(5)) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                row.timestamp_us,
                row.gx, row.gy, row.gz,
                row.ax, row.ay, row.az);
            imuFile.write((uint8_t*)buffer, len);
            if (++count % 50 == 0) imuFile.flush();
        }
    }

    // Task closes its own file then deletes itself
    imuFile.flush();
    imuFile.close();
    imuWriterHandle = NULL;
    vTaskDelete(NULL);
}

// ─── Same pattern for baro and event tasks ────────────────────────────────────
void baroWriterTask(void* pvParameters) {
    baroFile = LittleFS.open(BARO_FILE, "a");
    if (!baroFile) {
        baroWriterHandle = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (baroFile.size() == 0)
        baroFile.println("timestamp_us,pressure_hpa,temp_c,altitude_m");

    BAROdata row;
    uint32_t count = 0;
    char buffer[96];

    while (loggingActive) {
        if (xQueueReceive(baroQueue, &row, pdMS_TO_TICKS(5)) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%.2f,%.2f,%.2f\n",
                row.timestamp_us,
                row.pressure, row.temp, row.altitude);
            baroFile.write((uint8_t*)buffer, len);
            if (++count % 20 == 0) baroFile.flush();
        }
    }

    baroFile.flush();
    baroFile.close();
    baroWriterHandle = NULL;
    vTaskDelete(NULL);
}

void eventWriterTask(void* pvParameters) {
    eventFile = LittleFS.open(EVENT_FILE, "a");
    if (!eventFile) {
        eventWriterHandle = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (eventFile.size() == 0)
        eventFile.println("timestamp_us,event");

    EVENTdata row;
    char buffer[96];

    while (loggingActive) {
        if (xQueueReceive(eventQueue, &row, pdMS_TO_TICKS(5)) == pdTRUE) {
            int len = snprintf(buffer, sizeof(buffer),
                "%lld,%s\n",
                row.timestamp_us, row.event);
            eventFile.write((uint8_t*)buffer, len);
            eventFile.flush();
        }
    }

    eventFile.flush();
    eventFile.close();
    eventWriterHandle = NULL;
    vTaskDelete(NULL);
}

void startLogging() {
    loggingActive = true;   // MUST be set before tasks are created

    imuQueue   = xQueueCreate(IMU_QUEUE_LENGTH,   sizeof(IMUdata));
    baroQueue  = xQueueCreate(BARO_QUEUE_LENGTH,  sizeof(BAROdata));
    eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH,  sizeof(EVENTdata));

    xTaskCreatePinnedToCore(imuWriterTask,   "IMUWriter",   8192, NULL, 2, &imuWriterHandle,   WRITE_CORE);
    xTaskCreatePinnedToCore(baroWriterTask,  "BaroWriter",  8192, NULL, 2, &baroWriterHandle,  WRITE_CORE);
    xTaskCreatePinnedToCore(eventWriterTask, "EventWriter", 8192, NULL, 2, &eventWriterHandle, WRITE_CORE);
}

// ─── stopLogging just clears the flag and waits for tasks to finish ───────────
void stopLogging() {
    loggingActive = false;

    // Wait for all tasks to self-terminate cleanly
    while (imuWriterHandle != NULL || baroWriterHandle != NULL || eventWriterHandle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Serial.println("Datalogging stopped, all files closed.");
}

void setup(void) {
  Serial.begin(115200);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens
  firstApogeeSample = true; 
  //RH 4/8/26

  //BEGIN RH - 04/09/2026 - Added WIFI server
  delay(1000);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
  }

  startWiFi();

  //END RH

  sensor_init(dso32,baro); 
  // Setup PinModes for continuity testing
  // Setting pins low for continuity testing, setting high opens MOSFETs

  // Initial timestamps
  prevIMUTime = micros();
  prevBAROTime = micros();

  // Mostfet 1
  // Note: Set to Apogee on PCB
  analogSetPinAttenuation(cont1,ADC_11db);
  pinMode(ig1,OUTPUT);
  pinMode(cont1,INPUT);
  digitalWrite(ig1,LOW); // Sets mosfet, LOW means off, HIGH means on
  float ADC = 0;
  // Mostfet 2
  // Note: Set to Main on PCB
  analogSetPinAttenuation(cont2,ADC_11db);
  pinMode(ig2,OUTPUT);
  pinMode(cont2,INPUT);
  digitalWrite(ig2,LOW); // Sets mosfet, LOW means off, HIGH means on
  // Mosfet 3
  // Note: Set to Motor on PCB
  analogSetPinAttenuation(cont3,ADC_11db);
  pinMode(ig3,OUTPUT);
  pinMode(cont3,INPUT);
  digitalWrite(ig3,LOW); // Sets mosfet, LOW means off, HIGH means on

  // Define the built-in LED as an output that we can control
  pinMode(LED_BUILTIN, OUTPUT);
  //Serial.begin(115200);
  Serial.println();

  digitalWrite(LED_BUILTIN,LOW); // Turns built-in LED off after startup 

  // Create queues ------------------------------------------------------------------
  imuQueue = xQueueCreate(IMU_QUEUE_LENGTH, sizeof(IMUdata));
  baroQueue  = xQueueCreate(BARO_QUEUE_LENGTH,  sizeof(BAROdata));
  eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH,  sizeof(EVENTdata));

  // Update angular position to starting tilt ------------------------------------------------------------------
  delay(1000); // Delay to measure angle at startup
  gravity_cal(dso32); // Function uses gravity vector to determine starting angle on pad
  Serial.print("Offset X:");
  Serial.print(ang_x);
  Serial.print(" Offset Y:");
  Serial.println(ang_y);
  
  // Set first value for time variables (microseconds)
  prev_micros = micros();
  currentIMU.timestamp_us = micros();
  currentBARO.timestamp_us = micros();
  // Set value for print delay
  print_delay = millis();

  gravity_cal(dso32);   // orientation.cpp

  //RH - 04/09/26 - added back WIFI server download for CSV
    File file = LittleFS.open(BARO_FILE, "w");
    file.println("t,temp,pressure,ax,ay,az,wx,wy,wz,alt,relAlt,event,eventTime");

    file.println("0,25.0,1013.25,0,0,0,0,0,0,0,0,TEST,0");
    file.println("1,25.1,1013.20,0,0,0,0,0,0,0,0,TEST,0");

    file.close();
}

void loop() {
    IMU_update(dso32, currentIMU.timestamp_us);
    BARO_update(baro, currentBARO.timestamp_us);

    handleWiFiServer();

    if (!launched && currentIMU.az >= (LAUNCH_ACCEL * 9.8f)) {
        launched = true;
        launch_us = micros();
        startLogging();   // creates tasks and sets flag atomically
        Serial.println("Launch detected, logging started.");
    }

    if (launched && (micros() - launch_us) < ((unsigned long)max_datalog * 1000000UL)) {
        xQueueSend(imuQueue,  &currentIMU,  0);
        xQueueSend(baroQueue, &currentBARO, 0);
    } else if (launched && (micros() - launch_us) >= ((unsigned long)max_datalog * 1000000UL)) {
        if (loggingActive) {   // guard so stopLogging only called once
            stopLogging();
            Serial.println("Logging complete.");
        }
    }
}