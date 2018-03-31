#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG
#define FORCE_BUTTON 3  //Force-button connection pin
#define FORCE_MODE_DELAY 5   //Forced mode in minutes

uint8_t pwm = 128; //range: 0 - 255
float tempThreshold_Lower = 5.0;
float tempThreshold_Upper = 100.0;

//Fan is connected to pwm pin X
#define fanControl 8

// Data wire is plugged into pin 2 on the Arduino ?????????????????????????????????
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress Thermometer1, Thermometer2;
// variables to hold temperature
float TempInput, TempOutput;


void setup() {
#if defined(DEBUG)
  while (!Serial) {   
    ; // wait for serial port to connect. Needed for native USB
  }
#else
  delay(10000);
#endif
    
  //Input pin connected to button. Used to force operation of the fans at max speed.
  pinMode(FORCE_BUTTON, INPUT_PULLUP);

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
  // assigns the first address found to Thermometer1
  if (!oneWire.search(Thermometer1)) {
#if defined(DEBUG)
      Serial.println("Unable to find address for Thermometer1");
#endif
  }
  // assigns the seconds address found to Thermometer2
  if (!oneWire.search(Thermometer2)) {
#if defined(DEBUG)
      Serial.println("Unable to find address for Thermometer2");
#endif
  }

#if defined(DEBUG)
    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(Thermometer1);
    Serial.println();
  
    Serial.print("Device 1 Address: ");
    printAddress(Thermometer2);
    Serial.println();
#endif
  
  // set the resolution to 9 bit per device
  sensors.setResolution(Thermometer1, TEMPERATURE_PRECISION);
  sensors.setResolution(Thermometer2, TEMPERATURE_PRECISION);

#if defined(DEBUG)
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(Thermometer1), DEC); 
  Serial.println();
  
  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(Thermometer2), DEC); 
  Serial.println();
#endif

  attachInterrupt(digitalPinToInterrupt(FORCE_BUTTON), onPulse, RISING);
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

  if (TempInput + 2 < TempOutput) { //Increase the flow
    if (pwm < 254) {
      pwm++;
    }
    analogWrite(fanControl, pwm);
  } else if (TempInput < TempOutput) { //Decrease the flow
    if (pwm > 1) {
      pwm--;
    }
  } else {
    //Do nothing
  }
  Serial.print("Input temperature: ");
  Serial.println(TempInput);
  Serial.print("Output temperature: ");
  Serial.println(TempOutput);
  Serial.print("PWM percentage: ");
  Serial.println(pwm * 0.4);
  delay(60000); 

}


void onPulse() {
  uint8_t tmp_pwm = pwm;
  analogWrite(fanControl, 255);
  delay(FORCE_MODE_DELAY*60000);
  analogWrite(fanControl, tmp_pwm);
}

