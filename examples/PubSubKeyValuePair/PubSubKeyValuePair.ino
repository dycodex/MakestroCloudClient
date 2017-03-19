#include <MakestroCloudClient.h>
#include <ESP8266WiFi.h>
#include <ESPectro.h>
#include <DCX_AppSetting.h>
#include <DCX_WifiManager.h>

#define WIFI_SSID "AccessPointName"
#define WIFI_PASS "AccessPointPassword"

ESPectro board(ESPectro_V3);
DCX_WifiManager wifiManager(AppSetting);
MakestroCloudClient makestroClient("YOUR_USERNAME", "YOUR_CONNECTION_TOKEN", "YOUR_PROJECT_NAME", "YOUR_DEVICE_ID");

void onSubscribedPropertyCallback(const String prop, const String value) {
  Serial.print("incoming: ");
  Serial.print(prop);
  Serial.print(" = ");
  Serial.print(value);
  Serial.println();

  if (prop.equals("switch")) {
    if (value.equals("1")) {
      board.turnOnLED();
    } else {
      board.turnOffLED();
    }
  }
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
  while (!Serial);

  DEBUG_SERIAL("\r\nInitializing....\r\n\r\n");
  AppSetting.load();
  AppSetting.debugPrintTo(Serial);

  wifiManager.onWifiConnectStarted([]() {
    DEBUG_SERIAL("WIFI CONNECING STARTED\r\n");
    board.turnOnLED();
  });

  wifiManager.onWifiConnected([](boolean newConn) {
    DEBUG_SERIAL("WIFI_CONNECTED");
    board.turnOffLED();
    makestroClient.onConnect(onMakestroConnected);
    makestroClient.onDisconnect(onMakestroDisconnected);
    makestroClient.onSubscribe(onSubscribeAck);
    makestroClient.connect();
  });

  wifiManager.onWifiConnecting([](unsigned long elapsed) {
    board.toggleLED();
  });

  wifiManager.onWifiDisconnected([](WiFiDisconnectReason reason) {
    DEBUG_SERIAL("WIFI GIVE UP\r\n");
    board.turnOffLED();
  });

  wifiManager.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
  wifiManager.loop();

  static uint16_t counter = 0;
  static unsigned long lastPublished = 0;

  if (makestroClient.connected()) {
    if (millis() - lastPublished > 2000) {
      lastPublished = millis();
      String payload = "{\"counter\":";
      payload = payload + String(counter);
      payload = payload + "}";

      // makestroClient.publishData("{\"counter\": " + String(counter) + "}")
      makestroClient.publishKeyValue("counter", counter);
      counter++;
    }
  }
}