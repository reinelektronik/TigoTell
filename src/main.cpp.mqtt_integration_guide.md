# MQTT Integration Guide for src/main.cpp

## Summary
This document provides the required modifications to integrate MQTT support into src/main.cpp.

## Changes Required

### 1. Add Include (after line 21)
```cpp
#include "MqttPublisher.h"
```

### 2. Update Config Struct (add after line 41)
```cpp
  // MQTT settings
  String mqttBroker = "";
  uint16_t mqttPort = 1883;
  String mqttUser = "";
  String mqttPass = "";
  String mqttTopicPrefix = "tigotell";
  bool mqttEnabled = false;
```

### 3. Add Global Instance (after line 46)
```cpp
MqttPublisher mqttPublisher;
```

### 4. Update loadSettings() (add before preferences.end() at line 68)
```cpp
  appConfig.mqttBroker = preferences.getString("mqttBroker", appConfig.mqttBroker);
  appConfig.mqttPort = preferences.getUShort("mqttPort", appConfig.mqttPort);
  appConfig.mqttUser = preferences.getString("mqttUser", appConfig.mqttUser);
  appConfig.mqttPass = preferences.getString("mqttPass", appConfig.mqttPass);
  appConfig.mqttTopicPrefix = preferences.getString("mqttTopic", appConfig.mqttTopicPrefix);
  appConfig.mqttEnabled = preferences.getBool("mqttEnabled", appConfig.mqttEnabled);
```

### 5. Update saveSettings() (add before preferences.end() at line 85)
```cpp
  preferences.putString("mqttBroker", appConfig.mqttBroker);
  preferences.putUShort("mqttPort", appConfig.mqttPort);
  preferences.putString("mqttUser", appConfig.mqttUser);
  preferences.putString("mqttPass", appConfig.mqttPass);
  preferences.putString("mqttTopic", appConfig.mqttTopicPrefix);
  preferences.putBool("mqttEnabled", appConfig.mqttEnabled);
```

### 6. Update /api/config GET Endpoint (add before line 330)
```cpp
    doc["mqttBroker"] = appConfig.mqttBroker;
    doc["mqttPort"] = appConfig.mqttPort;
    doc["mqttUser"] = appConfig.mqttUser;
    doc["mqttPass"] = appConfig.mqttPass;
    doc["mqttTopicPrefix"] = appConfig.mqttTopicPrefix;
    doc["mqttEnabled"] = appConfig.mqttEnabled;
```

### 7. Update /api/config POST Endpoint (add before saveSettings() at line 376)
```cpp
          if (doc["mqttBroker"].is<const char *>())
            appConfig.mqttBroker = doc["mqttBroker"].as<String>();
          if (doc["mqttPort"].is<int>())
            appConfig.mqttPort = doc["mqttPort"];
          if (doc["mqttUser"].is<const char *>())
            appConfig.mqttUser = doc["mqttUser"].as<String>();
          if (doc["mqttPass"].is<const char *>())
            appConfig.mqttPass = doc["mqttPass"].as<String>();
          if (doc["mqttTopicPrefix"].is<const char *>())
            appConfig.mqttTopicPrefix = doc["mqttTopicPrefix"].as<String>();
          if (doc["mqttEnabled"].is<bool>())
            appConfig.mqttEnabled = doc["mqttEnabled"].as<bool>();
          
          // Update MQTT configuration
          MqttConfig newMqttConfig;
          newMqttConfig.broker_host = appConfig.mqttBroker;
          newMqttConfig.broker_port = appConfig.mqttPort;
          newMqttConfig.username = appConfig.mqttUser;
          newMqttConfig.password = appConfig.mqttPass;
          newMqttConfig.topic_prefix = appConfig.mqttTopicPrefix;
          newMqttConfig.enabled = appConfig.mqttEnabled;
          mqttPublisher.updateConfig(newMqttConfig);
```

### 8. In setup() - Initialize MQTT (after line 461, after server.begin())
```cpp
  // Initialize MQTT Publisher
  MqttConfig mqttConfig;
  mqttConfig.broker_host = appConfig.mqttBroker;
  mqttConfig.broker_port = appConfig.mqttPort;
  mqttConfig.username = appConfig.mqttUser;
  mqttConfig.password = appConfig.mqttPass;
  mqttConfig.topic_prefix = appConfig.mqttTopicPrefix;
  mqttConfig.enabled = appConfig.mqttEnabled;
  mqttPublisher.init(mqttConfig);
```

### 9. In loop() - Add MQTT loop (after line 468)
```cpp
  mqttPublisher.loop();
```

### 10. In PowerData Frame Handler (after line 530 where sendUdpPayload is called)
```cpp
            // Send over MQTT
            mqttPublisher.publishPowerFrame(
                powerFrame->address, powerFrame->pv_node_id,
                powerFrame->voltage_in, powerFrame->voltage_out,
                powerFrame->current_in, powerFrame->temperature,
                powerFrame->duty_cycle, powerFrame->rssi);
```

### 11. In Announce Frame Handler (after line 574 where sendUdpPayload is called)
```cpp
          // Send over MQTT
          mqttPublisher.publishAnnounce(
              announceFrame->address, announceFrame->pv_node_id,
              announceFrame->barcode_fragment);
```

### 12. In Statistics Section (after line 638 where sendUdpPayload is called)
```cpp
    // Send over MQTT
    mqttPublisher.publishStats(statsLine);
```

## Testing

1. **Compile** with `pio run -t upload`
2. **Configure** via web UI at Configuration > MQTT Reporting
3. **Verify** connection logs in WebSerial console
4. **Check** MQTT topics in your broker client

## ioBroker Setup

1. Install MQTT adapter in ioBroker
2. Configure connection to your MQTT broker
3. TigoTell will publish to: `tigotell/power/{nodeId}` (JSON format)
4. Create states/objects for each topic as needed

## MQTT Topic Structure

```
tigotell/
├── power/{nodeId}          → {address, pv_node_id, voltage_in, voltage_out, current_in, temperature, duty_cycle, rssi, timestamp}
├── announce/{nodeId}       → {address, pv_node_id, barcode_fragment, timestamp}
└── stats                   → {stats_line, timestamp}
```
