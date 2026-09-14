#ifndef SensorClima_h
#define SensorClima_h

#include <Arduino.h>
#include <DHT.h>

// Encapsula la lectura del sensor DHT22 (temperatura y humedad) y del
// potenciometro que fija la referencia de temperatura del invernadero.
class SensorClima {
  public:
    SensorClima(uint8_t pinDHT, uint8_t pinPotenciometro);
    void begin();

    float leerTemperatura();           // grados Celsius (NAN si falla la lectura)
    float leerHumedad();               // % de humedad relativa (NAN si falla la lectura)
    float leerReferenciaTemperatura(); // grados Celsius, mapeado desde el potenciometro

  private:
    DHT _dht;
    uint8_t _pinPotenciometro;

    static const int ADC_MAX = 4095;      // resolucion del ADC del ESP32 (12 bits)
    static const int REF_TEMP_MIN_C = 0;
    static const int REF_TEMP_MAX_C = 50;
};

#endif
