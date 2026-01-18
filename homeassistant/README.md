# Home Assistant Integration for ESP32 Device

## Setup Instructions

### 1. Enable Packages in Home Assistant

Edit your `configuration.yaml` and add:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

### 2. Create Packages Directory

```bash
mkdir -p config/packages
```

### 3. Copy the Package File

Copy `esp32_device.yaml` to your Home Assistant `config/packages/` directory.

### 4. Restart Home Assistant

Go to **Settings** > **System** > **Restart**

## Features

### MQTT Entities

- **Sensor: ESP32 Temperature** - Displays temperature readings from the device
- **Binary Sensor: ESP32 Connected** - Shows connection status
- **Switch: ESP32 OTA Update** - Triggers OTA update mode
- **Input Text: ESP32 Custom Message** - Send custom messages to the device

### Automations

1. **Send Custom Message** - Automatically publishes text to the `beacon` topic when you type in the input field
2. **Trigger OTA Update** - Publishes OTA_UPDATE command when the switch is turned on

## Dashboard

Add this card to your Lovelace dashboard:

```yaml
type: vertical-stack
cards:
  - type: entities
    title: ESP32 Device Control
    entities:
      - entity: binary_sensor.esp32_connected
        name: Connection Status
      - entity: sensor.esp32_temperature
        name: Temperature
      - entity: switch.esp32_ota_update
        name: Trigger OTA Update
      - entity: input_text.esp32_message
        name: Send Message
  - type: history-graph
    title: Temperature History
    entities:
      - entity: sensor.esp32_temperature
    hours_to_show: 24
    refresh_interval: 0
```

Or use the visual editor:
1. Edit Dashboard
2. Add Card > Vertical Stack
3. Add the entities listed above

## OTA Update Workflow

1. Make code changes to your ESP32 project
2. Build the firmware: `platformio run`
3. In Home Assistant, toggle **ESP32 OTA Update** switch ON
4. The device will publish "updating" status
5. Upload via OTA: `platformio run --target upload --upload-port <ESP32_IP>`
6. Device automatically returns to "ready" state

## MQTT Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `temperature` | ESP32 → HA | Temperature sensor data |
| `beacon` | HA → ESP32 | Command/message topic |
| `esp32/status` | ESP32 → HA | Device online/offline status |
| `esp32/ota/state` | ESP32 → HA | OTA state: ready/updating |

## Customization

### Change Temperature Unit

Edit `esp32_device.yaml`:
```yaml
unit_of_measurement: "°C"  # Change from °F to °C
```

### Add More Sensors

Add to the ESP32 code (`device.cpp`):
```cpp
mqtt.publish("humidity", humidity_value);
```

Add to Home Assistant package:
```yaml
- name: "ESP32 Humidity"
  state_topic: "humidity"
  unit_of_measurement: "%"
  device_class: humidity
```

## Troubleshooting

**No entities appearing:**
- Check MQTT integration is configured in Home Assistant
- Verify ESP32 is connected and publishing
- Check logs: Settings > System > Logs

**Temperature not updating:**
- Verify MQTT broker is running
- Check ESP32 serial output for MQTT connection
- Test with MQTT Explorer tool

**OTA not working:**
- Ensure ESP32 IP address is accessible from HA
- Check firewall settings
- Verify OTA password matches (default: "admin")
