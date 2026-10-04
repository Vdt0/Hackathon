#define WOKWI
#include <Wire.h>
#ifndef WOKWI
#include <BH1750.h>
Bh1750 lightmeter;
#endif

const int sensorPins[4] = {32, 33, 25, 26};

const int lampPins[4] = {16, 17, 18, 19};

const int LUX_POT_PIN = 34;

const int pwmChannels[4] = {0, 1, 2, 3};
const int pwmFreq = 5000;
const int pwmResolution = 8;

const int OFF_LEVEL = 0;
const int DIM_LEVEL = 80;
const int NEIGHBOR_LEVEL = 150;
const int FULL_LEVEL = 255;

unsigned long lastMotionTime = 0;
const unsigned long timeoutMs = 5000;

const float dayThreshold = 80.0;

bool motionDetected = false;
unsigned long lastPrint = 0;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define PWM_CORE3 1
#endif

void setLamp(int lamp, int value)
{
#ifdef PWM_CORE3
  ledcWrite(lampPins[lamp], value);
#else
  ledcWrite(pwmChannels[lamp], value);
#endif
}

void setAllLamps(int value)
{
  for(int i = 0; i < 4; i++)
  {
    setLamp(i, value);
  }
}

float readLux()
{
#ifdef WOKWI
  return analogRead(LUX_POT_PIN) * 200.0 / 4095.0;
#else
  return lightMeter.readLightLevel();
#endif
}

void activateZone(int sensorIndex)
{
  setAllLamps(DIM_LEVEL);

  setLamp(sensorIndex, FULL_LEVEL);

  if(sensorIndex > 0)
    setLamp(sensorIndex - 1, NEIGHBOR_LEVEL);

  if(sensorIndex < 3)
    setLamp(sensorIndex + 1, NEIGHBOR_LEVEL);

  lastMotionTime = millis();
  motionDetected = true;
}

void setup()
{
  Serial.begin(115200);

#ifndef WOKWI
  Wire.begin(21, 22);
  lightMeter.begin();
#endif

  for(int i = 0; i < 4; i++)
  {
    pinMode(sensorPins[i], INPUT_PULLUP);

#ifdef PWM_CORE3
    ledcAttach(lampPins[i], pwmFreq, pwmResolution);
#else
    ledcSetup(
      pwmChannels[i],
      pwmFreq,
      pwmResolution
    );

    ledcAttachPin(
      lampPins[i],
      pwmChannels[i]
    );
#endif
  }

  setAllLamps(DIM_LEVEL);

  Serial.println("System started");
}

void loop()
{
  float lux = readLux();

  if(millis() - lastPrint > 1000)
  {
    lastPrint = millis();
    Serial.print("Lux: ");
    Serial.println(lux);
  }

  if(lux > dayThreshold)
  {
    setAllLamps(OFF_LEVEL);
    delay(200);
    return;
  }

  for(int i = 0; i < 4; i++)
  {
    if(digitalRead(sensorPins[i]) == LOW)
    {
      activateZone(i);

      Serial.print("Motion sensor ");
      Serial.println(i + 1);

      delay(100);
    }
  }

  if(motionDetected)
  {
    if(millis() - lastMotionTime > timeoutMs)
    {
      setAllLamps(DIM_LEVEL);
      motionDetected = false;
    }
  }
  else
  {
    setAllLamps(DIM_LEVEL);
  }

  delay(50);
}