#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED"Uso: %s <disco> <ruta_fichero_original> </ruta_enlace>\n"RESET, argv[0]);
        return FALLO;
    }

    const char *disco = argv[1];
    unsigned char ruta_original = argv[2];
    const char *ruta_enlace = argv[3];
    size_t len = strlen(ruta_original);

    // Comprobar que la ruta no termina en '/'
    if (len == 0 || ruta_original[len - 1] == '/') {
        fprintf(stderr, "Error sintaxis: la ruta no debe terminar en '/': %s\n", ruta_original);
        return FALLO;
    }

    // Montar dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return FALLO;
    }

    if()

    

    // Desmontar dispositivo
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return FALLO;
    }

    return EXITO;
}