#pragma once
#include <WiFi.h>
#include <WiFiAP.h>
#include <LittleFS.h>
#include "FS.h"

const char *ssid     = "XIAO_ESP32S3";
const char *password = "password";

WiFiServer server(80);

void startWiFi() {
    Serial.println("\nStarting WiFi Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(apIP);
    server.begin();
    Serial.println("Web server started.");
}

// Helper: streams a LittleFS file to the client as a CSV download
static void serveCSV(WiFiClient &client, const char *path, const char *filename) {
    File file = LittleFS.open(path, "r");
    if (file) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/csv");
        client.print("Content-Disposition: attachment; filename=\"");
        client.print(filename);
        client.println("\"");
        client.println("Connection: close");
        client.println();
        client.flush();

        uint8_t buf[1024];
        while (file.available()) {
            size_t len = file.read(buf, sizeof(buf));
            client.write(buf, len);
            yield();
        }
        file.close();
    } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println();
        client.println("File not found.");
    }
}

void handleWiFiServer() {
    WiFiClient client = server.available();
    if (!client) return;

    Serial.println("New Client.");
    String currentLine = "";

    while (client.connected()) {
        if (!client.available()) continue;

        char c = client.read();

        if (c == '\n') {

            // Add this inside handleWiFiServer() before the main HTML page block:
            if (currentLine.startsWith("GET /wipe")) {
                LittleFS.remove(IMU_FILE);
                LittleFS.remove(BARO_FILE);
                LittleFS.remove(EVENT_FILE);

                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                client.println("<html><body style='font-family:Arial;text-align:center;background:#1a1a2e;color:#eee'>");
                client.println("<h2 style='color:#e94560'>Files wiped.</h2>");
                client.println("<p>Reboot the flight computer to start fresh.</p>");
                client.println("<a href='/' style='color:#e94560'>Back</a>");
                client.println("</body></html>");
                break;
            }

            // ── File downloads ────────────────────────────────────────────────
            if (currentLine.startsWith("GET /imu.csv")) {
                serveCSV(client, "/IMU.csv", "imu.csv");
                break;
            }

            if (currentLine.startsWith("GET /baro.csv")) {
                serveCSV(client, "/BARO.csv", "baro.csv");
                break;
            }

            if (currentLine.startsWith("GET /events.csv")) {
                serveCSV(client, "/EVENT.csv", "events.csv");
                break;
            }

            // ── Main HTML page ────────────────────────────────────────────────
            if (currentLine.length() == 0) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();

                client.println("<!DOCTYPE html><html><head>");
                client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
                client.println("<style>");
                client.println("body { font-family: Arial, sans-serif; text-align: center; background: #1a1a2e; color: #eee; padding: 20px; }");
                client.println("h1 { color: #e94560; }");
                client.println("h2 { color: #aaa; font-size: 1rem; font-weight: normal; margin-bottom: 40px; }");
                client.println(".card { background: #16213e; border-radius: 12px; padding: 24px; margin: 16px auto; max-width: 400px; }");
                client.println(".card h3 { margin: 0 0 8px 0; color: #e94560; }");
                client.println("<div class='card'>");
                client.println("<h3 style='color:#e94560'>Danger Zone</h3>");
                client.println("<p>Wipe all CSV files before next flight</p>");
                client.println("<a class='btn' href='/wipe' "
                            "onclick=\"return confirm('Wipe all data?')\">Wipe Files</a>");
                client.println("</div>");
                client.println(".card p { margin: 0 0 16px 0; font-size: 0.85rem; color: #aaa; }");
                client.println("a.btn { display: inline-block; background: #e94560; color: white; padding: 12px 32px;");
                client.println("        border-radius: 8px; text-decoration: none; font-size: 1rem; }");
                client.println("a.btn:hover { background: #c73652; }");
                client.println(".led { margin-top: 40px; }");
                client.println("a.led-btn { display: inline-block; padding: 10px 28px; border-radius: 8px;");
                client.println("            text-decoration: none; font-size: 0.9rem; margin: 6px; color: white; }");
                client.println(".on  { background: #4CAF50; }");
                client.println(".off { background: #555; }");
                client.println("</style></head><body>");

                client.println("<h1>&#x1F680; Double Trouble Flight Computer</h1>");
                client.println("<h2>Connect to 192.168.4.1 to download flight data</h2>");

                // IMU card
                client.println("<div class='card'>");
                client.println("<h3>IMU Data</h3>");
                client.println("<p>Accelerometer (ax, ay, az) &amp; gyroscope (gx, gy, gz),<br>angular position (X, Y, Z) and tilt &mdash; 500 Hz</p>");
                client.println("<a class='btn' href='/imu.csv' download>Download IMU CSV</a>");
                client.println("</div>");

                // Barometer card
                client.println("<div class='card'>");
                client.println("<h3>Barometer Data</h3>");
                client.println("<p>Pressure (mBar), temperature (&deg;C), altitude (ft) &mdash; 100 Hz</p>");
                client.println("<a class='btn' href='/baro.csv' download>Download Barometer CSV</a>");
                client.println("</div>");

                // Events card
                client.println("<div class='card'>");
                client.println("<h3>Flight Events</h3>");
                client.println("<p>Timestamped log of launch, burnout, staging, and apogee events</p>");
                client.println("<a class='btn' href='/events.csv' download>Download Events CSV</a>");
                client.println("</div>");

                // LED controls
                client.println("<div class='led'>");
                client.println("<a class='led-btn on' href='/H'>LED ON</a>");
                client.println("<a class='led-btn off' href='/L'>LED OFF</a>");
                client.println("</div>");

                client.println("</body></html>");
                client.println();
                break;
            }

            currentLine = "";

        } else if (c != '\r') {
            currentLine += c;
        }

        // ── LED commands ──────────────────────────────────────────────────────
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