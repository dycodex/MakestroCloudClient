#ifndef MAKESTROCLOUDCLIENT_H
#define MAKESTROCLOUDCLIENT_H

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <map>

#define MAKESTROCLOUD_HOST "cloud.makestro.com"
#define MAKESTROCLOUD_PORT 1883
#define MAKESTROCLOUD_DEFAULT_SUBSCRIBE_QOS 2
#define MAKESTROCLOUD_DEFAULT_PUBLISH_QOS 2
#define MAKESTROCLOUD_DEVICE_ID_MAX_LENGTH 127
#define MAKESTROCLOUD_KEEPALIVE 10

typedef std::function<void(String, String)> MakestroCloudSubscribedTopicMessageCallback;
typedef std::function<void(String, String)> MakestroCloudSubscribedPropertyCallback;
typedef std::map<String, MakestroCloudSubscribedTopicMessageCallback> TopicsHandlerMap;
typedef std::map<String, MakestroCloudSubscribedPropertyCallback> PropsHandlerMap;
typedef std::map<const char*, JsonVariant> JsonKeyValueMap;

class MakestroCloudClient : public AsyncMqttClient {
public:
  MakestroCloudClient(const char* username, const char* key, const char* projectName, const char* clientId = nullptr);
  MakestroCloudClient();
  ~MakestroCloudClient();

  AsyncMqttClient& onMessage(AsyncMqttClientInternals::OnMessageUserCallback callback);
  void subscribeWithCallback(String topic, MakestroCloudSubscribedTopicMessageCallback callback);
  void subscribeProperty(String property, MakestroCloudSubscribedPropertyCallback callback);
  void subscribePropertyWithTopic(String topic, String property, MakestroCloudSubscribedPropertyCallback callback);
  void publish(String topic, String payload);
  void publishData(String payload);
  void triggerIFTTTEvent(const char* eventName, const char* eventKey = nullptr);
  void publishMap(JsonKeyValueMap keyValMap, const char* iftttEvent = nullptr, const char* iftttKey = nullptr);
  template<typename Value>
  void publishKeyValue(const char* key, Value val) {
    const int bufferSize = JSON_OBJECT_SIZE(2);
    StaticJsonBuffer<bufferSize> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root[key] = val;

    String jsonString;
    root.printTo(jsonString);
    publishData(jsonString);
  }
private:
  const char* userName = nullptr;
  const char* userKey = nullptr;
  const char* projectName = nullptr;
  char* deviceId = nullptr;

  boolean parseMessageAsJson = false;
  TopicsHandlerMap subscribedTopics;
  PropsHandlerMap subscribedProperties;

  AsyncMqttClientInternals::OnMessageUserCallback onMakestroCloudMessageUserCallback;
  void onMakestroCloudMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties msgProperties, size_t len, size_t index, size_t total);
};


#endif