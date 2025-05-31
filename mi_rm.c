#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {
    // Verificar sintaxis
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rm <disco> </ruta>\n");
        return -1;
    }

    // Montar dispositivo
    if (bmount(argv[1]) < 0) return -1;

    // Comprobar si es el directorio raíz
    if (strcmp(argv[2], "/") == 0) {
        fprintf(stderr, "Error: No se puede borrar el directorio raíz\n");
        bumount();
        return -1;
    }

    // Llamar a mi_unlink() de la capa de directorios
    int error = mi_unlink(argv[2]);
    
    if (error < 0) {
        bumount();
        return -1;
    }

    // Desmontar dispositivo
    bumount();
    return 0;
}