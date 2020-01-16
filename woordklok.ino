#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <WiFiUdp.h>
//for LED status
#include <Ticker.h>
Ticker ticker;

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);
/* for normal hardware wire use above */
#define countof(a) (sizeof(a) / sizeof(a[0]))
#include <FastLED.h>



#define LED_PIN     2
#define NUM_LEDS    56
#define BRIGHTNESS  64 //keep at 128 or below when using pc usb power
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

const long utcOffsetInSeconds = 3600;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void setup() {
  Serial.begin(115200);
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

 pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  timeClient.begin();
  
  //--------RTC SETUP ------------
  // if you are using ESP-01 then uncomment the line below to reset the pins to
  // the available pins for SDA, SCL
  Wire.begin(4, 5); // due to limited pins, use pin 0 and 2 for SDA, SCL
  
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
      if (Rtc.LastError() != 0) {
          // we have a communications error
          // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
          // what the number means
          Serial.print("RTC communications error = ");
          Serial.println(Rtc.LastError());
      }
      else {
          // Common Cuases:
          //    1) first time you ran and the device wasn't running yet
          //    2) the battery on the device is low or even missing

          Serial.println("RTC lost confidence in the DateTime!");
          // following line sets the RTC to the date & time this sketch was compiled
          // it will also reset the valid flag internally unless the Rtc device is
          // having an issue

          Rtc.SetDateTime(compiled);
      }
  }

    if (!Rtc.GetIsRunning()) {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) {
        Serial.println("RTC is newer than compile time. (this is expected)");
        Rtc.SetDateTime(compiled);
    }
    else if (now == compiled) {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low); 

  // put your setup code here, to run once:
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  //display the "het is ... uur" always yellow.
  //Het
  leds[48] = CRGB::Yellow;
  leds[47] = CRGB::Yellow;
  leds[34] = CRGB::Yellow;

  //Is
  leds[20] = CRGB::Yellow;
  leds[19] = CRGB::Yellow;

  FastLED.show();
}

void rtcUpdate(){
  //update RTC once an hour.
  timeClient.forceUpdate();
  Serial.println(timeClient.getFormattedTime());
  //Rtc.adjust(DateTime(2020, 1, 15, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
}

void loop() {
  timeClient.forceUpdate();
  //rtcUpdate();
  // put your main code here, to run repeatedly:
  RtcDateTime now = Rtc.GetDateTime();
  
  //write the hour
  //nums(now.Hour());
  int minute = timeClient.getMinutes();
  int hour = timeClient.getHours();
  int hournext;
  if(hour == 23){
    hournext = 0;
  } else {
    hournext = hour + 1;
  }
  if(minute == 0){
    precies();
    nums(hour);
    uur();
  } else if(minute > 0 && minute <= 2){
    ruim();
    over();
    nums(hour); 
    uur();
  } else if(minute > 2 && minute <= 4){
    bijna();
    five_min();
    over();
    nums(hour);
  } else if(minute == 5){
    precies();
    five_min();
    over();
    nums(hour);
  } else if(minute > 5 && minute <= 7){
    ruim();
    five_min();
    over();
    nums(hour);
  } else if(minute > 7 && minute <= 9){
    bijna();
    ten_min();
    over();
    nums(hour);
  } else if(minute == 10){
    precies();
    ten_min();
    over();
    nums(hour);
  } else if(minute > 10 && minute <= 12){
    ruim();
    ten_min();
    over();
    nums(hour);    
  } else if(minute > 12 && minute <= 14){
    bijna();
    kwart();
    over();
    nums(hour);
  } else if(minute == 15){
    precies();
    kwart();
    over();
    nums(hour);    
  } else if(minute > 15 && minute <= 17){
    ruim();
    kwart();
    over();
    nums(hour);   
  } else if(minute > 17 && minute <= 19){
    bijna();
    ten_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute == 20){
    precies();
    ten_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute > 20 && minute <= 22){
    ruim();
    ten_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute > 22 && minute <= 24){
    bijna();
    five_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute == 25){
    precies();
    five_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute > 25 && minute <= 27){
    ruim();
    five_min();
    voor();
    half();
    nums(hournext);    
  } else if(minute > 27 && minute <= 29){
    bijna();
    half();
    nums(hournext);    
  } else if(minute == 30){
    precies();
    half();
    nums(hournext);
  } else if(minute > 30 && minute <= 32){
    ruim();
    half();
    nums(hournext);    
  }  else if(minute > 32 && minute <= 34){
    bijna();
    five_min();
    over();
    half();
    nums(hournext);    
  } else if(minute == 35){
    precies();
    five_min();
    over();
    half();
    nums(hournext);    
  } else if(minute > 35 && minute <= 37){
    ruim();
    five_min();
    over();
    half();
    nums(hournext);    
  } else if(minute > 37 && minute <= 39){
    bijna();
    ten_min();
    over();
    half();
    nums(hournext);    
  } else if(minute == 40){
    precies();
    ten_min();
    over();
    half();
    nums(hournext);    
  } else if(minute > 40 && minute <= 42){
    ruim();
    ten_min();
    over();
    half();
    nums(hournext);    
  } else if(minute > 42 && minute <= 44){
    bijna();
    kwart();
    voor();
    nums(hournext);    
  } else if(minute  == 45){
    precies();
    kwart();
    voor();
    nums(hournext);    
  } else if(minute > 45 && minute <= 47){
    ruim();
    kwart();
    voor();
    nums(hournext);    
  } else if(minute > 47 && minute <= 49){
    bijna();
    ten_min();
    voor();
    nums(hournext);    
  } else if(minute == 50){
    precies();
    ten_min();
    voor();
    nums(hournext);    
  } else if(minute > 50 && minute <= 52){
    ruim();
    ten_min();
    voor();
    nums(hournext);    
  } else if(minute > 52 && minute <= 54){
    bijna();
    five_min();
    voor();
    nums(hournext);    
  } else if(minute == 55){
    precies();
    five_min();
    voor();
    nums(hournext);    
  } else if(minute > 55 && minute <= 57){
    ruim();
    five_min();
    voor();
    nums(hournext);    
  } else if(minute > 57 && minute <= 59){
    bijna();
    nums(hournext); 
    uur();   
  }
  
  
  
  
  
  //update every 5 seconds. 
  delay(5000);
  clearAll();
  /*
  ruim();
  delay(1000);
  clearAll();
  delay(500);
  bijna();
  kwart();
  voor();
  nums(9);
  delay(1000);
  clearAll();
  delay(500);
  precies();
  delay(1000);
  clearAll();
  delay(500);
  */
}
void clearAll(){
  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  leds[2] = CRGB::Black;
  leds[3] = CRGB::Black;
  leds[4] = CRGB::Black;
  leds[5] = CRGB::Black;
  leds[6] = CRGB::Black;
  leds[7] = CRGB::Black;
  leds[8] = CRGB::Black;
  leds[9] = CRGB::Black;
  leds[10] = CRGB::Black;
  leds[11] = CRGB::Black;
  leds[12] = CRGB::Black;
  leds[13] = CRGB::Black;
  leds[14] = CRGB::Black;
  leds[15] = CRGB::Black;
  leds[16] = CRGB::Black;
  leds[17] = CRGB::Black;
  leds[18] = CRGB::Black;
  leds[21] = CRGB::Black;
  leds[22] = CRGB::Black;
  leds[23] = CRGB::Black;
  leds[24] = CRGB::Black;
  leds[25] = CRGB::Black;
  leds[26] = CRGB::Black;
  leds[27] = CRGB::Black;
  leds[28] = CRGB::Black;
  leds[29] = CRGB::Black;
  leds[30] = CRGB::Black;
  leds[31] = CRGB::Black;
  leds[32] = CRGB::Black;
  leds[33] = CRGB::Black;
  leds[35] = CRGB::Black;
  leds[36] = CRGB::Black;
  leds[37] = CRGB::Black;
  leds[38] = CRGB::Black;
  leds[39] = CRGB::Black;
  leds[40] = CRGB::Black;
  leds[41] = CRGB::Black;
  leds[42] = CRGB::Black;
  leds[43] = CRGB::Black;
  leds[44] = CRGB::Black;
  leds[45] = CRGB::Black;
  leds[46] = CRGB::Black;
  leds[49] = CRGB::Black;
  leds[50] = CRGB::Black;
  leds[51] = CRGB::Black;
  leds[52] = CRGB::Black;
  leds[53] = CRGB::Black;
  leds[54] = CRGB::Black;
  leds[55] = CRGB::Black;
  
  FastLED.show();
}
void nums(int number){
  switch (number){
    case 0:
      leds[55] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 1:
      leds[14] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 2:
      leds[10] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 3:
      leds[2] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 4: 
      leds[54] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 5:
      leds[41] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 6: 
      leds[39] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 7:
      leds[28] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 8: 
      leds[25] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 9:
      leds[13] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 10:
      leds[11] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 11: 
      leds[1] = CRGB::White;
      leds[12] = CRGB::White;
      break;
    case 12:
      leds[55] = CRGB::White;
      leds[12] = CRGB::White;
      break;
      case 13:
      leds[14] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 14:
      leds[10] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 15:
      leds[2] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 16: 
      leds[54] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 17:
      leds[41] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 18: 
      leds[39] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 19:
      leds[28] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 20: 
      leds[25] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 21:
      leds[13] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 22:
      leds[11] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    case 23: 
      leds[1] = CRGB::White;
      leds[0] = CRGB::White;
      break;
    
  }
  FastLED.show();
}
void uur(){
  leds[40] = CRGB::White;
  leds[27] = CRGB::White;
  leds[26] = CRGB::White;
  FastLED.show(); 
}
void ruim(){
  leds[49] = CRGB::White;
  leds[46] = CRGB::White;
  leds[35] = CRGB::White;
  leds[33] = CRGB::White;
  FastLED.show(); 
}
void bijna(){
  leds[21] = CRGB::White;
  leds[18] = CRGB::White;
  leds[6] = CRGB::White;
  leds[5] = CRGB::White;
  FastLED.show(); 
}
void precies(){
  leds[50] = CRGB::White;
  leds[45] = CRGB::White;
  leds[36] = CRGB::White;
  leds[32] = CRGB::White;
  leds[22] = CRGB::White;
  leds[17] = CRGB::White;
  leds[7] = CRGB::White;
  FastLED.show();
}
void five_min(){
  leds[51] = CRGB::White;
  FastLED.show();
}
void ten_min(){
  leds[44] = CRGB::White;
  FastLED.show();
}
void kwart(){
  leds[31] = CRGB::White;
  leds[23] = CRGB::White;
  leds[16] = CRGB::White;
  leds[8] = CRGB::White;
  leds[4] = CRGB::White;
  FastLED.show();
}
void over(){
  leds[52] = CRGB::White;
  leds[43] = CRGB::White;
  leds[37] = CRGB::White;
  leds[30] = CRGB::White;
  FastLED.show();
}
void voor(){
  leds[24] = CRGB::White;
  leds[15] = CRGB::White;
  leds[9] = CRGB::White;
  leds[3] = CRGB::White;
  FastLED.show();
}

void half(){
  leds[53] = CRGB::White;
  leds[42] = CRGB::White;
  leds[38] = CRGB::White;
  leds[29] = CRGB::White;
  FastLED.show();
}

void printDateTime(const RtcDateTime& dt){
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
