#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define BATPIN      33
#define DHTPIN      17    
#define DHTTYPE    DHT22    

#define uS_conversion_factor 1000000
#define time_to_wakeup 30 //seconds

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;
uint8_t num_of_index = 0;
const uint8_t num_of_sample = 4;


bool isSucces;

struct dpackage {
  uint32_t UID;
  float bat;
  float data1;
  float data2;
  float data3;
  uint32_t data4;
  uint32_t data5;
  uint32_t data6;
} message;



// Replace the next variables with your SSID/Password combination
const char* ssid = "Somkiat_2.4G";
const char* password = "0818187888";

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "esp32/scos";
const char *mqtt_username = "cosnxcils";
const char *mqtt_password = "cils2023";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length);

void setup() {
  // Serial.begin(115200);

  // Serial.println("---------------Wakeup-----------------");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      // Serial.println("Connecting to WiFi..");
  }

  uint64_t tempUID = (ESP.getEfuseMac() >> 8);
  message.UID = (uint32_t) (tempUID+555 & 0x00000000FFFFFFFF);
  // Serial.println(message.UID);

  // Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  


  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected()) {
      String client_id = "esp32-client-";
      client_id += String(WiFi.macAddress());
      // Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          // Serial.println("Public emqx mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.println(client.state());
          delay(2000);
      }
  }

  // Serial.println(analogRead(BATPIN));
  // publish and subscribe
  // client.publish(topic, "Hi EMQX I'm ESP32 ^^");
  // client.subscribe(topic);
  // Initialize device.
  dht.begin();

  sensor_t sensor;

  dht.temperature().getSensor(&sensor);

  dht.humidity().getSensor(&sensor);

  delayMS = sensor.min_delay / 1000;

  num_of_index = 0;

}

float tempBuff[num_of_sample];
float humiBuff[num_of_sample];

float vOUT = 0.0;
int value = 0;

void loop() {
  if(num_of_index<num_of_sample) {
    // Delay between measurements.
    delay(delayMS);
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (~isnan(event.temperature)) {
  
      tempBuff[num_of_index] = event.temperature;
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (~isnan(event.relative_humidity)) {
      humiBuff[num_of_index] = event.relative_humidity;
    }

    // Serial.print(tempBuff[num_of_index]);
    // Serial.print(" ");
    // Serial.print(humiBuff[num_of_index]);
    // Serial.println();

    num_of_index++;
  } else {

    float temp=0.0;
    float humi=0.0;

    for(int i = 1;  i<  num_of_sample; i++) 
    {

      if(i == 1)
      {
        temp = tempBuff[i];
        humi = humiBuff[i];
      }
      temp = (temp+tempBuff[i])/2;
      humi = (humi+humiBuff[i])/2;

    }

    message.data1 = temp;
    message.data2 = humi;

    value = analogRead(BATPIN);
    message.bat = value*5.21/1023;

    // Serial.print(value);
    // Serial.print(" ");
    // Serial.print(message.UID);
    // Serial.print(" ");
    // Serial.print(message.data1);
    // Serial.print(" ");
    // Serial.print(message.data2);
    // Serial.print(" ");
    // Serial.println((float) message.bat);

    client.publish(topic, (uint8_t*) &message, sizeof(message), false);
    isSucces = client.subscribe(topic);

    delay(100);

    if(!isSucces){
      Serial.println("failed send mqtt");
    }

    esp_sleep_enable_timer_wakeup(time_to_wakeup * uS_conversion_factor);

    // Serial.println("----------Enter deepsleep----------");

    esp_deep_sleep_start();
  }


}

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}