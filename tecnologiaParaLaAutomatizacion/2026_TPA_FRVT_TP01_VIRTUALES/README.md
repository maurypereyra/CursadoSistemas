# TP01 - Control automatico para un invernadero

UTN - Facultad Regional Venado Tuerto - Tecnologias para la Automatizacion - TP01/26

Integrantes: Christian Schneider, Marcela Gaiga, Mauricio Pereyra

Simulacion en Wokwi (ESP32) desarrollada con PlatformIO.

## 1. Artefactos del circuito

| Artefacto | Tipo de parte Wokwi | Pin(es) en el ESP32 | Que hace en el proyecto |
|---|---|---|---|
| ESP32 DevKit | `board-esp32-devkit-c-v4` | - | Microcontrolador principal, corre todo el sketch. |
| Sensor DHT22 | `wokwi-dht22` | GPIO33 (dato, digital) | Mide temperatura y humedad del invernadero cada 2 segundos. |
| Potenciometro | `wokwi-potentiometer` | GPIO32 (entrada analogica) | Fija la referencia de temperatura para la ventilacion (mapeada de 0 a 50 C). |
| OLED SH1106 128x64 | `board-ssd1306` (0x3C) | GPIO21 (SDA), GPIO22 (SCL) | Muestra la pantalla de inicio, las 2 pantallas obligatorias, el menu y el estado completo. |
| LED de ventilacion (verde) | `wokwi-led` | GPIO23 | Se enciende fijo cuando la temperatura supera la referencia. |
| LED de riego (azul) | `wokwi-led` | GPIO27 | Parpadea (500 ms) cuando la humedad cae por debajo del umbral. |
| Pulsador de pantallas | `wokwi-pushbutton` | GPIO4 (INPUT_PULLUP) | Alterna entre Pantalla 1 y Pantalla 2 del OLED. |
| Encoder KY-040 | `wokwi-ky-040` | GPIO18 (CLK), GPIO5 (DT), GPIO19 (SW) | Gira para entrar y navegar el menu opcional; el pulsador del encoder confirma una opcion. |
| Pin de semilla aleatoria | - | GPIO34 (analogico, sin conectar) | Se lee una sola vez al inicio (`randomSeed`) para que el umbral de humedad varie en cada corrida. |

## 2. Estructura del codigo

### 2.1 src/main.cpp

Orquesta todo el sketch: inicializa los sensores, el pulsador, el encoder y la pantalla
OLED, lee el puerto Serie para procesar comandos, y decide en cada ciclo que pantalla se
dibuja (modo normal o menu), delegando la logica de negocio a las librerias de `lib/`.

### 2.2 lib/SensorClima

Encapsula la lectura del DHT22 (temperatura y humedad) y del potenciometro (referencia de
temperatura), exponiendo los valores ya convertidos y listos para usar en el resto del
sketch.

### 2.3 lib/ControlInvernadero

Contiene la logica de control de ventilacion y riego: compara los valores de SensorClima
contra la referencia y el umbral, maneja el parpadeo del LED de riego, y el modo forzado
manual (AUTOMATICO / FORZADO_ON / FORZADO_OFF) tanto para ventilacion como para riego.

### 2.4 lib/PantallaOLED

Agrupa todas las pantallas que se dibujan en el OLED: la pantalla de inicio, las pantallas
1 y 2 del modo normal, el menu de opciones y la pantalla de estado completo.

## 3. Pantallas y menu

**Modo normal** (el que arranca el sistema):
- Pantalla 1: temperatura actual, referencia (potenciometro) y estado de la ventilacion.
- Pantalla 2: humedad actual, umbral aleatorio y estado del riego.
- El pulsador alterna entre ambas.

**Menu opcional** (se entra girando el encoder desde cualquiera de las 2 pantallas):
1. *Estado completo*: temperatura, referencia, modo de ventilacion, humedad, umbral y modo
   de riego, todo junto.
2. *Forzar Ventilacion*: cada click del encoder alterna AUTOMATICO -> FORZADO_ON ->
   FORZADO_OFF -> AUTOMATICO.
3. *Forzar Riego*: mismo ciclo que el anterior, para el riego.
4. *Volver*: sale del menu y vuelve al modo normal.

Se navega girando el encoder (mueve la seleccion) y se confirma con el pulsador del encoder
(SW). Dentro de "Estado completo", el pulsador del encoder vuelve al menu.

## 4. Comandos por el puerto Serie (115200 baudios)

Se escriben en el Monitor Serie y se confirman con Enter. El sketch hace eco de cada
caracter tipeado.

| Comando | Ejemplo | Efecto |
|---|---|---|
| `AYUDA` o `HELP` | `AYUDA` | Lista todos los comandos disponibles. |
| `STATUS` | `STATUS` | Muestra el estado completo del invernadero. |
| `SET_TEMP <valor\|AUTO>` | `SET_TEMP 28` | Fija manualmente la referencia de temperatura (en C). `SET_TEMP AUTO` vuelve a tomarla del potenciometro. |
| `SET_HUM <valor\|AUTO>` | `SET_HUM 45` | Fija manualmente el umbral de humedad (en %). `SET_HUM AUTO` vuelve al umbral aleatorio generado al inicio. |
| `FORCE_VENT <ON\|OFF\|AUTO>` | `FORCE_VENT ON` | Fuerza el estado de la ventilacion (o la vuelve a automatica). |
| `FORCE_RIEGO <ON\|OFF\|AUTO>` | `FORCE_RIEGO OFF` | Fuerza el estado del riego (o lo vuelve a automatico). |

## 5. Como probar todo el proyecto

### 5.1 Compilar y simular en Wokwi

1. Abrir la carpeta del proyecto en VS Code (con las extensiones de PlatformIO y Wokwi
   instaladas).
2. Ejecutar **PlatformIO: Build** para el entorno `esp32doit-devkit-v1` (la primera vez
   descarga las librerias de `platformio.ini`).
3. Con el build ya generado (`.pio/build/esp32doit-devkit-v1/firmware.bin`), iniciar la
   simulacion con **Wokwi: Start Simulator**.
4. Abrir el Monitor Serie de Wokwi para ver los logs.

### 5.2 Checklist de pruebas

- **Inicio**: al arrancar, el OLED debe mostrar el umbral de humedad generado (entre 40% y
  60%), y el mismo valor debe aparecer en el Monitor Serie.
- **Ventilacion**: en Wokwi, hacer click sobre el DHT22 y subir su "temperature" simulada por
  encima del valor de referencia (o girar el potenciometro para bajar la referencia). El LED
  verde (GPIO23) debe encenderse fijo, la Pantalla 1 debe mostrar "Ventilacion: ON", y debe
  aparecer un log "Ventilacion ACTIVADA" por Serie. Bajar la temperatura (o subir la
  referencia) y verificar que se apaga.
- **Riego**: bajar la "humidity" simulada del DHT22 por debajo del umbral mostrado al inicio.
  El LED azul (GPIO27) debe parpadear cada 500 ms, la Pantalla 2 debe mostrar "Riego: ON", y
  debe aparecer "Riego ACTIVADO" por Serie. Subir la humedad y verificar que se apaga.
- **Pulsador**: hacer click sobre el pulsador y verificar que el OLED alterna entre Pantalla 1
  y Pantalla 2, y que aparece "Cambio de pantalla (boton)" por Serie.
- **Encoder - menu**: girar el encoder desde cualquier pantalla normal y verificar que entra
  al menu ("Entrando al menu (encoder)" por Serie). Seguir girando para recorrer los 4 items.
  Hacer click en el encoder (SW) sobre "Estado completo" y verificar que muestra todos los
  datos juntos; click de nuevo para volver al menu. Hacer click sobre "Forzar Ventilacion" y
  "Forzar Riego" varias veces seguidas y verificar que el LED correspondiente responde
  inmediatamente al modo forzado (ON fijo / OFF apagado), ignorando el sensor mientras dure.
  Seleccionar "Volver" para salir del menu.
- **Comandos por Serie**: probar cada comando de la tabla del punto 4, en particular:
  - `SET_TEMP 15` y `SET_TEMP 45` para forzar que la ventilacion se prenda o apague sin
    tocar el potenciometro, y `SET_TEMP AUTO` para devolver el control al potenciometro.
  - `SET_HUM 90` y `SET_HUM 10` para forzar riego prendido/apagado, y `SET_HUM AUTO` para
    volver al umbral aleatorio original.
  - `FORCE_VENT ON` / `FORCE_VENT OFF` / `FORCE_VENT AUTO` y lo mismo con `FORCE_RIEGO`,
    confirmando que tambien se ve reflejado en el menu del encoder (mismo estado
    compartido).
- **Robustez**: en Wokwi se puede desconectar momentaneamente el DHT22 (o dejarlo fuera de
  rango) para verificar que el sketch imprime "ERROR: no se pudo leer el sensor DHT22" en
  vez de trabarse o mostrar valores invalidos en pantalla.
