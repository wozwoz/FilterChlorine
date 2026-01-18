# MQTT JSON Examples - ESP32 to Home Assistant

## Overview

Sending JSON allows multiple sensor values in a single MQTT message, reducing network traffic and organizing related data.

## ESP32 - Sending JSON

### Method 1: Using snprintf (Current Implementation)

```cpp
void Device::loop() {
    float temperature = 25.5;
    float humidity = 60.2;
    int rssi = WiFi.RSSI();
    
    char jsonBuffer[256];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"temperature\":%.2f,\"humidity\":%.2f,\"rssi\":%d}",
        temperature, humidity, rssi);
    
    mqtt.publish("esp32/sensors", jsonBuffer);
}
```

**Output:** `{"temperature":25.50,"humidity":60.20,"rssi":-45}`

### Method 2: Using String (Simple but uses more memory)

```cpp
void Device::loop() {
    float temperature = 25.5;
    float humidity = 60.2;
    
    String json = "{";
    json += "\"temperature\":" + String(temperature, 2) + ",";
    json += "\"humidity\":" + String(humidity, 2) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";
    
    mqtt.publish("esp32/sensors", json.c_str());
}
```

### Method 3: Using ArduinoJson Library (Recommended for Complex JSON)

**Add to platformio.ini:**
```ini
lib_deps = 
    knolleary/PubSubClient@^2.8.0
    bblanchon/ArduinoJson@^6.21.0
```

**Code:**
```cpp
#include <ArduinoJson.h>

void Device::loop() {
    StaticJsonDocument<256> doc;
    
    doc["temperature"] = 25.5;
    doc["humidity"] = 60.2;
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    
    // Nested objects
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = WiFi.SSID();
    wifi["ip"] = WiFi.localIP().toString();
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    
    mqtt.publish("esp32/sensors", jsonBuffer);
}
```

**Output:**
```json
{
  "temperature": 25.5,
  "humidity": 60.2,
  "rssi": -45,
  "uptime": 3600,
  "wifi": {
    "ssid": "MyNetwork",
    "ip": "192.168.1.100"
  }
}
```

## Home Assistant - Decoding JSON

### Basic Value Extraction

```yaml
mqtt:
  sensor:
    - name: "ESP32 Temperature"
      state_topic: "esp32/sensors"
      unit_of_measurement: "°C"
      value_template: "{{ value_json.temperature }}"
```

### With Error Handling

```yaml
mqtt:
  sensor:
    - name: "ESP32 Temperature"
      state_topic: "esp32/sensors"
      unit_of_measurement: "°C"
      value_template: >
        {% if value_json.temperature is defined %}
          {{ value_json.temperature | round(1) }}
        {% else %}
          {{ states('sensor.esp32_temperature') }}
        {% endif %}
```

### Accessing Nested JSON

For JSON like: `{"wifi":{"rssi":-45,"ip":"192.168.1.100"}}`

```yaml
mqtt:
  sensor:
    - name: "ESP32 WiFi Signal"
      state_topic: "esp32/sensors"
      value_template: "{{ value_json.wifi.rssi }}"
      
    - name: "ESP32 IP Address"
      state_topic: "esp32/sensors"
      value_template: "{{ value_json.wifi.ip }}"
```

### Converting Values

```yaml
mqtt:
  sensor:
    # Convert Fahrenheit to Celsius
    - name: "ESP32 Temperature Celsius"
      state_topic: "esp32/sensors"
      value_template: "{{ ((value_json.temperature - 32) * 5/9) | round(1) }}"
      unit_of_measurement: "°C"
      
    # Convert uptime seconds to hours
    - name: "ESP32 Uptime Hours"
      state_topic: "esp32/sensors"
      value_template: "{{ (value_json.uptime / 3600) | round(1) }}"
      unit_of_measurement: "h"
      
    # Calculate from multiple values
    - name: "ESP32 Heat Index"
      state_topic: "esp32/sensors"
      value_template: >
        {% set t = value_json.temperature | float %}
        {% set h = value_json.humidity | float %}
        {{ (t + (0.5555 * (6.11 * exp(5417.7530 * ((1/273.16) - (1/(273.15 + t)))) - 10))) | round(1) }}
```

### Multiple Attributes from JSON

```yaml
mqtt:
  sensor:
    - name: "ESP32 Main Sensor"
      state_topic: "esp32/sensors"
      value_template: "{{ value_json.temperature }}"
      unit_of_measurement: "°C"
      json_attributes_topic: "esp32/sensors"
      json_attributes_template: >
        {
          "humidity": {{ value_json.humidity }},
          "rssi": {{ value_json.rssi }},
          "uptime": {{ value_json.uptime }}
        }
```

Access attributes in templates:
```yaml
{{ state_attr('sensor.esp32_main_sensor', 'humidity') }}
```

## ESP32 - Receiving JSON Commands

```cpp
void Device::message_handler(char* topic, char* payload) {
    // Using ArduinoJson
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Extract values
    if (doc.containsKey("command")) {
        const char* cmd = doc["command"];
        if (strcmp(cmd, "restart") == 0) {
            ESP.restart();
        }
    }
    
    if (doc.containsKey("led_brightness")) {
        int brightness = doc["led_brightness"];
        // Set LED brightness
    }
}
```

## Home Assistant - Sending JSON Commands

```yaml
automation:
  - alias: "Send JSON Command to ESP32"
    trigger:
      - platform: state
        entity_id: input_number.led_brightness
    action:
      - service: mqtt.publish
        data:
          topic: "beacon"
          payload: >
            {
              "command": "set_brightness",
              "value": {{ states('input_number.led_brightness') | int }},
              "timestamp": {{ now().timestamp() | int }}
            }
```

## Common Value Templates

### Check if value exists
```yaml
{{ value_json.temperature if value_json.temperature is defined else 0 }}
```

### Round to decimals
```yaml
{{ value_json.temperature | round(2) }}
```

### Convert to integer
```yaml
{{ value_json.rssi | int }}
```

### Convert to float
```yaml
{{ value_json.humidity | float }}
```

### Default value if missing
```yaml
{{ value_json.temperature | default(0) }}
```

### Multiple conditions
```yaml
{% if value_json.temperature > 30 %}
  hot
{% elif value_json.temperature > 20 %}
  warm
{% else %}
  cold
{% endif %}
```

## Debugging

### View raw MQTT messages in Home Assistant

Developer Tools → MQTT → Listen to topic: `esp32/sensors`

### Test JSON parsing

Developer Tools → Template:
```jinja2
{% set payload = '{"temperature":25.5,"humidity":60.2}' %}
{% set json = payload | from_json %}
Temperature: {{ json.temperature }}
Humidity: {{ json.humidity }}
```

### ESP32 Serial Output

```cpp
Serial.print("Publishing JSON: ");
Serial.println(jsonBuffer);
```

## Best Practices

1. **Keep JSON compact** - Use short key names for bandwidth
2. **Validate JSON** - Always check for parse errors
3. **Use appropriate buffer sizes** - Calculate based on max message size
4. **Send related data together** - Group sensors that update at the same rate
5. **Include timestamp** - Helps track message freshness
6. **Use static allocation** - `StaticJsonDocument` for embedded systems
7. **Round float values** - Use `%.2f` to limit decimal places
