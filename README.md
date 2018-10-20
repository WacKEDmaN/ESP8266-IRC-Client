# ESP8266-IRC-Client

### V1.1 ESP8266-IRC-Client-LCD.ino
Search sketch for "XXXX" lines for quick startup!<br>
Added ability to remove individual sensors .. comment out #ifdef USE_XXXX lines<br> 
Added 16x2 LCD on I2C<br>
Thanks to Andreas Spiess for the preprocessor tips!<br>
https://www.youtube.com/watch?v=1eL8dXL3Wow

### V1.0 ESP8266-IRC-Client.ino
ESP8266 IRC Client with BME280, PCF8591, IR Temp Sensors that respond to triggers from IRC channels<br>
Uses ArduinoIRC library: IRCClient.h<br>
Edit WIFI settings to match your network<br>
Edit IRC settings to match your irc server, port, bot nick, and bot user<br>
Edit REPLY_TO to match your IRC Nick, else you wont be able to join a channel<br>
Edit Channels to match the IRC channels to join (towards bottom of script)<br>
use '/msg IRC_NICKNAME !join' from IRC, once the bot has connected to IRC to join channels, it will only join if your IRC nick matches REPLY_TO (IRC_NICKNAME and REPLY_TO are case sensitive!!)<br>
once its joined the channels you can send '!help' for a list of available commands (sends to PM)<br>

## Pinouts

Device   |ESP8266 Pins     |Device Pins 
---------|-----------------|-------------------
18x2 LCD | D1(SCL),D2(SDA) | (SCL),(SDA)
BME280   | D1(SCL),D2(SDA) | (SCL),(SDA)
PCF8591  | D1(SCL),D2(SDA) | (SCL),(SDA)
IRTemp   | D3, D4, D5      | Data,Clock,Acquire


