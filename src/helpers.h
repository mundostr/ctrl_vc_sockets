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

void cbCtrlNotificacion();

Task ctrlLed(100, TASK_FOREVER, &cbCtrlLed);
Task ctrlAceleracion(50, TASK_FOREVER, &cbCtrlAceleracion);
Task ctrlHall(FREC_ACT_RPM, TASK_FOREVER, &cbCtrlHall);
Task ctrlSocket(3000, TASK_FOREVER, &cbCtrlSocket);
Task ctrlNotificacion(1000, TASK_FOREVER, &cbCtrlNotificacion);

void cbCtrlNotificacion() {
	if (!ctrlNotificacion.isFirstIteration()) {
		frec_parpadeo_activo = 1000;
		ctrlLed.setInterval(frec_parpadeo_activo);
		ctrlNotificacion.disable();
	}
}

void inicializarFS() {
	while (!LittleFS.begin()) delay(500);
}

bool recuperarConfig() {
	File archivoConfig = LittleFS.open("/config.txt", "r");
	if (!archivoConfig) return false;

	int valor = 0;
	float valorFloat = 0.0;
	String parametro;
	
	while (archivoConfig.available()) {
		parametro = archivoConfig.readStringUntil('=');
		parametro == "rpmMotor" ? valorFloat = archivoConfig.readStringUntil('\n').toFloat() : valor = archivoConfig.readStringUntil('\n').toInt();
		// valor = archivoConfig.readStringUntil('\n').toInt();

		if (parametro == "retardoPartida") { parametros.partida = valor; }
		if (parametro == "tiempoVuelo") { parametros.vuelo = valor; }
		if (parametro == "rpmMotor") { parametros.rpm = valorFloat; }
		if (parametro == "polosMotor") { parametros.polos = valor; }
		if (parametro == "offsetMotor") { parametros.offsetrpm = valor; }
		if (parametro == "toleranciaRpm") { parametros.tolajrpm = valor; }
		if (parametro == "ajusteRpmTrepadas") { parametros.ajrpmtrep = valor; }
		if (parametro == "ajusteRpmBajadas") { parametros.ajrpmbaj = valor; }
	}

	rpmObj = MIN_ESC + (parametros.rpm * 10);
	// kRpm = ((1000 / FREC_ACT_RPM) * 60) / parametros.polos;

	archivoConfig.close();

	return (parametros.partida !=  -9999) && (parametros.vuelo != -9999) && (parametros.rpm != -9999) & (parametros.polos != -9999) && (parametros.offsetrpm != -9999) && (parametros.tolajrpm != -9999) && (parametros.ajrpmtrep != -9999) && (parametros.ajrpmbaj != -9999) ? true : false;
}

bool actualizarConfig() {
	static bool retorno = false;

	File archivoConfig = LittleFS.open("/config.txt", "w");
	if (!archivoConfig) return false;

	String cadena = "retardoPartida=" + String(parametros.partida) + "\ntiempoVuelo=" + String(parametros.vuelo) + "\nrpmMotor=" + String(parametros.rpm) + "\npolosMotor=" + String(parametros.polos) + "\noffsetMotor=" + String(parametros.offsetrpm) + "\ntoleranciaRpm=" + String(parametros.tolajrpm) + "\najusteRpmTrepadas=" + String(parametros.ajrpmtrep) + "\najusteRpmBajadas=" + String(parametros.ajrpmbaj) + "\n";
	int escrito = archivoConfig.print(cadena);
	escrito > 0 ? retorno = true : retorno = false;

	archivoConfig.close();

	if (retorno) {
		frec_parpadeo_activo = 100;
		ctrlLed.setInterval(frec_parpadeo_activo);
		ctrlNotificacion.enableIfNot();
	}

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
	tareas.addTask(ctrlNotificacion);
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

		// DynamicJsonDocument doc(2048);
		// vJsonObject objJson = doc.to<JsonObject>();
		// objJson["topico"] = "config";
		// objJson["modo"] = modo;
		
		partida = "{ \"id\": 1, \"titulo\": \"Demora partida (segs)\", \"min\": 0, \"max\": 60, \"steps\": 5, \"actual\": " + String(parametros.partida) + " }";
		vuelo = "{ \"id\": 2, \"titulo\": \"Tiempo vuelo (segs)\", \"min\": 30, \"max\": 360, \"steps\": 1, \"actual\": " + String(parametros.vuelo) + " }";
		rpm = "{ \"id\": 3, \"titulo\": \"Regimen motor (% ESC)\", \"min\": 20, \"max\": 100, \"steps\": 0.1, \"actual\": " + String(parametros.rpm) + " }";
		polos = "{ \"id\": 4, \"titulo\": \"Polos motor\", \"min\": 1, \"max\": 16, \"steps\": 1, \"actual\": " + String(parametros.polos) + " }";
		offsetrpm = "{ \"id\": 5, \"titulo\": \"Offset lectura rpm\", \"min\": -500, \"max\": 500, \"steps\": 1, \"actual\": " + String(parametros.offsetrpm) + " }";
		tolajrpm = "{ \"id\": 6, \"titulo\": \"Tolerancia variación rpm\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.tolajrpm) + " }";
		ajrpmtrep = "{ \"id\": 7, \"titulo\": \"Ajuste rpm trepadas\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.ajrpmtrep) + " }";
		ajrpmbaj = "{ \"id\": 8, \"titulo\": \"Ajuste rpm bajadas\", \"min\": 0, \"max\": 2000, \"steps\": 50, \"actual\": " + String(parametros.ajrpmbaj) + " }";
		
		// objJson["datos"] = "[ " + partida + ", " + vuelo + ", " + rpm + ", " + polos + ", " + offsetrpm + ", " + tolajrpm + ", " + ajrpmtrep + ", " + ajrpmbaj + " ]";
		
		// char bufferJson[2048];
		// serializeJson(objJson, bufferJson);
		
		// char cadenaJson[30] = "{'topico':'config','dato':10}";
		// String cadenaJson = "{ \"topico\": \"config\", \"datos\": { \"partida\": " + String(parametros.partida) + ", \"vuelo\": " + String(parametros.vuelo) + ", \"rpm\": " + String(parametros.rpm) + ", \"offset\": " + String(parametros.offsetrpm) + ", \"polos\": " + String(parametros.polos) + " } }";
		// char bufferMsj[64];
		// sprintf(bufferMsj, "'{\"topico\": \"config\", \"polos\": %d}'", parametros.polos);
		
		String buffer = "{ \"topico\": \"config\", \"modo\": " + String(modo) + ", \"datos\": [ " + partida + ", " + vuelo + ", " + rpm + ", " + polos + ", " + offsetrpm + ", " + tolajrpm + ", " + ajrpmtrep + ", " + ajrpmbaj + " ] }";
		webSocket.sendTXT(num, buffer);
		ctrlSocket.enableIfNot();
		break;
	}

	case WStype_TEXT: {
		String cadenaRecibida = "";
		for (int i = 0; i < (int)length; i++) cadenaRecibida += (char)payload[i];
		
		StringSplitter *cadenaProcesada = new StringSplitter(cadenaRecibida, '|', 2);
		const String topico = cadenaProcesada->getItemAtIndex(0);
		const String datos = cadenaProcesada->getItemAtIndex(1);

		if (topico == "config") {
			// DynamicJsonDocument doc(2048);
			// DeserializationError errorJson = deserializeJson(doc, cadenaRecibida);
			// JsonObject objJson = doc.as<JsonObject>();
			// const char *topico = objJson["topico"];

			// Recordar actualizar en StringSplitter.h el límite MAX!!!
			StringSplitter *cadenaDatos = new StringSplitter(datos, ',', 8);

			const int partida = cadenaDatos->getItemAtIndex(0).toInt();
			const int vuelo = cadenaDatos->getItemAtIndex(1).toInt();
			const float rpm = cadenaDatos->getItemAtIndex(2).toFloat();
			const int polos = cadenaDatos->getItemAtIndex(3).toInt();
			const int offsetrpm = cadenaDatos->getItemAtIndex(4).toInt();
			const int tolajrpm = cadenaDatos->getItemAtIndex(5).toInt();
			const int ajrpmtrep = cadenaDatos->getItemAtIndex(6).toInt();
			const int ajrpmbaj = cadenaDatos->getItemAtIndex(7).toInt();

			if (parametros.partida != partida) parametros.partida = partida;
			if (parametros.vuelo != vuelo) parametros.vuelo = vuelo;
			if (parametros.rpm != rpm) parametros.rpm = rpm;
			if (parametros.polos != polos) parametros.polos = polos;
			if (parametros.offsetrpm != offsetrpm) parametros.offsetrpm = offsetrpm;
			if (parametros.tolajrpm != tolajrpm) parametros.tolajrpm = tolajrpm;
			if (parametros.ajrpmtrep != ajrpmtrep) parametros.ajrpmtrep = ajrpmtrep;
			if (parametros.ajrpmbaj != ajrpmbaj) parametros.ajrpmbaj = ajrpmbaj;

			if (actualizarConfig()) {
				webSocket.sendTXT(0, "{ \"topico\": \"confirmacion\", \"resultado\": \"OK\", \"mensaje\": \"Configuración actualizada\" }");
			} else {
				webSocket.sendTXT(0, "{ \"topico\": \"confirmacion\", \"resultado\": \"ERROR\", \"mensaje\": \"ERROR al actualizar\" }");
			}
		}

		if (topico == "altbateria") { // Act/des reporte estado bateria
			reportarBateria = !reportarBateria;
		}

		if (topico == "altrpm") { // Act/des reporte rpm
			reportarRpm = !reportarRpm;
		}

		if (topico == "altvuelo") { // Act/des secuencia vuelo
			if (modo == LISTO) {
				activarPartida();
			} else {
				modo = DETENIDO;
				webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 4 }");
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
	// webSocket.enableHeartbeat(5000, 5000, 0); // Heartbeat
}

void activarSrvArchivos() {
	// server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/index.html", "text/html");
	// });

	// server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/favicon.ico", "image/ico");
	// });

	// server.on("/mui.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/mui.min.css", "text/css");
	// });

	// server.on("/estilos.css", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/estilos.css", "text/css");
	// });

	// server.on("/mui.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/mui.min.js", "text/javascript");
	// });

	// server.on("/vue.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send(LittleFS, "/vue.min.js", "text/javascript");
	// });

	// server.begin();
}