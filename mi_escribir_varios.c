#include "directorios.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Programa para escribir texto repetido en 10 bloques en un fichero,
 * usando la caché de última entrada de escritura.
 * Uso: mi_escribir_varios <nombre_dispositivo> </ruta_fichero> <texto> <offset>
 */
int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "\033[31mUso: %s <nombre_dispositivo> </ruta_fichero> <texto> <offset>\033[0m\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *disco   = argv[1];
    const char *camino  = argv[2];
    const char *texto   = argv[3];
    unsigned int offset = atoi(argv[4]);
    unsigned int nbytes = strlen(texto);

    // Montar el dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    // Mostrar longitud del texto
    printf("longitud texto: %u\n", nbytes);

    int total_bytes = 0;
    for (int i = 0; i < 10; i++) {
        int bytes = mi_write(camino, texto, offset + i * nbytes, nbytes);
        if (bytes < 0) {
            fprintf(stderr, "Error al escribir en %s (bloque %d)\n", camino, i);
            bumount();
            return EXIT_FAILURE;
        }
        total_bytes += bytes;
    }

    printf("Bytes escritos: %d\n", total_bytes);

    // Desmontar el dispositivo virtual
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
