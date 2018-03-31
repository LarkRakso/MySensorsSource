//#include <SoftwareSerial.h>
//Define software serial port
//SoftwareSerial mySerial(2, 3); // RX, TX

// Enable debug prints
//#define MY_DEBUG
unsigned long SEND_FREQUENCY = 15000; // Minimum time between send (in milliseconds). We don't want to spam the gateway.
// ******* POWER MEASUREMENT DECLARATIONS *******
#define DIGITAL_INPUT_SENSOR 3  // The digital input you attached your light sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 1000       // Nummber of blinks per KWH of your meeter
#define MAX_WATT 50000          // Max watt value to report. This filetrs outliers.

double ppwh = ((double)PULSE_FACTOR) / 1000; // Pulses per watt hour
bool pcReceived = false;
volatile unsigned long pulseCount = 0;
volatile unsigned long lastBlink = 0;
volatile unsigned long watt = 0;
unsigned long oldPulseCount = 0;
unsigned long oldWatt = 0;
double oldKwh;
unsigned long lastSend;

// a string to hold incoming data
String inputString = "";
// ******* END POWER MEASUREMENT DECLARATIONS *******


// ******* LIGHT LEVEL MEASUREMENT DECLARATIONS *******
#define LIGHT_SENSOR_ANALOG_PIN 0
int16_t lastLightLevel;
// ******* END LIGHT LEVEL MEASUREMENT DECLARATIONS *******

// ******* TEMPERATURE MEASUREMENT DECLARATIONS *******
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress garageThermometer, outdoorThermometer;
// variables to hold temperature
float lasttempC_garage, lasttempC_outdoor;

// ******* END TEMPERATURE MEASUREMENT DECLARATIONS *******



void setup() {
#if defined(MY_DEBUG)
  while (!Serial) {   
    ; // wait for serial port to connect. Needed for native USB
  }
#else
  delay(10000);
#endif
    
  //Set baud rate and initialize software serial port
  Serial1.begin(9600);

  // Use the internal pullup to be able to hook up this sketch directly to an energy meter with S0 output
  // If no pullup is used, the reported usage will be too high because of the floating pin
  pinMode(DIGITAL_INPUT_SENSOR, INPUT_PULLUP);

  // Start up the library for 1-wire Dallas Sensor
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them. It might be a good idea to 
  // check the CRC to make sure you didn't get garbage. The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  oneWire.reset_search();
  // assigns the first address found to insideThermometer
  if (!oneWire.search(garageThermometer)) {
#if defined(MY_DEBUG)
      Serial.println("Unable to find address for garageThermometer");
#endif
  }
  // assigns the seconds address found to outsideThermometer
  if (!oneWire.search(outdoorThermometer)) {
#if defined(MY_DEBUG)
      Serial.println("Unable to find address for outdoorThermometer");
#endif
  }

#if defined(MY_DEBUG)
    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(garageThermometer);
    Serial.println();
  
    Serial.print("Device 1 Address: ");
    printAddress(outdoorThermometer);
    Serial.println();
#endif
  
  // set the resolution to 9 bit per device
  sensors.setResolution(garageThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outdoorThermometer, TEMPERATURE_PRECISION);

#if defined(MY_DEBUG)
    Serial.print("Device 0 Resolution: ");
    Serial.print(sensors.getResolution(garageThermometer), DEC); 
    Serial.println();
  
    Serial.print("Device 1 Resolution: ");
    Serial.print(sensors.getResolution(outdoorThermometer), DEC); 
    Serial.println();
#endif
  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, RISING);
  lastSend = millis();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void loop() {
  double kWh = ((double)pulseCount / ((double)PULSE_FACTOR));
  unsigned long now = millis();
  // Only send values at a maximum frequency or woken up from sleep
  bool sendTime = now - lastSend > SEND_FREQUENCY;

//Serial.println(sendTime);
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
//    Serial.print("New char: ");
//    Serial.println(inChar);
    // add it to the inputString:
    inputString += inChar;
//    Serial.print("inputString: ");    
//    Serial.println(inputString);
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      inputString.remove(0, 3);
#if defined(MY_DEBUG)
      Serial.print("Formatted string: ");
      Serial.print(inputString);
#endif
      pulseCount = inputString.toInt();
      pcReceived = true;
      inputString = "";
    }
  }    
   
  if (pcReceived && sendTime) {
    // POWER MEASUREMENT
    // New watt value has been calculated
    if (watt != oldWatt) {
      // Check that we dont get unresonable large watt value.
      // could hapen when long wraps or false interrupt triggered
      if (watt < ((unsigned long)MAX_WATT)) {
        Serial1.print("P:");  // Momentary power consumption
        Serial1.println(watt);  // Send watt value to gw
      }
      oldWatt = watt;
    }

    // Pulse count has changed
    if (pulseCount != oldPulseCount) {
      Serial1.print("C:"); //Pulse Count
      Serial1.println(pulseCount);  // Send pulse count value to gw
      oldPulseCount = pulseCount;
      if (kWh != oldKwh) {
        Serial1.print("E:"); //Energy Consumption
        Serial1.println(kWh, 4);  // Send kwh value to gw
        oldKwh = kWh;
      }
    }
    //END - POWER MEASUREMENT

    //LIGHT SENSOR
    int16_t lightLevel = int(float((1023-analogRead(LIGHT_SENSOR_ANALOG_PIN)))/10.23);   
    if (lightLevel != lastLightLevel) {
      Serial1.print("L:");
      Serial1.println(lightLevel);
      lastLightLevel = lightLevel;
    }
    //END - LIGHT SENSOR

    //TEMPERATURE
    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus
    sensors.requestTemperatures(); // Send the command to get temperatures
    float tempC_garage = sensors.getTempC(garageThermometer);
    if (tempC_garage != lasttempC_garage) {
      Serial1.print("T1:");
      Serial1.println(tempC_garage);
      lasttempC_garage = tempC_garage;
    }
    
    float tempC_outdoor = sensors.getTempC(outdoorThermometer);
    if (tempC_outdoor != lasttempC_outdoor) {
      Serial1.print("T2:");
      Serial1.println(tempC_outdoor);
      lasttempC_outdoor = tempC_outdoor;
    }
  
    //https://www.milesburton.com/Dallas_Temperature_Control_Library

    //END - TEMPERATURE

    //DEBUG
    #if defined(MY_DEBUG)
      Serial.print("Watt: ");
      Serial.println(watt);
      Serial.print("kWh: ");
      Serial.println(kWh,4);
      Serial.print("Pulse Count: ");
      Serial.println(pulseCount);
      Serial.print("Light Level: ");
      Serial.println(lightLevel);
      Serial.print("Garage temperature: ");
      Serial.println(tempC_garage);
      Serial.print("Outdoor temperature: ");
      Serial.println(tempC_outdoor);
    #endif
    
    lastSend = now;
  } else if (sendTime && !pcReceived) {
    // No count received. Try requesting it again
    Serial1.println("R:?");
    lastSend=now;
  }
  
}

void onPulse() {
  unsigned long newBlink = micros();
  unsigned long interval = newBlink - lastBlink;
  if (interval < 10000L) { // Sometimes we get interrupt on RISING
    return;
  }
  watt = (3600000000.0 / interval) / ppwh;
  lastBlink = newBlink;
  pulseCount++;
}

