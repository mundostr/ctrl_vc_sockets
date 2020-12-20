void setearMotor(int pulso) {
	esc.writeMicroseconds(pulso);
	regimenActualMotor = pulso;
}

void cbCtrlLed() {
	digitalWrite(LED, !digitalRead(LED));
}

void cbCtrlAceleracion() {
	if (rpmActual < rpmObj) {
		rpmActual += 10;
		setearMotor(rpmActual);
	} else {
		setearMotor(rpmObj);
		aceleracionFinalizada = true;
	}
}

void cbCtrlVuelo() {
	static int ultimoAjuste = 1; // 0 bajada, 2 trepada
	if (procesarUnaVez) {
		rpmTolMin = rpmDisplay - parametros.tolajrpm;
		rpmTolMax = rpmDisplay + parametros.tolajrpm;
		webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"confirmacion\", \"mensaje\": \"Governor activo\", \"rpmDisplay\": \"" + String(rpmDisplay) + "\", \"rpmObj\": \"" + String(rpmObj) + "\", \"rpmTolMin\": \"" + String(rpmTolMin) + "\", \"rpmTolMax\": \"" + String(rpmTolMax) + "\" }");
		procesarUnaVez = false;
	}

	if (rpmDisplay < rpmTolMin) {
		if (ultimoAjuste == 0) {
			setearMotor(rpmObj);
		} else {
			setearMotor(rpmObj + parametros.ajrpmtrep);
			ultimoAjuste = 2;
		}
	} else if (rpmDisplay > rpmTolMax) {
		if (ultimoAjuste == 2) {
			setearMotor(rpmObj);
		} else {
			setearMotor(rpmObj - parametros.ajrpmbaj);
			ultimoAjuste = 0;
		}
	}
}

Task ctrlVuelo(200, TASK_FOREVER, &cbCtrlVuelo);
void cbCtrlHall() {
	static unsigned int rpmDisplayAnterior = 0;
		
	bool cond1 = !ctrlVuelo.isEnabled();
	bool cond2 = (rpmDisplay - rpmDisplayAnterior < (unsigned)parametros.tolajrpm) && ctrlVuelo.isEnabled();
	rpmDisplay = (((1000 / FREC_ACT_RPM) * contadorHall * 60) / (parametros.polos / 2)) + parametros.offsetrpm;

	if ((rpmDisplay != rpmDisplayAnterior) && (cond1 || cond2)) {
		if (clienteAPConectado && reportarRpm) webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"rpm\", \"valor\": \"" + String(rpmDisplay) + "\" }");
		rpmDisplayAnterior = rpmDisplay;
	}

	contadorHall = 0;
}

void cbCtrlSocket() {
	String cadenaJson = "{ \"topico\": \"ping\" }";
	webSocket.broadcastTXT(cadenaJson);
}

Task ctrlLed(100, TASK_FOREVER, &cbCtrlLed);
Task ctrlAceleracion(50, TASK_FOREVER, &cbCtrlAceleracion);
Task ctrlHall(FREC_ACT_RPM, TASK_FOREVER, &cbCtrlHall);
Task ctrlSocket(3000, TASK_FOREVER, &cbCtrlSocket);

void ajustarParametrosRpm() {
	rpmObj = MIN_ESC + (parametros.rpm * 10);
	// kRpm = ((1000 / FREC_ACT_RPM) * 60) / parametros.polos;
}

void inicializarFS() {
	while (!LittleFS.begin()) delay(500);
}

bool recuperarConfigAnt() {
	// File archivoJson = LittleFS.open("/config.json", "r");
	// if (!archivoJson) return false;

	// // const size_t capacidad = JSON_OBJECT_SIZE(8) + 70;
	// DynamicJsonDocument configJson(8192);
	// deserializeJson(configJson, archivoJson);

	// parametros.partida = configJson["partida"];
	// parametros.vuelo = configJson["vuelo"];
	// parametros.rpm = configJson["rpm"];
	// parametros.polos = configJson["polos"];
	// parametros.offsetrpm = configJson["offset"];
	// parametros.tolajrpm = configJson["tolajrpm"];
	// parametros.ajrpmtrep = configJson["ajrpmtrep"];
	// parametros.ajrpmbaj = configJson["ajrpmbaj"];

	// ajustarParametrosRpm();

	// archivoJson.close();
	
	return true;
}

bool recuperarConfig() {
	File archivoConfig = LittleFS.open("/config.txt", "r");
	if (!archivoConfig) return false;

	String linea = "";
	linea = archivoConfig.readStringUntil('\n');
	parametros.partida = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.vuelo = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.rpm = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.polos = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.offsetrpm = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.tolajrpm = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.ajrpmtrep = linea.toInt();
	linea = archivoConfig.readStringUntil('\n');
	parametros.ajrpmbaj = linea.toInt();

	ajustarParametrosRpm();

	archivoConfig.close();

	return true;
}

bool actualizarConfigAnt() {
	// File archivoJson = LittleFS.open("/config.json", "w");
	// if (!archivoJson) return false;

	// // const size_t capacidad = JSON_OBJECT_SIZE(8) + 70;
	// DynamicJsonDocument configJson(8192);
	// configJson["partida"] = parametros.partida;
	// configJson["vuelo"] = parametros.vuelo;
	// configJson["rpm"] = parametros.rpm;
	// configJson["polos"] = parametros.polos;
	// configJson["offset"] = parametros.offsetrpm;
	// configJson["tolajrpm"] = parametros.tolajrpm;
	// configJson["ajrpmtrep"] = parametros.ajrpmtrep;
	// configJson["ajrpmbaj"] = parametros.ajrpmbaj;
	
	// // serializeJson(configJson, archivoJson);
	// Serial.println("actualiza");

	// ajustarParametrosRpm();
	// cambiarRpmObj = true;
	
	// archivoJson.close();
	
	return true;
}

bool actualizarConfig() {
	static bool retorno = false;

	File archivoConfig = LittleFS.open("/config.txt", "w");
	if (!archivoConfig) return false;

	String cadena = String(parametros.partida) + "\n" + String(parametros.vuelo) + "\n" + String(parametros.rpm) + "\n" + String(parametros.polos) + "\n" + String(parametros.offsetrpm) + "\n" + String(parametros.tolajrpm) + "\n" + String(parametros.ajrpmtrep) + "\n" + String(parametros.ajrpmbaj) + "\n";
	int escrito = archivoConfig.print(cadena);
	escrito > 0 ? retorno = true : retorno = false;

	archivoConfig.close();

	return retorno;
}

void ICACHE_RAM_ATTR contarHall() {
	if (!pulsoContado) {
		contadorHall++;
		pulsoContado = true;
	}
}

void inicializarPines() {
	pinMode(LED, OUTPUT);
	digitalWrite(LED, HIGH); // recordar led onboard negado ESP8266 12F

	pinMode(PULSADOR, INPUT_PULLUP);
	debouncer.attach(PULSADOR);
	debouncer.interval(FREC_PULSADOR);

	esc.attach(ESC, MIN_ESC, MAX_ESC);

	pinMode(SENSOR_HALL, INPUT);
	attachInterrupt(digitalPinToInterrupt(SENSOR_HALL), contarHall, RISING);
}

void activarPartida() {
	frec_parpadeo_activo = 250;
	ctrlLed.setInterval(frec_parpadeo_activo);
	tPartidaMs = parametros.partida * 1000;
	tVueloMs = parametros.vuelo * 1000;
	timerHall = 0;
	timerCentral = millis();
	procesarUnaVez = true;
	modo = EN_PARTIDA;
	webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 1 }");
}

void controlarPulsador() {
	debouncer.update();
	
	if (debouncer.fell()) {
		if (!btnPresionado) {
			timerInicio = millis();
			btnPresionado = true;
		}
	}

	if (debouncer.rose()) {
		if (btnPresionado) btnPresionado = false;
	}

	if (millis() - timerInicio >= FREC_INICIO && btnPresionado) {
		if (modo == LISTO) {
			activarPartida();
		} else {
			if (modo != DETENIDO) modo = LISTO;
		}
		timerInicio = millis();
	}
}

void armarVariador() {
	setearMotor(MAX_ESC);
	delay(2000);
	setearMotor(MIN_ESC);
}

void inicializarTareas() {
	tareas.init();
	tareas.addTask(ctrlLed);
	tareas.addTask(ctrlAceleracion);
	tareas.addTask(ctrlVuelo);
	tareas.addTask(ctrlHall);
	tareas.addTask(ctrlSocket);
}

void eventosWS(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
	String partida, vuelo, rpm, polos, offsetrpm, tolajrpm, ajrpmtrep, ajrpmbaj;

	switch (type) {
	case WStype_DISCONNECTED: {
		clienteAPConectado = false;
		ctrlSocket.disable();
		break;
	}

	case WStype_CONNECTED: {
		clienteAPConectado = true;
		idClienteSocket = num;
		// IPAddress ipCliente = webSocket.remoteIP(num);

		DynamicJsonDocument doc(2048);
		JsonObject objJson = doc.to<JsonObject>();
		objJson["topico"] = "config";
		objJson["modo"] = modo;
		
		partida = "{ \"id\": 1, \"titulo\": \"Demora partida (segs)\", \"min\": 0, \"max\": 60, \"steps\": 5, \"actual\": " + String(parametros.partida) + " }";
		vuelo = "{ \"id\": 2, \"titulo\": \"Tiempo vuelo (segs)\", \"min\": 30, \"max\": 180, \"steps\": 5, \"actual\": " + String(parametros.vuelo) + " }";
		rpm = "{ \"id\": 3, \"titulo\": \"Regimen motor (% ESC)\", \"min\": 30, \"max\": 100, \"steps\": 1, \"actual\": " + String(parametros.rpm) + " }";
		polos = "{ \"id\": 4, \"titulo\": \"Polos motor\", \"min\": 1, \"max\": 16, \"steps\": 1, \"actual\": " + String(parametros.polos) + " }";
		offsetrpm = "{ \"id\": 5, \"titulo\": \"Offset lectura rpm\", \"min\": -500, \"max\": 500, \"steps\": 1, \"actual\": " + String(parametros.offsetrpm) + " }";
		tolajrpm = "{ \"id\": 6, \"titulo\": \"Tolerancia variación rpm\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.tolajrpm) + " }";
		ajrpmtrep = "{ \"id\": 7, \"titulo\": \"Ajuste rpm trepadas\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.ajrpmtrep) + " }";
		ajrpmbaj = "{ \"id\": 8, \"titulo\": \"Ajuste rpm bajadas\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.ajrpmbaj) + " }";
		
		objJson["datos"] = "[ " + partida + ", " + vuelo + ", " + rpm + ", " + polos + ", " + offsetrpm + ", " + tolajrpm + ", " + ajrpmtrep + ", " + ajrpmbaj + " ]";
		
		char bufferJson[2048];
		serializeJson(objJson, bufferJson);
		
		// char cadenaJson[30] = "{'topico':'config','dato':10}";
		// String cadenaJson = "{ \"topico\": \"config\", \"datos\": { \"partida\": " + String(parametros.partida) + ", \"vuelo\": " + String(parametros.vuelo) + ", \"rpm\": " + String(parametros.rpm) + ", \"offset\": " + String(parametros.offsetrpm) + ", \"polos\": " + String(parametros.polos) + " } }";
		// char bufferMsj[64];
		// sprintf(bufferMsj, "'{\"topico\": \"config\", \"polos\": %d}'", parametros.polos);
		
		webSocket.sendTXT(num, bufferJson);
		ctrlSocket.enableIfNot();
		break;
	}

	case WStype_TEXT: {
		// Se parsea a JSON el mensaje recibido p/ejecutar el comando correspondiente
		String cadenaRecibida = "";
		for (int i = 0; i < (int)length; i++) cadenaRecibida += (char)payload[i];

		DynamicJsonDocument doc(2048);
		DeserializationError errorJson = deserializeJson(doc, cadenaRecibida);
		if (errorJson) {
			webSocket.sendTXT(num, "{ \"topico\": \"confirmacion\", \"mensaje\": \"ERROR al procesar json\" }");
			// webSocket.broadcastTXT("error");
		} else {
			JsonObject objJson = doc.as<JsonObject>();
			
			const char *topico = objJson["topico"];

			if (strcmp("config", topico) == 0) { // Configuración de parámetros
				const int partida = objJson["parametros"]["1"];
				const int vuelo = objJson["parametros"]["2"];
				const int rpm = objJson["parametros"]["3"];
				const int polos = objJson["parametros"]["4"];
				const int offsetrpm = objJson["parametros"]["5"];
				const int tolajrpm = objJson["parametros"]["6"];
				const int ajrpmtrep = objJson["parametros"]["7"];
				const int ajrpmbaj = objJson["parametros"]["8"];

				if (parametros.partida != partida) parametros.partida = partida;
				if (parametros.vuelo != vuelo) parametros.vuelo = vuelo;
				if (parametros.rpm != rpm) parametros.rpm = rpm;
				if (parametros.polos != polos) parametros.polos = polos;
				if (parametros.offsetrpm != offsetrpm) parametros.offsetrpm = offsetrpm;
				if (parametros.tolajrpm != tolajrpm) parametros.tolajrpm = tolajrpm;
				if (parametros.ajrpmtrep != ajrpmtrep) parametros.ajrpmtrep = ajrpmtrep;
				if (parametros.ajrpmbaj != ajrpmbaj) parametros.ajrpmbaj = ajrpmbaj;

				bool actualizacion = actualizarConfig();
				if (actualizacion) {
					webSocket.sendTXT(0, "{ \"topico\": \"confirmacion\", \"mensaje\": \"Configuración actualizada\" }");
				} else {
					webSocket.sendTXT(0, "{ \"topico\": \"confirmacion\", \"mensaje\": \"ERROR al actualizar\" }");
				}
			}

			if (strcmp("altbateria", topico) == 0) { // Act/des reporte estado bateria
				reportarBateria = !reportarBateria;
			}

			if (strcmp("altrpm", topico) == 0) { // Act/des reporte rpm
				reportarRpm = !reportarRpm;
			}

			if (strcmp("altvuelo", topico) == 0) { // Act/des secuencia vuelo
				if (modo == LISTO) {
					activarPartida();
				} else {
					modo = DETENIDO;
					webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 4 }");
				}
			}
		}
		break;
	}

	case WStype_BIN: {
		// USE_SERIAL.printf("[%u] binario long: %u\n", num, length);
		// hexdump(payload, length);
		// webSocket.sendBIN(num, payload, length);
		break;
	}
	
	case WStype_PING: {
		break;
	}

	case WStype_PONG: {
		break;
	}

	case WStype_ERROR:
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN: {
		break;
	}
	}
}

void activarSocket() {
	webSocket.begin();
	webSocket.onEvent(eventosWS); // Gestor de eventos socket
	// webSocket.setAuthorization("pepe", "pompin");
	webSocket.enableHeartbeat(5000, 500, 2); // Marca de vida
}