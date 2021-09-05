#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "apror/160818733079"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define REPORTING_PERIOD_MS    1000

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

float BPM, SpO2;
PulseOximeter pox;
uint32_t tsLastReport = 0;
const int pin = 34;
float fahrenheit;

void onBeatDetected()
{
  Serial.println("Beat Detected!");
}


void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["BPM"] = BPM;
  doc["SpO2"] = SpO2;
  doc["Temparature"] = temperature();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void setupPox()
{
  
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");

    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
}

void checkPox_checkTemp()
{
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {

    Serial.print("BPM: ");
    Serial.println(BPM);

    Serial.print("SpO2: ");
    Serial.print(SpO2);
    Serial.println("%");

    Serial.println("*********************************");
    Serial.println();

    tsLastReport = millis();
     publishMessage();
  }
  }

float temperature()
{
  int analogValue = analogRead(pin);
  float millivolts = (analogValue/4096.0) * 5000; 
  float celsius = millivolts/10;
  Serial.print("in DegreeC=   ");
  Serial.println(celsius);
  
  fahrenheit = ((celsius * 9)/5 + 32);
  Serial.print(" in Farenheit=   ");
  Serial.println(fahrenheit);
  return fahrenheit;
  }

void setup() {
  Serial.begin(115200);
  pinMode(19, OUTPUT);
  connectAWS();
  
  setupPox();
}

void loop() {

  pox.update();
  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();
  
  checkPox_checkTemp();
  //client.loop();
  //delay(1000);
}
