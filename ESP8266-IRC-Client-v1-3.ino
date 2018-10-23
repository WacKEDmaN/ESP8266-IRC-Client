#ifndef ESP8266
die();  only compile for ESP8266!  (overkill?! hey look no comments!!)
#endif 
extern "C" {
  #include "user_interface.h"  //ESP8266 SDK access!
} 
#include <ESP8266WiFi.h>  // required
#include <IRCClient.h>    // required
#include <Wire.h>         // for I2C sensors
#include <TimeLib.h>      // needed for time
#include <WiFiUdp.h>      // needed for NTP time


//////////////// Config /////////////////////////////////////////////////////////////////////

// Wifi settings
const char* ssid             =    "ssid";       // WiFi SSID to connect to
const char* password         =    "pass";     // WiFi Password

// IRC settings
#define IRC_SERVER           "irc.XXXX.net"           // irc server address
#define IRC_PORT              6667                            // irc server port 
#define IRC_NICKNAME         "XXXX"                           // bot nickname
#define IRC_USER             "XXXX"                           // bot username
#define REPLY_TO             "Owner"                          // Your IRC nick..used for "admin" only triggers
#define numCHANNELS           2                               // how many channels do you want to join?...
const char* CHANNEL[]     = {  "#offtopic", "#ESP8266"  };  // channels to join..
const char* JOIN_TRIGGER_TXT  =   "End of /MOTD command.";    // auto join channels trigger, when server sends this line..

// I2C Bus Devices
#define     USE_IRTemp    // comment out if you dont have a IRTemp sensor (not i2c)
#define     USE_BME280    // comment out if you dont have a i2c BME280 connected
#define     USE_PCF8591   // comment out if you dont have a i2c PCF8591 connected
#define     USE_LCD       // comment out if you dont have a i2c LCD connected

// Serial
#define     DBG_OUTPUT_PORT Serial // can change serial out to Serial2 pins..
#define     DEBUG true    // set to FALSE to turn off Serial output

//////////////// Sensor Specific ////////////////////////////////////////////////////////////

#ifdef USE_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;     // I2C  
#endif

#ifdef USE_PCF8591
// ADC-DAC address
#define PCF8591 (0x90>> 1)    // I2C bus address
#define ADC0 0x00             // control bytes for reading individual ADCs
#define ADC1 0x01
#define ADC2 0x02
#define ADC3 0x03
byte value0, value1, value2, value3;
#endif  

#ifdef USE_LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);        // LCD backpack I2C address, col ,row
#endif 

#ifdef USE_IRTemp    
#include "IRTemp.h"
static const byte PIN_DATA    = D3;       // Choose any free GPIO pins you like for these
static const byte PIN_CLOCK   = D4;
static const byte PIN_ACQUIRE = D5;       // can be tied low to free up pin, but constantly runs the device
static const TempUnit SCALE=CELSIUS;      // Options are CELSIUS, FAHRENHEIT
IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);
#endif 

IPAddress timeServer(203,100,61,10); // au.pool.ntp.org NTP Time server
const int timeZone = 11;     // AEST TimeZone

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WiFiClient wiFiClient;
IRCClient client(IRC_SERVER, IRC_PORT, wiFiClient);

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

unsigned long prevDisplay = 0; // when the digital clock was displayed
unsigned long started = 0; //
unsigned long lcdRefresh = 1000UL;  // timer 1 // LCD Clock Refresh 
unsigned long delayTime = 5000UL;  // timer 2 // LCD Return to Clock from info Trigger

ADC_MODE(ADC_VCC);  // set nodemcu ADC Mode to read board voltage 

byte degC[] = { 0x1C,0x14,0x1C,0x06,0x09,0x08,0x09,0x06 }; // deg C character
byte hPa[] = { 0x10,0x10,0x1C,0x17,0x15,0x07,0x04,0x04 }; // hp character for hPa.. 

void setup() {    
  pinMode(LED_BUILTIN, OUTPUT);  // nodemcu led
  digitalWrite(LED_BUILTIN, HIGH); 
  DBG_OUTPUT_PORT.begin(115200);   
  delay(100); //wait for serial 
#ifdef USE_LCD
  DBG_OUTPUT_PORT.print("Starting LCD...");
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(String(IRC_NICKNAME));
  lcd.setCursor(0, 1);
  lcd.print("Starting up....");
  lcd.createChar(0, degC); // create custom character
  lcd.createChar(1, hPa);
  DBG_OUTPUT_PORT.println("Done!");
  delay(1000); 
#endif
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);
  WiFi.begin(ssid, password);  // WIFI Startup
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  } 
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.println("WiFi connected");
  DBG_OUTPUT_PORT.println("IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());
  //WiFi.printDiag(Serial);  // displays wifi info including user and pass on wifi connect..!! 
#ifdef USE_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected..");
  lcd.setCursor(0, 1);
  lcd.print("IP: " + String(WiFi.localIP()));
  delay(2000);
#endif
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.println("Starting UDP...");
  Udp.begin(localPort);
#ifdef USE_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Syncing Time..");
#endif
  DBG_OUTPUT_PORT.print("Local port: ");
  DBG_OUTPUT_PORT.println(Udp.localPort());
  DBG_OUTPUT_PORT.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(86400); // check NTP every 24hrs (60*60*24)
  DBG_OUTPUT_PORT.println("Time Synced!");
  digitalClockDisplay();
#ifdef USE_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time Synced!");
  lcd.setCursor(0, 1);
  lcd.print("");
  LCDClockDisplay();
#endif
#ifdef USE_BME280
  DBG_OUTPUT_PORT.println("Starting BME280");
  bme.begin();    
#endif
  DBG_OUTPUT_PORT.println("Starting IRC Client - " + String(IRC_NICKNAME));
  client.setCallback(callback);
  client.setSentCallback(debugSentCallback);
}



void loop() {
    // update LCD Clock
#ifdef USE_LCD
  if (millis() > prevDisplay + lcdRefresh) {  // run every second...
    prevDisplay = millis();
    lcdRefresh = 1000UL; // make sure wait is set back to update every second if comming from a trigger..
    //lcd.clear();  causes flicker
    lcd.setCursor(0, 1); // we want the clock to display on the 2nd line..
    LCDClockDisplay();
    float vccd = ESP.getVcc(); // lets add voltage for good mesasure! 
    lcd.print(" "); 
    lcd.print(String((vccd/1024)*100,2) + "v  ");
    #ifdef USE_BME280      // lets add Temp and Humidity on the top line
    float temp = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure();
    lcd.setCursor(0, 0);
    lcd.print(String((temp),1));
    lcd.write(0); // print degC char
    lcd.print(" "+ String((humidity),0) + "% " + String((pressure/100.0F),0));
    lcd.write(1); // print hP char
    lcd.print("a");
    #endif
    }
#endif
  
  //  IRC Client connection check..
if (!client.connected()) { 
  DBG_OUTPUT_PORT.println("Attempting IRC connection...");
  // Attempt to connect
  if (client.connect(IRC_NICKNAME, IRC_USER)) {
    DBG_OUTPUT_PORT.println("connecting...");
    #ifdef USE_LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected to... ");
    lcd.setCursor(0, 1);
    lcd.print("     IRC");
    #endif
    } else {
      DBG_OUTPUT_PORT.println("failed... try again in 5 seconds");
      
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Connected failed");
      #endif
      delay(5000); // Wait 5 seconds before retrying
    }
    return;
  } 
  
  client.loop();
  
}      // main loop end 

// Funcitions..

//  IRC Triggers and output functions
void callback(IRCMessage ircMessage) {
  // PRIVMSG ignoring CTCP messages
  if (ircMessage.command == "PRIVMSG" && ircMessage.text[0] != '\001') {
    String message("<" + ircMessage.nick + "> " + ircMessage.text);
    DBG_OUTPUT_PORT.println(message);
  }

  // Join Channels
  if (ircMessage.parameters == IRC_NICKNAME  && ircMessage.text == (String(JOIN_TRIGGER_TXT))) { 
      DBG_OUTPUT_PORT.println("Joining Channels...");
      int i;
      for (i = 0; i < numCHANNELS; i++) { 
        String channel = CHANNEL[i];
        client.sendRaw("JOIN " + channel);
        delay(150);
      }
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Joined channels");
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
      #endif
  }
  
  //  ESP/NodeMCU Specific
    if (ircMessage.text == "!uptime") {
      client.sendMessage(ircMessage.parameters, "NodeMCU Uptime:" + String(millis2time()));
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Uptime:"); 
      lcd.setCursor(0, 1);
      lcd.print(String(millis2time()));
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
      #endif   
  }
  if (ircMessage.text == "!ledon") {
      digitalWrite(LED_BUILTIN, LOW);    // nodemcu
      client.sendMessage(ircMessage.parameters, ircMessage.nick + " turned NodeMCU Onboard LED: ON");
      #ifdef USE_LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(ircMessage.nick); 
      lcd.setCursor(0, 1);
      lcd.print("Turned LED: ON");
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock 
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
       lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
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
      lcdRefresh = delayTime;  // display for 5sec before going back to clock
     #endif 
    }
    #endif
    
    if (ircMessage.text == "!help") {  
      client.sendMessage(ircMessage.nick, "ESP8266 IRC-Bot Help");  
      client.sendMessage(ircMessage.nick, "ESP Commands:");
      client.sendMessage(ircMessage.nick, "!uptime :show current uptime");
     // client.sendMessage(ircMessage.nick, "!localtime :show current time and timezone");
      //client.sendMessage(ircMessage.nick, "!unixtime :show current time in unix format");
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
  DBG_OUTPUT_PORT.print("prefix: " + ircMessage.prefix);
  DBG_OUTPUT_PORT.print(" nick: " + ircMessage.nick);
  DBG_OUTPUT_PORT.print(" user: " + ircMessage.user);
  DBG_OUTPUT_PORT.print(" host: " + ircMessage.host);
  DBG_OUTPUT_PORT.print(" command: " + ircMessage.command);
  DBG_OUTPUT_PORT.print(" parameters: " + ircMessage.parameters);
  DBG_OUTPUT_PORT.println(" text: " + ircMessage.text);
  */
  DBG_OUTPUT_PORT.println(ircMessage.original);
  return;
}

void debugSentCallback(String data) {
  DBG_OUTPUT_PORT.println(data);
}
// end IRC client

///-------- NTP code ----------///

// send an NTP request to the time server at the given address
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  DBG_OUTPUT_PORT.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      DBG_OUTPUT_PORT.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  DBG_OUTPUT_PORT.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

String printTime() {
  String TimeNow = "";
  String hours = String(hour());
  String minutes = String(minute());
  String seconds = String(second());
  if (minute() < 10) {
    minutes = "0" + minutes;
  }
  if (second() < 10) {
    seconds = "0" + seconds;
  }
  TimeNow = (hours + ":" + minutes + ":" + seconds);
  return String(TimeNow);
}
String millis2time() {
  String Time = "";
  unsigned long ss;
  byte mm, hh;
  ss = millis() / 1000;
  hh = ss / 3600;
  mm = (ss - hh * 3600) / 60;
  ss = (ss - hh * 3600) - mm * 60;
  if (hh < 10)Time += "0";
  Time += (String)hh + ":";
  if (mm < 10)Time += "0";
  Time += (String)mm + ":";
  if (ss < 10)Time += "0";
  Time += (String)ss;
  return String(Time);
}

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  String hours = String(hour());
  String minutes = String(minute());
  String seconds = String(second());
  if (minute() < 10) {
    String minutes = "0" + String(minutes);
  }
  if (second() < 10) {
    String seconds = "0" + String(seconds);
  }
  DBG_OUTPUT_PORT.print(hours);
  DBG_OUTPUT_PORT.print(":");
  DBG_OUTPUT_PORT.print(minutes);
  DBG_OUTPUT_PORT.print(":");
  DBG_OUTPUT_PORT.print(seconds);
  DBG_OUTPUT_PORT.print(" ");
  DBG_OUTPUT_PORT.print(day());
  DBG_OUTPUT_PORT.print("/");
  DBG_OUTPUT_PORT.print(month());
  DBG_OUTPUT_PORT.print("/");
  DBG_OUTPUT_PORT.print(year()); 
  DBG_OUTPUT_PORT.print(" "); 
}

#ifdef USE_LCD
void LCDClockDisplay() {
  String hours = String(hour());
  String minutes = String(minute());
  String seconds = String(second());
  if (hour() < 10 ){
   hours = String(" " + hours);
  }
  if (minute() < 10) {
   minutes = String("0" + minutes);
  }
  if (second() < 10) {
   seconds = String("0" + seconds);
  }
  lcd.print(hours + ":" + minutes + ":" + seconds);
}
#endif
