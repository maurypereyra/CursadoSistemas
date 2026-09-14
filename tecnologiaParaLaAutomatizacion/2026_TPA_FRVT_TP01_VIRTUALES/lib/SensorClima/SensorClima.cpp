#include "SensorClima.h"

SensorClima::SensorClima(uint8_t pinDHT, uint8_t pinPotenciometro)
  : _dht(pinDHT, DHT22), _pinPotenciometro(pinPotenciometro) {
}

void SensorClima::begin() {
  _dht.begin();
  pinMode(_pinPotenciometro, INPUT);
}

float SensorClima::leerTemperatura() {
  return _dht.readTemperature();
}

float SensorClima::leerHumedad() {
  return _dht.readHumidity();
}

float SensorClima::leerReferenciaTemperatura() {
  int lectura = analogRead(_pinPotenciometro);
  return (lectura / (float)ADC_MAX) * (REF_TEMP_MAX_C - REF_TEMP_MIN_C) + REF_TEMP_MIN_C;
}
