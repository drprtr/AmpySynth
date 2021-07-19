#include "Arduino.h"

#include "FS.h"
#include "SD.h"
#include "SPIFFS.h"
#include "HTTPClient.h"
#include "SPI.h"
#include "Wire.h"

#include "I2Cdev.h"
#include "TCA6424A.h"

#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorRTTTL.h"

#include "FastLED.h" 

#include <ESP32Encoder.h>

#include <IotWebConf.h>
#include <IotWebConfESP32HTTPUpdateServer.h>

#define CONFIG_VERSION "2"

#define SDA 17
#define SCL 5

#define POT_1 36
#define POT_2 39
#define POT_3 35

#define ENC_A 21
#define ENC_B 19

#define WEBCONF_RESET_PIN 32


#define LED_DATA_PIN 2
#define COLOR_ORDER GRB
#define LED_TYPE WS2812
#define NUM_LEDS 16


const char tune[] PROGMEM = "JurassicPark:d=32,o=6,b=28:p,b5,a#5,8b5,16p,b5,a#5,8b5,16p,b5,a#5,16b.5,c#,16c#.,e,8e,16p,d#,b5,16c#.,a#5,16f#5,d#,b5,8c#,16p,f#,b5,16e.,d#,16d#.,c#,8c#.,1p";

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
IotWebConf iotWebConf("AmpySynth", &dnsServer, &server, "amplience", CONFIG_VERSION);

AudioOutputI2S *out;
AudioGeneratorRTTTL *rtttl;
AudioFileSourcePROGMEM *file;

TCA6424A tca;

struct CRGB leds[NUM_LEDS];

ESP32Encoder enc;

void handleRoot();

int r = 0;
int g = 0;
int b = 0;

int encPos = 0;
float flash = 25;
float beatInterval = 950;

void setup() {
  Serial.begin(115200);
  delay(1000);

  ESP32Encoder::useInternalWeakPullResistors=UP;
  enc.attachSingleEdge(ENC_A, ENC_B);

  iotWebConf.setupUpdateServer(
    [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
    [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });
  iotWebConf.init();
  iotWebConf.setConfigPin(WEBCONF_RESET_PIN);
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  LEDS.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 250);


  Serial.println("Initializing I2C devices...");
  Wire.begin(SDA, SCL);
  tca.initialize();
  Serial.println("Testing device connections...");
  if (!tca.testConnection()) {
    Serial.println("TCA6424A Connection Failed");
  }
  tca.setBankDirection(0, TCA6424A_INPUT);
  tca.setBankDirection(1, TCA6424A_INPUT);
  tca.setBankDirection(2, TCA6424A_INPUT);
  Serial.println("Initialised TCA6424A as input");

  out = new AudioOutputI2S();
  out -> SetGain(0.05);
  out -> SetPinout(26,25,22);

  file = new AudioFileSourcePROGMEM( tune, strlen_P(tune) );
  rtttl = new AudioGeneratorRTTTL();
  rtttl->begin(file, out);
}

void loop() {
  iotWebConf.doLoop();

  // if (rtttl->isRunning()) {
  //   if (!rtttl->loop()) {
  //     rtttl->stop();
  //   }
  // }

  //bool val = tca.readPin(TCA6424A_P00);

  //uint8_t thisHue = beat8(20,255);
  //fill_rainbow(leds, NUM_LEDS, thisHue, 16);


  

  //Encoder direction
  int newEncPos = enc.getCount();
  if(newEncPos > encPos) {
    encPos--;
  } else if (newEncPos < encPos) {
    encPos++;
  }
  encPos = encPos % NUM_LEDS;
  if(encPos < 0) {
    encPos += NUM_LEDS;
  }
  enc.setCount(encPos);

  float bpm = 60;
  switch (encPos) {
    case 0: bpm = 60; break;
    case 1: bpm = 65; break;
    case 2: bpm = 70; break;
    case 3: bpm = 75; break;
    case 4: bpm = 80; break;
    case 5: bpm = 85; break;
    case 6: bpm = 90; break;
    case 7: bpm = 100; break;
    case 8: bpm = 110; break;
    case 9: bpm = 120; break;
    case 10: bpm = 130; break;
    case 11: bpm = 140; break;
    case 12: bpm = 160; break;
    case 13: bpm = 180; break;
    case 14: bpm = 200; break;
    case 15: bpm = 240; break;
  }

  float beatRate = 60 / bpm;
  float beatTime = beatRate * 1000;
  beatInterval = beatTime - flash;

  FastLED.clear();
  leds[encPos].r = map(analogRead(POT_1), 0, 4096, 255, 0);
  leds[encPos].g = map(analogRead(POT_2), 0, 4096, 255, 0);
  leds[encPos].b = map(analogRead(POT_3), 0, 4096, 255, 0);
  FastLED.show();
  delay(flash);
  leds[encPos].r = 0;
  leds[encPos].g = 0;
  leds[encPos].b = 0;
  FastLED.show();
  delay(beatInterval);



}

void handleRoot()
{
  if (iotWebConf.handleCaptivePortal()) {
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>AmpySynth!</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}