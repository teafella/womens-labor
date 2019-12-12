
/*
  Women's Labor: Iron
  By: Ronald Sardarian @ Sleepy Ciruits
  
  Controlls sensors within an iron and sends data along
  to a PI via serial to be used in an audio synthesis engine.

  Triggering:
  Proximity Sensing w/ the VCNL4040 IR based sensor
  Material Detection w/ AS7265X Spectral Triad
  
*/

#define DEBUG true
#define USE_DISTANCE_SENSOR 1

#include <Wire.h>
#include "SparkFun_VCNL4040_Arduino_Library.h" // //Click here to get the library: http://librarymanager/All#SparkFun_VCNL4040
#include "SparkFun_AS7265X.h" //Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <ComponentObject.h>
#include <RangeSensor.h>
#include <SparkFun_VL53L1X.h>
#include <vl53l1x_class.h>
#include <vl53l1_error_codes.h>
#include "SparkFun_VL53L1X.h"

AS7265X spectral_sensor;
const int SPECTRUM_SIZE = 18;
float spectrum[SPECTRUM_SIZE] = {0};

VCNL4040 proximity_sensor;
#define PROXIMITY_INTERRUPT_PIN 5

SFEVL53L1X distance_sensor;

#define PIEZO_PIN A0

void setup()
{
  Serial.begin(9600);
  InitSensors();
}

void loop()
{ 
//  Serial.println(ReadPiezo());
  GetDistanceSensor();
  if(GetProximityValue() > 1400){ // can also use GetProximityInterrupt() Here
    GetSpectralValues();
  }
  else{
    SetAllBulbs(false);
  }
  delay(10);
}


void InitSensors(){
  Wire.begin(); //Join i2c bus

  InitPiezo();
  
    InitProximitySensor();
    InitSpectralSensor();
     if(USE_DISTANCE_SENSOR){
      InitDistanceSensor();
    }
   Wire.setClock(400000);
   Serial.print("All Sensors Initialized");
}

//Piezo Functions

void InitPiezo(){
  pinMode(PIEZO_PIN, INPUT);
}

int ReadPiezo(){
  return analogRead(PIEZO_PIN);
}

//VCNL4040 Proximity Sensor Functions

void InitProximitySensor(){
  if (proximity_sensor.begin() == false)
  {
    Serial.println("VCNL4040 not found. Please check wiring.");
    while (1); //Freeze!
  }

  //setup interrupt pin
  pinMode(PROXIMITY_INTERRUPT_PIN, INPUT);

  int interrupt_high_threshold = 50;
  int interrupt_low_threshold = 45;

   //If sensor sees more than this, interrupt pin will go low
  proximity_sensor.setProxHighThreshold(interrupt_high_threshold); 

  //The int pin will stay low until the value goes below the low threshold value
  proximity_sensor.setProxLowThreshold(interrupt_low_threshold); 

  proximity_sensor.setProxInterruptType(VCNL4040_PS_INT_BOTH); 
  proximity_sensor.enableProxLogicMode(); 
  
  Serial.println("VCNL4040 Proximity Sensor Initialized");
}

unsigned int GetProximityValue(){
  //Get proximity value. The value ranges from 0 to 65535
  //so we need an unsigned integer or a long.
  unsigned int proxValue = proximity_sensor.getProximity(); 
  if(DEBUG){
//      Serial.print("Proximity Value: ");
//      Serial.print(proxValue);
//      Serial.println();
  }
  
  return proxValue;
}

bool GetProximityInterrupt(){
  int val = digitalRead(PROXIMITY_INTERRUPT_PIN);
  return val;
}

//VL53L1X Distance Sensor (Long Range)
void InitDistanceSensor(){
  Serial.println(distance_sensor.begin());
  if (distance_sensor.begin() != 0) //Begin returns 0 on a good init
  {
    Serial.println("VL53L1X Not Found, Please Check Your Wiring");
//    while(1);
  }

  Serial.println("VL53L1X Initialized");
}

void GetDistanceSensor(){
  if( USE_DISTANCE_SENSOR ){
//    Serial.println("STARTING RANGING");
    distance_sensor.startRanging(); //Write configuration bytes to initiate measurement
    int distance = distance_sensor.getDistance(); //Get the result of the measurement from the sensor
    distance_sensor.stopRanging();
//    Serial.println("STOPPED RANGING");
  
    if(DEBUG){
      Serial.print("Distance(mm): ");
      Serial.print(distance);
      Serial.println();
    }
  
//    return distance;
  }
//  return 0;
}


//Spectral Sensor Functions

void InitSpectralSensor(){
  if(spectral_sensor.begin() == false)
  {
    Serial.println("AS7265x Not Found. Please check wiring.");
    while(1);
  }
  
   spectral_sensor.setMeasurementMode(AS7265X_MEASUREMENT_MODE_6CHAN_CONTINUOUS); //All 6 channels on all devices
   spectral_sensor.setIntegrationCycles(1); 
   spectral_sensor.disableIndicator();

     Serial.println("A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W");
  Serial.println("AS7265x Spectral Triad Initialized");
}

int spectral_read_index = 0; // index of round robin reading
void GetSpectralValues(){ 
  // Read the 18 channels of spectral light over I2C using the Spectral Triad
  // Done in a round robin fashion giving priority to fast interrupt based proximity reading trigger
  if(spectral_sensor.dataAvailable() == false) {
    return;
  }
  SetAllBulbs(true);
  
  spectrum[spectral_read_index] = GetSpectralValue(spectral_read_index);


  if(DEBUG){
    Serial.print(spectrum[spectral_read_index]);
    Serial.println();
  }
  
  spectral_read_index +=1;
  if(spectral_read_index >= SPECTRUM_SIZE){
    spectral_read_index = 0;
  }
  
}

float GetSpectralValue(int x){
  if( x== 0){
    spectral_sensor.getCalibratedA();
  }
  else if( x == 1){
    spectral_sensor.getCalibratedB();
  }
  else if( x == 2){
    spectral_sensor.getCalibratedC();
  }
  else if( x == 3){
    spectral_sensor.getCalibratedD();
  }
  else if( x == 4){
    spectral_sensor.getCalibratedE();
  }
  else if( x == 5){
    spectral_sensor.getCalibratedF();
  }
  else if( x == 6){
    spectral_sensor.getCalibratedG();
  }
  else if( x == 7){
    spectral_sensor.getCalibratedH();
  }
  else if( x == 8){
    spectral_sensor.getCalibratedI();
  }
  else if( x == 9){
    spectral_sensor.getCalibratedJ();
  }
  else if( x == 10){
    spectral_sensor.getCalibratedK();
  }
  else if( x == 11){
    spectral_sensor.getCalibratedL();
  }
  else if( x == 12){
    spectral_sensor.getCalibratedR();
  }
  else if( x == 13){
    spectral_sensor.getCalibratedS();
  }
  else if( x == 14){
    spectral_sensor.getCalibratedT();
  }
  else if( x == 15){
    spectral_sensor.getCalibratedU();
  }
  else if( x == 16){
    spectral_sensor.getCalibratedV();
  }
  else if( x == 17){
    spectral_sensor.getCalibratedW();
  }
}

bool bulbs_on = false;

void SetAllBulbs(bool on){
  if(on && !bulbs_on){
   spectral_sensor.enableBulb(AS7265x_LED_WHITE);
   spectral_sensor.enableBulb(AS7265x_LED_IR);
   spectral_sensor.enableBulb(AS7265x_LED_UV);
   bulbs_on = true;
  }
  if(!on && bulbs_on){
    spectral_sensor.disableBulb(AS7265x_LED_WHITE);
   spectral_sensor.disableBulb(AS7265x_LED_IR);
   spectral_sensor.disableBulb(AS7265x_LED_UV);
   bulbs_on = false;
  }
}
