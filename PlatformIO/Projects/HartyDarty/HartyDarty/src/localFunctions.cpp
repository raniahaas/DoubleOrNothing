#include "localFunctions.h"

//declarations from main
//RH - 4/7
const float noiseThreshold = 0.5;
const int decentCount = 5;
const float gyroMax = 20.0;

static bool gyroMaxed = false;
static float prevAlt = 0;
static int descentCount = 0;
float apogeeTime = 0;

void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro) {
    delay(500);
    // Serial.print("firstApogeeSample = ");
    // Serial.println(firstApogeeSample ? "TRUE" : "FALSE");

    // Serial.print("ADDR check = ");
    // Serial.println((uintptr_t)&firstApogeeSample, HEX);


    if (apogeeDetected)
        return;

    //read sensor values
    sensors_event_t accel, gyro, temp2;
    //COMMENT out for testing purposes
    imu.getEvent(&accel, &gyro, &temp2);
    baro.read();
    float altitude = baro.getAltitude();


    float pressure = baro.getPressure();
    float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
    float pyrolog = seaLevelAltitude - 226.2;   //Convert to reative level; this is CI in metres

    float gx = gyro.gyro.x;
    float gy = gyro.gyro.y;
    float gz = gyro.gyro.z;

    //euclidean norm eq to determine with gyro
    float gyrolog = sqrt(gx*gx + gy*gy + gz*gz);


    //BEGIN testing data
    // static int t = 0;
    // t += 1;
    // Serial.print("t = ");
    // Serial.println(t);

    // 1. Boost phase (gyro rising)
    // if (t < 20) {
    //     gyrolog = t * 2;          // rising gyro
    //     pyrolog = t * 5;          // rising altitude
    // }
    // // 2. Coast (gyro falling)
    // else if (t < 40) {
    //     gyrolog = 40 - (t - 20);  // clean fall from 40 → 0
    //     pyrolog = 100;            // flat altitude at peak
    // }
    // // 3. Apogee + descent
    // else {
    //     gyrolog = 0;
    //     pyrolog = 100 - (t - 40); // descending altitude
    // }
    //END


    if (firstApogeeSample) {
        gyroPrev = gyrolog;
        pyroPrev = pyrolog;
        firstApogeeSample = false;

        Serial.println("Apogee sampling started.\n");
        return;
    }

    //gyro detection first
    if(!gyroMaxed) {
        if (gyroPrev > gyrolog) {
            gyroMaxed = true;
            Serial.println("Apogee Function detected max gyro value. Now coasting.\n");
        }
    }

    //confirm descending altitude
    if (gyroMaxed && !apogeeDetected) {
        if (pyrolog < pyroPrev - noiseThreshold) {
            descentCount++;
            Serial.printf("Apogee Function logging descending altitude values. Sample %d\n", descentCount);
            apogeeTime = millis();
        } else {
            if (descentCount > 0) {
                Serial.println("Apogee Function error logging descent. Resetting values.\n");
            }
            descentCount = 0;
        }
    }

    //need three samples of descent to confirm
    if (descentCount >=3) {
        Serial.println("Apogee Detected");
        Serial.printf("Gyro data: X=%.5f, Y=%.5f, Z=%.5f\n", gx, gy, gz);
        Serial.printf("Gyro magnitude: %.5f\n", gyrolog);
        //UNCOMMENT BEFORE FLIGHT
        //float timeTillApogee = apogeeTime - launchTime;
        //Serial.printf("Time till apogee: %d", apogeeTime);
        //Serial.printf("Altitude data: %.5f m\n", seaLevelAltitude);
        Serial.printf("Altitude relative to Ci: %.5f m\n", pyrolog);

        apogeeDetected = true;

        apogee_gx = gx;
        apogee_gy = gy;
        apogee_gz = gz;

        apogee_gyroMag = gyrolog;

        apogee_alt_raw = seaLevelAltitude;
        apogee_alt_rel = pyrolog;

        Serial.print("Apogee Function would now be deploying Pyro");
    }

    //update previous values
    gyroPrev = gyrolog;
    pyroPrev = pyrolog;
}

// BEGIN AJ - 03/19/2026
// Staging detection
bool staged = false;
float stagingTime = 0;
float burnoutTime = 0;
bool launch = false;
bool burnout = false;


// thresholds (i will tune these)
float burnout_acc_threshold = 5.0f * 9.80665f;   // ≈ 14.71 m/s^2hold = 5.0; // m/s^2, potentially change to g's, but for now just testing with m/s^2. This is the acceleration threshold for burnout detection, which is when the acceleration drops significantly after launch. We can tune this value based on expected acceleration profiles of the rocket. A value of 5 m/s^2 means that if the average acceleration drops below 5 m/s^2, we might be in burnout.
float burnout_delta_threshold = -3.0f; // sudden drop

float staging_buffer[25];

// for delta method
float prevAccel = 0; 
float launchTime = 0;
int i = 0;
int j = 0;

// END AJ

// BEGIN AJ - 03/19/2026
void checkStaging(MS5611 &baro, Adafruit_LSM6DSO32 &dso32) {
    //read sensor values
    sensors_event_t accel, gyro, temp2;
    dso32.getEvent(&accel, &gyro, &temp2);
    baro.read();
    float accelMag = sqrt(accel.acceleration.x * accel.acceleration.x +
                      accel.acceleration.y * accel.acceleration.y +
                      accel.acceleration.z * accel.acceleration.z);

    // Staging detection
    static bool prevAccelInit = false;
    //RH - set to 5Gs
    float launchThreshold = 5.1f * 9.80665f;
        if (!launch && accelMag > launchThreshold) {
            launch = true;
            launchTime = millis();
        }

    //END RH
    if(launch && !staged){
        float delta = 0.0f;
        for (int z=0; z<25; z++){
        dso32.getEvent(&accel, &gyro, &temp2);

        staging_buffer[z] = accel.acceleration.z; // why is it not acceleration.z? idk, just testing with x for now

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
                if (i == 0) {
                Serial.println("Event: Launch detected");
                Serial.print("LaunchTime (ms): ");
                Serial.println(launchTime);
                i++;}
            }
        }
        //RH - modifications to G threshold

        if(!launch)
            return;

        if (accelMag < (5.5f * 9.80665f)) {
            firstApogeeSample = true;
            return;
        }

        //include burnout staging detection
        if (!burnout) {
            if (average <= burnout_acc_threshold ||
                (average - prevAccel) <= burnout_delta_threshold) {
                burnoutTime = millis();
                burnout = true;
                Serial.println("Burnout detected");
                Serial.print("Burnout (ms): ");
                Serial.println(burnoutTime);
            }
        }

        //END RH

        if (!prevAccelInit) {
        prevAccel = average;
        prevAccelInit = true;
        }else{
        // Trigger if absolute low accel OR sudden drop compared to previous average
        if (burnout && !staged) {
            float ignitionThreshold = 3.0f * 9.80665f;
            staged = true;
            stagingTime = millis();
            if (Serial) {
            if (j == 0) {
            Serial.println("Event: Staging detected");
            Serial.print("StagingTime (ms): ");
            Serial.println(stagingTime);
            j++;}
            }
        }
        prevAccel = average;
    }
  }
  // END AJ
}