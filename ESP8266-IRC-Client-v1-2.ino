#ifndef ESP8266
die();  only compile for ESP8266!  (overkill?! hey look no comments!!)
#endif 
extern "C" {
#include "user_interface.h" //ESP8266 SDK access!
}
#include <ESP8266WiFi.h>  // required
#include <IRCClient.h>    // required
#include <Wire.h> //  for I2C sensors

// Wifi settings
#define ssid         "XXXX"   
#define password     "XXXX"

// IRC settings
#define IRC_SERVER   "irc.XXXX.net"
#define IRC_PORT     6667
#define IRC_NICKNAME "XXXX"
#define IRC_USER     "XXXX"
#define REPLY_TO     "XXXX" // Reply only to this nick..your irc nick

#define USE_IRTemp    // comment out if you dont have a IRTemp sensor (not i2c)

#define USE_BME280    // comment out if you dont have a i2c BME280 connected
#define USE_PCF8591   // comment out if you dont have a i2c PCF8591 connected
#define USE_LCD       // comment out if you dont have a i2c LCD connected

#ifdef USE_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C  
#endif

#ifdef USE_PCF8591
// ADC-DAC address
#define PCF8591 (0x90>> 1) // I2C bus address
#define ADC0 0x00 // control bytes for reading individual ADCs
#define ADC1 0x01
#define ADC2 0x02
#define ADC3 0x03
byte value0, value1, value2, value3;
#endif  

#ifdef USE_LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // LCD backpack I2C address, col ,row
#endif // USE_LCD

#ifdef USE_IRTemp    
#include "IRTemp.h"
static const byte PIN_DATA    = D3; // Choose any free GPIO pins you like for these
static const byte PIN_CLOCK   = D4;
static const byte PIN_ACQUIRE = D5; // can be tied low to free up pin, but constantly runs the device
static const TempUnit SCALE=CELSIUS;  // Options are CELSIUS, FAHRENHEIT
IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);
#endif // USE_IRTemp

WiFiClient wiFiClient;
IRCClient client(IRC_SERVER, IRC_PORT, wiFiClient);

ADC_MODE(ADC_VCC);  // set nodemcu ADC Mode to read board voltage 

void setup() {  
  pinMode(LED_BUILTIN, OUTPUT);  // nodemcu led
  digitalWrite(LED_BUILTIN, HIGH); 
  Serial.begin(115200);   
  delay(100); //wait for serial
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  // WIFI Startup
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  } 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //WiFi.printDiag(Serial);  // displays wifi info including user and pass on wifi connect..!! 

  
#ifdef USE_BME280
  Serial.println("Starting BME280");
  bme.begin();    
#endif

#ifdef USE_LCD
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(String(IRC_NICKNAME));
  lcd.setCursor(0, 1);
  lcd.print("Starting up....");
#endif
  
  client.setCallback(callback);
  client.setSentCallback(debugSentCallback);

}

void loop() {

  //  IRC Client connection check..
  if (!client.connected()) { 
    Serial.println("Attempting IRC connection...");
    // Attempt to connect
    if (client.connect(IRC_NICKNAME, IRC_USER)) {
      Serial.println("connected");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Connected to... ");
      lcd.setCursor(0, 1);
      lcd.print("     IRC");
      #endif
    } else {
      Serial.println("failed... try again in 5 seconds");
      // Wait 5 seconds before retrying
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Connected failed");
      #endif
      delay(5000); 
    }
    return;
  } 
  
  client.loop();
  
}      // main loop end 

//  IRC Triggers and output functions
void callback(IRCMessage ircMessage) {
  // PRIVMSG ignoring CTCP messages
  if (ircMessage.command == "PRIVMSG" && ircMessage.text[0] != '\001') {
    String message("<" + ircMessage.nick + "> " + ircMessage.text);
    Serial.println(message);
  }
  // Channels to JOIN  **Edit Channels Here!**
  if (ircMessage.nick == REPLY_TO  && ircMessage.parameters == IRC_NICKNAME  && ircMessage.text == "!join") { 
      //client.sendMessage(ircMessage.nick, "Hi " + ircMessage.nick + "! I'm your IRC bot.");
      //client.sendMessage(ircMessage.nick, "joining channels...");
      client.sendRaw("JOIN #XXXX");  // Channel to join (include #)
      client.sendRaw("JOIN #XXXX");  // commennt out this channel if not needed
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("joining channels...");
      #endif
  }
  
  //  ESP/NodeMCU Specific
  if (ircMessage.text == "!ledon") {
      digitalWrite(LED_BUILTIN, LOW);    // nodemcu
      client.sendMessage(ircMessage.parameters, ircMessage.nick + " turned NodeMCU Onboard LED: ON");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(ircMessage.nick); 
      lcd.setCursor(0, 1);
      lcd.print("Turned LED: ON");
      #endif   
  }
  if (ircMessage.text == "!ledoff") {
      digitalWrite(LED_BUILTIN, HIGH);    
      client.sendMessage(ircMessage.parameters, ircMessage.nick + " turned NodeMCU Onboard LED: OFF"); 
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(ircMessage.nick); 
      lcd.setCursor(0, 1);
      lcd.print("Turned LED: OFF");
      #endif    
  }
  if (ircMessage.text == "!getvcc") {
      float vccd = ESP.getVcc();
      
      client.sendMessage(ircMessage.parameters, "NodeMCU Voltage: " + String((vccd/1024)*100,2) + "v");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("NodeMCU Voltage:"); 
      lcd.setCursor(0, 1);
      lcd.print(String((vccd/1024)*100,2) + " Volts");
      #endif
  }
  if (ircMessage.text == "!getfreeheap") {
      float freeheap = ESP.getFreeHeap();
      client.sendMessage(ircMessage.parameters, "ESP8266 Free Heap: " + String((freeheap),0) + " bytes");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 Heap:"); 
      lcd.setCursor(0, 1);
      lcd.print(String((freeheap),0) + " bytes");
      #endif
  }
  if (ircMessage.text == "!getcycles") {
      uint32_t cycles = ESP.getCycleCount();
      client.sendMessage(ircMessage.parameters, "ESP8266 Instruction Cycles since startup: " + String(cycles));
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 Cycles:"); 
      lcd.setCursor(0, 1);
      lcd.print(String(cycles));
      #endif
  }
  if (ircMessage.text == "!getcpu") {
      float freq = ESP.getCpuFreqMHz();
      client.sendMessage(ircMessage.parameters, "ESP8266 CPU Frequency: " + String((freq),0) + "MHz");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 Freq:"); 
      lcd.setCursor(0, 1);
      lcd.print(String((freq),0) + " MHz");
      #endif
  }
 
  if (ircMessage.text == "!getchipid") {
      uint32_t chipid = ESP.getChipId();
      client.sendMessage(ircMessage.parameters, "ESP8266 ChipID: 0x" + String((chipid),HEX));
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 Chip ID:"); 
      lcd.setCursor(0, 1);
      lcd.print("    0x" + String((chipid),HEX));
      #endif
  }
  if (ircMessage.text == "!getresetinfo") {
      String resetinfo = ESP.getResetInfo();
      client.sendMessage(ircMessage.parameters, "ESP8266 Reset Info: " + resetinfo);
  }
  if (ircMessage.text == "!getsketchsize") {
      float sketchsize = ESP.getSketchSize();
      client.sendMessage(ircMessage.parameters, "ESP8266 Sketch Size: " + String((sketchsize),0) + " bytes");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 Sketch:"); 
      lcd.setCursor(0, 1);
      lcd.print(String((sketchsize),0) + " bytes");
      #endif
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
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SDK: " + String(sdkversion)); 
      lcd.setCursor(0, 1);
      lcd.print("CORE: " + String(coreversion));
      #endif
  }
  if (ircMessage.text == "!scanwifi") {
     int n = WiFi.scanNetworks();
     client.sendMessage(ircMessage.parameters, "ESP8266 found " + String(n) + " SSIDs:");
     for (int i = 0; i < n; i++) {
      client.sendMessage(ircMessage.parameters, String(i+1) + ") " + String(WiFi.SSID(i)) + " Ch: " + String(WiFi.channel(i)) + " (" + String(WiFi.RSSI(i)) + "dBm) " + String(WiFi.encryptionType(i) == ENC_TYPE_NONE ? ":OPEN" : ""));  
     #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP8266 WiFi"); 
      lcd.setCursor(0, 1);
      lcd.print("      Scanned...");
     #endif
     }
  }
  
#ifdef USE_BME280 ////   BME280 Temp, humidity, pressure
  if (ircMessage.text == "!temp") {
      float temp = bme.readTemperature();
      float humidity = bme.readHumidity();
      float pressure = bme.readPressure();
      client.sendMessage(ircMessage.parameters, "BME280 Temperature: " + String(temp) + "*C - Humidity: " + String(humidity) + "% - Pressure: " + String(pressure / 100.0F) + "hPa"); 
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BME280 T:" + String((temp),1) + "c ");
      lcd.setCursor(0, 1);
      lcd.print("RH:" + String((humidity),0) + "% P:" + String((pressure/ 100.0F),0) + "hPa ");
      #endif 
  } 
#endif

  #ifdef USE_PCF8591 
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
     #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("A0: " + String(value0) + " A1: " + String(value1)); 
      lcd.setCursor(0, 1);
      lcd.print("A2: " + String(value2) + " A3: " + String(value3));
     #endif
    }
    #endif

    #ifdef USE_IRTemp
    //    IR Remote Temp sensor
    if (ircMessage.text == "!irtemp") {
       float irtemp = irTemp.getIRTemperature(SCALE);
       float irambient = irTemp.getAmbientTemperature(SCALE);
       client.sendMessage(ircMessage.parameters, "IR Surface Temp: " + String((irtemp),2) + "*C Ambient Temp: " + String((irambient),2) + "*C"); 
     #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IR Temp: " + String((irtemp),2) + "c"); 
      lcd.setCursor(0, 1);
      lcd.print("Ambient: " + String((irambient),2) + "c");
     #endif
    }
    #endif
    
    if (ircMessage.text == "!help") {  
      client.sendMessage(ircMessage.nick, "ESP8266 IRC-Bot Help");  
      client.sendMessage(ircMessage.nick, "ESP Commands:");
      client.sendMessage(ircMessage.nick, "!ledon :turn on onboard LED");
      client.sendMessage(ircMessage.nick, "!ledoff :turn off onboard LED");
      client.sendMessage(ircMessage.nick, "!getvcc :shows ESP8266 voltage");
      client.sendMessage(ircMessage.nick, "!getfreeheap :shows ESP8266 free heap memory");
      client.sendMessage(ircMessage.nick, "!getcycles :shows ESP8266 cpu instruction cycle count since start as an unsigned 32-bit");
      client.sendMessage(ircMessage.nick, "!getcpu :shows ESP8266 CPU Frequency");
      client.sendMessage(ircMessage.nick, "!getresetinfo :shows ESP8266 Reset debug info");
      client.sendMessage(ircMessage.nick, "!getchipid :shows ESP8266 Chip ID");
      client.sendMessage(ircMessage.nick, "!getsketchsize :shows ESP8266 Arduino Sketch Size");
      client.sendMessage(ircMessage.nick, "!getversion :shows ESP8266 SDK, Core, Full, and Boot Versions");
      client.sendMessage(ircMessage.nick, "!scanwifi :scans wifi and displays SSIDs, Channel, RSSI, encryption type");
      client.sendMessage(ircMessage.nick, " ");
      
      client.sendMessage(ircMessage.nick, "Sensor Commands:");
      #ifdef USE_BME280
      client.sendMessage(ircMessage.nick, "!temp :shows BME280 Temperature, humidity and air pressure");
      #endif
      #ifdef USE_IRTemp
      client.sendMessage(ircMessage.nick, "!irtemp :shows IR Remote Surface Temperature, and Ambient Temperature");
      #endif
      #ifdef USE_PCF8591
      client.sendMessage(ircMessage.nick, "!adc :shows PCF8591 AD Converter values");
      #endif
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
// end IRC client
