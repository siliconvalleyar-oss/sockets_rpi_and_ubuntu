# Sockets — Transferencia de archivos TCP (C++17)

Cliente/servidor TCP para enviar archivos entre PC (Ubuntu) y Raspberry Pi,
escrito en C++17 con sockets POSIX.

## Requisitos

| Componente | Versión mínima |
|------------|----------------|
| g++        | 7.0 (C++17)    |
| make       | 4.0            |
| Linux      | kernel 4.x+    |

Compatible con **Ubuntu** (x86_64) y **Raspberry Pi OS** (ARM).
El servidor corre en la Raspberry Pi y el cliente en cualquier máquina.

## Compilación

```bash
make all        # compila master y slave
make master     # solo el cliente
make slave      # solo el servidor
```

### Compilación cruzada para Raspberry Pi

```bash
sudo apt install g++-arm-linux-gnueabihf
make CROSS_COMPILE=arm-linux-gnueabihf- all
```

### Instalación

```bash
make install                 # instala en /usr/local/bin
make install PREFIX=/opt     # instalación personalizada
sudo make install            # instala globalmente
make uninstall               # desinstala
```

## Uso

### Servidor (Raspberry Pi / receptor)

```bash
./bin/slave [opciones] <puerto>

Opciones:
  -o <dir>   directorio de salida (defecto: directorio actual)
  -d         modo daemon (segundo plano)

Ejemplos:
  ./bin/slave 5000
  ./bin/slave -o ~/recibidos 5000
  ./bin/slave -d 5000            # corre en background
```

### Cliente (PC / emisor)

```bash
./bin/master [opciones] <archivo> <IP> <puerto>

Opciones:
  -q         modo silencioso (sin barra de progreso)
  -t <ms>    timeout en milisegundos (defecto: 5000)

Ejemplos:
  ./bin/master documento.pdf 192.168.1.100 5000
  ./bin/master -q foto.jpg 10.0.0.5 5000
  ./bin/master -t 10000 video.mp4 192.168.1.100 5000
```

### Flujo típico

```bash
# 1. En la Raspberry Pi:
./bin/slave -o ~/recibidos 5000

# 2. En la PC:
./bin/master ~/Documentos/informe.pdf 192.168.1.100 5000
```

## Características

- **ACK por paquete**: confirmación de cada bloque enviado
- **Múltiples conexiones**: el servidor atiende clientes concurrentes con fork
- **Timeout configurable**: el cliente no se cuelga si el servidor no responde
- **SO_REUSEADDR**: el servidor se reinicia sin esperar TIME_WAIT
- **Modo daemon**: el servidor puede correr en segundo plano
- **Progreso visible**: barra de porcentaje durante la transferencia
- **Protocolo binario**: metadatos (nombre + tamaño) seguidos de datos crudos

## Estructura del proyecto

```
sockets/
├── bin/              # Binarios compilados (gitignored)
├── include/          # Cabeceras compartidas
│   ├── protocol.h    # Constantes del protocolo
│   └── socket_utils.h# RAII Socket y helpers
├── src/
│   ├── master.cpp    # Cliente (envía archivos)
│   └── slave.cpp     # Servidor (recibe archivos)
├── obj/              # Objetos compilados (gitignored)
├── Makefile          # Build system
├── build_sockets.sh  # Generador de estructura inicial
└── README.md         # Este archivo
```

## Protocolo

```
Cliente                          Servidor
  |                                |
  |--- metadata (nom\nsize\n) --->|
  |--- datos (N bytes) ---------->|
  |<-- ACK ('K') -----------------|
  |--- ... (repite) ------------->|
  |--- END ('E') ----------------->|
  |                                |
```

## Notas para Raspberry Pi

- Verificar que `g++` esté instalado: `sudo apt install g++ make`
- El servidor escucha en `0.0.0.0` (todas las interfaces)
- La IP del cliente debe ser accesible desde la Raspberry (misma red)
- Para probar localmente en una sola máquina usar `127.0.0.1`
