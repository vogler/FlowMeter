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
// #include <Fonts/FreeSans12pt7b.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 OLED(0); // default I2C: D1=SCK, D2=SDA

// WiFi
#include <ESP8266WiFi.h>
WiFiClient wifi;
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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_connect(){
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.printf("connected as %s to mqtt://%s\n", clientId.c_str(), MQTT_SERVER);
      mqtt.publish(MQTT_TOPIC "/status", json("time: %ul, event: 'connect', clientId: '%s'", millis(), clientId.c_str()));
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
  setup_wifi();
  setup_mqtt();
  Serial.println("Ready to measure!");

  OLED.begin();
  // OLED.setFont(&FreeSans12pt7b);
  OLED.clearDisplay();
  OLED.setTextColor(WHITE);

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

void loop() {
  curTime = millis();
  if (curTime - oldTime > 1000) { // only process counters once per second
    mqtt.loop();
    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output. We also apply the pulseFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
    flow_l_min = ((1000.0 / (curTime - oldTime)) * pulseCount) / pulseFactor;
    flow_ml_s = (flow_l_min / 60) * 1000;
    total_ml += flow_ml_s;
    oldTime = curTime;

    if (flow_ml_s > 0) {
      flowTime = curTime;
      if (!mqtt.connected()) mqtt_connect();
      if (total_ml == 0) {
        flowStartTime = curTime;
        mqtt.publish(MQTT_TOPIC "/start", json("time: %lu", flowStartTime));
      }
      OLED.clearDisplay();
      OLED.setCursor(0, 0);

      Serial.printf("Pulses: %d", pulseCount); // since last interrupt
      Serial.printf("  Flow rate: %.2f l/min %d ml/sec", flow_l_min, flow_ml_s);
      Serial.printf("  Total: %d ml", total_ml);
      OLED.print(flow_l_min, 2);
      OLED.println(" l/min");
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
      OLED.print(T);
      OLED.println(" C");

      int s = round((flowTime - flowStartTime)/1000.0);
      Serial.printf("  Time: %02d:%02d", s/60, s%60);
      OLED.printf("%02d m %02d s", s/60, s%60);

      mqtt.publish(MQTT_TOPIC "/flow", json("time: %lu, flow: %u, temp: %f", curTime, flow_ml_s, T));

      Serial.println();
      OLED.display();
    } else if (curTime - flowTime > 30000) {
      mqtt.publish(MQTT_TOPIC "/stop", json("time: %lu, startTime: %ul, duration: %ul, total_ml: %ul", curTime, flowStartTime, curTime - flowStartTime, total_ml));
      total_ml = 0;
      OLED.clearDisplay();
      OLED.display();
    }

    pulseCount = 0;
  }
}