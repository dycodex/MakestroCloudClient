#include <MakestroCloudClient.h>
#include <ESP8266WiFi.h>

#define WIFI_SSID "AccessPointName"
#define WIFI_PASS "AccessPointPassword"

MakestroCloudClient makestroClient("YOUR_USERNAME", "YOUR_CONNECTION_TOKEN", "YOUR_PROJECT_NAME", "YOUR_DEVICE_ID");

void onSubscribedPropertyCallback(const String prop, const String value) {
  Serial.print("incoming: ");
  Serial.print(prop);
  Serial.print(" = ");
  Serial.print(value);
  Serial.println();
}

void onMakestroConnected(bool sessionPresent) {
  Serial.println("Connected to Makestro Cloud!");

  makestroClient.subscribeProperty("switch", onSubscribedPropertyCallback);
}

void onMakestroDisconnected(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from Makestro Cloud");
}

void onSubscribeAck(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged");
  Serial.printf("ID: %x. QOS: %d\n", packetId, qos);
}

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("Waiting for connection....");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnected to WiFi!");
  makestroClient.onConnect(onMakestroConnected);
  makestroClient.onDisconnect(onMakestroDisconnected);
  makestroClient.onSubscribe(onSubscribeAck);
  makestroClient.connect();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && makestroClient.connected()) {
    makestroClient.publishData("{\"temp\": 28}");
    delay(5000);
  }
}