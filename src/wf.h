void activarAP() {
	// https://github.com/esp8266/Arduino/blob/41074245a8d3029270622a8b489d53fdfedb2a22/libraries/ESP8266WiFi/src/ESP8266WiFiGeneric.cpp#L169-L179
	// uint8_t val = (20.5 * 4.0f);
	// system_phy_set_max_tpw(val);
	WiFi.disconnect(true);
	while (!WiFi.softAP(SSIDWF, CLAVEWF, 1, false, 2)) delay(500);
}

void activarST() {
	// IPAddress staticIP(192, 168, 0, 220);
	// IPAddress gateway(192, 168, 0, 1);
	// IPAddress subnet(255, 255, 255, 0);
	WiFi.disconnect(true);
	WiFi.mode(WIFI_STA);
	WiFi.begin(SSIDWF_ST, CLAVEWF_ST);
	while (WiFi.status() != WL_CONNECTED) delay(500);
	// Serial.println(WiFi.localIP());
}
