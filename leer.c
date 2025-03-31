#include "ficheros.h"

/**
 * Programa para leer un fichero--> Es para pruebas no se usa en la práctica
 * Uso: leer <nombre_dispositivo> <ninodo>
 */

#define TAMBUFFER 1500  // Puedes ajustar este valor según lo necesites

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <ninodo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo virtual.\n");
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[TAMBUFFER];
    unsigned int offset = 0;
    int leidos = 0, total_leidos = 0;

    // Volver a leer el inodo para obtener tamEnBytesLog
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, "Error al leer el inodo %u.\n", ninodo);
        bumount(nombre_dispositivo);
        exit(EXIT_FAILURE);
    }

    // Leer bloque a bloque hasta que mi_read_f() devuelva 0
    do {
        memset(buffer, 0, TAMBUFFER);
        leidos = mi_read_f(ninodo, buffer, offset, TAMBUFFER);
        if (leidos < 0) {
            printf("\nTotal bytes leidos:%u\n", 0); //Si leidos < 0 contamos con que no ha leído nada
            printf("TamEnBytesLog:%u\n", inodo.tamEnBytesLog);
            bumount(nombre_dispositivo);
            exit(EXIT_FAILURE);
        }
        if (leidos > 0) {
            write(1, buffer, leidos);
            total_leidos += leidos;
            offset += leidos;  // Avanzamos el offset según los bytes leídos
        }
    } while (leidos > 0);

    

    // Mostrar resultados en stderr para no interferir con la salida estándar
    char info[128];
    sprintf(info, "\ntotal_leidos %d\ntamEnBytesLog %d\n", total_leidos, inodo.tamEnBytesLog);
    write(2, info, strlen(info));

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo virtual.\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
