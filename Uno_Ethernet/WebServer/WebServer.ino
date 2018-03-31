#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include "HomeEasy.h"

/***************************
Pins used in this project:
D2 = software serial port - RX
D3 = software serial port - TX
D7 = OneWire DS1820
D8 = HomeEasy - RX
D9 = HomeEasy - TX
***************************/

//Input pins
int pinA = 4;
int pinB = 5;
int pinC = 6;

//OneWire
#define NUMBER_OF_DEVICES 1
OneWire  ds(7);  // on pin 4 (a 4.7K resistor is necessary)
byte addr1[8];
int16_t celsius;

unsigned long currentTime = 0;
unsigned long startTime = 0;
const unsigned int intervalTime = 3000;  
  
//HomeEasy
HomeEasy homeEasy;

//Software serial port and data variables
SoftwareSerial mySerial(2, 3); // RX, TX
String received_data;
String temp1, temp2, daylight, energy, power;

int index, oldindex = 0;

  
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,99);

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

#define PREFIX ""

WebServer webserver(PREFIX, 80);

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void DefaultOutput(WebServer &server, WebServer::ConnectionType type, bool addControls = false)
{
  P(htmlHead) =
    "<html>"
    "<head>"
    "<title>Arduino Web Server</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: sans-serif }"
    "H1 { font-size: 20pt; text-decoration: underline }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);

  if (addControls)
    server << "<form action='" PREFIX "/form' method='post'>";

  server << "<h1>Information!</h1><p>";

  server << "Temperature 1 - " << temp1 << "<br>";
  server << "Temperature 2 - "  << temp2 << "<br>";
  server << "Temperature 3 - "  << celsius/16.0 << "<br>";
  server << "Pin A - " << digitalRead(pinA) << "<br>";
  server << "Pin B - " << digitalRead(pinB) << "<br>";
  server << "Pin C - " << digitalRead(pinC) << "<br>";  
  server << "Daylight - "  << daylight << "<br>";
  server << "Energy (kWh) -  " << energy << "<br>";
  server << "Current power (W) -  " << power << "<br>";  
  
  server << "</p>";

  if (addControls)
    server << "<input type='submit' value='Read'/></form>";

  server << "</body></html>";
}

void GetValue(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{

  if (type == WebServer::GET)
  {
    server.httpSuccess();
    server << "<root>"
      	"<eth_temp1>" << temp1 << "</eth_temp1>"
      	"<eth_temp2>" << temp2 << "</eth_temp2>"
      	"<eth_temp3>" << celsius/16.0 << "</eth_temp3>"
        "<eth_pinA>" << digitalRead(pinA) << "</eth_pinA>"
        "<eth_pinB>" << digitalRead(pinB) << "</eth_pinB>"
        "<eth_pinC>" << digitalRead(pinC) << "</eth_pinC>"       
      	"<eth_daylight>" << daylight << "</eth_daylight>"
      	"<eth_power>" << energy << "</eth_power>"
      	"<eth_energy>" << power << "</eth_energy>"
      "</root>\n";  
  }
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  bool repeat;
  char name[16], value[16];
  String Svalue;
  int protocol = -1;
  unsigned long sender = 0;
  unsigned int recipient = 0;
  byte cmd = 0;
  byte group = 0;
  int dim = 8;
  
  if (type == WebServer::GET)
  {
    P(htmlHead) =
        "<html>"
        "<head>"
        "<title>Arduino Web Server</title>"
        "<style type=\"text/css\">"
        "BODY { font-family: sans-serif }"
        "H1 { font-size: 20pt; text-decoration: underline }"
        "P  { font-size: 10pt; }"
        "</style>"
        "</head>"
        "<body>";
    
    int i;
    server.httpSuccess();
    server.printP(htmlHead);

    if (strlen(url_tail))
    {
      while (strlen(url_tail))
      {
        repeat = server.nextURLparam(&url_tail, name, 16, value, 16);
        if (repeat == URLPARAM_EOS)
        {
          //What to do here?
        }
        else
        {
          Svalue = String(value);
          if (name[0] == 'p')
          {
            protocol = Svalue.toInt();
          }
          else if (name[0] == 's')
          {
            sender = Svalue.toInt();
          }
          else if (name[0] == 'r')
          {
            recipient = Svalue.toInt();
          }
          else if (name[0] == 'c')
          {
            cmd = Svalue.toInt();
/*          server << name;
          server << "-";
          server << cmd;
          server << "<p>";            
*/  
          }
          else if (name[0] == 'g')
          {
            group = Svalue.toInt();
          }
          else if (name[0] == 'd')
          {
            dim = Svalue.toInt();      
          }
/*    Serial.print(name);
    Serial.print("-");
    Serial.println(value);
*/
        }
      }

/*
      //Check for valid signals
      server << "OK";
      server << "<p>";
*/
      switch (protocol)
      {
        case 0:
          homeEasy.sendSimpleProtocolMessage(sender, recipient, cmd);
          break;
        case 1:
          homeEasy.sendAdvancedProtocolMessage(sender, recipient, cmd, group);
          break;
        case 2:
          homeEasy.sendAdvancedProtocolDimmingMessage(sender, recipient, cmd, group, dim);            
          break;
        default:
          break;
      }
      
    }
    server << "</body></html>";
  }  
  else
  {
    DefaultOutput(server, type, true);
  }
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  DefaultOutput(server, type, false);  
}

/* DON'T USE THIS
// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);
*/

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //Initislize mySerial
  mySerial.begin(1200);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  //Start webserver
  webserver.begin();

  //Register pages
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("form", &formCmd);
  webserver.addCommand("GetValue", &GetValue);
  
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //Setup OneWire temperature sensor
  //Store all OneWire adresses in one array.
  ds.search(addr1);
  if (OneWire::crc8(addr1, 7) != addr1[7]) {
    for (int s = 0; s < 8; s++) {
      addr1[s] = 0; 
    }
    Serial.println("CRC is not valid for sensor 1!");
  }  
  ds.reset_search();
  delay(250);

  //Setup HomeEasy
  homeEasy = HomeEasy();
	
  homeEasy.registerSimpleProtocolHandler(printSimpleResult);
  homeEasy.registerAdvancedProtocolHandler(printAdvancedResult);
	
  homeEasy.init();

  //Init input pins
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinC, INPUT);
  
  Serial.println("Init completed");
}


void loop() {

  //Read software serial port
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c == '\n') {
      Serial.println(received_data);
      index = received_data.indexOf(';');
      temp1 = received_data.substring(oldindex, index);
      oldindex = index + 1;
      index = received_data.indexOf(';', index + 1);
      temp2 = received_data.substring(oldindex, index);
      oldindex = index + 1;
      index = received_data.indexOf(';', index + 1);
      daylight = received_data.substring(oldindex, index);
      oldindex = index + 1;
      index = received_data.indexOf(';', index + 1);
      energy = received_data.substring(oldindex, index);
      oldindex = index + 1;
      index = received_data.indexOf(';', index + 1);
      power = received_data.substring(oldindex, index);      
      received_data = "";

    } else {
      received_data += c;
    }
  }

  //Read from Temperature Sensor 
  currentTime = millis();
//  Serial.print("C: ");
//  Serial.println(currentTime);
  if (currentTime < startTime) { //Overflow
    startTime = 0;
  }
  if (currentTime - startTime > intervalTime) {
    //Continue the reading  
    celsius = ReadSensor(ds, addr1, 1);    
    startTime = millis();
    ReadSensor(ds, addr1, 0);
//  Serial.print("start: ");
//  Serial.println(startTime);

  }
  
  //Check for incoming server requests
  webserver.processConnection();

}

/**
 * Print the details of the advanced protocol message.
 */
void printAdvancedResult(unsigned long sender, unsigned int recipient, bool on, bool group)
{
	Serial.println();

	Serial.println("apm");
	
	Serial.print("s ");
	Serial.println(sender);
	
	Serial.print("r ");
	Serial.println(recipient);
	
	Serial.print(" ");
	Serial.println(on);
	
	Serial.print("g ");
	Serial.println(group);
	
	Serial.println();
}


/**
 * Print the details of the simple protocol message.
 */
void printSimpleResult(unsigned int sender, unsigned int recipient, bool on)
{
	Serial.println();

	Serial.println("spm");
	
	Serial.print("s ");
	Serial.println(sender);
	
	Serial.print("r ");
	Serial.println(recipient);
	
	Serial.print(" ");
	Serial.println(on);
	
	Serial.println();
}

int16_t ReadSensor(OneWire &ow, uint8_t *addr, byte continue_with_reading) {
    byte present = 0;
    byte data[12];
    byte type_s = 0;
    float celsius;     
    
    if (continue_with_reading == 0)
    {
      ow.reset();
      ow.select(addr);
      ow.write(0x44, 1);        // start conversion, with parasite power on at the end
      return -1;
//    delay(1000);     // maybe 750ms is enough, maybe not
    }
    else if (continue_with_reading == 1)
    {
      // we might do a ds.depower() here, but the reset will take care of it.
      present = ow.reset();
      ow.select(addr);    
      ow.write(0xBE);         // Read Scratchpad
    
      for (uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ow.read();
  //      Serial.print(data[i], HEX);
  //      Serial.print(" ");
      }
      // Convert the data to actual temperature
      // because the result is a 16 bit signed integer, it should
      // be stored to an "int16_t" type, which is always 16 bits
      // even when compiled on a 32 bit processor.
      int16_t raw = (data[1] << 8) | data[0];
      if (type_s) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
      }
  //    celsius = (float)raw / 16.0;
  //  return celsius;
      return raw;
    }
}

