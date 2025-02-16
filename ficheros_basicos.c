#include "ficheros_basicos.h"\

/**
 * tamMB --> Función para calcular el tamaño del mapa de bits en bloques
 * @param nbloques: Número de bloques del dispositivo
 * @return Tamaño del mapa de bits en bloques
 */
int tamMB(unsigned int nbloques){
    // Calculamos el tamaño del mapa de bits en bloques
    int tamMB = (nbloques / 8) / BLOCKSIZE;

    // Si el tamaño del mapa de bits en bloques no es exacto, añadimos un bloque más
    if (nbloques % (8 * BLOCKSIZE) != 0) {
        tamMB++;
    }

    // Devolvemos el tamaño del mapa de bits en bloques
    return tamMB;
}

int tamAI(unsigned int ninodos);

int initSB(unsigned int nbloques, unsigned int ninodos);

int initMB();

int initAI();
