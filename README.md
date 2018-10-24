# ESP8266-IRC-Client

### V1.3 ESP8266-IRC-Client-v1-3.ino
New config section at top of script<br>
Added NTP<br>
Added !uptime command (!localtime !gmttime and !unixtime comming soon!)<br> 
Added auto join IRC Channels (triggers via line send from server currently set for end of motd)<br>
Added custom LCD characters<br>
LCD now displays live BME280 (if defined!) Time and voltage data, IRC Triggers show for 5 seconds before returning to the "live" page..<br>
10hr Live stream of it in action... https://www.youtube.com/watch?v=Qyn5Yd8giZI (not much activity sorry we're all idle whores!)<br>

### V1.2 ESP8266-IRC-Client-v1-2.ino
Search sketch for "XXXX" lines for quick startup!<br>
Added ability to remove individual sensors .. comment out #ifdef USE_XXXX lines<br> 
Added 16x2 LCD on I2C<br>
Thanks to Andreas Spiess for the preprocessor tips! https://www.youtube.com/watch?v=1eL8dXL3Wow

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

Device   |NodeMCU Pins     |Device Pins 
---------|-----------------|-------------------
18x2 LCD | D1(SCL),D2(SDA) | (SCL),(SDA)
BME280   | D1(SCL),D2(SDA) | (SCL),(SDA)
PCF8591  | D1(SCL),D2(SDA) | (SCL),(SDA)
IRTemp   | D3, D4, D5      | Data,Clock,Acquire


