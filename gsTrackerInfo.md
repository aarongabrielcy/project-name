#Markdown File

# GSTracker V1.0.0

## Comandos Útiles
- `idf.py build` → Compilar el proyecto
- `idf.py flash` → Subir firmware al ESP32
- `idf.py monitor` → Ver la salida en serie

## Nomenclaturas usadas en comandos
- `KLRP` → keep a live report
- `PWMC` → power microcontroler
- `PWMS` → power modulo SIM
- `RTMC` → reset microcontroler
- `RTMS` → reset modulo SIM
- `DRNV` → delete read NVS
- `TMRP` → time report tracking
- `TKRP` → tracking report
- `SVPT` → servidor y puerto TCP
- `CLOP` → operador celular

# Datos guardados en NVS MENORIA NO VOLATIL
- `dev_imei` → imei del modulo SIM
- `dev_id` → id del dispositivo
- `sim_id` → operador celular (validar al reiniciar)
- `Keep_a_live` → tiempo de reporte de latido
- `Time_report_tkg` → tiempo de reporte de trackeo
- `dev_password` → contraseña para ingresar a modificar parametros
- `life_time` → tiempo de vida encendido el dispositivo
- `trackings_sent` → numero de mensajes enviados desde encendido

