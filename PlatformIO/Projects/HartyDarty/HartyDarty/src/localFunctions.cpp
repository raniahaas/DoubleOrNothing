#include "localFunctions.h"



//4/7 edits
const float noiseThreshold = 0.5;
const int decentCount = 5;
const float gyroMax = 20.0;

static bool gyroMaxed = false;
static float prevAlt = 0;
static int descentCount = 0;
float apogeeTime = 0;



//declarations from main


void Flight(Adafruit_LSM6DSO32) {

}

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