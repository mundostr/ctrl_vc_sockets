#define LED 2 // Led testigo
#define ESC 12 // Variador
#define SENSOR_HALL 13 // Sensor rpm
#define PULSADOR 14 // Botón de inicio

#define FREC_INICIO 3000 // tiempo a aguardar c/botón pulsado p/iniciar secuencia vuelo
#define FREC_PARPADEO 250 // frecuencia standard de parpadeo led
#define FREC_PULSADOR 25 // frecuencia refresco lectura pulsador
#define FREC_CHEQUEO_ESTACIONES 100 // frecuencia control cliente wf conectado
#define FREC_ACT_RPM 100 // Frecuencia actualización cálculo rpm

#define CTD_CONFIG 8 // items configurables en interfaz
#define MIN_ESC 1000 // Pulso mínimo p/ESC (ms)
#define MED_ESC 1500 // Pulso medio p/ESC (ms)
#define MAX_ESC 2000 // Pulso máximo p/ESC (ms)
#define PORC_RPM_NOTIF 0.85 // % tiempo vuelo p/notificación parada
#define PORC_RPM_ATERRIZAJE 1.2 // % extra motor p/aterrizaje
#define CONF_INICIO 500 // Duración aviso de confirmación inicio
#define ANTICIPO_INICIO 100 // Momento notificación inicio (ms)
#define TIEMPO_NOTIF 1000 // duración reducción rpm notificación parada
#define TIEMPO_RPM_REF 10000 // Momento p/registro rpm referencia luego del despegue
#define ANTICIPO_NOTIF 5000 // Momento notificación parada (ms)

const char* SSIDWF = "ctrl_vc_promo"; // SSID AP WiFi, cualquier nombre válido
const char* CLAVEWF = "ctrl_vc_promo"; // Clave AP WiFi, cualquier clave válida (mínimo 8 caracteres)
const char* SSIDWF_ST = "ssid"; // Datos wifi local (solo para depuración, no son necesarios en el uso habitual)
const char* CLAVEWF_ST = "clave"; // Datos wifi local (solo para depuración, no son necesarios en el uso habitual)

bool procesarUnaVez = true; // Helper p/ ejecutar procesos aislados solo cuando es necesario
bool btnPresionado = false; // Flag indicador estado pulsador
bool aceleracionFinalizada = false; // Flag estado aceleración
bool cambiarRpmObj = false;
bool clienteAPConectado = false;
bool pulsoContado = false; // Flag antirebote pulsos rpm
bool reportarRpm = false; // Switch p/envío remoto datos rpm
bool reportarBateria = false; // Switch p/envío remoto datos bateria

int tPartidaMs; // demora partida pasado a ms
int tVueloMs; // tiempo de vuelo pasado a ms
uint8_t idClienteSocket; // ID del cliente websocket
unsigned int regimenActualMotor; // Helper p/almacenamiento regimen actual
unsigned int frec_parpadeo_activo = 0; // Helper indicador frecuencia parpadeo actual
unsigned int rpmObj, rpmActual, rpmDisplay, rpmTolMin, rpmTolMax;
volatile int contadorHall = 0; // Variable conteo pulsos sensor hall
unsigned long timerInicio, timerCentral, timerHall; // Timer ctrl inicio secuencia, tiempos generales y espera hall
float kRpm = 0; // Coeficiente para cálculo de rpm

struct formatoParametros { int partida = -9999; int vuelo = -9999; int rpm = -9999; int polos = -9999; int offsetrpm = -9999; int tolajrpm = -9999; int ajrpmtrep = -9999; int ajrpmbaj = -9999; } parametros; // parámetros de vuelo

enum modos { LISTO, EN_PARTIDA, ACELERANDO, EN_VUELO, DETENIDO } modo; // modos de vuelo

Scheduler tareas; // Gestor de tareas
Bounce debouncer = Bounce(); // Obj ctrl pulsador
// AsyncWebServer server(80); // Ojc ctrl servidor archivos
WebSocketsServer webSocket = WebSocketsServer(1337); // Obj ctrl socket
Servo esc; // Obj ctrl variador