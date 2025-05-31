#ifndef SIMULACION_H
#define SIMULACION_H

#include <sys/types.h>
#include <time.h>
#include "directorios.h"

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
// Estructura de registro (24 bytes)
struct REGISTRO {
    time_t fecha;       // Timestamp en segundos
    pid_t pid;          // PID del proceso
    int nEscritura;     // Número de escritura (1-50)
    int nRegistro;      // Número de registro (0-499999)
};

#endif