/*
	CONTROLADOR VC PROMOCIONAL
	Características:
		1) Seteo rpm objetivo motor
		2) Confirmación inicio ciclo
		3) Gestión demora partida
		4) Aceleración progresiva despegue
		5) Control tiempo vuelo
		6) Ajuste dinámico rpm vuelo entre rango min y max s/fluctuación rpm
		7) Aviso detención previo a corte
		8) Interfaz móvil configuración, reporte rpm y parada emergencia
*/

#include <Bounce2.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <TaskScheduler.h>
#include <WebSocketsServer.h>
// #include <ESPAsyncWebServer.h>
#include "Servo.h"
#include "StringSplitter.h"
#include "config.h"
#include "wf.h"
#include "helpers.h"

void setup() {
	Serial.begin(115200); // Inicialización consola serial (solo para debug)
	// Serial.setDebugOutput(true);

	delay(1000);
	ESP.wdtDisable(); // Deshabilitación watchdog por software

	inicializarPines(); // Seteo de pines
	armarVariador(); // Pulsos p/armado ESC, ejecutado primero para mayor compatibilidad
	inicializarTareas(); // Activación de tareas
	inicializarFS(); // Inicialización sistema de archivos (p/lectura datos config)

	if (recuperarConfig()) { // Si se recupera el archivo de config
		activarAP(); // Activación AP wifi p/configuración
		// activarST(); // Activación como cliente wifi p/depuración
		activarSocket(); // Activación socket acceso remoto
		// activarSrvArchivos(); // Activación servidor archivos
		frec_parpadeo_activo = 1000;
		modo = LISTO;
	} else { // sino el sistema queda detenido
		frec_parpadeo_activo = 100;
		modo = DETENIDO;
	}
	
	ctrlLed.setInterval(frec_parpadeo_activo); // se ajusta la frecuencia de parpadeo del led testigo
	ctrlLed.enableIfNot();
}

void loop() {
	static unsigned long timerAvisoParada;
	static bool confirmarPartida, confirmarDetencion;

	switch (modo) {
		case LISTO: {
			confirmarPartida = true;
			confirmarDetencion = true;
			// Se envía por única vez el pulso mínimo al variador y se queda en espera indefinida
			if (regimenActualMotor != MIN_ESC) setearMotor(MIN_ESC);
			break;
		}

		case EN_PARTIDA: {
			// Se da un arranque muy breve al motor p/notificar visualmente que inició el ciclo de vuelo
			if (millis() - timerCentral >= ANTICIPO_INICIO) {
				if (confirmarPartida) setearMotor(MED_ESC);
					
				// Se detiene luego el motor
				if (millis() - timerCentral >= ANTICIPO_INICIO + CONF_INICIO && confirmarPartida) {
					confirmarPartida = false;
					setearMotor(MIN_ESC);
				}

				// Si ha transcurrido la demora de partida, se activan las tareas necesarias y se pasa de modo
				if (millis() - timerCentral >= (unsigned)tPartidaMs) {
					rpmActual = MIN_ESC;
					timerCentral = millis();
					frec_parpadeo_activo = 500;
					ctrlLed.setInterval(frec_parpadeo_activo);
					ctrlHall.enableIfNot();
					ctrlAceleracion.enableIfNot();
					aceleracionFinalizada = false;
					modo = ACELERANDO;
					webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 2 }");
				}
			}
			break;
		}

		case ACELERANDO: {
			// La secuencia de aceleración es controlada por la tarea ctrlAceleracion
			// por lo cual en el modo simplemente se espera el cambio del flag aceleracionFinalizada p/pasar de modo
			if (aceleracionFinalizada) {
				ctrlAceleracion.disable();
				frec_parpadeo_activo = 250;
				// ctrlLed.setInterval(frec_parpadeo_activo);
				ctrlLed.disable();
				digitalWrite(LED, HIGH);
				timerCentral = millis();
				modo = EN_VUELO;
				webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 3 }");
			}
			break;
		}

		case EN_VUELO: {
			// Unos momentos después de iniciado el vuelo y estabilizadas las rpm
			// se registran los límites tolerables y se inicia la tarea de ctrl
			// parametros.tolajrpm en 0 deshabilita el governor
			if (millis() - timerCentral >= TIEMPO_RPM_REF && parametros.tolajrpm > 0) ctrlVuelo.enableIfNot();

			// Unos segs antes del cierre del vuelo, se genera una pequeña caída de rpm en el motor
			// como aviso de que el tiempo está pronto a terminar.
			if (millis() - timerCentral >= (unsigned)(tVueloMs - ANTICIPO_NOTIF)) {
				ctrlVuelo.disable();
				if (confirmarDetencion) {
					setearMotor(rpmObj * PORC_RPM_NOTIF);
					confirmarDetencion = false;
					timerAvisoParada = millis();
				} else {
					// y se pasa a un regimen ligeramente por arriba de las rpm promedio seteadas, para un mejor aterrizaje
					if (millis() - timerAvisoParada >= TIEMPO_NOTIF) {
						setearMotor(rpmObj * PORC_RPM_ATERRIZAJE);
						timerAvisoParada = millis();
					}
				}

				// Cumplido el tiempo total, simplemente se anulan las tareas de ctrl y se pasa de modo
				if (millis() - timerCentral >= (unsigned)tVueloMs) {
					ctrlHall.disable();
					modo = DETENIDO;
					frec_parpadeo_activo = 3000;
					ctrlLed.setInterval(frec_parpadeo_activo);
					webSocket.sendTXT(idClienteSocket, "{ \"topico\": \"estado\", \"modo\": 4 }");
				}
			}
			break;
		}

		case DETENIDO: {
			// Solo se detiene el motor, quedando en espera indefinida, el sistema requiere un reinicio para volver a operar
			if (regimenActualMotor != MIN_ESC) {
				setearMotor(MIN_ESC);
				digitalWrite(LED, HIGH);
			}
			break;
		}
	}

	tareas.execute(); // Control general de tareas
	webSocket.loop(); // Control del socket
	ESP.wdtFeed(); // Alimentación watchdog por hardware
	
	if (modo != DETENIDO) { // Controles en cualquier modo, excepto DETENIDO
		controlarPulsador();
		if (!digitalRead(SENSOR_HALL)) pulsoContado = false;
	}
}