#include "ficheros.h"

/**
 * Programa que cambia los permisos de un inodo--> No se usa en la práctica
 * Uso: ./permitir <nombre_dispositivo> <ninodo> <permisos>
 */

int main(int argc, char **argv) {
    // Validación de sintaxis
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <ninodo> <permisos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = (unsigned char) atoi(argv[3]);

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", nombre_dispositivo);
        exit(EXIT_FAILURE);
    }
    
    // Llamar a mi_chmod_f() para cambiar los permisos del inodo
    if (mi_chmod_f(ninodo, permisos) == FALLO) {
        fprintf(stderr, "Error al cambiar permisos del inodo %u\n", ninodo);
        bumount(nombre_dispositivo);
        exit(EXIT_FAILURE);
    }
    
    // Desmontar el dispositivo virtual
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", nombre_dispositivo);
        exit(EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS;
}
