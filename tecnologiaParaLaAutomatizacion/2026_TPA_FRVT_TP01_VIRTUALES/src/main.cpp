/*
 * ==========================================================================
 * UTN - Facultad Regional Venado Tuerto
 * Tecnologias para la Automatizacion - Trabajo Practico 01 (TP01/26)
 *
 * Control automatico para un invernadero, simulado con ESP32 en Wokwi.
 *
 * Integrantes: Christian Schneider, Marcela Gaiga, Mauricio Pereyra
 *
 * Requerimientos obligatorios:
 * - Monitoreo de temperatura y humedad con DHT22 (pin 33), visualizado en
 *   el OLED.
 * - Control de ventilacion: el potenciometro (pin 32) fija la referencia
 *   de temperatura; si la temperatura medida la supera, se enciende el
 *   LED de ventilacion (LED verde, pin 23).
 * - Control de riego: al inicio se genera un umbral aleatorio de humedad
 *   en [40%-60%]; si la humedad medida cae por debajo, se enciende de
 *   forma intermitente el LED de riego (pin 27).
 * - Un pulsador (pin 4) alterna entre 2 pantallas del OLED.
 * - Se registran por puerto Serie el inicio, y los cambios de estado de
 *   ventilacion y riego.
 *
 * Requerimientos opcionales (implementados):
 * - Menu navegable con un encoder KY-040 (pines 18/5/19):
 *   * Estado completo del invernadero.
 *   * Forzar manualmente ventilacion ON/OFF/AUTOMATICO.
 *   * Forzar manualmente riego ON/OFF/AUTOMATICO.
 * - Modificacion manual de las referencias (temperatura y humedad) desde
 *   el puerto Serie (comandos SET_TEMP / SET_HUM). Escribir AYUDA por
 *   Serie para ver todos los comandos disponibles.
 * ==========================================================================
 */

#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>
#include "SensorClima.h"
#include "ControlInvernadero.h"
#include "PantallaOLED.h"

// ---------------------------------------------------------------------------
// Pines
// ---------------------------------------------------------------------------
const uint8_t PIN_DHT22 = 33;            // sensor de temperatura y humedad
const uint8_t PIN_POTENCIOMETRO = 32;    // referencia de temperatura (ventilacion)
const uint8_t PIN_LED_VENTILACION = 23;  // LED verde de ventilacion
const uint8_t PIN_LED_RIEGO = 27;        // LED azul de riego
const uint8_t PIN_BOTON_PANTALLAS = 4;   // pulsador (cambia de pantalla)
const uint8_t PIN_ENCODER_CLK = 18;      // encoder (menu opcional)
const uint8_t PIN_ENCODER_DT = 5;
const uint8_t PIN_ENCODER_SW = 19;
const uint8_t PIN_SEMILLA_RANDOM = 34;   // pin analogico sin conectar, solo para semilla

// ---------------------------------------------------------------------------
// Configuracion
// ---------------------------------------------------------------------------
const unsigned long INTERVALO_LECTURA_MS = 2000; // DHT22 requiere ~2s entre lecturas
const unsigned long ANTIRREBOTE_MS = 50;
const uint8_t HUMEDAD_MINIMA_PORCENTAJE = 40;
const uint8_t HUMEDAD_MAXIMA_PORCENTAJE = 60;

// ---------------------------------------------------------------------------
// Objetos principales
// ---------------------------------------------------------------------------
SensorClima sensores(PIN_DHT22, PIN_POTENCIOMETRO);
ControlInvernadero control(PIN_LED_VENTILACION, PIN_LED_RIEGO);
PantallaOLED pantalla;
ESP32Encoder encoder;

// ---------------------------------------------------------------------------
// Estado de la interfaz (pantallas / menu opcional)
// ---------------------------------------------------------------------------
enum class Pantalla : uint8_t { TEMPERATURA, HUMEDAD };
enum class EstadoUI : uint8_t { NORMAL, MENU, ESTADO_COMPLETO };

Pantalla pantallaActual = Pantalla::TEMPERATURA;
EstadoUI estadoUI = EstadoUI::NORMAL;

const char* OPCIONES_MENU[] = { "Estado completo", "Forzar Ventilacion", "Forzar Riego", "Volver" };
const uint8_t CANTIDAD_OPCIONES_MENU = 4;
uint8_t indiceMenu = 0;

// ---------------------------------------------------------------------------
// Variables de sensado y umbrales
// ---------------------------------------------------------------------------
float temperaturaActual = NAN;
float humedadActual = NAN;
float referenciaTemperatura = 0;
float umbralHumedadGenerado = 0; // el aleatorio original, generado una sola vez

bool referenciaTemperaturaManual = false;
float referenciaTemperaturaManualValor = 0;

bool umbralHumedadManual = false;
float umbralHumedadManualValor = 0;

unsigned long ultimaLectura = 0;

// ---------------------------------------------------------------------------
// Antirrebote del pulsador de pantallas
// ---------------------------------------------------------------------------
int botonEstadoRaw = HIGH;
int botonEstadoEstable = HIGH;
unsigned long botonUltimoCambio = 0;

// ---------------------------------------------------------------------------
// Encoder: posicion y antirrebote de su pulsador (SW)
// ---------------------------------------------------------------------------
long conteoEncoderAnterior = 0;
int encoderSwRaw = HIGH;
int encoderSwEstable = HIGH;
unsigned long encoderSwUltimoCambio = 0;

// ---------------------------------------------------------------------------
// Declaraciones
// ---------------------------------------------------------------------------
void leerBoton();
void leerEncoder();
void actualizarPantalla();
void procesarComandoSerie();
void ejecutarComando(String linea);
void imprimirEstadoSerie();
void imprimirAyuda();
float referenciaTemperaturaEfectiva();
float umbralHumedadEfectivo();

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BOTON_PANTALLAS, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_SEMILLA_RANDOM, INPUT);

  sensores.begin();
  control.begin();
  pantalla.begin();

  encoder.attachSingleEdge(PIN_ENCODER_CLK, PIN_ENCODER_DT);
  encoder.setCount(0);

  randomSeed(analogRead(PIN_SEMILLA_RANDOM));
  umbralHumedadGenerado = random(HUMEDAD_MINIMA_PORCENTAJE, HUMEDAD_MAXIMA_PORCENTAJE + 1);

  Serial.println("=== Sistema de control de invernadero iniciado ===");
  Serial.print("Umbral de humedad para riego generado: ");
  Serial.print(umbralHumedadGenerado);
  Serial.println(" %");
  imprimirAyuda();

  pantalla.mostrarInicio(umbralHumedadGenerado);
  delay(2000); // tiempo de estabilizacion recomendado para el DHT22

  // Primera lectura, para no esperar otro ciclo completo antes de mostrar datos validos
  temperaturaActual = sensores.leerTemperatura();
  humedadActual = sensores.leerHumedad();
  referenciaTemperatura = referenciaTemperaturaEfectiva();
  if (!isnan(temperaturaActual) && !isnan(humedadActual)) {
    control.actualizar(temperaturaActual, referenciaTemperatura, humedadActual, umbralHumedadEfectivo());
  }
  ultimaLectura = millis();
}

void loop() {
  unsigned long ahora = millis();

  // El potenciometro (o la referencia manual) se puede actualizar en todo momento
  referenciaTemperatura = referenciaTemperaturaEfectiva();

  // El DHT22 solo se relee cada INTERVALO_LECTURA_MS
  if (ahora - ultimaLectura >= INTERVALO_LECTURA_MS) {
    ultimaLectura = ahora;

    float nuevaTemperatura = sensores.leerTemperatura();
    float nuevaHumedad = sensores.leerHumedad();

    if (isnan(nuevaTemperatura) || isnan(nuevaHumedad)) {
      Serial.println("ERROR: no se pudo leer el sensor DHT22");
    } else {
      temperaturaActual = nuevaTemperatura;
      humedadActual = nuevaHumedad;
    }
  }

  // El control (y el parpadeo del LED de riego) se actualiza en cada ciclo
  if (!isnan(temperaturaActual) && !isnan(humedadActual)) {
    bool ventilacionAnterior = control.ventilacionActiva();
    bool riegoAnterior = control.riegoActivo();

    control.actualizar(temperaturaActual, referenciaTemperatura, humedadActual, umbralHumedadEfectivo());

    if (control.ventilacionActiva() != ventilacionAnterior) {
      Serial.print("Ventilacion ");
      Serial.println(control.ventilacionActiva() ? "ACTIVADA (LED encendido)" : "DESACTIVADA (LED apagado)");
    }
    if (control.riegoActivo() != riegoAnterior) {
      Serial.print("Riego ");
      Serial.println(control.riegoActivo() ? "ACTIVADO (LED intermitente)" : "DESACTIVADO (LED apagado)");
    }
  }

  leerBoton();
  leerEncoder();
  procesarComandoSerie();
  actualizarPantalla();
}

// ---------------------------------------------------------------------------
// Referencias efectivas (automaticas o forzadas por Serie)
// ---------------------------------------------------------------------------
float referenciaTemperaturaEfectiva() {
  return referenciaTemperaturaManual ? referenciaTemperaturaManualValor : sensores.leerReferenciaTemperatura();
}

float umbralHumedadEfectivo() {
  return umbralHumedadManual ? umbralHumedadManualValor : umbralHumedadGenerado;
}

// ---------------------------------------------------------------------------
// Pulsador: alterna entre Pantalla1 y Pantalla2 (solo fuera del menu)
// ---------------------------------------------------------------------------
void leerBoton() {
  int lecturaRaw = digitalRead(PIN_BOTON_PANTALLAS);
  unsigned long ahora = millis();

  if (lecturaRaw != botonEstadoRaw) {
    botonUltimoCambio = ahora;
    botonEstadoRaw = lecturaRaw;
  }

  if ((ahora - botonUltimoCambio) > ANTIRREBOTE_MS && botonEstadoRaw != botonEstadoEstable) {
    botonEstadoEstable = botonEstadoRaw;
    if (botonEstadoEstable == LOW && estadoUI == EstadoUI::NORMAL) {
      pantallaActual = (pantallaActual == Pantalla::TEMPERATURA) ? Pantalla::HUMEDAD : Pantalla::TEMPERATURA;
      Serial.println("Cambio de pantalla (boton)");
    }
  }
}

// ---------------------------------------------------------------------------
// Encoder: gira para entrar/navegar el menu opcional, y su pulsador confirma
// ---------------------------------------------------------------------------
void leerEncoder() {
  long conteoActual = encoder.getCount();

  if (conteoActual != conteoEncoderAnterior) {
    if (estadoUI == EstadoUI::NORMAL) {
      estadoUI = EstadoUI::MENU;
      indiceMenu = 0;
      Serial.println("Entrando al menu (encoder)");
    } else if (estadoUI == EstadoUI::MENU) {
      if (conteoActual > conteoEncoderAnterior) {
        indiceMenu = (indiceMenu + 1) % CANTIDAD_OPCIONES_MENU;
      } else {
        indiceMenu = (indiceMenu == 0) ? (CANTIDAD_OPCIONES_MENU - 1) : (indiceMenu - 1);
      }
    }
  }
  conteoEncoderAnterior = conteoActual;

  int lecturaSw = digitalRead(PIN_ENCODER_SW);
  unsigned long ahora = millis();

  if (lecturaSw != encoderSwRaw) {
    encoderSwUltimoCambio = ahora;
    encoderSwRaw = lecturaSw;
  }

  if ((ahora - encoderSwUltimoCambio) > ANTIRREBOTE_MS && encoderSwRaw != encoderSwEstable) {
    encoderSwEstable = encoderSwRaw;
    if (encoderSwEstable == LOW) {
      if (estadoUI == EstadoUI::MENU) {
        switch (indiceMenu) {
          case 0:
            estadoUI = EstadoUI::ESTADO_COMPLETO;
            break;
          case 1:
            control.alternarForzadoVentilacion();
            Serial.print("Ventilacion forzada: ");
            Serial.println(control.textoModo(control.modoVentilacion()));
            break;
          case 2:
            control.alternarForzadoRiego();
            Serial.print("Riego forzado: ");
            Serial.println(control.textoModo(control.modoRiego()));
            break;
          case 3:
            estadoUI = EstadoUI::NORMAL;
            Serial.println("Saliendo del menu");
            break;
        }
      } else if (estadoUI == EstadoUI::ESTADO_COMPLETO) {
        estadoUI = EstadoUI::MENU;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// OLED: dibuja la pantalla correspondiente al estado actual de la interfaz
// ---------------------------------------------------------------------------
void actualizarPantalla() {
  switch (estadoUI) {
    case EstadoUI::NORMAL:
      if (pantallaActual == Pantalla::TEMPERATURA) {
        pantalla.mostrarTemperatura(temperaturaActual, referenciaTemperatura, control.ventilacionActiva());
      } else {
        pantalla.mostrarHumedad(humedadActual, umbralHumedadEfectivo(), control.riegoActivo());
      }
      break;

    case EstadoUI::MENU:
      pantalla.mostrarMenu(OPCIONES_MENU, CANTIDAD_OPCIONES_MENU, indiceMenu);
      break;

    case EstadoUI::ESTADO_COMPLETO:
      pantalla.mostrarEstadoCompleto(temperaturaActual, referenciaTemperatura, control.textoModo(control.modoVentilacion()),
                                      humedadActual, umbralHumedadEfectivo(), control.textoModo(control.modoRiego()));
      break;
  }
}

// ---------------------------------------------------------------------------
// Puerto Serie: se lee caracter a caracter y se hace eco de cada uno (el
// monitor serie de Wokwi, entre otros, no hace eco local de lo tipeado).
// Al llegar '\r' o '\n' se ejecuta el comando acumulado en bufferComandoSerie.
// ---------------------------------------------------------------------------
String bufferComandoSerie = "";

void procesarComandoSerie() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r' || c == '\n') {
      // Se acepta cualquiera de los dos como fin de linea, porque segun el
      // monitor serie que se use (Wokwi, PlatformIO, Arduino IDE) el Enter
      // puede mandar '\r', '\n' o ambos. Si llegan los dos juntos, el
      // segundo no hace nada porque el buffer ya quedo vacio.
      if (bufferComandoSerie.length() > 0) {
        Serial.println(); // eco del Enter
        ejecutarComando(bufferComandoSerie);
        bufferComandoSerie = "";
      }
    } else if (c == 8 || c == 127) { // backspace o delete
      if (bufferComandoSerie.length() > 0) {
        bufferComandoSerie.remove(bufferComandoSerie.length() - 1);
        Serial.print("\b \b"); // borra el ultimo caracter en pantalla
      }
    } else {
      bufferComandoSerie += c;
      Serial.print(c); // eco del caracter tipeado
    }
  }
}

void ejecutarComando(String linea) {
  linea.trim();
  if (linea.length() == 0) return;
  linea.toUpperCase();

  int espacio = linea.indexOf(' ');
  String comando = (espacio == -1) ? linea : linea.substring(0, espacio);
  String argumento = (espacio == -1) ? "" : linea.substring(espacio + 1);
  argumento.trim();

  if (comando == "SET_TEMP") {
    if (argumento == "AUTO") {
      referenciaTemperaturaManual = false;
      Serial.println("Referencia de temperatura: vuelve a tomarse del potenciometro");
    } else {
      referenciaTemperaturaManual = true;
      referenciaTemperaturaManualValor = argumento.toFloat();
      Serial.print("Referencia de temperatura fijada manualmente en ");
      Serial.print(referenciaTemperaturaManualValor);
      Serial.println(" C");
    }
  } else if (comando == "SET_HUM") {
    if (argumento == "AUTO") {
      umbralHumedadManual = false;
      Serial.println("Umbral de humedad: vuelve a ser el generado aleatoriamente al inicio");
    } else {
      umbralHumedadManual = true;
      umbralHumedadManualValor = argumento.toFloat();
      Serial.print("Umbral de humedad fijado manualmente en ");
      Serial.print(umbralHumedadManualValor);
      Serial.println(" %");
    }
  } else if (comando == "FORCE_VENT") {
    if (argumento == "ON") control.forzarVentilacion(ModoActuador::FORZADO_ON);
    else if (argumento == "OFF") control.forzarVentilacion(ModoActuador::FORZADO_OFF);
    else control.forzarVentilacion(ModoActuador::AUTOMATICO);
    Serial.print("Ventilacion: ");
    Serial.println(control.textoModo(control.modoVentilacion()));
  } else if (comando == "FORCE_RIEGO") {
    if (argumento == "ON") control.forzarRiego(ModoActuador::FORZADO_ON);
    else if (argumento == "OFF") control.forzarRiego(ModoActuador::FORZADO_OFF);
    else control.forzarRiego(ModoActuador::AUTOMATICO);
    Serial.print("Riego: ");
    Serial.println(control.textoModo(control.modoRiego()));
  } else if (comando == "STATUS") {
    imprimirEstadoSerie();
  } else if (comando == "AYUDA" || comando == "HELP") {
    imprimirAyuda();
  } else {
    Serial.println("Comando no reconocido. Escribi AYUDA para ver los comandos disponibles.");
  }
}

void imprimirEstadoSerie() {
  Serial.println("---- Estado completo del invernadero ----");
  Serial.print("Temperatura: "); Serial.print(temperaturaActual);
  Serial.print(" C | Referencia: "); Serial.print(referenciaTemperatura);
  Serial.print(" C | Ventilacion: "); Serial.println(control.textoModo(control.modoVentilacion()));
  Serial.print("Humedad: "); Serial.print(humedadActual);
  Serial.print(" % | Umbral: "); Serial.print(umbralHumedadEfectivo());
  Serial.print(" % | Riego: "); Serial.println(control.textoModo(control.modoRiego()));
  Serial.println("------------------------------------------");
}

void imprimirAyuda() {
  Serial.println("Comandos disponibles por puerto Serie:");
  Serial.println("  SET_TEMP <valor|AUTO>    - fija/libera la referencia de temperatura");
  Serial.println("  SET_HUM <valor|AUTO>     - fija/libera el umbral de humedad");
  Serial.println("  FORCE_VENT <ON|OFF|AUTO> - fuerza/libera la ventilacion");
  Serial.println("  FORCE_RIEGO <ON|OFF|AUTO>- fuerza/libera el riego");
  Serial.println("  STATUS                   - muestra el estado completo");
  Serial.println("  AYUDA                    - muestra esta ayuda");
}
