/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File mangement/logging
03/13/26 - LT - Forked from "Code" branch to create this simplified branch for easy testing of functions
03/15/26 - LT - Updated the continuity_test function to add a port indicator mode, commented out sensor calls and un-commented continuity test calls
03/16/26 - LT - Re-added standalone barometer read function to test I2C functionality with PCB, created a function to exclusively read the barometer and not the IMU for this testing



**/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>

// Check that all components are up and running

// Init IMU
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name" or "not a name type"
MS5611 MS5611(0x77);
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


void setup(void) {
  Serial.begin(115200);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // All sensor initializations offloaded to below function
  //sensor_init(dso32,MS5611); // Commented out for testing w/ initial PCB that doesn't have sensors

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

  // ** Starup barometer for I2C testing
  Wire.begin();
  if (MS5611.begin() == true){
      Serial.println("MS5611 found.");
  } else{
      Serial.println("MS5611 not found. halt.");
      while (1);
  }
  Serial.println();
  MS5611.setOversampling(OSR_STANDARD);
  // ** Remove all code between asterisks once initial I2C function testing complete!
}

void loop() {
  // Prints sensor data (Commented out for now)
  // data_print_test(dso32,MS5611,1); // Commented out for testing w/ initial PCB that doesn't have sensors
  
  // Tests continuity
  // Turn the GPIO ports for ignition and continuity into integer arrays for input to function
  int ig[3]={ig1,ig2,ig3};
  int cont[3]={cont1,cont2,cont3};
  //continuity_test(2,ig,cont); // Commented out for ease of testing

  // Quick and dirty barometer reading code for I2C testing (moved to seperate function) **
  barometer_test(MS5611,0);
  // ** Remove all code between asterisks once initial I2C function testing complete!

  // NOTE: Below code may be redundant, commented out for now but delete if confirmed redundant with testing 
/*   float zG = accel.acceleration.z;
  float xG = accel.acceleration.x;
  float yG = accel.acceleration.y;

  float gyroX = gyro.gyro.x;
  float gyroY = gyro.gyro.y;
  float gyroZ = gyro.gyro.z;

  dso32.getEvent(&accel, &gyro, &temp2); // Gets data from IMU */
  // End Note!
}