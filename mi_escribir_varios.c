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
        return FALLO;
    }

    const char *disco   = argv[1];
    const char *camino  = argv[2];
    const char *texto   = argv[3];
    unsigned int offset = atoi(argv[4]);
    unsigned int nbytes = strlen(texto);
    int bytes=0;
    int varios=10;

    // Montar el dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return FALLO;
    }

    // Mostrar longitud del texto
    printf("longitud texto: %u\n", nbytes);

    for (int i = 0; i < varios; i++) {
        bytes += mi_write(camino, texto, offset + BLOCKSIZE * i, nbytes);
        if (bytes < 0) {
            fprintf(stderr, "Error al escribir en %s (bloque %d)\n", camino, i);
            bumount();
            return FALLO;
        }
        
    }

    printf("Bytes escritos: %d\n", bytes);

    // Desmontar el dispositivo virtual
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return FALLO;
    }

    return FALLO;
}
