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
    //             apogee_