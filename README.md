# mqtt2sql - Simple app to store MQTT messages in a SQL database

This little application subscribes to MQTT topics and stores the payload of matching messages into a SQL databalse like [mysql](https://www.mysql.com) or [mariadb](https://mariadb.com/).

It can be used to record sensor readings for later analysis or visualization. For example temperature data that is sent in MQTT messages can be recorded in a SQL table. This data can be used to plot a temperature graph over time.

I use a simple setup based on a [BME280](https://de.aliexpress.com/item/32849462236.html) sensor connected to an [D1 Mini ESP8266 board](https://de.aliexpress.com/item/32651747570.html)  which send temperature, humidity and pressure data as MQTT messages. 

It sends it's data in messages like:

```
/YardControl/Sensor/BB-9985/BME280/Temperature 27.46
/YardControl/Sensor/BB-9985/BME280/Pressure 1013.66
/YardControl/Sensor/BB-9985/BME280/Humidity 32.57
```
I have a table for each data stream with a column for the value and a timestamp with this structure:

Name  |  Typ  |   Attribute  |   Null  |   Standard 
------- | ------ | ------------ | ------- | ------------
time    | timestamp | on update CURRENT_TIMESTAMP  |  No  | CURRENT_TIMESTAMP 
value   |  double     |                                                            | Ja   |   NULL                             

The collected data looks like this:

time | value    
----- | ------
2020-08-26 00:13:44 |   27.15    
2020-08-26 00:08:44 |    27.17    
2020-08-26 00:03:44   |  27.16    
2020-08-25 23:58:44   |  27.07    
2020-08-25 23:53:44   |  27.09    
2020-08-25 23:48:44   |  27.04    
2020-08-25 23:43:44    | 27.03    
2020-08-25 23:38:44 |    27    
2020-08-25 23:33:44 |    27.03    
2020-08-25 23:28:44 |    27.06    
2020-08-25 23:23:44 |    27.1    


