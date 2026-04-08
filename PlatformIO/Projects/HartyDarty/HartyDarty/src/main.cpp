/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File mangement/logging
03/13/26 - LT - Forked from "Code" branch to create this simplified branch for easy testing of functions
03/15/26 - LT - Updated the continuity_test function to add a port indicator mode, commented out sensor calls and un-commented continuity test calls
03/16/26 - LT - Re-added standalone barometer read function to test I2C functionality with PCB, created a function to exclusively read the barometer and not the IMU for this testing
03/21/26 - LT - Added function to test IMU wihout use of serial monitor by activating pyro ports depending on board orientation
03/21/26 - LT - Added function to ignite pyro channels via the serial monitor, currently only supports a single port at a time and doesn't test for continuity
04/08/26 - LT - Created new branch to test out datalogging and multithreading


**/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>
#include <globals.h> // Header file for the global variables 
#include <orientation.h> 
#include <LittleFS.h> // File system library
#include <FreeRTOS.h> // Multithreading library
#include <task.h> // Library for defining tasks that can be pinned to threads
#include <queue.h> // Library for creating ques that send data between cores

// Check that all components are up and running ------------------------------------------------------------------
// Init IMU
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name" or "not a name type"
MS5611 MS5611(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

// Define pins for continuity testing ------------------------------------------------------------------
// Note: Try to keep all definitions all caps, this helps differentiate them from regular variables!
// NOTE! Reflects ports on final flight computer, not breadboard computer!
#define ig1 4
#define cont1 3
#define ig2 8
#define cont2 9
#define ig3 2
#define cont3 1

// Define files for datalogging ------------------------------------------------------------------
#define IMU_FILE "/IMU.csv" // File for saving IMU data, at a higher rate than baro data
#define BARO_FILE "/BARO.csv" // File for saving barometer data
#define EVENT_FILE "/EVENT.csv" // File for event log

// Define data rates for sensors
#define IMU_RATE 500
#define BARO_RATE 100

// Define data structures for datalogging ------------------------------------------------------------------
typedef struct{ // Structure for IMU data that will be sent to 
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

// Define Variables ------------------------------------------------------------------
// Define global variables for angular position - ONLY DONE IN THIS FILE!
float ang_x = 0;
float ang_y = 0;
float ang_z = 0;

// Define time variables
long prev_micros;
long print_delay;

// Calculate time per read for each sensor (in microseconds) ------------------------------------------------------------------
float IMUmicrosPerRead = 1000000.0/IMU_RATE; // Microseconds per IMU read
float BAROmicrosPerRead = 1000000.0/BARO_RATE; // Microseconds per barometer read

void setup(void) {
  Serial.begin(9600);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // All sensor initializations offloaded to below function
  sensor_init(dso32,MS5611); // Commented out for testing w/ initial PCB that doesn't have sensors

  // Setup PinModes for continuity testing
  // Setting pins low for continuity testing, setting high opens MOSFETs

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

  // Start file system ------------------------------------------------------------------
  if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed!");
        while (1);
    }

    

  // Set first value for time variables (microseconds)
  prev_micros = micros();
  // Set value for print delay
  print_delay = millis();
}

void loop() {

}