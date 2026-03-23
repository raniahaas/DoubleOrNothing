/**
Documentation block
03/04/26 - RH - Modified HTML code to look better
03/12/26 - RH - Cleaned up comments and created separate file for File mangement/logging
03/23/26 - RH - Added function for apogee detection for console log/CSV

*/
#include <Arduino.h>
#include <Adafruit_LSM6DSO32.h>
#include <MS5611.h>
#include <test_functions.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

//link external funcitons
#include <LittleFS.h>
#include "FS.h"
#include "fileManagement.h"
#include "localFunctions.h"

//formats file system if not already formatted
#define FORMAT_LITTLEFS_IF_FAILED true

//WIFI CREDS
const char *ssid = "XIAO_ESP32S3";
const char *password = "password";

WiFiServer server(80);

//data rate parameters 
unsigned long data_rate = 100; // Data rate in Hz
unsigned long iter = 0;

//storage arrays
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

//Check that all components are up and running
Adafruit_LSM6DSO32 dso32;

// Set I2C adress for barometer - Ignore any error here relating to "not a class name"
MS5611 MS5611(0x77);
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp2;

#define ig1 2
#define cont1 1
#define ig2 4
#define cont2 3
#define ig3 9
#define cont3 8


void setup(void) {
  Serial.begin(115200);
  delay(100); // will pause Zero, Leonardo, etc until serial console opens

  // Initialize & Format file system
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("LittleFS Mount Failed");
    return;
  }
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
    dso32.getEvent(&accel, &gyro, &temp2); // Gets data from IMU
    MS5611.read(); // Must be called each time before getting pressure or temp using below functions!
    float temp = MS5611.getTemperature();
    float pressure = MS5611.getPressure();
    // Altitude relative to sea level
    float seaLevelAltitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));

    // Altitude relative to launch site
    int j =0;
    float relativeAltitude = seaLevelAltitude - 226.2;
    temp_[j]=temp;
    pressure_[j]=pressure;
    Ax_[j]=xG;
    Ay_[j]=yG;
    Az_[j]=zG;
    Wx_[j]=gyroX;
    Wy_[j]=gyroY;
    Wz_[j]=gyroZ;
    Alt_[j]=seaLevelAltitude;
    RelativeAlt_[j]=relativeAltitude;


    // Read 25 samples of acceleration
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

  //RH - apogee funciton in LocalFunction.cpp
  checkApogee(dso32, MS5611, launch);

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

  //Reference for how the sensor reading works -------------------------------------------------------------------------------------------------------
  if (loggingActive && launch){
      for (int i=0; i<20; i++) {
        // Computes current time 
        float currentTime = millis();
        float normalTime = currentTime - launchTime; 

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
          

          //RH - Log apogee to file
          if (apogeeDetected) {
            file.print("Apogee Log");
            file.printf("Gyro raw: X=%.5f, Y=%.5f, Z=%.5f\n", apogee_gx, apogee_gy, apogee_gz);
            file.printf("Gyro magnitude: %.5f\n", apogee_gyroMag);
            file.printf("Altitude raw: %.5f\n", apogee_alt_raw);
            file.printf("Altitude relative: %.5f\n", apogee_alt_rel);

            //only write once
            apogeeDetected = false;
          }
          //END RH
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


  //Setting local host Wifi
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

          //HTML page
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

        //sample for website to test activity with LEDs
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

    //Ensure closing client and TCP connection
    client.stop();
    if(Serial){
      Serial.println("Client Disconnected.");
    }
  }
}