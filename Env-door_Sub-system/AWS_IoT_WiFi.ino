/*
  AWS IoT WiFi

  This sketch securely connects to an AWS IoT using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a public
  certificate for SSL/TLS authetication.

  It publishes a message every 5 seconds to arduino/outgoing
  topic and subscribes to messages on the arduino/incoming
  topic.

  The circuit:
  - Arduino MKR WiFi 1010 or MKR1000

  The following tutorial on Arduino Project Hub can be used
  to setup your AWS account and the MKR board:

  https://create.arduino.cc/projecthub/132016/securely-connecting-an-arduino-mkr-wifi-1010-to-aws-iot-core-a9f365

  This example code is in the public domain.
*/

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include <Arduino_MKRENV.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <Servo.h>

#include "arduino_secrets.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiUDP ntpUDP;
Servo servo;
NTPClient timeClient(ntpUDP);
WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

const int pinLight = A1;

// Variables to save date and time
time_t rawtime;
struct tm  ts;
char buf[80];

unsigned long lastMillis = 0;
float temperature, humidity, pressure, illuminance, uva, uvb, uvIndex;
int doorState = 49;
bool motorCon = false, lastMotorCon = true; // true is open, false is close
int doorReport, lastDoorReport;
float doorUsage=0.0;

void setup() {
  Serial.begin(115200);
  controlMotor(motorCon);   // open the door to initialise
  while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV Shield!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");

  // Initialise a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(0);

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // while(!timeClient.update()){
  timeClient.forceUpdate();
  // }
  // The formattedDate comes with following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  rawtime = timeClient.getEpochTime();
  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  ts = *localtime(&rawtime);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  Serial.println(buf);

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  int sensorValue = lightsensorValue();
  Serial.print("Sensor = ");
  Serial.println(sensorValue);
  if (sensorValue > 120){
    doorReport = 1;
  } else {
    doorReport = 0;
  }
  Serial.print("doorReport  = ");
  Serial.println(doorReport);
  if (doorReport != lastDoorReport) {
    doorUsage += 0.5;
  }
  lastDoorReport = doorReport;
  Serial.print("doorUsage   = ");
  Serial.println(doorUsage);

  Serial.print("cloudSignal = ");
  Serial.println(doorState);
  motorCon = doorState != 48;
  Serial.print("motorSignal = ");  
  Serial.println(motorCon);
  delay(200);
  if (motorCon != lastMotorCon) {
    controlMotor(motorCon);
  }
  delay(200);
    

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    getEnvValues();
    publishMessage();
  }

  lastMotorCon = motorCon;

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
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("robertBranch/bakery/door");
}

void getEnvValues() {
  //read all the sensor values
  temperature = ENV.readTemperature();
  humidity    = ENV.readHumidity();
  pressure    = ENV.readPressure();
  illuminance = ENV.readIlluminance();
  uva         = ENV.readUVA();
  uvb         = ENV.readUVB();
  uvIndex     = ENV.readUVIndex();

  // print each of the sensor values
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity    = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Pressure    = ");
  Serial.print(pressure);
  Serial.println(" kPa");

  Serial.print("Illuminance = ");
  Serial.print(illuminance);
  Serial.println(" lx");

  Serial.print("UVA         = ");
  Serial.println(uva);

  Serial.print("UVB         = ");
  Serial.println(uvb);

  Serial.print("UV Index    = ");
  Serial.println(uvIndex);

  // print an empty line
  Serial.println();

  // wait 5 second to read again
  delay(5000);  
}

void publishMessage() {
  Serial.println("Publishing message");

  //send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("robertBranch/bakery/env");
  mqttClient.print(buf);
  mqttClient.print(",");
  mqttClient.print(temperature);
  mqttClient.print(",");
  mqttClient.print(humidity);
  mqttClient.print(",");
  mqttClient.print(pressure);
  mqttClient.print(",");
  mqttClient.print(illuminance);
  mqttClient.print(",");
  mqttClient.print(uva);
  mqttClient.print(",");
  mqttClient.print(uvb);
  mqttClient.print(",");
  mqttClient.print(uvIndex);
  mqttClient.endMessage();

  mqttClient.beginMessage("robertBranch/bakery/doorReport");
  mqttClient.print(buf);
  mqttClient.print(",");
  mqttClient.print(doorReport);
  mqttClient.print(",");
  mqttClient.print(doorUsage);
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
    doorState = (int)mqttClient.read();
    Serial.print(doorState);
  }
  Serial.println();

  Serial.println();
}

void controlMotor(bool inputState) {
  const int servoPin = 13;
  if (inputState) {
    servo.attach(servoPin);
    int angle = 90;
    for(angle = 90; angle < 127; angle++) {                                  
      servo.write(angle);               
      delay(15);                   
    } 
    servo.detach();
  }
  else if (inputState == false) {
    servo.attach(servoPin);
    int angle = 100;
    for(angle = 100; angle > 65; angle--) {                                
      servo.write(angle);           
      delay(15);       
    } 
    servo.detach();    
  }
}

int lightsensorValue() {
  int _sensorValue;
  _sensorValue = analogRead(pinLight);
  return _sensorValue;
}
