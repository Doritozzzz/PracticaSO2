#include <stdio.h>
#include <stdlib.h>

#include "directorios.h"

int main(int argc, char **argv) {
    // Validar sintaxis
    if (argc != 4) {
        fprintf(stderr, RED"Sintaxis: ./mi_chmod <nombre_dispositivo> <permisos> </ruta>\n"RESET);
        return FALLO;
    }

    // Validar permisos
    int permisos = atoi(argv[2]);
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED"Error: Permisos deben estar entre 0 y 7\n"RESET);
        return FALLO;
    }

    // Montar dispositivo
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED"Error al montar dispositivo\n"RESET);
        return FALLO;
    }

    // Cambiar permisos
    int resultado = mi_chmod(argv[3], permisos);

    // Desmontar y retornar
    bumount();
    return resultado;
}