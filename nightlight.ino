/*
Copyright 2018 Adam Demuri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include <FastLED.h>

const static int kMotionSensorPin = 0;
const static int kLightSensorPin = A9; // aka pin 23
static const int kLedPin = 17;
static const int kTouchPin = 1;

// Greater than this is active (i.e. dark enough)
const static int kLightSensorThreshold = 850;

static const int kTouchThreshold = 2000;

const static int kNumLeds = 10;
CRGB leds[kNumLeds];

// How long to wait after starting up before checking the PIR sensor
const static long kWarmUpDelay = 60 * 1000;

const static long kOnDelay = 5 * 60 * 1000;
//const static long kOnDelay = 5 * 1000;

// How long to stay off (and ignore the motion sensor) after manually turned off
const static long kTouchCooloff = 30 * 1000;

void setup() {
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  pinMode(kMotionSensorPin, INPUT_PULLUP);
  pinMode(kLightSensorPin, INPUT_PULLUP);
  pinMode(kTouchPin, INPUT);
  pinMode(13, OUTPUT);
  pinMode(kLedPin, OUTPUT);

   FastLED.addLeds<NEOPIXEL, kLedPin>(leds, 10);

  setBrightness(20);
  for (int i = 0; i < 20; i++) {
    doLeds();
    delay(10);
  }
  setBrightness(0);
  for (int i = 0; i < 20; i++) {
    doLeds();
    delay(10);
  }

  // TODO: delay for ~1m to allow the PIR to warm up
  
}

static const int kLightSamples = 5;
static const int kLightSampleEvery = 500;
long lightSampleAt = 0;
int lightSampleIndex = 0;
// Danger
int lightSamples[kLightSamples] = {0, 0, 0, 0, 0};

long readLightSensor() {
  if (millis() > lightSampleAt) {
    lightSamples[lightSampleIndex] = analogRead(kLightSensorPin);
    lightSampleIndex = (lightSampleIndex + 1) % kLightSamples;
    lightSampleAt = millis() + kLightSampleEvery;
  }

  long sum = 0;
  for (int i = 0; i < kLightSamples; i++) {
    sum += lightSamples[i];
  }
  return sum / kLightSamples;
}

boolean touchPressed() {
  static boolean prevState = false;
  if (touchRead(kTouchPin) > kTouchThreshold) {
    if (prevState == false) {
      prevState = true;
      return true;
    }
  } else {
    prevState = false;
  }
  return false;
}

int brightness = 0;
int targetBrightness = 0;
void setBrightness(int b) {
  targetBrightness = b;
  doLeds();
}

static const long kLedStepMillis = 10;
long ledUpdateAt = 0;
void doLeds() {
  if (millis() > ledUpdateAt) {
    if (targetBrightness > brightness) {
      brightness++;
    } else if (targetBrightness < brightness) {
      brightness--;
    }
    FastLED.showColor(CRGB(brightness, 0, 0));
    ledUpdateAt = millis() + kLedStepMillis;
  }
}

long cooloffEndedAt = 0;
void loop() {
  long brightness = readLightSensor();
  boolean touchButtonPressed = touchPressed();
  boolean motionDetected = millis() > kWarmUpDelay ? digitalRead(kMotionSensorPin) : false;
  
  if (brightness > kLightSensorThreshold || touchButtonPressed) {
    if ((motionDetected && millis() > cooloffEndedAt) || touchButtonPressed) {
      setBrightness(64);
      long offAt = millis() + kOnDelay;
      while (millis() < offAt) {
        doLeds();
        if (touchPressed()) {
          cooloffEndedAt = millis() + kTouchCooloff;
          break;        
        }
        delay(10);
      }
      setBrightness(0);
    }
  } else {
    setBrightness(0);
  }
  doLeds();
  delay(10);
}
