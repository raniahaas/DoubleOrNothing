#pragma once
#include <WiFi.h>
#include <WiFiAP.h>
#include <LittleFS.h>
#include "FS.h"

//192.168.4.1
const char *ssid = "XIAO_ESP32S3";
const char *password = "password";

// Web server
WiFiServer server(80);

// Start WiFi AP + server
void startWiFi() {
    Serial.println("\nStarting WiFi Access Point…");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(apIP);

    server.begin();
    Serial.println("Web server started.");
}

// Handle incoming HTTP requests
void handleWiFiServer() {
    WiFiClient client = server.available();
    if (!client) return;

    Serial.println("New Client.");
    String currentLine = "";

    while (client.connected()) {
        if (!client.available()) continue;

        char c = client.read();
        Serial.write(c);

        if (c == '\n') {

            // ---- FILE DOWNLOAD ----
        if (currentLine.startsWith("GET /data.csv")) {
            File file = LittleFS.open("/data.csv", "r");
            if (file) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/csv");
                client.println("Content-Disposition: attachment; filename=\"data.csv\"");
                client.println();
                client.flush();

                uint8_t buffer[1024];
                while (file.available()) {
                    size_t len = file.read(buffer, sizeof(buffer));
                    client.write(buffer, len);
                    yield();
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


            // ---- MAIN HTML PAGE ----
            if (currentLine.length() == 0) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println();

                client.println("<html><head>");
                client.println("<style>");
                client.println("html { font-family: 'Times New Roman'; display: inline-block; margin: 0px auto; text-align: center; }");
                client.println(".button { background-color: #FF0044; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 20px; }");
                client.println(".button2 { background-color: #4CAF50; }");
                client.println("</style>");
                client.println("</head><body>");

                client.println("<h1>ESP32 - Double Trouble Server</h1>");

                client.println("<p><a href=\"/H\" class=\"button\">Turn ON LED</a></p>");
                client.println("<p><a href=\"/L\" class=\"button button2\">Turn OFF LED</a></p>");

                client.println("<p><a href=\"/data.csv\" download>Download File</a></p>");

                client.println("</body></html>");
                client.println();
                break;
            }


            currentLine = "";
        }

        else if (c != '\r') {
            currentLine += c;
        }

        // ---- LED COMMANDS ----
        if (currentLine.endsWith("GET /H")) {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("LED ON");
        }
        if (currentLine.endsWith("GET /L")) {
            digitalWrite(LED_BUILTIN, HIGH);
            Serial.println("LED OFF");
        }
    }

    client.stop();
    Serial.println("Client Disconnected.");
}
