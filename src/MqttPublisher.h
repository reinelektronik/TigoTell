#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <string>
#include <memory>
#include <PubSubClient.h>
#include <WiFiClient.h>

// Configuration structure for MQTT
struct MqttConfig {
  String broker_host = "";
  uint16_t broker_port = 1883;
  String client_id = "";
  String username = "";
  String password = "";
  String topic_prefix = "tigotell";
  bool enabled = false;
};

class MqttPublisher {
public:
  MqttPublisher();
  ~MqttPublisher();

  // Initialize MQTT client
  void init(const MqttConfig &config);

  // Connect/disconnect
  bool connect();
  void disconnect();
  bool isConnected();

  // Publish methods
  void publishPowerFrame(uint16_t address, uint16_t pv_node_id, 
                        float voltage_in, float voltage_out, 
                        float current_in, float temperature, 
                        uint8_t duty_cycle, int rssi);

  void publishAnnounce(uint16_t address, uint16_t pv_node_id, 
                       uint32_t barcode_fragment);

  void publishStats(const std::string &statsLine);

  // Periodic loop (call from main loop)
  void loop();

  // Update configuration
  void updateConfig(const MqttConfig &config);

private:
  WiFiClient wifiClient;
  std::unique_ptr<PubSubClient> client;
  MqttConfig config;
  unsigned long lastReconnectAttempt = 0;
  const unsigned long reconnectInterval = 5000; // 5 seconds

  bool reconnect();
  std::string buildJsonPayload(const std::string &measurement, 
                               const std::string &tags, 
                               const std::string &fields) const;
};

#endif // MQTT_PUBLISHER_H
