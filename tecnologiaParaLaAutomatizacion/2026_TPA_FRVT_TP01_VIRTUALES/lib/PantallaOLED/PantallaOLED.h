#ifndef PantallaOLED_h
#define PantallaOLED_h

#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>

// Encapsula todas las pantallas del OLED: inicio, las 2 pantallas
// obligatorias, el menu de opciones y la pantalla de estado completo.
class PantallaOLED {
  public:
    PantallaOLED();
    void begin();

    void mostrarInicio(float umbralHumedad);
    void mostrarTemperatura(float temperatura, float referencia, bool ventilacionActiva);
    void mostrarHumedad(float humedad, float umbralHumedad, bool riegoActivo);
    void mostrarMenu(const char* opciones[], uint8_t cantidad, uint8_t seleccionado);
    void mostrarEstadoCompleto(float temperatura, float referencia, const char* modoVentilacion,
                                float humedad, float umbralHumedad, const char* modoRiego);

  private:
    Adafruit_SH1106G _display;
    void encabezado(const char* titulo);
};

#endif
