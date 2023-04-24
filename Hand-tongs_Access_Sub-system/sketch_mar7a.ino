#include <Arduino_MKRENV.h>
#include <Arduino_KNN.h>

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include "arduino_secrets.h"
#include <Wire.h>
#include <SeeedOLED.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// define the pins
#define S0 6 //color sensor
#define S1 7 //color sensor
#define S2 4 //color sensor
#define S3 5 //color sensor
#define sensorOut 3 //color sensor
#define OE 8 //color sensor
#define light_sensor A0 //light sensor
#define light_source 0 //light source for tongs storage bin
#define light_demo 1 //led for remind customer to use tongs
#define PIR_MOTION_SENSOR 2
#define light_threhold 120

//KNN color classifier
KNNClassifier myKNN(3);

float colors[3];
int y = 3;

const String BOARD_ORIENTATIONS[] = {
  "unknown",
  "hand",               // flat, processor facing up
  "tong",             // flat, processor facing down
  "nothing",  // landscape, analog pins down
};

//cloud connection
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

time_t rawtime;
struct tm  ts;
char buf[80];
int tong_shelf=0;
int LEDstate_pre = 0;
int LEDstate_curr = 1;


unsigned long lastMillis = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // while (!Serial);

  Wire.begin();
  SeeedOled.init();

  Serial.println("Board Orientation KNN");
  Serial.println();

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(light_source, OUTPUT);
  pinMode(light_demo, OUTPUT);
  pinMode(PIR_MOTION_SENSOR, INPUT);

  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);
  digitalWrite(OE,LOW);

  // float example1_0[] = {123.0, 155.0, 118.0};
  // // float example1_1[] = {253.0, 381.0, 259.0};
  // // float example1_2[] = {141.0, 192.0, 137.0};
  // float example1_3[] = {211.0, 317.0, 226.0};
  // // float example1_4[] = {300.0, 415.0, 288.0};
  // float example1_5[] = {236.0, 311.0, 215.0};
  // float example1_6[] = {495.0, 623.0, 405.0};
  // float example1_7[] = {607.0, 741.0, 487.0};
  // float example1_8[] = {598.0, 707.0, 463.0};
  // float example1_9[] = {645.0, 769.0, 489.0};
  // float example1_10[] = {215.0, 283.0, 197.0};
  // // float example1_11[] = {130.0, 182.0, 128.0};
  //heng
  float example1_0[] = {315.0, 392.0, 267.0};
  float example1_1[] = {314.0, 379.0, 261.0};
  float example1_2[] = {311.0, 387.0, 271.0};
  float example1_15[] = {356.0, 438.0, 295.0};
  float example1_16[] = {409.0, 488.0, 321.0};
  float example1_17[] = {311.0, 378.0, 256.0};
  float example1_18[] = {294.0, 362.0, 243.0};

  //kaiqi
  float example1_3[] = {237.0, 338.0, 232.0};
  float example1_4[] = {282.0, 368.0, 240.0};
  float example1_5[] = {287.0, 376.0, 250.0};
  //yang
  float example1_6[] = {115.0, 176.0, 120.0};
  float example1_7[] = {119.0, 166.0, 123.0};
  float example1_8[] = {126.0, 182.0, 128.0};
  //major
  float example1_9[] = {368.0, 444.0, 294.0};
  float example1_10[] = {267.0, 335.0, 235.0};
  // float example1_11[] = {426.0, 520.0, 340.0};
  // float example1_12[] = {466.0, 586.0, 383.0};
  // float example1_13[] = {452.0, 575.0, 365.0};
  float example1_14[] = {427.0, 559.0, 361.0};

  float example2_0[] = {131.0, 120.0, 76.0};
  float example2_1[] = {176.0, 260.0, 160.0};
  float example2_2[] = {168.0, 160.0, 100.0};
  float example2_3[] = {138.0, 104.0, 99.0};
  float example2_4[] = {120.0, 119.0, 85.0};
  float example2_5[] = {63.0, 62.0, 33.0};
  float example2_6[] = {63.0, 61.0, 39.0};
  float example2_7[] = {231.0, 237.0, 160.0};
  float example2_8[] = {149.0, 154.0, 99.0};
  float example2_9[] = {231.0, 233.0, 172.0};
  float example2_10[] = {193.0, 181.0, 116.0};

  // float example3_0[] = {245.0, 316.0, 245.0};
  // float example3_1[] = {239.0, 319.0, 244.0};
  // float example3_2[] = {245.0, 318.0, 246.0};
  // float example3_3[] = {250.0, 322.0, 248.0};
  // float example3_4[] = {247.0, 318.0, 245.0};
  // float example3_5[] = {247.0, 327.0, 246.0};
  // float example3_6[] = {243.0, 318.0, 242.0};
  // float example3_7[] = {267.0, 347.0, 261.0};
  float example3_9[] = {379.0, 535.0, 383.0};
  float example3_0[] = {481.0, 573.0, 378.0};
  float example3_1[] = {355.0, 467.0, 340.0};
  float example3_2[] = {444.0, 544.0, 364.0};
  float example3_3[] = {503.0, 595.0, 386.0};
  float example3_4[] = {500.0, 598.0, 380.0};
  float example3_5[] = {498.0, 597.0, 384.0};
  float example3_6[] = {378.0, 498.0, 361.0};
  float example3_7[] = {349.0, 464.0, 350.0};
  float example3_8[] = {349.0, 464.0, 350.0};


  myKNN.addExample(example1_0, 1);
  myKNN.addExample(example1_1, 1);
  myKNN.addExample(example1_2, 1);
  myKNN.addExample(example1_3, 1);
  myKNN.addExample(example1_4, 1);
  myKNN.addExample(example1_5, 1);
  myKNN.addExample(example1_6, 1);
  myKNN.addExample(example1_7, 1);
  myKNN.addExample(example1_8, 1);
  myKNN.addExample(example1_9, 1);
  myKNN.addExample(example1_10, 1);
  // myKNN.addExample(example1_11, 1);
  // myKNN.addExample(example1_12, 1);
  // myKNN.addExample(example1_13, 1);
  myKNN.addExample(example1_14, 1);
  myKNN.addExample(example1_15, 1);
  myKNN.addExample(example1_16, 1);
  myKNN.addExample(example1_17, 1);
  myKNN.addExample(example1_18, 1);

  myKNN.addExample(example2_0, 2);
  myKNN.addExample(example2_1, 2);
  myKNN.addExample(example2_2, 2);
  myKNN.addExample(example2_3, 2);
  myKNN.addExample(example2_4, 2);
  myKNN.addExample(example2_5, 2);
  myKNN.addExample(example2_6, 2);
  myKNN.addExample(example2_7, 2);
  myKNN.addExample(example2_8, 2);
  myKNN.addExample(example2_9, 2);
  myKNN.addExample(example2_10, 2);

  myKNN.addExample(example3_0, 3);
  myKNN.addExample(example3_1, 3);
  myKNN.addExample(example3_2, 3);
  myKNN.addExample(example3_3, 3);
  myKNN.addExample(example3_4, 3);
  myKNN.addExample(example3_5, 3);
  myKNN.addExample(example3_6, 3);
  myKNN.addExample(example3_7, 3);
  myKNN.addExample(example3_8, 3);
  myKNN.addExample(example3_9, 3);

  // float example3_9[] = {481.0, 566.0, 361.0};
  float example3_10[] = {478.0, 567.0, 365.0};
  float example3_11[] = {484.0, 560.0, 360.0};
  myKNN.addExample(example3_9, 3);
  myKNN.addExample(example3_10, 3);
  myKNN.addExample(example3_11, 3);

  //cloud connection
  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }
  ArduinoBearSSL.onGetTime(getTime);
  sslClient.setEccSlot(0, certificate);
  timeClient.begin();
  timeClient.setTimeOffset(0);
  mqttClient.onMessage(onMessageReceived);
}

void loop(){
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // PIR motion detection
  int temp_flag;
  temp_flag = digitalRead(PIR_MOTION_SENSOR);
  if(temp_flag){//if it detects the moving people?
    Serial.println("Hi,people is coming");
  }else{
    Serial.println("No people nearby");
    // Serial.println();
  }
  
  timeClient.forceUpdate();
  rawtime = timeClient.getEpochTime();
  ts = *localtime(&rawtime);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  Serial.println(buf);
  mqttClient.poll();
  

  // tong in the shelf dectection
  LEDstate_curr = LEDsense(); 
  if (LEDstate_curr==0 && LEDstate_pre==1){
    tong_shelf += 1;
    publishMessage();
  }
  Serial.print("tong use count:");
  Serial.println(tong_shelf);
  LEDstate_pre = LEDstate_curr;

  // color sensor motion detection and classification
  y = classifyBoardOrientation();
  if(y==1){
    Serial.println(LEDstate_curr);
    if(LEDstate_curr==1){
      digitalWrite(light_demo, HIGH);
    }else{
      digitalWrite(light_demo, LOW);      
    }
    display("Use The Tong");
  }else if(y==2){
    display("Thank You");
    digitalWrite(light_demo, LOW);
  }else{
    display("Wellcome");
    digitalWrite(light_demo, LOW);
  }

  Serial.println();
  // delay(3000);

}

void display(char site[]){
  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setPageMode();           //Set addressing mode to Page Mode
  SeeedOled.setTextXY(0, 0);         //Set the cursor to Xth Page, Yth Column
  SeeedOled.putString(site); //Print the String
  SeeedOled.activateScroll();
  // delay(3000);
}

bool LEDsense(){
  int y,out;
  // put your main code here, to run repeatedly:
  digitalWrite(light_source, HIGH);
  delay(410);
  y = analogRead(A0);
  Serial.print("Output of light sensor is: ");
  Serial.println(y);
  out = map(y,0,726,0,255);

  if (y < light_threhold)
  {//if it detects the moving people?
    Serial.println("Tong in the shelf");
    // Serial.println();
    digitalWrite(light_source, LOW);
    return 1;
  }
  else
  {
    Serial.println("No tong in the shelf");
    // Serial.println();
    digitalWrite(light_source, LOW);
    return 0;
  }
}

int classifyBoardOrientation() {
  // read acceleration into array
  readColors(colors);
  int boardOrientation;
  // use the KNN classifier to classify acceleration data
  // int boardOrientation;
  boardOrientation = myKNN.classify(colors, 3); // K = 1

  // print the classification out
  if (boardOrientation >= 1 && boardOrientation <= 3) {
    Serial.print("Predicted in bakery shelf is: ");
    Serial.println(BOARD_ORIENTATIONS[boardOrientation]);
    // Serial.println(y);
    // Serial.println(boardOrientation);
  } else {
    Serial.println("Unknown category in in bakery shelf! Did you train me?");
  }
  // Serial.println();

  return boardOrientation;
}

void readColors(float acceleration[]) {
  float frequency;
  // Setting red filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  Serial.print("R= ");//printing namebd
  Serial.print(frequency);//printing RED color frequencyh
  Serial.print("  ");
  delay(100);
  acceleration[0] = frequency;

  // Setting Green filtered photodiodes to be read
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  Serial.print("G= ");//printing name
  Serial.print(frequency);//printing RED color frequency
  Serial.print("  ");
  acceleration[1] = frequency;
  delay(100);

  // Setting Blue filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  Serial.print("B= ");//printing name
  Serial.print(frequency);//printing RED color frequency
  Serial.println("  ");
  acceleration[2] = frequency;
  delay(100);
}

// reads a number from the Serial Monitor
// expects new line
int readNumber() {
  String line;

  while (1) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c == '\r') {
        // ignore
        continue;
      } else if (c == '\n') {
        break;
      }

      line += c;
    }
  }

  return line.toInt();
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
  display("connect to internet");
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");
  display("Attempt to MQTT");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("robertBranch/bakery/tong");
  mqttClient.print(buf);
  mqttClient.print(",");
  mqttClient.print(tong_shelf);

  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}