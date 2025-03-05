#include "ficheros.h"

/**
 * Programa que trunca un fichero a los bytes indicados como nbytes
 * Uso: ./truncar <nombre_dispositivo> <ninodo> <nbytes>
 */
int main (int argc, char **argv){

    // Validación de sintaxis
    if (argc != 4){
        fprintf(stderr,RED "Sintaxis: %s <nombre_dispositivo> <ninodo> <nbytes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Definimos las variables necesarias
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);
    int resultado = 0;

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO){
        fprintf(stderr,RED "Error al montar el dispositivo %s\n", nombre_dispositivo);
        exit(EXIT_FAILURE);
    }

    // Si nbytes es 0, se invoca liberar_inodo(), sino se llama a mi_truncar_f()
    if (nbytes == 0) {
        resultado = liberar_inodo(ninodo);
        if (resultado == FALLO) {
            fprintf(stderr,RED "Error al liberar el inodo.\n");
            bumount();
            exit(EXIT_FAILURE);
        }
    } else {
        // Se valida que se deje al menos 1 byte escrito en el inodo
        if (nbytes < 1) {
            fprintf(stderr,RED "Error: nbytes debe ser 0 o al menos 1.\n");
            bumount();
            exit(EXIT_FAILURE);
        }
        resultado = mi_truncar_f(ninodo, nbytes);
        if (resultado == FALLO) {
            fprintf(stderr,RED "Error en mi_truncar_f().\n");
            bumount();
            exit(EXIT_FAILURE);
        }
    }

    // Desmontar dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr,RED "Error al desmontar el dispositivo virtual.\n");
        exit(EXIT_FAILURE);
    }

    // Mostrar estadísticas del inodo para verificar tamEnBytesLog y numBloquesOcupados
    struct STAT inodo_stat;
    if (mi_stat_f(ninodo, &inodo_stat) == FALLO) {
        fprintf(stderr,RED "Error en mi_stat_f().\n");
        return EXIT_FAILURE;
    }
    printf("Tamaño en bytes lógicos: %u\n", inodo_stat.tamEnBytesLog);
    printf("Número de bloques ocupados: %d\n", inodo_stat.numBloquesOcupados);

    return EXIT_SUCCESS;
}
