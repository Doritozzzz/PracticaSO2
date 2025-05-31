#ifndef VERIFICACION_H
#define VERIFICACION_H

#include "simulacion.h"

struct INFORMACION {
    int pid;
    unsigned int nEscrituras;
    struct REGISTRO primera_escritura;
    struct REGISTRO ultima_escritura;
    struct REGISTRO menor_posicion;
    struct REGISTRO mayor_posicion;
};

#endif