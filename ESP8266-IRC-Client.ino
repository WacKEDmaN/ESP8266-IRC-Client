#include <ESP8266WiFi.h>
#include <IRCClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>
#include "IRTemp.h"

extern "C" {
#include "user_interface.h"
}

// IRTEMP pins
static const byte PIN_DATA    = D3; // Choose any pins you like for these
static const byte PIN_CLOCK   = D4;
static const byte PIN_ACQUIRE = D5;

static const TempUnit SCALE=CELSIUS;  // Options are CELSIUS, FAHRENHEIT

IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);

// Wifi settings
#define ssid         "XXXX"   // Edit These!
#define password     "XXXX"

// IRC settings
#define IRC_SERVER   "irc.efnet.org"  // Edit These!
#define IRC_PORT     6667
#define IRC_NICKNAME "ESPBot5000"
#define IRC_USER     "ESPBot5000"

#define REPLY_TO     "XXXX" // Reply only to this nick.. Edit This!

WiFiClient wiFiClient;
IRCClient client(IRC_SERVER, IRC_PORT, wiFiClient);

// ADC-DAC address
#define PCF8591 (0x90>> 1) // I2C bus address
#define ADC0 0x00 // control bytes for reading individual ADCs
#define ADC1 0x01
#define ADC2 0x02
#define ADC3 0x03
byte value0, value1, value2, value3;

Adafruit_BME280 bme; // I2C

ADC_MODE(ADC_VCC);

Ticker flipper;
int count = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  delay(100);

  bme.begin();  

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.printDiag(Serial);

  client.setCallback(callback);
  client.setSentCallback(debugSentCallback);
}

void loop() {
  if (!client.connected()) {
    Serial.println("Attempting IRC connection...");
    // Attempt to connect
    if (client.connect(IRC_NICKNAME, IRC_USER)) {
      Serial.println("connected");
    } else {
      Serial.println("failed... try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    return;
  }

  client.loop();
}

//  IRC Triggers and output
void callback(IRCMessage ircMessage) {
  // PRIVMSG ignoring CTCP messages
  if (ircMessage.command == "PRIVMSG" && ircMessage.text[0] != '\001') {
    String message("<" + ircMessage.nick + "> " + ircMessage.text);
    Serial.println(message);
  }
  // Channels to JOIN
  if (ircMessage.nick == REPLY_TO  && ircMessage.parameters == IRC_NICKNAME  && ircMessage.text == "!join") {
      //client.sendMessage(ircMessage.nick, "Hi " + ircMessage.nick + "! I'm your IRC bot.");
      //client.sendMessage(ircMessage.nick, "joining channels...");
      client.sendRaw("JOIN #XXXX");  // Channels EDIT THESE!
      client.sendRaw("JOIN #XXX");  // Channels
  }
  
  //  ESP Specific
  if (ircMessage.text == "!ledon") {
      digitalWrite(LED_BUILTIN, LOW);    
      client.sendMessage(ircMessage.parameters, "ESP8266 Onboard LED: ON");   
  }
  if (ircMessage.text == "!ledoff") {
      digitalWrite(LED_BUILTIN, HIGH);    
      client.sendMessage(ircMessage.parameters, "ESP8266 Onboard LED: OFF");   
  }
  if (ircMessage.text == "!ledblink") {
      flipper.attach(0.3, flip);    
      client.sendMessage(ircMessage.parameters, "ESP8266 Onboard LED: Blinking!");   
  }
  if (ircMessage.text == "!getvcc") {
      float vccd = ESP.getVcc();
      client.sendMessage(ircMessage.parameters, "ESP8266 Voltage: " + String(((vccd/1024)*100),2) + "v");
  }
  if (ircMessage.text == "!getfreeheap") {
      float freeheap = ESP.getFreeHeap();
      client.sendMessage(ircMessage.parameters, "ESP8266 Free Heap: " + String((freeheap),0) + " bytes");
  }
  if (ircMessage.text == "!getcycles") {
      float cycles = ESP.getCycleCount();
      client.sendMessage(ircMessage.parameters, "ESP8266 Instruction Cycles since startup: " + String((cycles),0));
  }
  if (ircMessage.text == "!getcpu") {
      float freq = ESP.getCpuFreqMHz();
      client.sendMessage(ircMessage.parameters, "ESP8266 CPU Frequency: " + String((freq),0) + "MHz");
  }
  if (ircMessage.text == "!getresetreason") {
      String reason = ESP.getResetReason();
      client.sendMessage(ircMessage.parameters, "ESP8266 Restart Reason: " + String(reason));
  }
  if (ircMessage.text == "!getchipid") {
      float chipid = ESP.getChipId();
      client.sendMessage(ircMessage.parameters, "ESP8266 ChipID: 0x" + String((chipid),HEX));
  }
  if (ircMessage.text == "!getresetinfo") {
      String resetinfo = ESP.getResetInfo();
      client.sendMessage(ircMessage.parameters, "ESP8266 Reset Info: " + resetinfo);
  }
  if (ircMessage.text == "!getsketchsize") {
      float sketchsize = ESP.getSketchSize();
      client.sendMessage(ircMessage.parameters, "ESP8266 Sketch Size: " + String((sketchsize),0));
  }
  if (ircMessage.text == "!getversion") {
      const char* sdkversion = ESP.getSdkVersion();
      String coreversion = ESP.getCoreVersion();
      String fullversion = ESP.getFullVersion();
      uint8_t bootversion = ESP.getBootVersion();
      client.sendMessage(ircMessage.parameters, "ESP8266 SDK Version: " + String(sdkversion));
      client.sendMessage(ircMessage.parameters, "ESP8266 Core Version: " + String(coreversion));
      client.sendMessage(ircMessage.parameters, "ESP8266 Full Version: " + String(fullversion));
      client.sendMessage(ircMessage.parameters, "ESP8266 Boot Version: " + String(bootversion));
  }

  //     Wifi scan
  if (ircMessage.text == "!scanwifi") {
     int n = WiFi.scanNetworks();
     client.sendMessage(ircMessage.parameters, "ESP8266 found " + String(n) + " SSIDs:");
     for (int i = 0; i < n; i++) {
      client.sendMessage(ircMessage.parameters, String(i+1) + ") " + String(WiFi.SSID(i)) + " Ch: " + String(WiFi.channel(i)) + " (" + String(WiFi.RSSI(i)) + "dBm) " + String(WiFi.encryptionType(i) == ENC_TYPE_NONE ? ":OPEN" : ""));  
     }
  }
    
  ////   BME280 Temp, humidity, pressure 
  if (ircMessage.text == "!temp") {
      float temp = bme.readTemperature();
      float humidity = bme.readHumidity();
      float pressure = bme.readPressure();
      client.sendMessage(ircMessage.parameters, "BMP280 Temperature: " + String(temp) + "*C - Humidity: " + String(humidity) + "% - Pressure: " + String(pressure / 100.0F) + "hPa"); 
  }
  //    PCF8591 ADC-DAC
  if (ircMessage.text == "!adc") {
     Wire.beginTransmission(PCF8591); // wake up PCF8591
     Wire.write(ADC0); // control byte - read ADC0
     Wire.endTransmission(); // end tranmission
     Wire.requestFrom(PCF8591, 2);
     value0=Wire.read();
     value0=Wire.read();
     Wire.beginTransmission(PCF8591); // wake up PCF8591
     Wire.write(ADC1); // control byte - read ADC1
     Wire.endTransmission(); // end tranmission
     Wire.requestFrom(PCF8591, 2);
     value1=Wire.read();
     value1=Wire.read();
     Wire.beginTransmission(PCF8591); // wake up PCF8591
     Wire.write(ADC2); // control byte - read ADC2
     Wire.endTransmission(); // end tranmission
     Wire.requestFrom(PCF8591, 2);
     value2=Wire.read();
     value2=Wire.read();
     Wire.beginTransmission(PCF8591); // wake up PCF8591
     Wire.write(ADC3); // control byte - read ADC3
     Wire.endTransmission(); // end tranmission
     Wire.requestFrom(PCF8591, 2);
     value3=Wire.read();
     value3=Wire.read();
     client.sendMessage(ircMessage.parameters, "PCF8591 ADC0: " + String(value0) + " ADC1: " + String(value1) + " ADC2: " + String(value2) + " ADC3: " + String(value3)); 
    }
    //    IR Remote Temp sensor
    if (ircMessage.text == "!irtemp") {
       float irtemp = irTemp.getIRTemperature(SCALE);
       float irambient = irTemp.getAmbientTemperature(SCALE);
       client.sendMessage(ircMessage.parameters, "IR Temp: " + String((irtemp),2) + "*C Ambient Temp: " + String((irambient),2) + "*C"); 
    }
    
    
    if (ircMessage.text == "!help") {  
      client.sendMessage(ircMessage.nick, "ESP8266 IRC-Bot Help");  
      client.sendMessage(ircMessage.nick, "ESP Commands:");
      client.sendMessage(ircMessage.nick, "!ledon :turn on onboard LED");
      client.sendMessage(ircMessage.nick, "!ledoff :turn off onboard LED");
      client.sendMessage(ircMessage.nick, "!ledblink : blinks onboard LED 120 times");
      client.sendMessage(ircMessage.nick, "!getvcc :shows ESP8266 voltage");
      client.sendMessage(ircMessage.nick, "!getfreeheap :shows ESP8266 free heap memory");
      client.sendMessage(ircMessage.nick, "!getcycles :shows ESP8266 cpu instruction cycle count since start as an unsigned 32-bit");
      client.sendMessage(ircMessage.nick, "!getcpu :shows ESP8266 CPU Frequency");
      client.sendMessage(ircMessage.nick, "!getresetreason :shows ESP8266 Reset Reason");
      client.sendMessage(ircMessage.nick, "!getresetinfo :shows ESP8266 Reset debug info");
      client.sendMessage(ircMessage.nick, "!getchipid :shows ESP8266 Chip ID");
      client.sendMessage(ircMessage.nick, "!getsketchsize :shows ESP8266 Arduino Sketch Size");
      client.sendMessage(ircMessage.nick, "!getversion :shows ESP8266 SDK, Core, Full, and Boot Versions");
      client.sendMessage(ircMessage.nick, "!scanwifi :scans wifi and displays SSIDs, Channel, RSSI, encryption type");
      client.sendMessage(ircMessage.nick, " ");
      client.sendMessage(ircMessage.nick, "Sensor Commands:");
      client.sendMessage(ircMessage.nick, "!temp :shows BMP280 Temperature, humidity and air pressure");
      client.sendMessage(ircMessage.nick, "!irtemp :shows IR Remote Surface Temperature, and Ambient Temperature");
      client.sendMessage(ircMessage.nick, "!adc :shows PCF8591 AD Converter values");
    }
  /*
  // debug
  Serial.print("prefix: " + ircMessage.prefix);
  Serial.print(" nick: " + ircMessage.nick);
  Serial.print(" user: " + ircMessage.user);
  Serial.print(" host: " + ircMessage.host);
  Serial.print(" command: " + ircMessage.command);
  Serial.print(" parameters: " + ircMessage.parameters);
  Serial.println(" text: " + ircMessage.text);
  */
  Serial.println(ircMessage.original);
  return;
}

void debugSentCallback(String data) {
  Serial.println(data);
}

void flip() {
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state

  ++count;
  // when the counter reaches a certain value, start blinking like crazy
  if (count == 20) {
    flipper.attach(0.1, flip);
  }
  // when the counter reaches yet another value, stop blinking
  else if (count == 120) {
    flipper.detach();
  }
}
