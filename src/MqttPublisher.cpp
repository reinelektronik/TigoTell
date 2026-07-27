#include "MqttPublisher.h"
#include <ArduinoJson.h>
#include <WebSerial.h>

MqttPublisher::MqttPublisher() 
    : client(std::make_unique<PubSubClient>(wifiClient)) {
}

MqttPublisher::~MqttPublisher() {
  if (client && client->connected()) {
    client->disconnect();
  }
}

void MqttPublisher::init(const MqttConfig &cfg) {
  config = cfg;
  
  if (config.enabled && config.broker_host.length() > 0) {
    client->setServer(config.broker_host.c_str(), config.broker_port);
    WebSerial.printf("MQTT initialized: broker=%s:%d\n", 
                     config.broker_host.c_str(), config.broker_port);
  }
}

void MqttPublisher::updateConfig(const MqttConfig &cfg) {
  bool needsReconnect = (config.broker_host != cfg.broker_host || 
                         config.broker_port != cfg.broker_port ||
                         config.enabled != cfg.enabled);
  
  config = cfg;
  
  if (needsReconnect) {
    if (client->connected()) {
      client->disconnect();
    }
    if (config.enabled && config.broker_host.length() > 0) {
      client->setServer(config.broker_host.c_str(), config.broker_port);
      WebSerial.println("MQTT configuration updated");
    }
  }
}

bool MqttPublisher::isConnected() {
  return config.enabled && client && client->connected();
}

bool MqttPublisher::connect() {
  if (!config.enabled || config.broker_host.length() == 0) {
    return false;
  }

  if (client->connected()) {
    return true;
  }

  // Use hostname as default client ID if not specified
  String clientId = config.client_id;
  if (clientId.length() == 0) {
    clientId = "TigoTell_" + String(WiFi.macAddress());
  }

  WebSerial.printf("MQTT: Attempting to connect as %s...\n", clientId.c_str());

  bool connected = false;
  if (config.username.length() > 0 && config.password.length() > 0) {
    connected = client->connect(clientId.c_str(), 
                               config.username.c_str(), 
                               config.password.c_str());
  } else {
    connected = client->connect(clientId.c_str());
  }

  if (connected) {
    WebSerial.printf("MQTT: Connected to %s:%d\n", 
                     config.broker_host.c_str(), config.broker_port);
    return true;
  } else {
    WebSerial.printf("MQTT: Connection failed, rc=%d\n", client->state());
    return false;
  }
}

void MqttPublisher::disconnect() {
  if (client && client->connected()) {
    client->disconnect();
    WebSerial.println("MQTT: Disconnected");
  }
}

void MqttPublisher::loop() {
  if (!config.enabled || config.broker_host.length() == 0) {
    return;
  }

  if (client->connected()) {
    client->loop();
  } else {
    // Attempt to reconnect periodically
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      reconnect();
    }
  }
}

bool MqttPublisher::reconnect() {
  if (!config.enabled || config.broker_host.length() == 0) {
    return false;
  }
  return connect();
}

void MqttPublisher::publishPowerFrame(uint16_t address, uint16_t pv_node_id,
                                      float voltage_in, float voltage_out,
                                      float current_in, float temperature,
                                      uint8_t duty_cycle, int rssi) {
  if (!isConnected()) {
    return;
  }

  JsonDocument doc;
  doc["address"] = address;
  doc["pv_node_id"] = pv_node_id;
  doc["voltage_in"] = voltage_in;
  doc["voltage_out"] = voltage_out;
  doc["current_in"] = current_in;
  doc["power_out"] = voltage_out * current_in;
  doc["temperature"] = temperature;
  doc["duty_cycle"] = duty_cycle;
  doc["rssi"] = rssi;
  doc["timestamp"] = millis();

  String topic = config.topic_prefix + "/power/" + String(pv_node_id);
  String payload;
  serializeJson(doc, payload);

  if (client->publish(topic.c_str(), payload.c_str())) {
    WebSerial.printf("MQTT: Published power data to %s\n", topic.c_str());
  } else {
    WebSerial.printf("MQTT: Failed to publish to %s (rc=%d)\n", 
                     topic.c_str(), client->state());
  }
}

void MqttPublisher::publishAnnounce(uint16_t address, uint16_t pv_node_id,
                                    uint32_t barcode_fragment) {
  if (!isConnected()) {
    return;
  }

  JsonDocument doc;
  doc["address"] = address;
  doc["pv_node_id"] = pv_node_id;
  char barcode_str[16];
  snprintf(barcode_str, sizeof(barcode_str), "%06X", barcode_fragment);
  doc["barcode_fragment"] = barcode_str;
  doc["timestamp"] = millis();

  String topic = config.topic_prefix + "/announce/" + String(pv_node_id);
  String payload;
  serializeJson(doc, payload);

  if (client->publish(topic.c_str(), payload.c_str())) {
    WebSerial.printf("MQTT: Published announce to %s\n", topic.c_str());
  } else {
    WebSerial.printf("MQTT: Failed to publish announce (rc=%d)\n", 
                     client->state());
  }
}

void MqttPublisher::publishStats(const std::string &statsLine) {
  if (!isConnected()) {
    return;
  }

  JsonDocument doc;
  doc["stats_line"] = statsLine;
  doc["timestamp"] = millis();

  String topic = config.topic_prefix + "/stats";
  String payload;
  serializeJson(doc, payload);

  if (client->publish(topic.c_str(), payload.c_str())) {
    WebSerial.printf("MQTT: Published stats to %s\n", topic.c_str());
  } else {
    WebSerial.printf("MQTT: Failed to publish stats (rc=%d)\n", 
                     client->state());
  }
}

std::string MqttPublisher::buildJsonPayload(const std::string &measurement,
                                            const std::string &tags,
                                            const std::string &fields) const {
  JsonDocument doc;
  doc["measurement"] = measurement;
  doc["tags"] = tags;
  doc["fields"] = fields;
  
  String result;
  serializeJson(doc, result);
  return std::string(result.c_str());
}
