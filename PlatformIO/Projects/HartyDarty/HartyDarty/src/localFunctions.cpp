#include "localFunctions.h"



//4/7 edits
const float noiseThreshold = 0.5;
const int decentCount = 5;
const float gyroMax = 20.0;

static bool gyroMaxed = false;
static float prevAlt = 0;
static int descentCount = 0;



//declarations from main


void Flight(Adafruit_LSM6DSO32) {

}

void checkApogee(Adafruit_LSM6DSO32 &imu, MS5611 &baro) {

    // Serial.print("firstApogeeSample = ");
    // Serial.println(firstApogeeSample ? "TRUE" : "FALSE");

    if (apogeeDetected)
        return;

    //read sensor values
    sensors_event_t accel, gyro, temp2;
    //commment out for testing purposes
    // imu.getEvent(&accel, &gyro, &temp2);
    // baro.read();
    float altitude = baro.getAltitude();

    //testing data
    

    // float pressure = baro.getPressure();
    //float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
    // float pyrolog = seaLevelAltitude - 226.2;   //Convert to reative level; this is CI in metres

    // float gx = gyro.gyro.x;
    // float gy = gyro.gyro.y;
    // float gz = gyro.gyro.z;


    float gx = 0, gy = 0, gz = 0;
    float gyrolog;
    float pyrolog;
    //euclidean norm eq to determine with gyro
    //float gyrolog = sqrt(gx*gx + gy*gy + gz*gz);

    // Simulate a flight profile
    static int t = 0;
    t+=5;

    

    // 1. Boost phase (gyro rising)
    if (t < 20) {
        gyrolog = t * 2;      // rising gyro
        pyrolog = t * 5;      // rising altitude
    }
    // 2. Coast (gyro falling)
    else if (t < 40) {
        gyrolog = 40 - (t-20);     // falling gyro
        pyrolog = 100 + (20-(t-20));  // still rising altitude
    }
    // 3. Apogee + descent
    else {
        gyrolog = 0;
        pyrolog = 100 - (t - 40);  // descending altitude
    }


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
        } else {
            if (decentCount > 0) {
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
        //Serial.printf("Altitude data: %.5f m\n", seaLevelAltitude);
        Serial.printf("Altitude relative to Ci: %.5f m\n", pyrolog);

        apogeeDetected = true;

        apogee_gx = gx;
        apogee_gy = gy;
        apogee_gz = gz;

        apogee_gyroMag = gyrolog;

        //apogee_alt_raw = seaLevelAltitude;
        apogee_alt_rel = pyrolog;

        Serial.print("Apogee Function would now be deploying Pyro");
    }

    //skip first
    // if (firstApogeeSample) {
    //     gyroPrev = gyrolog;
    //     pyroPrev = pyrolog;
    //     firstApogeeSample = false;
    // }
    // else {
    //     //apogee = gyro peaks & the altitude drops
    //     if (!p) {
    //         if ((gyroPrev > gyrolog) && (pyroPrev > pyrolog)) {
    //             //print into serial monitor
    //             Serial.println("Apogee Detected");
    //             Serial.printf("Gyro data: X=%.5f, Y=%.5f, Z=%.5f\n", gx, gy, gz);
    //             Serial.printf("Gyro magnitude: %.5f\n", gyrolog);
    //             Serial.printf("Altitude data: %.5f m\n", seaLevelAltitude);
    //             Serial.printf("Altitude relative to Ci: %.5f m\n", pyrolog);

    //             //update values to write into csv file
    //             apogeeDetected = true;

    //             apogee_gx = gx;
    //             apogee_gy = gy;
    //             apogee_gz = gz;

    //             apogee_gyroMag = gyrolog;

    //             apogee_alt_raw = seaLevelAltitude;
    //             apogee_alt_rel = pyrolog;

    //             p = true;
    //         }
    //     }
        
    // }

    //update previous values
    gyroPrev = gyrolog;
    pyroPrev = pyrolog;
}