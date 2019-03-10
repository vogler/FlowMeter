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

void setup() {
  Serial.begin(38400);
  Serial.println("Start FlowMeter");

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
unsigned long curTime, oldTime;

void loop() {
  curTime = millis();
  if (curTime - oldTime > 1000) { // only process counters once per second
    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output. We also apply the pulseFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
    flow_l_min = ((1000.0 / (curTime - oldTime)) * pulseCount) / pulseFactor;
    flow_ml_s = (flow_l_min / 60) * 1000;
    total_ml += flow_ml_s;
    oldTime = curTime;

    OLED.clearDisplay();
    OLED.setCursor(0, 0);
    Serial.print("Pulses: "); // since last interrupt
    Serial.print(int(pulseCount));
    Serial.print("  Flow rate: ");
    Serial.print(flow_l_min, 2);
    OLED.print(flow_l_min, 2);
    OLED.println(" L/min");
    Serial.print(" L/min = ");
    Serial.print(flow_ml_s);
    Serial.print(" mL/Sec");
    Serial.print("  Total: ");
    Serial.print(total_ml);
    Serial.print("mL");
    OLED.print(total_ml / 1000.0, 2);
    OLED.println(" L");

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
    Serial.print("  Temperature: ");
    Serial.print(T);
    Serial.println(" C");
    OLED.print(T);
    OLED.println(" C");

    OLED.display();

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
  }
}