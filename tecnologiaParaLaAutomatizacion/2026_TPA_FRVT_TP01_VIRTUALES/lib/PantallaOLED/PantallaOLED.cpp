#include "PantallaOLED.h"

PantallaOLED::PantallaOLED() : _display(128, 64, &Wire, -1) {
}

void PantallaOLED::begin() {
  _display.begin(0x3C, true);
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
}

void PantallaOLED::encabezado(const char* titulo) {
  _display.clearDisplay();
  _display.setCursor(0, 0);
  _display.println(titulo);
  _display.println("--------------------");
}

void PantallaOLED::mostrarInicio(float umbralHumedad) {
  encabezado("INVERNADERO - TP01");
  _display.printf("Umbral de riego:\n%.1f %%\n\nIniciando...", umbralHumedad);
  _display.display();
}

void PantallaOLED::mostrarTemperatura(float temperatura, float referencia, bool ventilacionActiva) {
  encabezado("PANT.1 TEMPERATURA");
  _display.printf("Actual: %.1f C\nRef: %.1f C\n\nVentilacion: %s",
                   temperatura, referencia, ventilacionActiva ? "ON" : "OFF");
  _display.display();
}

void PantallaOLED::mostrarHumedad(float humedad, float umbralHumedad, bool riegoActivo) {
  encabezado("PANT.2 HUMEDAD");
  _display.printf("Actual: %.1f %%\nUmbral: %.1f %%\n\nRiego: %s",
                   humedad, umbralHumedad, riegoActivo ? "ON" : "OFF");
  _display.display();
}

void PantallaOLED::mostrarMenu(const char* opciones[], uint8_t cantidad, uint8_t seleccionado) {
  encabezado("MENU (giro=mover)");
  for (uint8_t i = 0; i < cantidad; i++) {
    _display.print(i == seleccionado ? "> " : "  ");
    _display.println(opciones[i]);
  }
  _display.display();
}

void PantallaOLED::mostrarEstadoCompleto(float temperatura, float referencia, const char* modoVentilacion,
                                          float humedad, float umbralHumedad, const char* modoRiego) {
  encabezado("ESTADO COMPLETO");
  _display.printf("Temp:%.1fC Ref:%.1fC\nVent: %s\n\nHum:%.1f%% Umb:%.1f%%\nRiego: %s",
                   temperatura, referencia, modoVentilacion, humedad, umbralHumedad, modoRiego);
  _display.display();
}
