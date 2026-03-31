/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/19/26 - AJ - Added staging detection variables and thresholds, will implement in loop later



*/





#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
// File system setup
#include <LittleFS.h>
#include "FS.h"

// Formats file system if not already formatted
#define FORMAT_LITTLEFS_IF_FAILED true

// Set these to your desired credentials.
const char *ssid = "XIAO_ESP32S3";
const char *password = "password";

WiFiServer server(80);

// Set data rate parameters 
unsigned long data_rate = 100; // Data rate in Hz
unsigned long iter = 0;
// Create storage arrays, has underscores after name for temporary deconfliction w/ existing code
float t_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float temp_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float pressure_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Ax_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Ay_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Az_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Wx_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Wy_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Wz_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float Alt_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float RelativeAlt_[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//delays for file writing
unsigned long startTime = 0;
float launchTime = 0;
unsigned long lastWriteTime = 0;
const unsigned long writeInterval = 1000/100; // 100Hz
// Time to touchdown minimum 160, probably do 200
const unsigned long runDuration = 180*1000;  // 180 Seconds
bool loggingActive = true;
bool launch = false;
bool startTimeLogged = false;

// Launch detection constants
float launch_acc = 1*9.80665; // Launch acceleration threshold (g), 5g is actually 
float launch_buffer[25]; // Buffer that contains launch detection acceleration readings

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

// END AJ

//check that all components are up and running
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name"
MS5611 MS5611(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

// Define pins for continuity testing
// Ig for ignition wires and cont for continuity wires
// NOTE! Reflects ports on final flight computer, not breadboard computer!
#define ig1 2
#define cont1 1
#define ig2 4
#define cont2 3
#define ig3 9
#define cont3 8

// Functions for using the LittleFS file system - Will be moved out of main later! 
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("- failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  Serial.printf("Testing file I/O with %s\r\n", path);

  static uint8_t buf[512];
  size_t len = 0;
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }

  size_t i;
  Serial.print("- writing" );
  uint32_t start = millis();
  for(i=0; i<2048; i++){
    if ((i & 0x001F) == 0x001F){
      Serial.print(".");
    }
    file.write(buf, 512);
  }
  Serial.println("");
  uint32_t end = millis() - start;
  Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
  file.close();

  file = fs.open(path);
  start = millis();
  end = start;
  i = 0;
  if(file && !file.isDirectory()){
    len = file.size();
    size_t flen = len;
    start = millis();
    Serial.print("- reading" );
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      if ((i++ & 0x001F) == 0x001F){
        Serial.print(".");
      }
      len -= toRead;
      }
    Serial.println("");
    end = millis() - start;
    Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
    file.close();
  } else {
    Serial.println("- failed to open file for reading");
  }
}

void setup(void) {
  Serial.begin(115200);
  //while (!Serial) // Comment out if not running via USB, otherwise program won't run if serial doesn't open!
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // Initialize & Format file system
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("LittleFS Mount Failed");
    return;
  }
  // All sensor initializations offloaded to 
  sensor_init(dso32,MS5611);

  // Setup PinModes for continuity testing
  // Setting low for continuity testing
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

  // WiFi Setup -----------------

  //get call for wifi setup
  //hostname: esp32s3-000000
  /*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
    OR
    Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
  */

  //#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  //Serial.begin(115200);
  Serial.println();

  // Start Access Point (local-only WiFi)
  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  // Start web server
  server.begin();
  Serial.println("Server started");

  // Create file 
  writeFile(LittleFS,"/data.txt","");

  // Gets start time at the end of the startup cycle 
  startTime = millis();
}

void loop() {
  // Prints sensor data (Commented out for now)
  //data_print_test(dso32,MS5611,1);
  

  // Turn the GPIO ports for ignition and continuity into integer arrays for input to function
  int ig[3]={ig1,ig2,ig3};
  int cont[3]={cont1,cont2,cont3};
  // continuity_test(0,ig,cont); // Commented out for ease of testing

  float zG = accel.acceleration.z;
  float xG = accel.acceleration.x;
  float yG = accel.acceleration.y;

  float gyroX = gyro.gyro.x;
  float gyroY = gyro.gyro.y;
  float gyroZ = gyro.gyro.z;


  unsigned long currentTime = millis();

  dso32.getEvent(&accel, &gyro, &temp2); // Gets data from IMU

  // Launch detection ----------------------------------------
  if (!launch){
    // Read 100 samples of acceleration
    float delta = 0;
    for (int z=0; z<25; z++){
      dso32.getEvent(&accel, &gyro, &temp2);

      launch_buffer[z] = accel.acceleration.x;

      delta += launch_buffer[z];
      if(Serial){
        Serial.println(accel.acceleration.x);
      }
    }
    float average = delta/25;

    if(Serial) {
      Serial.print("Average: ");
      Serial.print(average);
      Serial.println(" m/s");
    }

    if (average>=launch_acc) {
      launch = true;
      launchTime = millis();
    }
  }
  // BEGIN AJ - 03/19/2026
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

  // Stop logging after 2 minutes
  currentTime = millis();
  if (loggingActive && (currentTime - launchTime >= runDuration) && launch) {
    loggingActive = false;
    if(Serial){
      Serial.print("Logging complete after ");
      Serial.print((currentTime-startTime)/1000.0);
      Serial.println(" seconds");
    }
  }

  // Log start time once
  if (loggingActive && !startTimeLogged && launch) {
    File file = LittleFS.open("/data.txt", "w");  // overwrite any previous content
    if (file) {
      time_t now = time(nullptr);  // optional: if RTC or NTP is available
      file.print("Logging started at millis: ");
      file.println(startTime);
      file.println("t+ (ms),temp (c),Pressure (mbar),ax (m/s^2),ay (m/s^2),az (m/s^2),wx (deg/s),wy (deg/s),wz (deg/s),Sea Level Alt,Relative (Dayton) Alt");
      file.close();
      startTimeLogged = true;
      if(Serial){
        Serial.println("Start time logged.");
      }
    }
  }

  // Log sensor data every 20 seconds
  // if (loggingActive && (currentTime - lastWriteTime >= writeInterval)) {
  //   lastWriteTime = currentTime;
  
  //   MS5611.read(); // Must be called each time before getting pressure or temp using below functions!
  //   float temp = MS5611.getTemperature();
  //   float pressure = MS5611.getPressure();
  //   // Altitude relative to sea level
  //   float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));

  //   // Altitude relative to launch site
  //   float relativeAltitude = seaLevelAltitude - 226.2;


  //   // Old version
  //   // File file = LittleFS.open("/data.txt", "a");  // append mode
  //   // if (file) {
  //   //   // Print to file
  //   //   file.printf("T+: %.2f ms\n",startTime-currentTime);
  //   //   file.printf("Temp: %.2f C, Pressure: %.2f mbar\n", temp, pressure);
  //   //   file.printf("Accel (g): X=%.2f, Y=%.2f, Z=%.2f\n", xG, yG, zG);
  //   //   file.printf("Gyro (°/s): X=%.2f, Y=%.2f, Z=%.2f\n\n", gyroX, gyroY, gyroZ);
  //   //   file.printf("Altitude (ASL): %.2f m\n", seaLevelAltitude);
  //   //   file.printf("Altitude (from Dayton): %.2f m\n", relativeAltitude); 

  //   //   // Print to serial monitor upon logging completion 
  //   //   Serial.println("Logging Completed");
  //   // } else {
  //   //   Serial.println("Failed to open /data.txt");
  //   // }

  //   // New version
  //   File file = LittleFS.open("/data.txt","a"); // "a" is for append mode
  //   if (file) { // Makes sure file open
  //     // Computes current time 
  //     float normalTime = currentTime - startTime; // Float for formatting, maybe fix later

  //     // Print to file
  //     file.printf("%.0f,",normalTime); // Logs time
  //     file.printf("%.2f,%.2f,",temp,pressure); // Logs temperature and pressure
  //     file.printf("%.5f,%.5f,%.5f,",xG,yG,zG); // Logs acceleration
  //     file.printf("%.5f,%.5f,%.5f,",gyroX,gyroY,gyroZ); // Logs gyro readings
  //     file.printf("%.5f,%.5f \n",seaLevelAltitude,relativeAltitude); // Logs Altitude
  //     file.close(); // Closes file

  //     // Serial print upon logging completion
  //     if(Serial){
  //       Serial.print("Logging Complete at T+: ");
  //       Serial.print((currentTime-startTime)/1000.0); // Converts current time from ms to seconds
  //       Serial.println(" seconds");
  //     }
  //   } else {
  //     if(Serial){
  //       Serial.println("Failed to open /data.txt");
  //     }
  //   }
  // }

  // Even newer version!
  // Reference for how the sensor reading works -------------------------------------------------------------------------------------------------------
  if (loggingActive && launch){
      for (int i=0; i<20; i++) {
        // Computes current time 
        float currentTime = millis();
        float normalTime = currentTime - launchTime; // Float for formatting, maybe fix later

        dso32.getEvent(&accel, &gyro, &temp2); // Gets data from IMU
        MS5611.read(); // Must be called each time before getting pressure or temp using below functions!
        float temp = MS5611.getTemperature();
        float pressure = MS5611.getPressure();
        // Altitude relative to sea level
        float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));

        // Altitude relative to launch site
        float relativeAltitude = seaLevelAltitude - 226.2;

        float zG = accel.acceleration.z;
        float xG = accel.acceleration.x;
        float yG = accel.acceleration.y;

        float gyroX = gyro.gyro.x;
        float gyroY = gyro.gyro.y;
        float gyroZ = gyro.gyro.z;

        t_[i]=normalTime;
        temp_[i]=temp;
        pressure_[i]=pressure;
        Ax_[i]=xG;
        Ay_[i]=yG;
        Az_[i]=zG;
        Wx_[i]=gyroX;
        Wy_[i]=gyroY;
        Wz_[i]=gyroZ;
        Alt_[i]=seaLevelAltitude;
        RelativeAlt_[i]=relativeAltitude;

        iter=1;
      }

      if (iter==1) {
        File file = LittleFS.open("/data.txt","a");
        if (file) {
          for (int k=0; k<20; k++) {

            file.printf("%.0f,",t_[k]); // Logs time
            file.printf("%.2f,%.2f,",temp_[k],pressure_[k]); // Logs temperature and pressure
            file.printf("%.5f,%.5f,%.5f,",Ax_[k],Ay_[k],Az_[k]); // Logs acceleration
            file.printf("%.5f,%.5f,%.5f,",Wx_[k],Wy_[k],Wz_[k]); // Logs gyro readings
            file.printf("%.5f,%.5f \n",Alt_[k],RelativeAlt_[k]); // Logs Altitude
          }
          file.close(); // close the file after the loop

          if (Serial) {
            float normalTime = currentTime - startTime;
            Serial.print("Logging Complete at : ");
            Serial.print((currentTime-startTime)/1000.0); // Converts current time from ms to seconds
            Serial.print(" seconds (since start)");
            Serial.print(" & T+: ");
            Serial.print((currentTime-launchTime)/1000);
            Serial.println(" seconds");
          }
        }

        iter = 0;
      }
    }


  WiFiClient client = server.available();
  if (client) {
    if(Serial){
      Serial.println("New Client.");
    }
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if(Serial){
          Serial.write(c);
        }
        if (c == '\n') {
          //file download for html
          if (currentLine.startsWith("GET /data.txt")) {
            File file = LittleFS.open("/data.txt", "r");
            if (file) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/plain");
              client.println("Content-Disposition: attachment; filename=\"data.txt\"");
              client.println();
              while (file.available()) {
                client.write(file.read());
              }
              file.close();
            } else {
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-Type: text/plain");
              client.println();
              client.println("File not found");
            }
            break;
          }

          //html edit page
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            //BEGIN RH - 03/04/2026
            //clean up look of HTML page
            client.println("<style>html { font-family: Times New Roman; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #FF0044; border: none; color: white; padding: 16px 40px;");
            client.println(".button2 {background-color: #4CAF50;}</style></head>");
            client.println("<html><body>");
            client.println("<h1>ESP32 - Double Trouble Server</h1>");
            client.println("<p><a href=\"/H\">button class=\"button\">Turn ON LED</button></a></p>");
            client.println("<p><a href=\"/L\">button class=\"button button2\">Turn OFF LED</button></a></p>"); //possible br end
            client.println("<a href=\"/data.txt\" download>Download File</a><br>");
            client.println("</body></html>");
            client.println();
            break;

            //END RH
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        //testing with LEDs
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, LOW);
          if(Serial){
            Serial.println("LED turned ON");
          }
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, HIGH);
          if(Serial){
            Serial.println("LED turned OFF");
          }
        }
      }
    }
    client.stop();
    if(Serial){
      Serial.println("Client Disconnected.");
    }
  }
}