#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

void data_print_test(Adafruit_LSM6DSO32& IMU, MS5611& BARO,int plot=1);
void continuity_test(int mode, int ig[3], int cont[3]);
void sensor_init(Adafruit_LSM6DSO32& IMU, MS5611& BARO);
void barometer_test(MS5611& BARO, int mode=0);
void mosfet_IMU_test(Adafruit_LSM6DSO32& IMU, int ig[3]);
void pyro_serial(int ig[3], int cont[3]);

#endif