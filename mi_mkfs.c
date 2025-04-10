// mi_mkfs.c
#include "directorios.h"
#include <string.h>

/**
 * Programa para formatear el dispositivo virtual, inicializando todos los bloques a 0s
 * Para usarlo, ejecutar: ./mi_mkfs <nombre_dispositivo> <nbloques>
 * 
 * Usamos argc y argv para recibir los argumentos del usuario
 * argv[0]: "mi_mkfs" --> nombre del programa
 * argv[1]: nombre_dispositivo --> nombre del fichero que representa el dispositivo virtual
 * argv[2}: nbloques --> número de bloques del dispositivo virtual
 * 
 */

 int main(int argc, char **argv) {
    // Verificación de argumentos
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <nbloques>\n", argv[0]);
        return FALLO;
    }

    // Obtener parámetros del usuario
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]); // Convertir string a entero usando atoi

    // Verificar que el número de bloques es válido
    if (nbloques <= 0) {
        fprintf(stderr, "Error: El número de bloques debe ser mayor que 0.\n");
        return FALLO;
    }

    // el numero de inodos es algo inexacto,
    // dividimos entre 4 la cantidad de bloques y dan un número suficientemente alto.
    int ninodos = nbloques / 4; 

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error en el montaje del dispositivo virtual.\n");
        return FALLO;
    }

    // Crear un buffer de 1024 bytes (BLOCKSIZE) e inicializarlo a 0s 
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    // Escribir 0s en todos los bloques del dispositivo virtual
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, "Error al escribir en el bloque %d.\n", i);
            bumount();
            return FALLO;
        }
    }

    // Inicializar el superbloque
    if (initSB(nbloques, ninodos) == FALLO) {
        fprintf(stderr, "Error al inicializar el superbloque.\n");
        bumount();
        return FALLO;
    }

    // Inicializar el mapa de bits
    if (initMB(nbloques, ninodos) == FALLO) {
        fprintf(stderr, "Error al inicializar el mapa de bits.\n");
        bumount();
        return FALLO;
    }

    // Inicializar el array de inodos
    if (initAI() == FALLO) {
        fprintf(stderr, "Error al inicializar el array de inodos.\n");
        bumount();
        return FALLO;
    }

    // Creamos directorio raiz
    if (reservar_inodo('d', 7) == FALLO) {
        fprintf(stderr, "Error al reservar el inodo del directorio raíz.\n");
        bumount();
        return FALLO;
    }

    // Desmontar el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el dispositivo virtual.\n");
        return FALLO;
    }
    return EXITO;
}