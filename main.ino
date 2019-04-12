// License: WTFPL
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <dht.h>
extern "C" {
#include "user_interface.h"
}

/*
 * This is a simple controller for wood boilers like the Atmos wood gasification boilers:
These wood boilers will usually be started as follwing:
1. Turn fan knob to "firing up" position
2. start fire
3. wait some time for boiler to get warm enough that you can turn it off (40 minutes sometimes)
4. Turn knob to some more or less arbitrary "stop" position
5. Discover that boiler either turned off too early is still running with no fuel left (dragging cold air, wasting heat)
Added bonus of the analog thermostat: The fan will turn on and off for some period of time after fuel is used up, which is not only annoying to listen to but also causes unnessecary wear on the fan.

With this box, which is installed WITHOUT removing any existing saftey features! The procedure is:
1. Press button
2. Start fire
Added bonus: You can monitor temperature by connecting to the box over wifi (the wifi part is optional, the box will work without).
I use the monitor feature to draw a graph on a picture frame in the office, then in winter, I can see when it is optimal to refuel, if required (when temperature starts to drop, you have 20 minutes or so to refuel).

The box will turn off the furnace when the fuel is used up.
It will turn off after a set period of time (default 1 hour) if the fire didn't catch.
It will NOT turn on again when temperature rises (it will creep up a little after the fuel is used up, before falling for good)


Installation:
1. Lift up the top of the furnace, identify the live wire going to the fan.
2. This wire terminates in a scew-terminal row, pull it out, reroute to the movable contact of the relay.
3. Route new wire from screw terminal row onto the normally-open contact of the relay.
4. Mount in rugged, certified box, use a rugged pushbutton, I used an arcade button.
5. Attach thermocouple to flue-pipe, I planned to drill a hole and do it right, but ended up using alu tape and that works just fine.
Look! 5 steps! Just as easy as originally starting up the damn thing! :)

Hardware:
D1 mini
a max 6675 thermocouple connected to pins DO:15 CS:12 CLK:13
a (transistor controlled) relay connected to pin 5 
a DHT11 connected to pin 0 (optional part)

Setting minTemp to 200:
write to socket, 2 passes:
thePassword\n
sa200\n

Setting startingTime to 3600 seconds
write to socket, 2 passes:
thePassword\n
sb3600\n

Example:
echo "thePassword" | nc the.ip.addr.ess 9000
echo "sa200" | nc the.ip.addr.ess 9000

 */

#include <max6675.h>

const int btnOnMinTimeDown = 150; // How many MS must the button be pressed down before turning on
const int btnOffMinTimeDown = 2000; // How many MS must the button be pressed down before turning off

const char* ssid = "the name of the wifi";
const char* password = "the paassword for said wifi";

WiFiServer wifiServer(9000);

const uint8_t RELAYPIN = 5;
const uint8_t BTNPIN = 4;
const uint8_t LEDPIN = 2;
const uint8_t LEDON = 0;
const uint8_t LEDOFF = 1;



int thermoDO = 15;
int thermoCS = 12;
int thermoCLK = 13;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

WiFiClient client;

dht DHT;

#define DHT22_PIN 0

char buf[128];
char tempStr[12];

const uint8_t STOPPED = 0;
const uint8_t RUNNING = 1;


double minTemp = 0;
int startingTime = 0;

int grace = 0;
uint8_t state = STOPPED;

void fanOff() {
  digitalWrite(RELAYPIN, 0);
  state = STOPPED;
  grace = 0;
}

void fanOn() {
  digitalWrite(RELAYPIN, 1);
  state = RUNNING;
  grace = startingTime;
}

const int confMinTempBeginEepromAddr = 0;
const int confStartTimeBeginEepromAddr = sizeof(double);

char cmdStr[32];

void setup(void) {

  EEPROM.begin(512);
  digitalWrite(LEDPIN, LEDON);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  digitalWrite(RELAYPIN, 0);
  pinMode(LEDPIN, OUTPUT);
  pinMode(RELAYPIN, OUTPUT);
  pinMode(BTNPIN, INPUT);

  // Start wifi stuff
  wifiServer.begin();
  client = wifiServer.available();

  cmdStr[0]=0;
  
  EEPROM.get( confMinTempBeginEepromAddr, minTemp );
  EEPROM.get( confStartTimeBeginEepromAddr, startingTime );
  

  delay(500);
}

int ems = 0;

char inhum[8];
char ints[8];

bool first=true;

int uptime=0;
int numSessions=0;

int cmdPos=0;
int inByte=0;

bool armed=false;

void loop(void) {


  int btime = 0;
  while (digitalRead(BTNPIN)) {
    btime++;
    delay(1);

    if (btime > btnOffMinTimeDown) {
      fanOff();
    } else if (btime > btnOnMinTimeDown) {
      fanOn();
    }
  }


  ems += 10;
  if (ems == 1000) {
    ems = 0;
    double temp = thermocouple.readCelsius();

    DHT.read22(DHT22_PIN);

    if (grace) {
      grace--;
    } else if (temp < minTemp) {
      fanOff();
    }

    uptime++;


    if(WiFi.status() == WL_CONNECTED ) {
       digitalWrite(LEDPIN, LEDOFF);
    } else {
      digitalWrite(LEDPIN, LEDON);
    }

    //Check socket
    if (client && client.connected()) {
      cmdPos=0;
      
      while ( (inByte=client.read()) != -1) {
        if( (char)inByte=='\n' ) {
          cmdStr[cmdPos]=0;
        } else {
          cmdStr[cmdPos]=(char)inByte;
        }
        cmdPos++;
        if(cmdPos==32) {
          cmdStr[0]=0;
          cmdPos=0;
        }
      }

      if(strcmp(cmdStr, "thePassword") == 0 ) {
        armed=true;
        client.write("armed\n");
      }

      if(armed) {
        
        if( cmdStr[0] == 's' && cmdStr[1] == 'a') {
          minTemp = (double)atof(cmdStr+2);
          cmdStr[0]=0;
          EEPROM.put( confMinTempBeginEepromAddr, minTemp );
          EEPROM.commit();
          armed=false;
          client.write("minTemp set\n");
        } else if( cmdStr[0] == 's' && cmdStr[1] == 'b') {
          startingTime = atoi(cmdStr+2);
          cmdStr[0]=0;
          EEPROM.put( confStartTimeBeginEepromAddr, startingTime );
          EEPROM.commit();
          armed=false;
          client.write("startingTime set\n");
        }
      }

      uint32_t memFree = system_get_free_heap_size();

      if(first) {
        first=false;
        numSessions++;
        dtostrf(minTemp, 4, 2, tempStr);

        sprintf(buf, "{\"type\":\"sysinfo\",\"numSessions\":%i,\"confMinTemp\":%s,\"confStartTime\": %i,\"uptime\":%i}\n", numSessions, tempStr, startingTime, uptime);
        client.write(buf);
      }

      dtostrf(temp, 4, 2, tempStr);
      dtostrf(DHT.temperature, 4,2, ints);
      dtostrf(DHT.humidity, 4,2, inhum);
      sprintf(buf, "{\"type\":\"update\",\"free\":%i,\"temp\":%s,\"state\":%i,\"grace\":%i,\"intemp\":%s,\"inhum\":%s}\n", memFree, tempStr, state, grace,ints, inhum);
      client.write(buf);
    } else {
      if (client) {
        client.stop();
      }
      first=true;
      client = wifiServer.available();
    }

  }

  delay(10);

}
