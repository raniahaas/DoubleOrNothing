#include "localFunctions.h"

//declarations from main
bool firstApogeeSample = true;
float gyroPrev = 0;
float pyroPrev = 0;

bool apogeeDetected = false;

float apogee_gx = 0;
float apogee_gy = 0;
float apogee_gz = 0;

float apogee_gyroMag = 0;

float apogee_alt_raw = 0;
float apogee_alt_rel = 0;
bool p = false;

void Flight(Adafruit_LSM6DSO32) {

}

void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro, bool launch) {
    if (!launch || apogeeDetected)
        return;

    //read sensor values
    sensors_event_t accel, gyro, temp2;
    imu.getEvent(&accel, &gyro, &temp2);
    baro.read();

    float pressure = baro.getPressure();
    float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
    float pyrolog = seaLevelAltitude - 226.2;   //Convert to reative level; this is CI in metres

    float gx = gyro.gyro.x;
    float gy = gyro.gyro.y;
    float gz = gyro.gyro.z;

    //euclidean norm eq to determine with gyro
    float gyrolog = sqrt(gx*gx + gy*gy + gz*gz);

    //skip first
    if (firstApogeeSample) {
        gyroPrev = gyrolog;
        pyroPrev = pyrolog;
        firstApogeeSample = false;
    }
    else {
        //apogee = gyro peaks & the altitude drops
        if (!p) {
            if ((gyroPrev > gyrolog) && (pyroPrev > pyrolog)) {
                //print into serial monitor
                Serial.println("Apogee Detected");
                Serial.printf("Gyro data: X=%.5f, Y=%.5f, Z=%.5f\n", gx, gy, gz);
                Serial.printf("Gyro magnitude: %.5f\n", gyrolog);
                Serial.printf("Altitude data: %.5f m\n", seaLevelAltitude);
                Serial.printf("Altitude relative to Ci: %.5f m\n", pyrolog);

                //update values to write into csv file
                apogeeDetected = true;

                apogee_gx = gx;
                apogee_gy = gy;
                apogee_gz = gz;

                apogee_gyroMag = gyrolog;

                apogee_alt_raw = seaLevelAltitude;
                apogee_alt_rel = pyrolog;

                p = true;
            }
        }
        
    }

    //update previous values
    gyroPrev = gyrolog;
    pyroPrev = pyrolog;
}

// BEGIN AJ - 03/19/2026
// Staging detection
bool staged = false;
float stagingTime = 0;

// thresholds (i will tune these)
float burnout_acc_threshold = 5.0; // m/s^2, potentially change to g's, but for now just testing with m/s^2. This is the acceleration threshold for burnout detection, which is when the acceleration drops significantly after launch. We can tune this value based on expected acceleration profiles of the rocket. A value of 5 m/s^2 means that if the average acceleration drops below 5 m/s^2, we might be in burnout.
float burnout_delta_threshold = -2.0; // sudden drop

float staging_buffer[25];

// for delta method
float prevAccel = 0; 
float launchTime = 0;

// END AJ

// BEGIN AJ - 03/19/2026
void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32, bool launch) {
    //read sensor values
    sensors_event_t accel, gyro, temp2;
    dso32.getEvent(&accel, &gyro, &temp2);
    baro.read();
  
    // Staging detection
  static bool prevAccelInit = false;
  if(launch && !staged){
    float delta = 0.0f;
    for (int z=0; z<25; z++){
      dso32.getEvent(&accel, &gyro, &temp2);

      staging_buffer[z] = accel.acceleration.x; // why is it not acceleration.z? idk, just testing with x for now

      delta += staging_buffer[z];
      // optional: delay(2); // add spacing if you want a time window
    }
    float average = delta/25.0f;

    if(Serial) {
      Serial.print("Average: ");
      Serial.print(average);
      Serial.println(" m/s^2");
    }

    // Ensure launch event print is also available here (keeps event prints within lines 398-435)
    if (launch) {
      if (Serial) {
        Serial.println("Event: Launch detected");
        Serial.print("LaunchTime (ms): ");
        Serial.println(launchTime);
      }
    }

    if (!prevAccelInit) {
      prevAccel = average;
      prevAccelInit = true;
    }else{
      // Trigger if absolute low accel OR sudden drop compared to previous average
      if (average <= burnout_acc_threshold || (average - prevAccel) <= burnout_delta_threshold) {
        staged = true;
        stagingTime = millis();
        if (Serial) {
          Serial.println("Event: Staging detected");
          Serial.print("StagingTime (ms): ");
          Serial.println(stagingTime);
        }
    }
    prevAccel = average;
    }
  }
  // END AJ
}