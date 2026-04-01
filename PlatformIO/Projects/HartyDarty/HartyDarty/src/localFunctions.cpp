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