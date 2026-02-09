
#include "wifi_tools.h"

WiFi_Tools::WiFi_Tools() {}

WiFi_Tools wifi_tools;

void WiFi_Tools::begin(const char * ssid, const char * pass) {
	// Store credentials for reconnection
	strncpy(_ssid, ssid, sizeof(_ssid) - 1);
	_ssid[sizeof(_ssid) - 1] = '\0';
	strncpy(_password, pass, sizeof(_password) - 1);
	_password[sizeof(_password) - 1] = '\0';
	
	WiFi.disconnect(true);  // Clear any previous connection state
	delay(100);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(false);
	WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power for better stability
	WiFi.setSleep(false);  // Disable WiFi sleep for stable connection
	WiFi.persistent(false);
	WiFi.onEvent(_event_handler);
	WiFi.begin(ssid, pass);
	Serial.println("WiFi connecting...");
}

void WiFi_Tools::reconnect() {
	
	if (!_should_reconnect) return;
	
	// Use longer interval after auth failures
	unsigned long interval = _last_was_auth_fail ? AUTH_FAIL_RETRY_INTERVAL : RECONNECT_INTERVAL;
	
	if (millis() - _reconnect_timer > interval) {
		Serial.print("\n\treconnecting");
		if (_last_was_auth_fail) {
			Serial.print(" (auth retry #");
			Serial.print(_auth_fail_count);
			Serial.print(")...");
		} else {
			Serial.print("...");
		}
		
		// For auth failures, do a full disconnect and reconnect with credentials
		if (_last_was_auth_fail) {
			WiFi.disconnect(true);
			delay(1000);  // Longer delay for auth failures
			WiFi.begin(_ssid, _password);  // Full reconnect with credentials
			_last_was_auth_fail = false;  // Reset flag
		} else {
			// Normal disconnect
			WiFi.disconnect();
			delay(500);
			WiFi.reconnect();
		}
		
		_reconnect_timer = millis();
	}
}

void WiFi_Tools::_event_handler(WiFiEvent_t event, WiFiEventInfo_t info) {

	if (wifi_tools._event_logging_enabled) wifi_tools._log_event(event, info);
	
	if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
		if (wifi_tools.is_connected) Serial.println("\n\tdisconnected...");
		wifi_tools.is_connected = false;
		uint8_t reason = info.wifi_sta_disconnected.reason;
		bool user_disconnected = (reason == WIFI_REASON_ASSOC_LEAVE);
		// Handle auth failures and timeouts specifically
		bool auth_fail = (reason == WIFI_REASON_AUTH_EXPIRE || 
		                  reason == WIFI_REASON_AUTH_FAIL ||
		                  reason == WIFI_REASON_ASSOC_FAIL ||
		                  reason == WIFI_REASON_HANDSHAKE_TIMEOUT ||
		                  reason == WIFI_REASON_CONNECTION_FAIL ||
		                  reason == 39); // TIMEOUT
		
		if (auth_fail) {
			wifi_tools._last_was_auth_fail = true;
			wifi_tools._auth_fail_count++;
			Serial.print("\tAuth failure #");
			Serial.print(wifi_tools._auth_fail_count);
			Serial.println(" - will retry in 30s...");
		} else {
			wifi_tools._last_was_auth_fail = false;
		}
		
		wifi_tools._should_reconnect = !(user_disconnected || wifi_tools._first_disconnect);
		wifi_tools._first_disconnect = false;
	}

	if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
		if (!wifi_tools.is_connected) {
			Serial.println("\n\tconnected!");
			Serial.print("\tIP: ");
			Serial.println(WiFi.localIP().toString());
			// Reset auth fail counter on successful connection
			wifi_tools._auth_fail_count = 0;
			wifi_tools._last_was_auth_fail = false;
		}
		wifi_tools.is_connected = true;
	}

}


void WiFi_Tools::log_events() {
	_event_logging_enabled = true;
}