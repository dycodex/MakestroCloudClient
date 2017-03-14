#include "MakestroCloudClient.h"

MakestroCloudClient::MakestroCloudClient(const char* username, const char* key, const char* project, const char* clientId) :
  userName(username), userKey(key), projectName(project), AsyncMqttClient() {

    deviceId = (char*)malloc(sizeof(char*) * MAKESTROCLOUD_DEVICE_ID_MAX_LENGTH);
    if (!clientId) {
      sprintf(deviceId, "%s-%s-default", userKey, projectName);
    } else {
      strcpy(deviceId, clientId);
    }

    setServer(MAKESTROCLOUD_HOST, MAKESTROCLOUD_PORT);
    setCredentials(userName, userKey);
    setClientId(deviceId);
    setKeepAlive(MAKESTROCLOUD_KEEPALIVE);
    setCleanSession(false);

    AsyncMqttClientInternals::OnMessageUserCallback cb = [=](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
      onMakestroCloudMqttMessage(topic, payload, properties, len, index, total);
    };
    AsyncMqttClient::onMessage(cb);
}

MakestroCloudClient::MakestroCloudClient() {

}

MakestroCloudClient::~MakestroCloudClient() {
  if (deviceId != nullptr) {
    free(deviceId);
  }
}

void MakestroCloudClient::publish(String topic, String payload) {
  String newTopic = topic;

  if (userName != nullptr && !topic.startsWith(String(userName))) {
    String project = String(projectName);
    project.replace(" ", "");
    newTopic = String(userName) + "/" + project + "/" + topic;
  }

  AsyncMqttClient::publish(newTopic.c_str(), MAKESTROCLOUD_DEFAULT_PUBLISH_QOS, true, payload.c_str(), payload.length());
}

void MakestroCloudClient::publishData(String payload) {
  publish("data", payload);
}

void MakestroCloudClient::subscribeWithCallback(String topic, MakestroCloudSubscribedTopicMessageCallback callback) {
  String newTopic = topic;

  if (userName != nullptr && !topic.startsWith(String(userName))) {
    String project = String(projectName);
    project.replace(" ", "");
    newTopic = String(userName) + "/" + project + "/" + topic;
  }

  subscribedTopics[newTopic] = callback;

  AsyncMqttClient::subscribe(topic.c_str(), MAKESTROCLOUD_DEFAULT_SUBSCRIBE_QOS);
}

void MakestroCloudClient::subscribePropertyWithTopic(String topic, String property, MakestroCloudSubscribedPropertyCallback callback) {
  parseMessageAsJson = true;

  if (subscribedProperties.size() == 0) {
    String project = String(projectName);
    project.replace(" ", "");
    String newTopic = String(userName) + "/" + project + "/" + topic;

    AsyncMqttClient::subscribe(newTopic.c_str(), MAKESTROCLOUD_DEFAULT_SUBSCRIBE_QOS);
  }

  subscribedProperties[property] = callback;
}

void MakestroCloudClient::subscribeProperty(String property, MakestroCloudSubscribedPropertyCallback callback) {
  subscribePropertyWithTopic("control", property, callback);
}

AsyncMqttClient &MakestroCloudClient::onMessage(AsyncMqttClientInternals::OnMessageUserCallback callback) {
  onMakestroCloudMessageUserCallback = callback;

  return *this;
}

void MakestroCloudClient::onMakestroCloudMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (subscribedTopics.size() > 0) {
    TopicsHandlerMap::const_iterator pos =subscribedTopics.find(topic);

    if (pos != subscribedTopics.end()) {
      MakestroCloudSubscribedTopicMessageCallback cb = pos->second;
      String realPayload = "";
      realPayload.reserve(len);

      for (uint32_t i = 0; i < len; i++) {
        realPayload += (char)payload[i];
      }

      cb(String(topic), realPayload);
    }
  }

  if (onMakestroCloudMessageUserCallback) {
    onMakestroCloudMessageUserCallback(topic, payload, properties, len, index, total);
  }

  if (parseMessageAsJson && subscribedProperties.size() > 0) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(payload);

    if (json.success()) {
      for (const auto &pair : subscribedProperties) {
        if (!json.containsKey(pair.first)) {
          continue;
        }

        MakestroCloudSubscribedPropertyCallback cb = pair.second;
        String val = json[pair.first].asString();
        cb(pair.first, val);
      }
    }
  }
}

void MakestroCloudClient::publishMap(JsonKeyValueMap keyValMap, const char* iftttEvent, const char* iftttKey) {
  if (keyValMap.size() == 0) {
    return;
  }

  const int bufferSize = JSON_OBJECT_SIZE(20);
  StaticJsonBuffer<bufferSize> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  for (const auto &keyVal : keyValMap) {
    root[keyVal.first] = keyVal.second;
  }

  if (iftttEvent != nullptr) {
    root["ifttt_event"] = iftttEvent;
  }

  if (iftttKey != nullptr) {
    root["ifttt_key"] = iftttKey;
  }

  String jsonStr;
  root.printTo(jsonStr);
  Serial.println(jsonStr);

  publishData(jsonStr);
}

void MakestroCloudClient::triggerIFTTTEvent(const char* eventName, const char* eventKey) {
  const int bufferSize = JSON_OBJECT_SIZE(3);
  StaticJsonBuffer<bufferSize> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["ifttt_event"] = eventName;

  if (eventKey != nullptr) {
    root["ifttt_key"] = eventKey;
  }

  String jsonStr;
  root.printTo(jsonStr);

  publishData(jsonStr);
}
