# ESP8266-IRC-Client
ESP8266 IRC Client with BME280, PCF8591, IR Temp Sensors that respond to triggers from IRC channels

Uses ArduinoIRC library: IRCClient.h

Edit WIFI settings to match your network

Edit IRC settings to match your irc server, port, bot nick, and bot user

Edit REPLY_TO to match your IRC Nick, else you wont be able to join a channel

Edit Channels to match the IRC channels to join

 use '/msg IRC_NICKNAME !join' from IRC, once the bot has connected to IRC to join channels, it will only join if your IRC nick matches REPLY_TO (IRC_NICKNAME and REPLY_TO are case sensitive!!)
 
once its joined the channels you can send '!help' for a list of available commands (sends to PM)

BME280 and PCF8591 connected to pins D1, D2 (I2C)

IRTemp Sensor connected to D3, D4, D5
