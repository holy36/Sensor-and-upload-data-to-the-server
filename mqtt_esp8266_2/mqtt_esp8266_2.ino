#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

// Khai bao bien cam bien bui
int measurePin = A0;  //Chan vout cua GP2Y10
int ledPower = 2;     //Chan led 
int samplingTime = 280;
int deltaTime = 40; 
int sleepTime = 9680;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0 ;

//wifi and server config
const char* ssid = "Redmi K40";
const char* password = "Hung2002";
const char* mqtt_server = "192.168.24.152";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  pinMode(ledPower,OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  digitalWrite(ledPower,LOW); // Bat IR LED
  delayMicroseconds(samplingTime);  //Delay 0.28ms
  voMeasured = analogRead(measurePin); // Doc gia tri ADC V0
  delayMicroseconds(deltaTime); //Delay 0.04ms
  digitalWrite(ledPower,HIGH); // Tat LED
  delayMicroseconds(sleepTime); //Delay 9.68ms
 
  // Tinh dien ap tu gia tri ADC
  calcVoltage = voMeasured * (3.3 / 1024); //Dien ap cua cam bien
  if (calcVoltage < 0.583) 
    dustDensity = 0;
  else
    dustDensity = 6 * calcVoltage / 35 - 0.1;

  // Serialize data to Json format
  StaticJsonDocument<1024> doc;
  char output[100];
  doc["dust"] = dustDensity;
  Serial.println("Read");
  serializeJson(doc, output);
  Serial.println(output);

  client.publish("home/esp8266", output); // publish to home/esp8266
  Serial.println("Sent");
  delay(5000);
  
}
