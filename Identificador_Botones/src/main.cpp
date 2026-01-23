#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PCF8574.h>
#include "secrets.h"


// ==========================================
// CREDENCIALES
// ==========================================


WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================
// HARDWARE
// ==========================================
Adafruit_PCF8574 pcf;      // 0x20
Adafruit_PCF8574 pcf2;     // 0x21
bool hasPcf2 = false;

// ===== Mapeo PCF 0x20 =====
const int PIN_PCF_SW1_MERMAR   = 0; // P0
const int PIN_PCF_SW2_CLEAR    = 1; // P1
const int PIN_PCF_SW3_AUMENTAR = 2; // P2 

const int PIN_PCF_SW4_RAPIDO   = 3; // P3
const int PIN_PCF_SW5_MEDIO    = 4; // P4
const int PIN_PCF_SW6_LENTO    = 5; // P5

const int PIN_PCF_SW7_START    = 6; // P6
const int PIN_PCF_SW8_VACIAR   = 7; // P7

// ===== Mapeo PCF 0x21 =====
const int PIN_PCF2_SW9_PARAR   = 0; // P0

// Alias usados por la lógica de control (subir/bajar) sobre el PCF 0x20
const int PIN_PCF_BAJAR = PIN_PCF_SW1_MERMAR;    
const int PIN_PCF_SUBIR = PIN_PCF_SW3_AUMENTAR;  

// Líneas de segmentos del display (8 señales leídas como entradas)
const uint8_t PIN_SEG[] = {36, 23, 39, 22, 34, 21, 35, 19};

// Líneas de selección del multiplexado (D1/D2/D3). Estos GPIO también se usan para detectar SW4..SW9 por continuidad.
const uint8_t SEL_D1[] = {5, 33, 17};
const uint8_t SEL_D2[] = {25, 16, 26};  
const uint8_t SEL_D3[] = {4, 27, 15};   



const uint8_t PIN_SW_COMUN = 32; // Cable 15 (Pin 40 PIC) -> GPIO32


// ==========================================
// VARIABLES
// ==========================================
volatile char textD1[4] = "   ";
volatile char textD2[4] = "   ";
volatile char textD3[4] = "   ";

// Lógica Widget
int paso = 1;
int valor_preview = 0;
int valor_objetivo = 0;

// Control Motor
bool buscando = false;
bool enModoTurbo = false;
unsigned long ultimaAccion = 0;
unsigned long ultimaTelemetria = 0;

// Historial
int lastD1 = -999;
int lastD2 = -999;
int lastD3 = -999;


enum Velocidad : uint8_t { VEL_RAPIDO=0, VEL_MEDIO=1, VEL_LENTO=2 };
enum Estado    : uint8_t { EST_START=0, EST_PARAR=1 };

volatile Velocidad velocidad = VEL_MEDIO;
volatile Estado estado = EST_PARAR;

static uint32_t lastWiFiAttempt = 0;
static uint32_t lastMQTTAttempt = 0;
static const uint32_t WIFI_RETRY_MS = 3000;   // Reintento periódico para no bloquear el loop
static const uint32_t MQTT_RETRY_MS = 2000;   // Reintento periódico para mantener telemetría/RPC

// ==========================================
// FUNCIONES SEGURIDAD Y UTILIDAD
// ==========================================
void detenerMotor() {
  pcf.digitalWrite(PIN_PCF_BAJAR, LOW);
  pcf.digitalWrite(PIN_PCF_SUBIR, LOW);
  enModoTurbo = false;
}

int wrapSafe(int v) {
  if (v > 999) return v % 1000;
  if (v < 0) return 1000 + (v % 1000);
  return v;
}

int bufferToInt(volatile char* buf) {
  if (buf[0] == '?' || buf[1] == '?' || buf[2] == '?') return -1;
  int d1 = (buf[0] >= '0' && buf[0] <= '9') ? (buf[0] - '0') : 0;
  int d2 = (buf[1] >= '0' && buf[1] <= '9') ? (buf[1] - '0') : 0;
  int d3 = (buf[2] >= '0' && buf[2] <= '9') ? (buf[2] - '0') : 0;
  return (d1 * 100) + (d2 * 10) + d3;
}

const char* velToStr(Velocidad v) {
  switch (v) {
    case VEL_RAPIDO: return "RAPIDO";
    case VEL_MEDIO:  return "MEDIO";
    case VEL_LENTO:  return "LENTO";
    default:         return "MEDIO";
  }
}

const char* estToStr(Estado e) {
  switch (e) {
    case EST_START: return "INICIAR";
    case EST_PARAR: return "PARAR";
    default:        return "PARAR";
  }
}

// ==========================================
// ISR de multiplexado: muestreo de segmentos para reconstruir D1/D2/D3
// ==========================================
char decodeBinaryToChar(uint8_t raw) {
  uint8_t cleanByte = raw & 0x7F;
  switch (cleanByte) {
    case 0x40: return '0'; case 0x79: return '1'; case 0x24: return '2';
    case 0x30: return '3'; case 0x19: return '4'; case 0x12: return '5';
    case 0x02: return '6'; case 0x58: return '7'; case 0x00: return '8';
    case 0x10: return '9'; case 0x7F: return ' '; default: return '?';
  }
}

// ISR: se dispara por el flanco de los selectores del multiplexado.
// Se muestrean los segmentos para reconstruir el dígito activo (mantener la rutina lo más corta posible).
void IRAM_ATTR onSelectorChange() {
  // D1: detectar qué selector está activo y muestrear segmentos
  for (int i=0; i<3; i++) {
    if (digitalRead(SEL_D1[i]) == LOW) {
      // Espera breve para estabilizar señales tras el flanco del selector antes de leer los segmentos
      delayMicroseconds(50); uint8_t raw=0;
      for(int s=0;s<8;s++) if(digitalRead(PIN_SEG[s])==HIGH) raw|=(1<<s);
      textD1[i]=decodeBinaryToChar(raw); return;
    }
  }
  // D2: detectar qué selector está activo y muestrear segmentos
  for (int i=0; i<3; i++) {
    if (digitalRead(SEL_D2[i]) == LOW) {
      delayMicroseconds(50); uint8_t raw=0;
      for(int s=0;s<8;s++) if(digitalRead(PIN_SEG[s])==HIGH) raw|=(1<<s);
      textD2[i]=decodeBinaryToChar(raw); return;
    }
  }
  // D3: detectar qué selector está activo y muestrear segmentos
  for (int i=0; i<3; i++) {
    if (digitalRead(SEL_D3[i]) == LOW) {
      delayMicroseconds(50); uint8_t raw=0;
      for(int s=0;s<8;s++) if(digitalRead(PIN_SEG[s])==HIGH) raw|=(1<<s);
      textD3[i]=decodeBinaryToChar(raw); return;
    }
  }
}

// ==========================================
// TELEMETRÍA
// ==========================================
void sendTelemetry() {
  StaticJsonDocument<420> doc;

  int d1 = bufferToInt(textD1);
  int d2 = bufferToInt(textD2);
  int d3 = bufferToInt(textD3);

  if (d1 != -1) doc["peso"] = d1;
  if (d2 != -1) doc["target"] = d2;
  if (d3 != -1) doc["contador"] = d3;

  doc["paso"] = paso;
  doc["valor"] = valor_preview;
  doc["valor_cargado"] = valor_objetivo;

  doc["velocidad"] = velToStr(velocidad);
  doc["estado"]    = estToStr(estado);

  char payload[420];
  serializeJson(doc, payload);
  client.publish("v1/devices/me/telemetry", payload);
}

void sendEventKV(const char* k, const char* v) {
  StaticJsonDocument<96> doc;
  doc[k] = v;
  char payload[96];
  serializeJson(doc, payload);
  client.publish("v1/devices/me/telemetry", payload);
}

// ==========================================
// PULSOS NO BLOQUEANTES POR PCF 
// ==========================================
struct PulseJob {
  Adafruit_PCF8574* dev = nullptr;
  uint8_t pin = 255;
  bool active = false;
  uint32_t offAt = 0;
};

PulseJob jobs[10];

void cancelPulse(Adafruit_PCF8574* dev, uint8_t pin) {
  for (auto &j : jobs) {
    if (j.active && j.dev == dev && j.pin == pin) {
      dev->digitalWrite(pin, LOW);
      j.active = false;
    }
  }
}

bool schedulePulse(Adafruit_PCF8574* dev, uint8_t pin, uint32_t ms) {
  if (!dev) return false;

  // Si ya hay un pulso activo en ese pin, se extiende el tiempo de apagado
  for (auto &j : jobs) {
    if (j.active && j.dev == dev && j.pin == pin) {
      j.offAt = millis() + ms;
      dev->digitalWrite(pin, HIGH);
      return true;
    }
  }

  for (auto &j : jobs) {
    if (!j.active) {
      j.dev = dev;
      j.pin = pin;
      j.active = true;
      j.offAt = millis() + ms;
      dev->digitalWrite(pin, HIGH);
      return true;
    }
  }
  return false;
}

void updatePulses() {
  uint32_t now = millis();
  for (auto &j : jobs) {
    if (j.active && (int32_t)(now - j.offAt) >= 0) {
      j.dev->digitalWrite(j.pin, LOW);
      j.active = false;
    }
  }
}

static const uint32_t PULSE_MS = 140;
static const uint32_t HOLD_VACIAR_MS_MIN = 1000;

// ==========================================
// SETTERS BIDIRECCIONALES 
// ==========================================
Velocidad strToVel(String s) {
  s.toUpperCase(); s.replace("\"", "");
  if (s == "RAPIDO" || s == "RÁPIDO") return VEL_RAPIDO;
  if (s == "MEDIO") return VEL_MEDIO;
  if (s == "LENTO") return VEL_LENTO;
  return VEL_MEDIO;
}

Estado strToEst(String s) {
  s.toUpperCase(); s.replace("\"", "");
  if (s == "INICIAR" || s == "START") return EST_START;
  if (s == "PARAR"   || s == "STOP")  return EST_PARAR;
  return EST_PARAR;
}

void setVelocidadLocal(Velocidad v, bool actuatePCF) {
  velocidad = v;

  if (actuatePCF) {
    // Emula pulsación en el canal del PCF asociado a la velocidad seleccionada
    if (v == VEL_RAPIDO) schedulePulse(&pcf, PIN_PCF_SW4_RAPIDO, PULSE_MS);
    if (v == VEL_MEDIO)  schedulePulse(&pcf, PIN_PCF_SW5_MEDIO,  PULSE_MS);
    if (v == VEL_LENTO)  schedulePulse(&pcf, PIN_PCF_SW6_LENTO,  PULSE_MS);
  }

  sendTelemetry();
}

void setEstadoLocal(Estado e, bool actuatePCF) {
  estado = e;

  if (actuatePCF) {
    if (e == EST_START) {
      schedulePulse(&pcf, PIN_PCF_SW7_START, PULSE_MS);
    } else {
      if (hasPcf2) schedulePulse(&pcf2, PIN_PCF2_SW9_PARAR, PULSE_MS);
    }
  }

  sendTelemetry();
}

void doVaciar(uint32_t holdMs, bool actuatePCF) {
  if (holdMs < HOLD_VACIAR_MS_MIN) holdMs = HOLD_VACIAR_MS_MIN;
  if (actuatePCF) schedulePulse(&pcf, PIN_PCF_SW8_VACIAR, holdMs);
  sendEventKV("accion", "VACIAR");
}

void doClear(uint32_t ms) {
  if (ms < 40) ms = 40;
  schedulePulse(&pcf, PIN_PCF_SW2_CLEAR, ms);
  sendEventKV("accion", "CLEAR");
}

// ==========================================
// LECTURA DE SW FÍSICOS
// ==========================================

// Anti-rebote por “igualdad” pin<->común. 25 ms para pulsadores mecánicos típicos.
static const uint32_t DETECT_MS = 25; 

struct LinkDetector {
  uint8_t pin;
  bool latched;
  uint32_t sameSince;
};

LinkDetector detSW4{25,false,0}; // RAPIDO
LinkDetector detSW5{16,false,0}; // MEDIO
LinkDetector detSW6{26,false,0}; // LENTO

LinkDetector detSW7{4,false,0};  // START
LinkDetector detSW8{27,false,0}; // VACIAR
LinkDetector detSW9{15,false,0}; // PARAR

void updateLinkDetector(LinkDetector &d, void (*onPress)()) {
  int a = digitalRead(d.pin);
  int b = digitalRead(PIN_SW_COMUN);

  // Detección por continuidad: cuando el switch cierra, el pin queda eléctricamente unido al común.
  // Si ambos niveles coinciden por DETECT_MS, se considera pulsación (anti-rebote).
  if (a == b) {
    if (d.sameSince == 0) d.sameSince = millis();
    if (!d.latched && (millis() - d.sameSince) >= DETECT_MS) {
      d.latched = true;
      onPress();
    }
  } else {
    d.sameSince = 0;
    d.latched = false;
  }
}

// Acciones al presionar en la máquina (solo cambia variables + telemetría)
void onPressSW4() { setVelocidadLocal(VEL_RAPIDO, false); }
void onPressSW5() { setVelocidadLocal(VEL_MEDIO,  false); }
void onPressSW6() { setVelocidadLocal(VEL_LENTO,  false); }
void onPressSW7() { setEstadoLocal(EST_START, false); }
void onPressSW9() { setEstadoLocal(EST_PARAR, false); }
void onPressSW8() { sendEventKV("accion", "VACIAR_FISICO"); }


// ==========================================
// RPC ThingsBoard: control remoto de paso/valor/velocidad/estado y acciones (vaciar/clear)
// ==========================================
bool parseStringParam(JsonVariant params, String &out) {
  out = "";
  if (params.is<const char*>()) { out = String(params.as<const char*>()); return true; }
  if (params.is<JsonObject>()) {
    JsonObject o = params.as<JsonObject>();
    if (o.containsKey("value")) { out = String((const char*)o["value"]); return true; }
  }
  return false;
}

uint32_t parseUintParam(JsonVariant params, uint32_t defVal) {
  if (params.is<uint32_t>()) return params.as<uint32_t>();
  if (params.is<int>()) return (uint32_t)params.as<int>();
  if (params.is<JsonObject>()) {
    JsonObject o = params.as<JsonObject>();
    if (o.containsKey("ms")) return (uint32_t)o["ms"].as<int>();
  }
  return defVal;
}

void handleRpc(const char* topic, const byte* payload, unsigned int length) {
  String reqId = String(topic).substring(26);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload, length);

  bool ok = true;
  bool update = false;

  if (err) {
    ok = false;
  } else {
    const char* method = doc["method"] | "";
    JsonVariant params = doc["params"];

    if (strcmp(method, "setStep") == 0) {
      int s = params["step"];
      if (s==1||s==10||s==100) paso = s;
      update = true;
    }
    else if (strcmp(method, "inc") == 0) {
      valor_preview = wrapSafe(valor_preview + paso);
      update = true;
    }
    else if (strcmp(method, "dec") == 0) {
      valor_preview = wrapSafe(valor_preview - paso);
      update = true;
    }
    else if (strcmp(method, "load") == 0) {
      valor_objetivo = valor_preview;
      buscando = true;
      update = true;
    }

    // RPC: velocidad (acepta "velocidad" o "setVelocidad")
    else if (strcmp(method, "velocidad") == 0 || strcmp(method, "setVelocidad") == 0) {
      String s;
      if (!parseStringParam(params, s)) ok = false;
      else setVelocidadLocal(strToVel(s), true); 
    }

    // RPC: estado (acepta "estado" o "setEstado")
    else if (strcmp(method, "estado") == 0 || strcmp(method, "setEstado") == 0) {
      String s;
      if (!parseStringParam(params, s)) ok = false;
      else setEstadoLocal(strToEst(s), true); 
    }

    else if (strcmp(method, "vaciar") == 0) {
      if (estado != EST_PARAR) {
        // Seguridad: "vaciar" solo se acepta si el estado actual es PARAR
        ok = false;
      } else {
        uint32_t ms = parseUintParam(params, HOLD_VACIAR_MS_MIN);
        doVaciar(ms, true);
      }
    }


    // RPC: clear (SW2)

    else if (strcmp(method, "clear") == 0 || strcmp(method, "sw2") == 0) {
      uint32_t ms = parseUintParam(params, 180);
      doClear(ms);
    }

    else {
      ok = false;
    }
  }

  // Respuesta al RPC (true/false)
  client.publish(("v1/devices/me/rpc/response/" + reqId).c_str(), ok ? "true" : "false");

  // Publica telemetría cuando solo se actualizan variables locales (paso/inc/dec/load)
  if (update) sendTelemetry();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  handleRpc(topic, payload, length);
}

// Conexión inicial (bloqueante). Usar en setup; en loop se usa maintainConnection().
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}


// Conexión inicial MQTT (bloqueante). En ejecución normal se evita bloquear el loop.
void ensureMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32_Final_Turbo", TB_TOKEN, NULL)) {
      client.subscribe("v1/devices/me/rpc/request/+");
      sendTelemetry();
    } else delay(1000);
  }
}

// Mantiene conectividad WiFi/MQTT con reintentos periódicos (sin bucles bloqueantes)
void maintainConnection() {
  uint32_t now = millis();

  // WiFi: reintentos cada WIFI_RETRY_MS (sin bucles bloqueantes)
  if (WiFi.status() != WL_CONNECTED) {
    if ((int32_t)(now - lastWiFiAttempt) >= (int32_t)WIFI_RETRY_MS) {
      lastWiFiAttempt = now;

      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(true);
      WiFi.setSleep(false);        
      WiFi.disconnect(false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return; 
  }

  // MQTT: reintentos cada MQTT_RETRY_MS cuando WiFi ya está OK
  if (!client.connected()) {
    if ((int32_t)(now - lastMQTTAttempt) >= (int32_t)MQTT_RETRY_MS) {
      lastMQTTAttempt = now;

      if (client.connect("ESP32_Final_Turbo", TB_TOKEN, NULL)) {
        client.subscribe("v1/devices/me/rpc/request/+");
        sendTelemetry(); // sincroniza apenas vuelva
      }
    }
  }
}



// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(13, 14);

  if (!pcf.begin(0x20, &Wire)) pcf.begin(0x27, &Wire);
  hasPcf2 = pcf2.begin(0x21, &Wire);

  for (int i = 0; i < 8; i++) {
  pcf.pinMode(i, OUTPUT);
  pcf.digitalWrite(i, LOW);
  }
  if (hasPcf2) {
    for (int i = 0; i < 8; i++) {
      pcf2.pinMode(i, OUTPUT);
      pcf2.digitalWrite(i, LOW);
    }
  }


  pinMode(PIN_SW_COMUN, INPUT);


  // Estado seguro al arranque: salidas del PCF en LOW para evitar activaciones involuntarias
  detenerMotor();
  pcf.digitalWrite(PIN_PCF_SW2_CLEAR, LOW);
  pcf.digitalWrite(PIN_PCF_SW4_RAPIDO, LOW);
  pcf.digitalWrite(PIN_PCF_SW5_MEDIO, LOW);
  pcf.digitalWrite(PIN_PCF_SW6_LENTO, LOW);
  pcf.digitalWrite(PIN_PCF_SW7_START, LOW);
  pcf.digitalWrite(PIN_PCF_SW8_VACIAR, LOW);
  if (hasPcf2) pcf2.digitalWrite(PIN_PCF2_SW9_PARAR, LOW);

  for (int i=0; i<8; i++) pinMode(PIN_SEG[i], INPUT);

  for (int i=0; i<3; i++) {
    pinMode(SEL_D1[i], INPUT); attachInterrupt(digitalPinToInterrupt(SEL_D1[i]), onSelectorChange, FALLING);
    pinMode(SEL_D2[i], INPUT); attachInterrupt(digitalPinToInterrupt(SEL_D2[i]), onSelectorChange, FALLING);
    pinMode(SEL_D3[i], INPUT); attachInterrupt(digitalPinToInterrupt(SEL_D3[i]), onSelectorChange, FALLING);
  }

  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  
  ensureWiFi();
  client.setServer(TB_HOST, TB_PORT);
  client.setCallback(mqttCallback);
  ensureMQTT();

  // Sincronización inicial
  delay(500);
  int startVal = bufferToInt(textD1);
  if (startVal != -1) {
    valor_preview = startVal;
    valor_objetivo = startVal;
  }

  sendTelemetry();
  Serial.println("=== SISTEMA TURBO + BIDIRECCIONAL ACTIVADO ===");
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  // Red: mantener WiFi/MQTT y procesar callbacks
  maintainConnection();
  if (client.connected()) client.loop();

  // Pulsos temporizados: emulación de pulsaciones sobre el PCF
  updatePulses();

  // SW físicos: detección por continuidad con anti-rebote
  updateLinkDetector(detSW4, onPressSW4);
  updateLinkDetector(detSW5, onPressSW5);
  updateLinkDetector(detSW6, onPressSW6);
  updateLinkDetector(detSW7, onPressSW7);
  updateLinkDetector(detSW8, onPressSW8);
  updateLinkDetector(detSW9, onPressSW9);

  // Telemetría: publica cambios de D1/D2/D3 y sincroniza preview/objetivo (cada ~150 ms)
  if (millis() - ultimaTelemetria > 150) {
    int curD1 = bufferToInt(textD1);
    int curD2 = bufferToInt(textD2);
    int curD3 = bufferToInt(textD3);

    if (curD1 != -1) {
      if (curD1 != lastD1 && !buscando) {
        valor_preview = curD1;
        valor_objetivo = curD1;
      }
      if (curD1 != lastD1 || curD2 != lastD2 || curD3 != lastD3) {
        sendTelemetry();
        lastD1 = curD1; lastD2 = curD2; lastD3 = curD3;
      }
    }
    ultimaTelemetria = millis();
  }

  // Control de motor: búsqueda de valor_objetivo con modo turbo y ajuste fino por pulsos
  if (buscando) {
    int actual = bufferToInt(textD1);
    if (actual == -1) return;

    int distArriba, distAbajo;
    if (valor_objetivo >= actual) {
      distArriba = valor_objetivo - actual;
      distAbajo  = actual + (1000 - valor_objetivo);
    } else {
      distArriba = (1000 - actual) + valor_objetivo;
      distAbajo  = actual - valor_objetivo;
    }

    if (actual == valor_objetivo) {
      buscando = false;
      detenerMotor();
      valor_preview = actual;
      sendTelemetry();
      Serial.println(">> META!");
    } else {
      int pinAccion;
      int distanciaReal;

      if (distArriba <= distAbajo) {
        pinAccion = PIN_PCF_SUBIR;
        distanciaReal = distArriba;
      } else {
        pinAccion = PIN_PCF_BAJAR;
        distanciaReal = distAbajo;
      }

      if (distanciaReal > 15) {
        pcf.digitalWrite((pinAccion == PIN_PCF_SUBIR) ? PIN_PCF_BAJAR : PIN_PCF_SUBIR, LOW);
        pcf.digitalWrite(pinAccion, HIGH);
        enModoTurbo = true;
        delay(10);
      } else {
        if (enModoTurbo) {
          detenerMotor();
          delay(50);
          enModoTurbo = false;
        }

        if (millis() - ultimaAccion > 200) {
          pcf.digitalWrite((pinAccion == PIN_PCF_SUBIR) ? PIN_PCF_BAJAR : PIN_PCF_SUBIR, LOW);
          pcf.digitalWrite(pinAccion, HIGH);
          delay(60);
          pcf.digitalWrite(pinAccion, LOW);
          ultimaAccion = millis();
        }
      }
    }
  }
}