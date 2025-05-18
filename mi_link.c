#include "directorios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Programa mi_link:
 * Crea un enlace a un fichero existente, llamando a la función mi_link()
 * de la capa de directorios.
 * Uso: mi_link <nombre_dispositivo> </ruta_fichero_original> </ruta_enlace>
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> </ruta_fichero_original> </ruta_enlace>\n", argv[0]);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *ruta_fichero_original = argv[2];
    const char *ruta_enlace = argv[3];

    // Comprobar que las rutas no terminan en '/'
    if (ruta_fichero_original[strlen(ruta_fichero_original) - 1] == '/') {
        fprintf(stderr, "Error: la ruta del fichero original no debe terminar en '/': %s\n", ruta_fichero_original);
        return FALLO;
    }
    if (ruta_enlace[strlen(ruta_enlace) - 1] == '/') {
        fprintf(stderr, "Error: la ruta del enlace no debe terminar en '/': %s\n", ruta_enlace);
        return FALLO;
    }

    // Montar el dispositivo
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return FALLO;
    }

    // Crear el enlace
    int res = mi_link(ruta_fichero_original, ruta_enlace);
    if (res < 0) {
        bumount(disco);
        return FALLO;
    }

    // Desmontar el dispositivo
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return FALLO;
    }

    return EXITO;
}
