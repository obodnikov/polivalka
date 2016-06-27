// This #include statement was automatically added by the Particle IDE.
#include <OneWire.h>
#include <mqtt.h>

#include "application.h"

#define MAX_NUMBER_OF_SENSORS_1WIRE 8

const int HUMIDY_MIN = 800;
const int HUMIDY_MAX = 2800;
const int MIN_1  = 60000;
const int MIN_5  = 300000;
const int MIN_10 = 600000;
const int MIN_15 = 900000;
const int MIN_20 = 1200000;



const int SensorsON = A3;
const int SensorH1  = A2;
const int SensorH2  = A4;
const int SensorL1  = A5;
const int SensorT1  = D2;
const int powerSW   = D3;





// Number and list of
byte numberOfSensors=0;
//byte typeOfSensor[MAX_NUMBER_OF_SENSORS_1WIRE];
byte addrOfSensor[8][MAX_NUMBER_OF_SENSORS_1WIRE];


OneWire ds = OneWire(SensorT1); //Sets Pin D1
unsigned long lastUpdate = 0;
double varTemp;






int dsAttempts = 0;
char szTemp[8]="";


int Humidy;
int Light;
char szHumidy[8]="";


void collectSensors() {

 unsigned long now = millis();
    // change the 3000(ms) to change the operation frequency
    // better yet, make it a variable!

        ds.reset_search();


        lastUpdate = now;
        byte i;
        byte present = 0;
        byte addr[8];


      while ( !ds.search(addr)) {

      // if we get here we have a valid address in addr[]
      // you can do what you like with it
      // see the Temperature example for one way to use
      // this basic code.

      // this example just identifies a few chip types
      // so first up, lets see what we have found

      // the first ROM byte indicates which chip family
      switch (addr[0]) {
        case 0x10:
          Serial.println("Chip = DS1820/DS18S20 Temp sensor");
          break;
        case 0x28:
          Serial.println("Chip = DS18B20 Temp sensor");
          break;
        case 0x22:
          Serial.println("Chip = DS1822 Temp sensor");
          break;
        case 0x26:
          Serial.println("Chip = DS2438 Smart Batt Monitor");
          break;
        default:
          Serial.println("Device type is unknown.");
          // Just dumping addresses, show them all
          //return;  // uncomment if you only want a known type
      }

      // Now print out the device address
      Serial.print("ROM = ");
      Serial.print("0x");
        Serial.print(addr[0],HEX);
      for( i = 1; i < 8; i++) {
        Serial.print(", 0x");
        Serial.print(addr[i],HEX);
      }

      // Show the CRC status
      // you should always do this on scanned addresses

      if (OneWire::crc8(addr, 7) != addr[7]) {
          Serial.println("CRC is not valid!");
          return;
      }
      for( i = 0; i < 8; i++) {
      addrOfSensor[i][numberOfSensors] = addr[i];
    }
      numberOfSensors++;

    }

      Serial.println("No more addresses.");
      Serial.println();
      ds.reset_search();
      Serial.println();

      ds.reset(); // clear bus for next use


}

unsigned int readHumidy(void)
{

  int i;
  unsigned int val = 0;
  unsigned int res = 0;

  for(i = 0; i < 99; i++) {
      delay(10);
      val += analogRead(SensorH1);
  }

  res = val/100;

  return res;
}

unsigned int readLight(void)
{

  int i;
  unsigned int val = 0;
  unsigned int res = 0;

  for(i = 0; i < 99; i++) {
      delay(10);
      val += analogRead(SensorL1);
  }

  res = val/100;

  return res;
}

double getTemp(byte tempSensor){



  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  double celsius, fahrenheit;
      for( i = 0; i < 8; i++) {
          addr[i] = addrOfSensor[i][tempSensor];
      }

  // this device has temp so let's read it

  ds.reset();               // first clear the 1-wire bus
  ds.select(addr);          // now select the device we just found
  // ds.write(0x44, 1);     // tell it to start a conversion, with parasite power on at the end
  ds.write(0x44, 0);        // or start conversion in powered mode (bus finishes low)

  // just wait a second while the conversion takes place
  // different chips have different conversion times, check the specs, 1 sec is worse case + 250ms
  // you could also communicate with other devices if you like but you would need
  // to already know their address to select them.

  delay(1000);     // maybe 750ms is enough, maybe not, wait 1 sec for conversion

  // we might do a ds.depower() (parasite) here, but the reset will take care of it.

  // first make sure current values are in the scratch pad

  present = ds.reset();
  ds.select(addr);
  ds.write(0xB8,0);         // Recall Memory 0
  ds.write(0x00,0);         // Recall Memory 0

  // now read the scratch pad

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE,0);         // Read Scratchpad
  if (type_s == 2) {
    ds.write(0x00,0);       // The DS2438 needs a page# to read
  }

  // transfer and print the values

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s == 2) raw = (data[2] << 8) | data[1];
  byte cfg = (data[4] & 0x60);

  switch (type_s) {
    case 1:
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
      celsius = (float)raw * 0.0625;
      break;
    case 0:
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      // default is 12 bit resolution, 750 ms conversion time
      celsius = (float)raw * 0.0625;
      break;

    case 2:
      data[1] = (data[1] >> 3) & 0x1f;
      if (data[2] > 127) {
        celsius = (float)data[2] - ((float)data[1] * .03125);
      }else{
        celsius = (float)data[2] + ((float)data[1] * .03125);
      }

  }

  return celsius;

}



void setup() {

int arVals[3];

    Serial.begin(9600);


    Time.zone(+4);
    Particle.syncTime();



     pinMode(SensorH1, INPUT);
     pinMode(SensorT1, INPUT);
     pinMode(SensorsON, OUTPUT);
     pinMode(SensorL1, INPUT);

     Particle.variable("varHumidy", &Humidy, INT);
     Particle.variable("varTemp", &varTemp, DOUBLE);
     Particle.variable("varLight", &Light, INT);


     collectSensors();




}

void loop() {

//  digitalWrite(SensorsON, HIGH);
//  delay(1000);




  varTemp = getTemp(0);
  if ( varTemp == -99.99)
  {
      delay(100);
      return;
  }



  Humidy = readHumidy();
  sprintf(szHumidy, "%04u", Humidy);
  sprintf(szTemp, "%2.2f", varTemp);

  Particle.publish("dsHumidy", szHumidy, PRIVATE);
  Particle.publish("dsTemp", szTemp, PRIVATE);

//  digitalWrite(SensorsON, LOW);


    delay(MIN_1);


}
