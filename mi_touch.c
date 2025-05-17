#include "directorios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Programa mi_touch (apartado opcional):
 * Crea un fichero en la ruta indicada, separando la funcionalidad de mi_mkdir.
 * No permite rutas que terminen en '/'.
 * Uso: mi_touch <nombre_dispositivo> <permisos> </ruta_fichero>
 *   permisos: valor octal de permisos (ej. 6 para lectura+escritura)
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <permisos> </ruta_fichero>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *disco = argv[1];
    unsigned char permisos = (unsigned char)atoi(argv[2]);
    const char *camino = argv[3];
    size_t len = strlen(camino);

    // Comprobar que la ruta no termina en '/'
    if (len == 0 || camino[len - 1] == '/') {
        fprintf(stderr, "Error sintaxis: la ruta no debe terminar en '/': %s\n", camino);
        return EXIT_FAILURE;
    }

    // Montar dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    // Crear el fichero con los permisos indicados
    int res = mi_creat(camino, permisos);
    if (res < 0) {
        fprintf(stderr, "Error al crear el fichero %s (código %d)\n", camino, res);
        bumount(disco);
        return EXIT_FAILURE;
    }
    printf("Fichero creado: %s (inodo %d) con permisos %o\n", camino, res, permisos);

    // Desmontar dispositivo
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}