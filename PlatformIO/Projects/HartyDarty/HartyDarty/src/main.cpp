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

**/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <MadgwickAHRS.h> // Madgwick filter library
#include <test_functions.h>
#include <localFunctions.h>
#include <globals.h> // Header file for the global variables 
#include <orientation.h> 

// Check that all components are up and running

// Init madgwick filter
Madgwick filter;

// Init IMU
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name" or "not a name type"
// BEGIN AJ - 03/19/2026
MS5611 baro(0x77);
// END AJ
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

//Universal variables from location functions
bool launch = false;

// Define pins for continuity testing
// NOTE! Reflects ports on final flight computer, not breadboard computer!
#define ig1 4
#define cont1 3
#define ig2 8
#define cont2 9
#define ig3 2
#define cont3 1

// Define global variables for angular position - ONLY DONE IN THIS FILE!
float ang_x = 0;
float ang_y = 0;
float ang_z = 0;

// Define time variables
long prev_micros;
long print_delay;

// Set rate for IMU (in Hz)
unsigned long rate = 500;
float microsPerRead = 1000000.0/rate; // Calculated the number of mircoseconds per reading of the IMU
// BEGIN AJ - 04/07/2026
void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32, bool launch){};
// END AJ
void setup(void) {
  Serial.begin(9600);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // All sensor initializations offloaded to below function
  // BEGIN AJ - 04/07/2026
  sensor_init(dso32,baro); // Commented out for testing w/ initial PCB that doesn't have sensors
  // END AJ
  // Setup Madwick filter for IMU, eventiall move to the IMU init function
  filter.begin(500);

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

/*   // ** Starup barometer for I2C testing
  Wire.begin();
  if (MS5611.begin() == true){
      Serial.println("MS5611 found.");
  } else{
      Serial.println("MS5611 not found. halt.");
      while (1);
  }
  Serial.println();
  MS5611.setOversampling(OSR_STANDARD);
  // ** Remove all code between asterisks once initial I2C function testing complete! */

  // Set first value for time variables (microseconds)
  prev_micros = micros();
  // Set value for print delay
  print_delay = millis();
}

void loop() {
  // Prints sensor data (Commented out for now)
  //data_print_test(dso32,MS5611,1); // Commented out for testing w/ initial PCB that doesn't have sensors
  
  // Tests continuity
  // Turn the GPIO ports for ignition and continuity into integer arrays for input to function
  int ig[3]={ig1,ig2,ig3};
  int cont[3]={cont1,cont2,cont3};

  //checkStaging(MS5611, dso32, launch);
  // BEGIN AJ - 04/07/2026
  checkStaging(baro, dso32, launch);
  // END AJ
  //continuity_test(2,ig,cont); // Commented out for ease of testing

  // Quick and dirty barometer reading code for I2C testing (moved to seperate function) **
  // BEGIN AJ - 04/07/2026
  //barometer_test(baro,0);
  // END AJ
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

  // MOSFET IMU Test function
  //mosfet_IMU_test(dso32,ig);

  // MOSFET Serial Test function
  pyro_serial(ig,cont); // Commented out to test global variables for angular position

  // Test global variables 

/*   // Print position every 5 seconds
  int delay_time = millis() - print_delay;
  if (delay_time >= (2500)){ // Prints the position every 2.5 seconds
    prev_micros = simple_position(dso32,prev_micros,1); // Position with rate info

    Serial.print("X: ");
    Serial.print(ang_x*(180.0/PI));
    Serial.print(" Y: ");
    Serial.print(ang_y*(180.0/PI));
    Serial.print(" Z ");
    Serial.println(ang_z*(180.0/PI));

    print_delay = millis(); // Update delay time
  } else {
    prev_micros = simple_position(dso32,prev_micros); // Position w/o rate info 
  } */

  // Test Madgwick filter
  //prev_micros = madgwick_position(dso32,filter,prev_micros,rate,1);

  // Test vector offset
  //vector_disp(dso32,0); // Commented out for testing
  //delay(10);
}