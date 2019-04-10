// flow rate code based on https://maker.pro/arduino/tutorial/how-to-interface-arduino-with-flow-rate-sensor-to-measure-liquid

// flow
byte hallPin = D5;      // hall-effect flow sensor
float pulseFactor = 10; // pulses/second per litre/minute

// temperature
byte tempPin = A0; // thermistor
float R1 = 46400;
float logR2, R2, T;
// float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float B = 3950;

// 128x32 OLED display SSD1306
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 OLED(0); // default I2C: D1=SCK, D2=SDA

// WiFi
#include <ESP8266WiFi.h>
WiFiClient wifi;
// OTA update
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// MQTT
#define MQTT_TOPIC "sensors/shower"
#include <PubSubClient.h>
PubSubClient mqtt(wifi);
// json
char buf[200];
#define json(s, ...) (sprintf(buf, "{ " s " }", __VA_ARGS__), buf)

#include <MyConfig.h> // credentials, servers, ports

void setup_wifi() {
  delay(5);
  Serial.printf("Connecting to AP %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
    ESP.restart();
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

#define OLED_stat(...) OLED.clearDisplay(); OLED.setCursor(0, 0); OLED.setTextSize(2); OLED.printf(__VA_ARGS__); OLED.display(); OLED.setTextSize(1)

void setup_OTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("OTA: Start updating " + type);
    OLED_stat("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: Done");
    OLED.clearDisplay(); OLED.display();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: Progress: %u%%\r", (progress / (total / 100)));
    OLED_stat("OTA: %u%%", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    OLED_stat("OTA fail: %u\n", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void mqtt_connect(){
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.printf("connected as %s to mqtt://%s\n", clientId.c_str(), MQTT_SERVER);
      mqtt.publish(MQTT_TOPIC "/status", json("time: %lu, event: 'connect', clientId: '%s'", millis(), clientId.c_str())); // TODO millis() is really just the ms, not full unix timestamp!
      // mqtt.subscribe("");
    } else {
      Serial.printf("failed, rc=%d. retry in 5s.\n", mqtt.state());
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("MQTT message on topic %s\n", topic);
}

void setup_mqtt() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);
  randomSeed(micros());
  mqtt_connect();
}

void setup() {
  Serial.begin(38400);
  Serial.println("Start FlowMeter");
  OLED.begin();
  OLED.setFont(&FreeSans9pt7b);
  OLED.setTextColor(WHITE);
  setup_wifi();
  setup_OTA();
  setup_mqtt();
  Serial.println("Ready to measure!");

  pinMode(hallPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPin), pulseCounter, FALLING); // FALLING = transition from HIGH to LOW
}

volatile byte pulseCount;
void pulseCounter() { // ISR
  pulseCount++;
}

float flow_l_min;
unsigned short flow_ml_s;
unsigned long total_ml;
unsigned long curTime, oldTime, flowTime, flowStartTime;
unsigned short interval;

void loop() {
  ArduinoOTA.handle();
  mqtt.loop();
  curTime = millis();
  interval = curTime - oldTime;
  if (interval > 1000) { // only process counters once per second
    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output. We also apply the pulseFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
    flow_l_min = (1000.0 / interval) * pulseCount / pulseFactor;
    flow_ml_s = (flow_l_min / 60) * 1000;
    oldTime = curTime;

    if (flow_ml_s > 0) {
      flowTime = curTime;
      if (!mqtt.connected()) mqtt_connect();
      if (total_ml == 0) {
        flowStartTime = curTime - interval; // flow started before this interval
        mqtt.publish(MQTT_TOPIC "/start", json("time: %lu", flowStartTime));
      }
      total_ml += flow_ml_s;
      OLED.clearDisplay();
      // frame for layouting during development
      OLED.drawLine(0, 0, 0, 64, WHITE);
      OLED.drawLine(127, 0, 127, 64, WHITE);
      OLED.setCursor(0, 12);

      Serial.printf("Pulses: %d", pulseCount); // since last interrupt
      Serial.printf("  Flow rate: %.2f l/min %d ml/sec", flow_l_min, flow_ml_s);
      Serial.printf("  Total: %d ml", total_ml);
      OLED.print(flow_l_min, 2);
      OLED.println(" l/m");
      OLED.setCursor(0, 30);
      OLED.print(total_ml / 1000.0, 2);
      OLED.println(" l");

      int vo = analogRead(A0);
      //Serial.print("  A0: "); Serial.print(vo);
      R2 = R1 * (1023.0 / (float)vo - 1.0);
      //logR2 = log(R2);
      Serial.print("  R2: ");
      Serial.print(R2);
      //Serial.print("  logR2: "); Serial.print(logR2);
      // T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15;
      // T2= T1*B/ln(R1/R2)  /  ( B/ln(R1/R2) - T1)
      float T1 = 25 + 273.15;
      float ln = log(50000 / R2);
      T = T1 * B / ln / (B / ln - T1) - 273.15;
      Serial.printf("  Temperature: %f C", T);
      OLED.setCursor(80, 12);
      OLED.print(T);
      // OLED.println(" C");

      int s = round((flowTime - flowStartTime)/1000.0);
      Serial.printf("  Time: %02d:%02d", s/60, s%60);
      OLED.setCursor(80, 30);
      OLED.printf("%02d:%02d", s/60, s%60);

      mqtt.publish(MQTT_TOPIC "/flow", json("time: %lu, flow: %u, temp: %f", curTime, flow_ml_s, T));

      Serial.println();
      OLED.display();
    } else if (curTime - flowTime > 30000 && total_ml > 0) {
      mqtt.publish(MQTT_TOPIC "/stop", json("time: %lu, startTime: %lu, duration: %lu, total_ml: %lu", curTime, flowStartTime, curTime - flowStartTime, total_ml));
      total_ml = 0;
      OLED.clearDisplay();
      OLED.display();
    }

    pulseCount = 0;
  }
}