#include "directorios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Programa para leer un fichero desde el sistema de archivos.
 * Uso: mi_cat <nombre_dispositivo> </ruta_fichero>
 *
 * @param argc Número de argumentos
 * @param argv Array de argumentos
 * @return EXIT_SUCCESS si todo va bien, EXIT_FAILURE en caso contrario
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> </ruta_fichero>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *nombre_dispositivo = argv[1];
    const char *camino = argv[2];
    unsigned int offset = 0;
    int leidos = 0;
    unsigned int total_leidos = 0;
    unsigned char buffer[TAMBUFFER];
    struct STAT stat;

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", nombre_dispositivo);
        return EXIT_FAILURE;
    }

    // Obtener metadatos del fichero
    if (mi_stat(camino, &stat) == FALLO) {
        fprintf(stderr, "Error: no se pudo obtener stat de %s\n", camino);
        bumount(nombre_dispositivo);
        return EXIT_FAILURE;
    }

    // Leer en bloques hasta agotar tamEnBytesLog
    do {
        leidos = mi_read(camino, buffer, offset, TAMBUFFER);
        if (leidos < 0) {
            fprintf(stderr, "Error al leer %s\n", camino);
            bumount(nombre_dispositivo);
            return EXIT_FAILURE;
        }
        if (leidos > 0) {
            write(1, buffer, leidos);
            total_leidos += leidos;
            offset += leidos;
        }
    } while (leidos > 0);

    // Mostrar estadísticas (stderr)
    dprintf(2, "\ntotal_leidos: %u\ntamEnBytesLog: %u\n", total_leidos, stat.tamEnBytesLog);

    // Desmontar el dispositivo virtual
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", nombre_dispositivo);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
