#include "directorios.h"


/**
 * Programa para leer un fichero en un dispositivo virtual.
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

    const char *disco = argv[1];
    const char *camino = argv[2];
    unsigned int offset = 0;
    int leidos;
    unsigned int total_leidos = 0;
    unsigned char buffer[TAMBUFFER];
    struct STAT stat;

    // Montar dispositivo
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    // Obtener información del fichero (tamaño lógico)
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    unsigned int res_busq = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4); // permisos lectura
    if (res_busq < 0) {
        fprintf(stderr, "Error: no existe %s\n", camino);
        bumount(disco);
        return EXIT_FAILURE;
    }
    if (mi_stat_f(p_inodo, &stat) == FALLO) {
        fprintf(stderr, "Error al obtener stat de %s\n", camino);
        bumount(disco);
        return EXIT_FAILURE;
    }

    // Leer iterativamente hasta agotar el fichero
    do {
        leidos = mi_read(camino, buffer, offset, TAMBUFFER);
        if (leidos < 0) {
            fprintf(stderr, "Error al leer %s\n", camino);
            bumount(disco);
            return EXIT_FAILURE;
        }
        if (leidos > 0) {
            write(1, buffer, leidos);
            total_leidos += leidos;
            offset += leidos;
        }
    } while (leidos > 0);

    // Mostrar estadísticas por stderr
    dprintf(2, "\ntotal_leidos %u\ntamEnBytesLog %u\n", total_leidos, stat.tamEnBytesLog);

    // Desmontar dispositivo
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar dispositivo %s\n", disco);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
