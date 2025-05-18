#include "directorios.h"


/**
 * Programa para escribir texto en un fichero en un offset dado
 * Uso: mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>
 */
int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "\033[31mUso: %s <nombre_dispositivo> </ruta_fichero> <texto> <offset>\033[0m\n", argv[0]);
        return FALLO;
    }

    const char *disco = argv[1];
    const char *camino = argv[2];
    const char *texto = argv[3];
    unsigned int offset = atoi(argv[4]);
    unsigned int nbytes = strlen(texto);

    // Montar el dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo %s\n", disco);
        return FALLO;
    }

    // Mostrar longitud del texto
    printf("Longitud de texto: %u\n", nbytes);

    // Escribir con la función de la capa de directorios
    int bytes_escritos = mi_write(camino, texto, offset, nbytes);
    if (bytes_escritos < 0) {
        bumount();
        printf("Bytes escritos: %d\n", 0);
        return FALLO;
    }

    printf("Bytes escritos: %d\n", bytes_escritos);

    // Desmontar el dispositivo virtual
    if (bumount(disco) == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo %s\n", disco);
        return FALLO;
    }

    return FALLO;
}
