// flow rate code based on https://maker.pro/arduino/tutorial/how-to-interface-arduino-with-flow-rate-sensor-to-measure-liquid

// First used a Wemos D1 mini (ESP8266) which broke. Then switched to a Doit ESP32 DevKit. See log.md.
#define ESP32 // Comment out this line to change to ESP8266 setup.

#ifdef ESP32 // Doit ESP32 DevKit V1
  #define hallPin 15 // D15
  #define tempPin 36  // VP
  #define analogMax 4095 // at 3.2 V
  #define tempFactor 1.25 // temperatures were too low (cold -3, middle -7, hot -10), calibrated with IR gun
  #define tempOffset 1.0
  // OLED: SCK/SCL=D22, SDA=D21
#else // Wemos D1 mini
  // If it fails to flash/boot, it's likely that pin D8 is high (boot from SD-card) and has to be connected to GND. https://github.com/esp8266/esp8266-wiki/wiki/Boot-Process#esp-boot-modes
  #define hallPin D5
  #define tempPin A0
  #define analogMax 1023 // at 3.2 V
  #define tempFactor 1.0
  #define tempOffset 0.0
  // OLED: SCK/SCL=D1, SDA=D2
#endif

// flow
// hallPin: hall-effect flow sensor signal pin used for interrupt
const float pulseFactor = 11; // pulses/second per litre/minute

// temperature
// tempPin: NTC thermistor connected to analog input pin
const float R1 = 47200; // 47 kOhm resistor for voltage divider (measured 39.18 kOhm between A0 and GND)
float logR2, R2, T;
// https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
// -> using simpler β model over Steinhart-Hart model
// float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
const float B = 3950;

// 128x32 OLED display SSD1306
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_SSD1306.h>
#ifdef ESP32
  #include <Wire.h> // only needed for ESP32; Wemos D1 mini initialized with Adafruit_SSD1306 OLED(0); which gives an IntegerDivideByZero exception on the ESP32
  Adafruit_SSD1306 OLED(128, 32, &Wire, -1); // default I2C pins for DOIT ESP32: SCK=D22, SDA=D21
#else
  Adafruit_SSD1306 OLED(0); // default I2C pins for Wemos D1 mini: SCK=D1, SDA=D2
#endif

// WiFi
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
WiFiClient wifi;
// OTA update
#ifdef ESP32
  #include <ESPmDNS.h>
#else
  #include <ESP8266mDNS.h>
#endif
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
  const unsigned long start_time = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; WiFi.waitForConnectResult() != WL_CONNECTED && i < 10; i++) {
    #ifdef ESP32
      WiFi.begin(WIFI_SSID, WIFI_PASS); // for ESP32 also had to be moved inside the loop, otherwise only worked on every second boot, https://github.com/espressif/arduino-esp32/issues/2501#issuecomment-548484502
    #endif
    delay(1000);
    Serial.print(".");
  }
  const float connect_time = (millis() - start_time) / 1000.;
  Serial.println();
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("Failed to connect to Wifi in %.3f seconds. Going to restart!", connect_time);
    ESP.restart();
  }
  Serial.printf("Connected in %.3f seconds. IP address: ", connect_time);
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
      mqtt.publish(MQTT_TOPIC "/status", json("\"millis\": %lu, \"event\": \"connect\", \"clientId\": \"%s\"", millis(), clientId.c_str())); // TODO millis() is really just the ms, not full unix timestamp!
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

volatile byte pulseCount;
// had to move this up (before attachInterrupt) because of the needed attribute after esp8266 upgrade from 2.5.0 to 2.6.3 in Arduino
// without ICACHE_RAM_ATTR it crashes with 'ISR not in IRAM!', see https://github.com/esp8266/Arduino/issues/6127
void ICACHE_RAM_ATTR pulseCounter() { // ISR
  pulseCount++;
}

void setup() {
  Serial.begin(38400);
  Serial.println("Start FlowMeter");
  OLED.begin();
  OLED.setFont(&FreeSans9pt7b);
  OLED.setTextColor(WHITE);
  OLED.clearDisplay(); OLED.display(); // clear to avoid noise
  setup_wifi();
  setup_OTA();
  setup_mqtt();
  Serial.println("Ready to measure!");

  pinMode(hallPin, INPUT_PULLUP); // there is no internal pull-down
  attachInterrupt(digitalPinToInterrupt(hallPin), pulseCounter, FALLING); // FALLING = transition from HIGH to LOW
}

float flow_l_min;
unsigned short flow_ml_s;
unsigned short flows; // intervals with flow; interrupt is triggered up to 3s by turning the light switch on/off (even if light is turned off, only if powered via socket, not via laptop, likely some ripple through power supply), so we filter those out by delaying mqtt messages by one interval. TODO check with oscilloscope - maybe a capacitor would help?
unsigned long total_ml;
unsigned long curTime, oldTime, flowTime, flowStartTime;
unsigned short interval;

void loop() {
  ArduinoOTA.handle();
  mqtt.loop();
  curTime = millis(); // will overflow after 49.71 days runtime
  interval = curTime - oldTime; // will be correct even on overflow since all are unsigned: 10 - (ULONG_MAX - 5) is 16
  if (interval > 1000) { // only process counters once per second; usually interval will be 1001
    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output (if the interval took longer than 1s, flow_l_min would be too high, so we have to devide by interval).
    // We also apply the pulseFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
    int pulses = pulseCount; // store in local, so that we can immediately set it to zero and don't lose updates from interrupts during the calculations below
    pulseCount = 0; // if we only set this at the end of the block, we'd overwrite up to 13 pulses (at full flow) that happened inbetween
    flow_l_min = (1000.0 / interval) * pulses / pulseFactor;
    flow_ml_s = (flow_l_min / 60) * 1000;
    oldTime = curTime;

    if (flow_ml_s > 0) {
      flowTime = curTime;
      flows++;
      if (!mqtt.connected()) mqtt_connect();
      if (total_ml == 0) {
        flowStartTime = curTime - interval; // flow started before this interval
      }
      if (flows == 2) { // if flow is triggered by light, we ignore flow 1 and it will be reset by the timeout
        mqtt.publish(MQTT_TOPIC "/start", json("\"millis\": %lu", flowStartTime));
      }
      total_ml += flow_ml_s;
      OLED.clearDisplay();
      // frame for layouting during development
      // OLED.drawLine(0, 0, 0, 64, WHITE);
      // OLED.drawLine(127, 0, 127, 64, WHITE);
      OLED.setCursor(0, 12);

      Serial.printf("Pulses: %d", pulses); // since last interrupt
      Serial.printf("  Flow rate: %.2f l/min %d ml/sec", flow_l_min, flow_ml_s);
      Serial.printf("  Total: %d ml", total_ml);
      OLED.print(flow_l_min, 2);
      OLED.println(" l/m");
      OLED.setCursor(0, 30);
      OLED.print(total_ml / 1000.0, 2);
      OLED.println(" l");

      int vo = analogRead(tempPin);
      Serial.printf("  A0: %d", vo);
      R2 = R1 * (analogMax / (float)vo - 1.0);
      // logR2 = log(R2);
      Serial.printf("  R2: %.2f", R2);
      // Serial.printf("  logR2: %.2f", logR2);
      // T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15;
      // T2= T1*B/ln(R1/R2)  /  ( B/ln(R1/R2) - T1)
      float T1 = 25 + 273.15;
      float ln = log(50000 / R2);
      T = T1 * B / ln / (B / ln - T1) - 273.15;
      T = (T + tempOffset) * tempFactor; // device-specific fix
      Serial.printf("  Temperature: %f C", T);
      OLED.setCursor(80, 12);
      OLED.print(T);
      // OLED.println(" C");

      int s = round((flowTime - flowStartTime)/1000.0);
      Serial.printf("  Time: %02d:%02d", s/60, s%60);
      OLED.setCursor(80, 30);
      OLED.printf("%02d:%02d", s/60, s%60);

      if (flows >= 2)
        mqtt.publish(MQTT_TOPIC "/flow", json("\"millis\": %lu, \"flow\": %u, \"temp\": %f", curTime, flow_ml_s, T));

      Serial.println();
      OLED.display();
    } else if (curTime - flowTime > 30000 && total_ml > 0) {
      if (flows >= 2)
        mqtt.publish(MQTT_TOPIC "/stop", json("\"millis\": %lu, \"startMillis\": %lu, \"duration\": %lu, \"total_ml\": %lu", curTime, flowStartTime, flowTime - flowStartTime, total_ml));
      flows = 0;
      total_ml = 0;
      OLED.clearDisplay(); OLED.display();
    }
    // Serial.printf("Pulses during prev. loop: %d  ", pulseCount);
  }
}
