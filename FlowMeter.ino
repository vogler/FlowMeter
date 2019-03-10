// https://maker.pro/arduino/tutorial/how-to-interface-arduino-with-flow-rate-sensor-to-measure-liquid

byte ledPin = BUILTIN_LED;
byte hallPin = D5;
byte tempPin = A0;
byte interrupt;

// The hall-effect flow sensor outputs approximately 10 pulses per second per litre/minute of flow.
float calibrationFactor = 10;
// temperature
float R1 = 46400;
float logR2, R2, T;
// float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float B = 3950;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

void setup()
{
  Serial.begin(38400);
  Serial.println("Start");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(hallPin, INPUT);
  digitalWrite(hallPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  interrupt = digitalPinToInterrupt(hallPin);
  Serial.print("Interrupt on pin ");
  Serial.println(interrupt);

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH state to LOW state)
  attachInterrupt(interrupt, pulseCounter, FALLING);
}


void loop()
{
  if ((millis() - oldTime) > 1000) {   // Only process counters once per second
    // Disable the interrupt while calculating flow rate and sending the value to the host
    detachInterrupt(interrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    Serial.print("Pulses: "); // since last interrupt
    Serial.print(int(pulseCount));
    // Print the flow rate for this second in litres / minute
    Serial.print("  Flow rate: ");
    Serial.print(flowRate, 2);
    Serial.print(" L/min = ");
    Serial.print(flowMilliLitres);
    Serial.print(" mL/Sec");

    Serial.print("  Total: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL");

    int vo = analogRead(A0);
    //Serial.print("  A0: "); Serial.print(vo);
    R2 = R1 * (1023.0 / (float)vo - 1.0);
    //logR2 = log(R2);
    Serial.print("  R2: "); Serial.print(R2);
    //Serial.print("  logR2: "); Serial.print(logR2);
    // T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15;
    // T2= T1*B/ln(R1/R2)  /  ( B/ln(R1/R2) - T1)
    float T1 = 25 + 273.15;
    float ln = log(50000/R2);
    T = T1 * B / ln / (B/ln - T1) - 273.15;
    Serial.print("  Temperature: ");
    Serial.print(T);
    Serial.println(" C");

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(interrupt, pulseCounter, FALLING);
  }
}

/*
  Insterrupt Service Routine
*/
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
