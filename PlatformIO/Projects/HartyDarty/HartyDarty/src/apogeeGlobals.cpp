#include "localFunctions.h"

bool firstApogeeSample = true;
bool apogeeDetected = false;

float gyroPrev = 0;
float pyroPrev = 0;

float apogee_gx = 0;
float apogee_gy = 0;
float apogee_gz = 0;

float apogee_gyroMag = 0;
float apogee_alt_raw = 0;
float apogee_alt_rel = 0;
