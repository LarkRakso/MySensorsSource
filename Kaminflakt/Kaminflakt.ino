/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
 * http://www.mysensors.org/build/temp
 */


// Enable debug prints to serial monitor
//#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define COMPARE_TEMP 1 // Send temperature only if changed? 1 = Yes 0 = No

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define fanPower 7
#define fanSpeed 5
unsigned long SLEEP_TIME = 3000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature;
bool receivedConfig = false;
bool metric = true;
// Initialize temperature message
#define temperatureId 0
#define fanStateId 1
#define speedId 2
#define onTempId 3
#define hysteresisId 4

MyMessage msgTemperature(temperatureId,V_TEMP);
MyMessage msgFanState(fanStateId, );
MyMessage msgFanSpeed(speedId, );
MyMessage msgOnTemp(onTempId, );
MyMessage msqHysteresis(hysteresisId, );

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()  
{ 
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temperature Sensor", "1.1");

  // Present all sensors to controller
  present(temperatureId, S_TEMP);
}

void loop()     
{     
  controlFan(readTemperature());
  sleep(SLEEP_TIME);
}


float readTemperature() {
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  // int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  //  sleep(conversionTime);
  sleep(200);

    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(0):sensors.getTempFByIndex(0)) * 10.)) / 10.;
    Serial.println(temperature);
    // Only send data if temperature has changed and no error
    #if COMPARE_TEMP == 1
    if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
    #else
    if (temperature != -127.00 && temperature != 85.00) {
    #endif

      // Send in the new temperature
      send(msg.setSensor(0).set(temperature,1));
      // Save new temperatures for next compare
      lastTemperature = temperature;
    }
    return temperature;
}

void controlFan(float temperature) {
  static bool running;

  if (!running && (temperature >= 30)) {
    digitalWrite(fanPower, HIGH);
    Serial.print("FAN ON");
    running = true;
  } else if (running && (temperature < 28)) {
    digitalWrite(fanPower, LOW);
    Serial.print("FAN OFF");
    running = false;
  }
}

