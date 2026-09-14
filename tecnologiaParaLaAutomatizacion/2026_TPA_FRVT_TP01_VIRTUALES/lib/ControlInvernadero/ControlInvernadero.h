#ifndef ControlInvernadero_h
#define ControlInvernadero_h

#include <Arduino.h>

// Modo de un actuador: automatico segun el umbral, o forzado manualmente
// (desde el menu del encoder o desde el puerto Serie).
enum class ModoActuador : uint8_t { AUTOMATICO, FORZADO_ON, FORZADO_OFF };

// Logica de control de ventilacion (por temperatura) y riego (por humedad),
// incluyendo el parpadeo del LED de riego y el modo forzado manual.
class ControlInvernadero {
  public:
    ControlInvernadero(uint8_t pinLedVentilacion, uint8_t pinLedRiego, unsigned long periodoParpadeoMs = 500);
    void begin();

    // Recalcula el estado de ambos actuadores y actualiza los LEDs fisicos.
    void actualizar(float temperatura, float referenciaTemperatura, float humedad, float umbralHumedad);

    bool ventilacionActiva() const;
    bool riegoActivo() const;

    void forzarVentilacion(ModoActuador modo);
    void forzarRiego(ModoActuador modo);
    void alternarForzadoVentilacion(); // AUTOMATICO -> FORZADO_ON -> FORZADO_OFF -> AUTOMATICO
    void alternarForzadoRiego();

    ModoActuador modoVentilacion() const;
    ModoActuador modoRiego() const;
    const char* textoModo(ModoActuador modo) const;

  private:
    uint8_t _pinLedVentilacion;
    uint8_t _pinLedRiego;
    unsigned long _periodoParpadeoMs;
    unsigned long _ultimoCambioParpadeo;
    bool _ledRiegoEncendido;

    bool _ventilacionActiva;
    bool _riegoActivo;

    ModoActuador _modoVentilacion;
    ModoActuador _modoRiego;

    static ModoActuador siguienteModo(ModoActuador modo);
};

#endif
