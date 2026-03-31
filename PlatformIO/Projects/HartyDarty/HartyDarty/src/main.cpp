/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File mangement/logging
03/13/26 - LT - Forked from "Code" branch to create this simplified branch for easy testing of functions




**/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>

// Check that all components are up and running

// Init IMU
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name"
MS5611 MS5611(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

// Define pins for continuity testing
// NOTE! Reflects ports on final flight computer, not breadboard computer!
#define ig1 2
#define cont1 1
#define ig2 4
#define cont2 3
#define ig3 9
#define cont3 8


void setup(void) {
  Serial.begin(115200);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // All sensor initializations offloaded to 
  sensor_init(dso32,MS5611);

  //Setup PinModes for continuity testing
  //Setting low for continuity testing

  // Mostfet 1
  analogSetPinAttenuation(cont1,ADC_11db);
  pinMode(ig1,OUTPUT);
  pinMode(cont1,INPUT);
  digitalWrite(ig1,LOW); // Sets mosfet, LOW means off, HIGH means on
  float ADC = 0;
  // Mostfet 2
  analogSetPinAttenuation(cont2,ADC_11db);
  pinMode(ig2,OUTPUT);
  pinMode(cont2,INPUT);
  digitalWrite(ig2,LOW); // Sets mosfet, LOW means off, HIGH means on
  // Mosfet 3
  analogSetPinAttenuation(cont3,ADC_11db);
  pinMode(ig3,OUTPUT);
  pinMode(cont3,INPUT);
  digitalWrite(ig3,LOW); // Sets mosfet, LOW means off, HIGH means on

  pinMode(LED_BUILTIN, OUTPUT);
  //Serial.begin(115200);
  Serial.println();
}

// Derive Angular Position from IMU data
float roll = 0.0;
float pitch = 0.0;
unsigned long last_micros = 0; 

void AngularPosition() {
  // Read IMU data
  dso32.getEvent(&accel, &gyro, &temp2);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  //Radians.
  float gx = gyro.gyro.x; 
  float gy = gyro.gyro.y;
  float gz = gyro.gyro.z;

  //dt (in seconds) using micros()
  unsigned long current_micros = micros();
  if (last_micros == 0) 
  {
      last_micros = current_micros; 
  }
  float dt = (current_micros - last_micros) / 1000000.0;
  last_micros = current_micros;

  //Accelerometer angles 
  float roll_acc  = atan2(ay, az);
  float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az));

  roll  = 0.98 * (roll + gx * dt) + 0.02 * roll_acc;
  pitch = 0.98 * (pitch + gy * dt) + 0.02 * pitch_acc;

  //Convert to degrees
  float roll_deg  = roll * 180.0 / PI;
  float pitch_deg = pitch * 180.0 / PI;

  //Output 
  Serial.print(">Roll:");
  Serial.print(roll_deg);
  Serial.print(",");
  Serial.print("Pitch:");
  Serial.println(pitch_deg);
}


void loop() {
  // Prints sensor data (Commented out for now)
  data_print_test(dso32,MS5611,1);
  AngularPosition();
  
  // Tests continuity
  // Turn the GPIO ports for ignition and continuity into integer arrays for input to function
  int ig[3]={ig1,ig2,ig3};
  int cont[3]={cont1,cont2,cont3};
  // continuity_test(0,ig,cont); // Commented out for ease of testing

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

