#include "ficheros_basicos.h"

/**
 * tamMB --> Función para calcular el tamaño del mapa de bits en bloques
 * @param nbloques: Número de bloques del dispositivo
 * @return Tamaño del mapa de bits en bloques
 */
int tamMB(unsigned int nbloques)
{
    // Calculamos el tamaño del mapa de bits en bloques
    int tamMB = (nbloques / 8) / BLOCKSIZE;

    // Si el tamaño del mapa de bits en bloques no es exacto, añadimos un bloque más
    if (nbloques % (8 * BLOCKSIZE) != 0)
    {
        tamMB++;
    }

    // Devolvemos el tamaño del mapa de bits en bloques
    return tamMB;
}

/**
 * tamAI --> Función para calcular el tamaño del array de inodos en bloques
 * @param ninodos: Número de inodos del dispositivo (ninodos=nbloques/4)
 * @return Tamaño del array de inodos en bloques
 */
int tamAI(unsigned int ninodos)
{
    // Calculamos el tamaño del array de inodos en bloques
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;

    // Si el tamaño del array de inodos en bloques no es exacto, añadimos un bloque más (como en tamMB)
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0)
    {
        tamAI++;
    }

    // Devolvemos el tamaño del array de inodos en bloques
    return tamAI;
}
/**
 * initSB --> Función para inicializar el superbloque
 * @param nbloques: Número de bloques del dispositivo
 * @param ninodos: Número de inodos del dispositivo (ninodos=nbloques/4)
 * @return EXITO si se ha inicializado correctamente, FALLO en caso contrario
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    // COmprobamos que nbloques y ninodos no sean cero
    if (nbloques == 0 || ninodos == 0) {
        fprintf(stderr, "Error: nbloques o ninodos no pueden ser cero.\n");
        return FALLO;
    }

    // Inicializamos el superbloque
    struct superbloque SB;
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // Escribimos el superbloque en el dispositivo virtual
    int resultado = bwrite(posSB, &SB);
    if (resultado != EXITO) {
        fprintf(stderr, "Error en la escritura del SB en el bloque %d: %d\n", posSB, resultado);
        return FALLO;
    }

    // Devolvemos EXITO si se ha inicializado correctamente
    return EXITO;
}

/**
 * initMB --> Función para inicializar el mapa de bits, pone a 1 los bloques ocupados por el SB, MB y AI
 * @return EXITO si se ha inicializado correctamente, FALLO en caso contrario
 */
int initMB(unsigned int nbloques, unsigned int ninodos) {
    // Calculamos el número de bloques que ocupa el MB
    int numBloquesMB = tamMB(nbloques);
    
    // Reservamos un buffer para un bloque del MB
    unsigned char *bufferMB = malloc(BLOCKSIZE);
    if (bufferMB == NULL) {
        fprintf(stderr, "Error al reservar memoria para el MB: %s\n", strerror(errno));
        return FALLO;
    }

    // Inicializamos el buffer a 0s
    memset(bufferMB, 0, BLOCKSIZE);

    // Calculamos el total de bloques ocupados por los metadatos (SB, MB y AI)
    int bloquesMetadatos = tamSB + numBloquesMB + tamAI(ninodos);

    // Marcamos los bits correspondientes en el bufferMB
    for (int i = 0; i < bloquesMetadatos; i++) {
        /**
         * Explicación de la operación:\
         * 1. Dividimos i entre 8 para obtener el índice del byte en el bufferMB --> si i=0, byte=0; si i=8, byte=1; si i=9, byte=1; ...
         * 2. Calculamos el resto de la división para obtener el bit que queremos marcar en el byte --> si i=0, bit=0; si i=8, bit=0; si i=9, bit=1; ...
         * 3. Marcamos el bit correspondiente en el byte con un OR bit a bit 
         */
        bufferMB[i / 8] |= (1 << (i % 8));
    }

    // Escribimos el MB en el dispositivo virtual
    for (int i = 0; i < numBloquesMB; i++) {
        if (bwrite(posSB + tamSB + i, bufferMB) == FALLO) {
            fprintf(stderr, "Error al escribir el MB en el bloque %d\n", posSB + tamSB + i);
            free(bufferMB);
            return FALLO;
        }
    }

    // Liberamos el bufferMB y retornamos EXITO
    free(bufferMB);
    return EXITO;
}


int initAI();
