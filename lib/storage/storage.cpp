
#include "storage.h"
#include "Preferences.h"
Preferences preferences;

Storage::Storage() {}

Storage storage;


bool Storage::creds_already_exist(char * ssid, char * pass) {
    bool creds_exist = false;
    preferences.begin("creds");
    if (preferences.isKey("ssid")) {
        Serial.println("\n\tstored credentials exist - will use them");
        strcpy(ssid, preferences.getString("ssid").c_str());
        strcpy(pass, preferences.getString("pass").c_str());
        creds_exist = true;
    } else {
        Serial.println("\n\tno stored credentials - will provision");
    }
    preferences.end();
    return creds_exist;
}

        
void Storage::store_creds(char * ssid, char * pass) {
    preferences.begin("creds");
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.end();
}


void Storage::clear_creds() {
    preferences.begin("creds");
    preferences.clear();
    preferences.end();
}