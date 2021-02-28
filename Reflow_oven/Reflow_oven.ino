/*
   Rui Santos
   Complete Project Details http://randomnerdtutorials.com
*/

// include TFT and SPI libraries
#include <TFT.h>
#include <SPI.h>
#include<stdio.h>
#include<stdlib.h>

// ThermoCoupler
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"

#define UPDATE_DATA_TIME 200 // ms update data rate tft
#define DELAY 10 // ms 
#define I2C_ADDRESS (0x67)

// Screen 
#define TFT_WIDTH 160
#define TFT_LENGTH 128
#define TFT_X1 4
#define TFT_X2 160-TFT_X1
#define TFT_Y1 4
#define TFT_Y2 95

#define SSR_PIN 7

// pin definition for Arduino UNO TFT
#define cs   10
#define dc   9
#define rst  8

unsigned long init_time, t1, t2;

  //generate a random color
  int redRandom = 50;
  int greenRandom = 125;
  int blueRandom = 125;

uint16_t solder_profile[] = {25, 100, 150, 183, 230, 183};
uint32_t solder_time[] =    {0, 30, 120, 150, 210, 240};



// create an instance of the library
TFT TFTscreen = TFT(cs, dc, rst);
Adafruit_MCP9600 mcp;

void time_to_ms(){
  for(uint8_t i = 0; i < sizeof(solder_time)/sizeof(solder_time[0]); i ++){
    solder_time[i] = solder_time[i]*1000; 
    Serial.println(solder_time[i]);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);
  //time_to_ms();

  /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
    if (! mcp.begin(I2C_ADDRESS)) {
        Serial.println("Sensor not found. Check wiring!");
        while (1);
    }
  mcp.setADCresolution(MCP9600_ADCRESOLUTION_12);
  mcp.setThermocoupleType(MCP9600_TYPE_K);
  mcp.setFilterCoefficient(3); // avg over three meas
  mcp.enable(true);

  //initialize the library
  TFTscreen.begin();
  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  TFTscreen.setTextSize(0.5);
  TFTscreen.stroke(redRandom, greenRandom, blueRandom);
}

void static_lines() {
  TFTscreen.line(TFT_X1, TFT_Y2, TFT_X2, TFT_Y2);
  TFTscreen.line(TFT_X1, TFT_Y2, TFT_X1, TFT_Y1);
}

void reflow_data(float ref, float meas, float error){
  char result[8];
   
  TFTscreen.fill(0,0,0);
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.rect(70,100,40,30);

  TFTscreen.stroke(redRandom, greenRandom, blueRandom);

  dtostrf(ref, 5, 2, result);
  TFTscreen.text(result, 70, 100);
  dtostrf(meas, 5, 2, result);
  TFTscreen.text(result, 70, 110);
  dtostrf(error, 5, 2, result);
  TFTscreen.text(result, 70, 120);
}

void reflow_text() {
  TFTscreen.text("Temp ref", 4, 100);
  TFTscreen.text("Temp meas", 4, 110);
  TFTscreen.text("Temp error", 4, 120);
}

void reflow_graph() {
  float time_max = solder_time[(sizeof(solder_time) / sizeof(solder_time[0])) - 1];
  float temperature_max = max_temperature(solder_profile, (sizeof(solder_profile) / sizeof(solder_profile[0])) - 1);
  float px_time = time_max / (TFT_X2 - TFT_X1);
  float px_temp = temperature_max / (TFT_Y2 - TFT_Y1);
  uint8_t x[2] = {TFT_X1, TFT_X1}, y[2] = {TFT_Y2, TFT_Y2};

  for (uint8_t i = 0; i < 5; i++) {
    if (i == 0) {
      x[0] = x[0] + solder_time[i] / px_time;
      x[1] = x[1] + solder_time[i + 1] / px_time;
      y[0] = y[0] - solder_profile[i] / px_temp;
      y[1] = y[1] - solder_profile[i + 1] / px_temp;
    }
    else {
      x[0] = x[1];
      x[1] = TFT_X1 + solder_time[i+1]/px_time;
      y[0] = y[1];
      y[1] = TFT_Y2 - solder_profile[i + 1] / px_temp;
    }
    TFTscreen.line(x[0], y[0], x[1], y[1]);
    /*
    Serial.print(x[0]);
    Serial.print(" ");
    Serial.println(y[0]);
    Serial.print(x[1]);
    Serial.print(" ");
    Serial.println(y[1]);
    Serial.println("");
    */
  }
}


float ref_calculator(uint32_t present_time){
  // y = ax + b
  uint8_t index;
  float  a, temp, b, dummy;

  float t_sek = (float)present_time/1000;
  
  for(index = 0; index < sizeof(solder_time)/sizeof(solder_time[0])-1 ; index++){
    if(t_sek < solder_time[index]){
    //temp = solder_time[index];
    break;
    }
  }
  a = (float)((int)solder_profile[index]-(int)solder_profile[index-1])/(float)(solder_time[index]-solder_time[index-1]);
  b = (float)solder_profile[index] - (float)solder_time[index]*a;
  temp = a*t_sek + b;

  return temp;
  /*
  Serial.print(index);
  Serial.print(" ");
  Serial.print(a);
  Serial.print(" ");
  Serial.print(b);
  Serial.print(" ");
  Serial.print(temp);
  Serial.print(" ");
  Serial.println(t_sek); 
  */
}


uint16_t max_temperature(uint16_t *ptAr, uint8_t len) {
  uint16_t temp = *ptAr;
  for (uint8_t i = 0; i < len; i++) {
    if (temp < *ptAr) {
      temp = *ptAr;
    }
    ptAr++;
  }
  return temp;
}

void plot_temp(float temp, uint32_t present_time){
  float time_max = solder_time[(sizeof(solder_time) / sizeof(solder_time[0])) - 1];
  float temperature_max = max_temperature(solder_profile, (sizeof(solder_profile) / sizeof(solder_profile[0])) - 1);
  float px_time = time_max / (TFT_X2 - TFT_X1);
  float px_temp = temperature_max / (TFT_Y2 - TFT_Y1);
  float t_sek = (float)present_time/1000;
  uint8_t x = TFT_X1, y = TFT_Y2;

  x = x + (uint8_t)(t_sek / px_time);
  y = y - (uint8_t)(temp / px_temp);

  TFTscreen.stroke(250, 0, 0);
  TFTscreen.point(x, y);
  TFTscreen.stroke(redRandom, greenRandom, blueRandom);
  /*
  Serial.print(x);
  Serial.print(" ");
  Serial.println(y);
  */
}


void ontime(uint8_t duty, uint16_t *_time){ 
  uint16_t procentage = 1000*duty/100;
  *_time = procentage;
  *_time++;
  *_time = 1000-procentage;  
}


uint8_t dummy = 0;
uint8_t test = 0;
float reference, temperature, temp_error;
uint16_t tt[2];;

uint32_t update_data_time; 


void loop() {
  TFTscreen.background(0, 0, 0); // Erase background
 
  reflow_graph();
  reflow_text();
  static_lines();
  reflow_data(1,2,3);

  init_time = millis();
  t1 = init_time;
  update_data_time = init_time + UPDATE_DATA_TIME;
  while(t1 < init_time + (solder_time[sizeof(solder_time)/sizeof(solder_time[0])-1])*1000){
    t1 = millis();
    reference = ref_calculator(t1-init_time);
    temperature = mcp.readThermocouple();
    temp_error = reference - temperature; 
    plot_temp(temperature , t1-init_time);
    if(t1 > update_data_time){
      reflow_data(reference, temperature, temp_error); 
      update_data_time = millis() + UPDATE_DATA_TIME;
    }
    if(temperature < reference){
      digitalWrite(SSR_PIN,HIGH);
    }else{
      digitalWrite(SSR_PIN,LOW);
    }
    delay(20);
    }
  Serial.println("Stop");
  while(1);
}
