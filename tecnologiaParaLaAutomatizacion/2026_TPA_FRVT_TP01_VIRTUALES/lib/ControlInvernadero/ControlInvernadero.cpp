#include "ControlInvernadero.h"

ControlInvernadero::ControlInvernadero(uint8_t pinLedVentilacion, uint8_t pinLedRiego, unsigned long periodoParpadeoMs)
  : _pinLedVentilacion(pinLedVentilacion), _pinLedRiego(pinLedRiego),
    _periodoParpadeoMs(periodoParpadeoMs), _ultimoCambioParpadeo(0),
    _ledRiegoEncendido(false), _ventilacionActiva(false), _riegoActivo(false),
    _modoVentilacion(ModoActuador::AUTOMATICO), _modoRiego(ModoActuador::AUTOMATICO) {
}

void ControlInvernadero::begin() {
  pinMode(_pinLedVentilacion, OUTPUT);
  pinMode(_pinLedRiego, OUTPUT);
  digitalWrite(_pinLedVentilacion, LOW);
  digitalWrite(_pinLedRiego, LOW);
}

void ControlInvernadero::actualizar(float temperatura, float referenciaTemperatura, float humedad, float umbralHumedad) {
  // -- Ventilacion --
  if (_modoVentilacion == ModoActuador::FORZADO_ON) _ventilacionActiva = true;
  else if (_modoVentilacion == ModoActuador::FORZADO_OFF) _ventilacionActiva = false;
  else _ventilacionActiva = (temperatura > referenciaTemperatura);

  digitalWrite(_pinLedVentilacion, _ventilacionActiva ? HIGH : LOW);

  // -- Riego --
  if (_modoRiego == ModoActuador::FORZADO_ON) _riegoActivo = true;
  else if (_modoRiego == ModoActuador::FORZADO_OFF) _riegoActivo = false;
  else _riegoActivo = (humedad < umbralHumedad);

  if (_riegoActivo) {
    unsigned long ahora = millis();
    if (ahora - _ultimoCambioParpadeo >= _periodoParpadeoMs) {
      _ultimoCambioParpadeo = ahora;
      _ledRiegoEncendido = !_ledRiegoEncendido;
      digitalWrite(_pinLedRiego, _ledRiegoEncendido ? HIGH : LOW);
    }
  } else {
    _ledRiegoEncendido = false;
    digitalWrite(_pinLedRiego, LOW);
  }
}

bool ControlInvernadero::ventilacionActiva() const { return _ventilacionActiva; }
bool ControlInvernadero::riegoActivo() const { return _riegoActivo; }

void ControlInvernadero::forzarVentilacion(ModoActuador modo) { _modoVentilacion = modo; }
void ControlInvernadero::forzarRiego(ModoActuador modo) { _modoRiego = modo; }

ModoActuador ControlInvernadero::siguienteModo(ModoActuador modo) {
  switch (modo) {
    case ModoActuador::AUTOMATICO: return ModoActuador::FORZADO_ON;
    case ModoActuador::FORZADO_ON: return ModoActuador::FORZADO_OFF;
    default: return ModoActuador::AUTOMATICO;
  }
}

void ControlInvernadero::alternarForzadoVentilacion() { _modoVentilacion = siguienteModo(_modoVentilacion); }
void ControlInvernadero::alternarForzadoRiego() { _modoRiego = siguienteModo(_modoRiego); }

ModoActuador ControlInvernadero::modoVentilacion() const { return _modoVentilacion; }
ModoActuador ControlInvernadero::modoRiego() const { return _modoRiego; }

const char* ControlInvernadero::textoModo(ModoActuador modo) const {
  switch (modo) {
    case ModoActuador::FORZADO_ON: return "FORZADO-ON";
    case ModoActuador::FORZADO_OFF: return "FORZADO-OFF";
    default: return "AUTOMATICO";
  }
}
