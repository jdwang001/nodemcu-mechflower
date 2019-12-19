// #define FASTLED_ALLOW_INTERRUPTS 0
// #define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
FASTLED_USING_NAMESPACE

#include <AccelStepper.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


// PWM LED config
uint8_t leafsleds[6] = { D5, D6, D7, D8, D9, D10 };

// WS2812 config
#define DATA_PIN      D4
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
// #define COLOR_ORDER   RGB
#define NUM_LEDS      7 // 花蕊led的个数
// #define NUM_LEDS      96

#define MILLI_AMPS         2000     // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  120 // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.
#define SPEED 5

// const uint8_t brightnessCount = 5;
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
uint8_t cooling = 49;

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
uint8_t sparking = 60;
CRGB leds[NUM_LEDS];
// CRGBPalette16 currentPalette;

unsigned int  ledLeftLoc = 0;
bool stopFlag = false;
// stepper config
AccelStepper stepper(4, D0, D2, D1, D3); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
// AccelStepper stepper(4, D5, D6, D7, D8); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
// AccelStepper stepper(4, D0, D1, D2, D3); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

AsyncWebServer server(80);

String flashMode = "nomode";
bool apMode = false;
// bool apMode = true;
const char* ssid = "mechflower";
const char* password = "mechflower";


const char* PARAM_MESSAGE = "step";
const char* MODE = "mode";
byte mac[6];

int defaultPostion = 0;
// int openPostion = 180;  //一圈大概2048
int openPostion = 100;  //一圈大概2048
unsigned long lasttime;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void printfMacAddress(){
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.println(mac[5],HEX);
}

void openFlower() {
  if (stepper.currentPosition() == 0) {
    // defaultPostion = stepper.currentPosition();
    stepper.moveTo(openPostion);
    Serial.printf("Open the door\n"); 
  } else {
    Serial.printf("Is Opening\n"); 
  }
}

void closeFlower() {
  if (stepper.currentPosition() >= openPostion ) {
    Serial.printf("current position is %d\n",stepper.currentPosition());
    stepper.moveTo(defaultPostion);
  }
}

void stopStepper() {
  // stepper.stop();
}
void heatMap(CRGBPalette16 palette, bool up)
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[256];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( uint16_t j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up) {
      leds[j] = color;
    }
    else {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}

void fire()
{
  heatMap(CloudColors_p, true);
}
uint8_t speed = 40;
const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};
uint8_t currentPaletteIndex = 5;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
CRGBPalette16 gCurrentPalette(CRGB::LightYellow);
void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8( speed, 64, 255);
  CRGBPalette16 palette = gCurrentPalette;
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}


void setxhl() {
  flashMode = "nomode";
  fill_solid(leds, NUM_LEDS, CRGB::Orange);
}
void setup()
{
  // set led
  pinMode(LED_BUILTIN, OUTPUT);
  // on led, nodemcu led
  digitalWrite(LED_BUILTIN, LOW); 

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS); // for APA102 (Dotstar)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();


  // Change these to suit your stepper if you want
  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(1000);
  Serial.begin(9600);
  Serial.setDebugOutput(true); // set debug output

  if (apMode) {
    // 设置内网
    IPAddress softLocal(192,168,6,1);   // 1 设置内网WIFI IP地址
    IPAddress softGateway(192,168,6,1);
    IPAddress softSubnet(255,255,255,0);
    WiFi.softAPConfig(softLocal, softGateway, softSubnet);
    
    String apName = ("ESP8266_"+(String)ESP.getChipId());  // 2 设置WIFI名称
    const char *softAPName = apName.c_str();
    
    WiFi.softAP(softAPName, password);      // 3创建wifi  名称 +密码  
    
    IPAddress myIP = WiFi.softAPIP();  // 4输出创建的WIFI IP地址
    Serial.print("AP IP address: ");     
    Serial.println(myIP);
    
    Serial.print("softAPName: ");  // 5输出WIFI 名称
    Serial.println(apName);
    Serial.print("over ap config\n");
    delay(3000);
  } else {
    // WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect (true);
    WiFi.setAutoReconnect (true);
  RECONNECT_WIFI:
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        delay(1000);
        goto RECONNECT_WIFI;
    }
    
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());
    printfMacAddress();
  }
  // disable serial, use uart as gpio
  Serial.end();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      setxhl();
      openFlower();
      Serial.printf("Blooming!\n");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "Blooming!");
      response->addHeader("charset","UTF-8");
      request->send(response);
  });

  server.on("/close", HTTP_GET, [] (AsyncWebServerRequest *request) {
      closeFlower();
      Serial.printf("Time backtracking!\n");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "时间回溯");
      response->addHeader("charset","UTF-8");
      request->send(response);
  });

  server.on("/stop", HTTP_GET, [] (AsyncWebServerRequest *request) {
      // stopStepper();
      stopFlag = true;
      Serial.printf("stop stepper!\n");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "紧急熄火");
      response->addHeader("charset","UTF-8");
      request->send(response);
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam(PARAM_MESSAGE)) {
          message = request->getParam(PARAM_MESSAGE)->value();
      } else {
          message = "No message sent";
      }
      Serial.printf("get request");
      stepper.moveTo(message.toInt());
      request->send(200, "text/plain", "Hello, GET: " + message);
  });
  server.on("/hm", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam(MODE)) {
          message = request->getParam(MODE)->value();
      } else {
          message = "No message sent";
      }
      Serial.printf("get request");
      flashMode = message;
      openFlower();
      request->send(200, "text/plain", "Hello, GET: " + message);
  });
  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
      String message;
      if (request->hasParam(PARAM_MESSAGE, true)) {
          message = request->getParam(PARAM_MESSAGE, true)->value();
      } else {
          message = "No message sent";
      }
      request->send(200, "text/plain", "Hello, POST: " + message);
  });

  server.onNotFound(notFound);
  server.begin();

  lasttime = millis();
  // set leafs leds
  for(int ledindex = 0; ledindex < 6; ledindex++) {
    pinMode(leafsleds[ledindex], OUTPUT);
  }
  analogWriteFreq(5);
  Serial.printf("begain program");
  fill_solid(leds, NUM_LEDS, CRGB::Orange);
  // fill_solid( currentPalette, 16, CRGB::Yellow);
}

void leafledset(uint8_t pin, int val){
  //esp12 led pin: D4 (2),  nodemcu led pin: LED_BUILTIN: D0 (16)
  analogWrite(pin, val);
}

void loop()
{
  unsigned long ticksum = 0;
  unsigned long intervalTime = 0;
  for(int i=0; i<10; i++)
  {
      ticksum += analogRead(A0);
      delay(3);
  }
  ticksum = ticksum / 10;

  // 设定开门动作间隔时间
  intervalTime =  (millis() - lasttime)/1000;
  // if ( intervalTime > 1 ) {
  //   Serial.printf("current position %ld\n", stepper.currentPosition() );
  // }

  if (ticksum > 130 && intervalTime > 6 ) {
    Serial.printf("press button\n");
    Serial.printf("------current position is %d\n",stepper.currentPosition());
    lasttime = millis();
    openFlower();
  }

  // Serial.printf("current position %d\n", stepper.currentPosition() );
  // If at the end of travel go to the other end
  // closeFlower();

  if (stepper.distanceToGo() != 0) {
    Serial.printf("get current position %d\n",stepper.currentPosition());
  }
  if ( stopFlag ) {
    stopStepper();
  }

  long curbrightness = stepper.currentPosition();
  for(int ledindex = 0; ledindex < 6; ledindex++) {
    leafledset(leafsleds[ledindex], stepper.currentPosition() * 11);
    FastLED.setBrightness(curbrightness);
  }
  if (flashMode == "bpm") {
    bpm();
  }
  if (flashMode == "fire") {
    fire();
  }
  if (flashMode == "xhl") {
    fill_solid(leds, NUM_LEDS, CRGB::LightPink);
  }
  if (flashMode == "pk") {
    fill_solid(leds, NUM_LEDS, CRGB::Pink);
  }
  if (flashMode == "dpk") {
    fill_solid(leds, NUM_LEDS, CRGB::DeepPink);
  }
  if (flashMode == "hpk") {
    fill_solid(leds, NUM_LEDS, CRGB::HotPink);
  }   
  if (flashMode == "nomode") {
    setxhl();
  }  
  

  // if ( curbrightness <= 10 ) {
  //   curbrightness = 0;
  // }
  // fill_rainbow( leds, NUM_LEDS, 0, 255 / NUM_LEDS);
  FastLED.show();
  // FastLED.delay(50);
  stepper.run();


}

