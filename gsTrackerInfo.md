#Markdown File

# GSTracker V1.0.0

## Comandos Útiles
- `idf.py build` → Compilar el proyecto
- `idf.py flash` → Subir firmware al ESP32
- `idf.py monitor` → Ver la salida en serie

## Nomenclaturas usadas en comandos
- `KLRP` → keep a live report
- `PWMC` → * power microcontroler
- `PWMS` → * power modulo SIM
- `RTMC` → reset microcontroler
- `RTMS` → reset modulo SIM
- `DRNV` → delete read NVS
- `TMRP` → time report tracking
- `TKRP` → tracking report
- `SVPT` → servidor y puerto TCP
- `CLOP` → operador celular
- `DLBF` → Borrar block spiffs por indice
- `RABF` → Leer block spiffs por indice (ECEPTO SMS)
- `WTBF` → Leer y borrar todos los blocks spiffs por indice
- `SAIO` → Estado Y Activación de inpus/outputs
- `DBMD` → Modo debug
- `RTCT` → Numero de reinicios del dispositivo.
- `CLDT` → Cellular data
- `CLRP` → Cellular report 
- `OPCT` → Output control
- `OPST` → Output state
- `RTDV` → Reinicio del dispositivo completo

# Datos guardados en NVS memoria no volatil
- `dev_imei` → imei del modulo SIM → AT command
- `dev_id` → id del dispositivo → AT command
- `sim_id` → operador celular (validar al reiniciar) → AT command
- `Keep_a_live` → tiempo de reporte de latido → UART/http/TCP/SMS
- `Time_report_tkg` → tiempo de reporte de trackeo 
- `dev_password` → contraseña para ingresar a modificar parametros
- `life_time` → tiempo de vida encendido el dispositivo
- `dev_reboots` → Reinicios del dispositivo
- `last_evt_gen` → ultimo evento generado
- `trackings_sent` → numero de mensajes enviados desde encendido
- `debug_mode` → estado del modo debug

# Errores GSTracker generados
- `0` → error sending data to the server

# Estructura de cadenas guardadas en spiffs

- `block_n.txt` →  RABF=n (n = numero de block)
- `block_1.txt` →  RABF=1

| CMD          | CODE         | DESCRIPTION                     | RESPONSE            |
|--------------|--------------|---------------------------------|---------------------|
| KLRP         | 11           | Keep a live report              | <HEAD>,<IMEI>       |
| PWCM         | 12           | Power microcontroler            |                     |
| PWMS         | 13           | Power SIM module                |                     |
| RTMS         | 14           | Reset SIM moduleSS              |                     |
| RTMC         | 15           | Reset microcontroller           |                     |
|              |