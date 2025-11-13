#Markdown File

# GSTracker V1.0.0

## Comandos Útiles
- `idf.py build` → Compilar el proyecto
- `idf.py flash` → Subir firmware al ESP32
- `idf.py monitor` → Ver la salida en serie

## Nomenclaturas usadas en comandos
- `KLRP` → Keep a live timer report
- `RTMC` → Reset microcontroler
- `RTMS` → Reset modulo SIM
- `SVPT` → servidor y puerto TCP
- `TMTR` → Timer tracking report
- `DITR` → Distance tracking report
- `DRNV` → Delete read NVS
- `CLOP` → operador celular
- `DLBF` → Borrar block spiffs por indice
- `RABF` → Leer block spiffs por indice (ECEPTO SMS)
- `WTBF` → Leer y borrar todos los blocks spiffs por indice
- `SAIO` → Estado Y Activación de inpus/outputs
- `DBMD` → Modo debug
- `RTCT` → Numero de reinicios del dispositivo.
- `CLDT` → Cellular data (CPSI -> Cellular Protocol System Information)
- `CLLR` → Cellular report 
- `GNSR` → GNSS report
- `OPCT` → Output control
- `OPST` → Output state
- `MRST` → Reinicio del dispositivo completo (MASTER RESET)
- `LOCA` → última posición valida (LOCATION)

# Datos guardados en NVS memoria no volatil
- `dev_imei` → imei del modulo SIM → AT command
- `dev_id` → id del dispositivo → AT command
- `sim_id` → operador celular (validar al reiniciar) → AT command
- `Keep_a_live` → tiempo de reporte de latido → UART/http/TCP/SMS
- `time_report_tkg` → tiempo de reporte de trackeo → timer
- `dev_password` → contraseña para ingresar a modificar parametros
- `life_time` → tiempo de vida encendido el dispositivo
- `dev_reboots` → Reinicios del dispositivo
- `last_evt_gen` → ultimo evento generado
- `trackings_sent` → numero de mensajes enviados desde encendido
- `debug_mode` → estado del modo debug
- `pass_reset` → contraseña para reiniciar el dispositivo
- `auth_phone` → telefono autorizado para mandar SMS (MAX 2)
- `last_valid_lat` → ultima latitude valida
- `last_valid_lon` → ultima longitude valida
- `gnss_report` → tiempo de reporte en segundos (maximo 5 seg. normal 3 seg.)
- `cpsi_report` → tiempo reporte informacion de la red celular (minimo 10 seg, normal 32 seg.)

# Errores GSTracker generados
- `0` → error sending data to the server

# Estructura de cadenas guardadas en spiffs

- `block_n.txt` →  RABF=n (n = numero de block)
- `block_1.txt` →  RABF=1

| CMD          | CODE  | DESCRIPTION                 | <DEVID>,<CMD><SYMB><VALUE><ENDSYM>     |
|--------------|-------|-----------------------------|----------------------------------------|
| KLRP         | 11    | Keep a live report          | {DEVID},11#{min}$ (minino cada 10 min) |
| RTMS         | 12    | Reset SIM module            | {DEVID},12#1$                          |
| RTMC         | 13    | Reset microcontroller       | {DEVID},13#1$                          |
| SVPT         | 14    | Server and port             | {DEVID},14#{ip:port}$                  |
| TMTR         | 15    | Timer report tracking       | {DEVID},15#{seg}$ (minimo cada 10 seg) |
| DITR         | 16    | Distance tracking report    | {DEVID},16#{mts}$ (minimo cada 100 mts)|