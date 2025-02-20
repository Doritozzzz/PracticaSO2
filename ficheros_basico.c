#include "ficheros_basico.h"

/**
 * tamMB --> Función para calcular el tamaño del mapa de bits en bloques
 * @param nbloques: Número de bloques del dispositivo
 * @return Tamaño del mapa de bits en bloques
 */
int tamMB(unsigned int nbloques)
{
    // Calculamos el tamaño del mapa de bits en bloques
    // (bitsNecesarios/8 bits)/BytesPorBloque=Bloques de Mapa de bits
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
int tamAI(unsigned int ninodos){
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
    // Comprobamos que nbloques y ninodos no sean cero
    if (nbloques == 0 || ninodos == 0) {
        fprintf(stderr, "Error: nbloques o ninodos no pueden ser cero.\n");
        return FALLO;
    }

    // Calculamos el total de bloques ocupados por los metadatos (SB, MB y AI)
    int bloquesMetadatos = tamSB + tamMB(nbloques) + tamAI(ninodos);

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
    SB.cantBloquesLibres = nbloques - bloquesMetadatos;
    SB.cantInodosLibres = ninodos; 
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // Escribimos el superbloque en el dispositivo virtual
    int resultado = bwrite(posSB, &SB);
    if (resultado == FALLO) {
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

/**
 * initAI --> Función para inicializar la lista de inodos libres
 * @return EXITO si se ha inicializado correctamente, FALLO en caso contrario
 */
int initAI() {
    struct superbloque SB;

    // Leemos el bloque SB
    if (bread(posSB, &SB) == -1) {
        return FALLO;
    }

    // Inicializamos el buffer de inodos y apuntamos al primer inodo libre, garantizando que cada inodo apunta al siguiente
    unsigned int contInodos = SB.posPrimerInodoLibre + 1;
    char buffer[BLOCKSIZE];
    struct inodo *inodos = (struct inodo *)buffer;

    // Recorremos los bloques de inodos en busca de inodos libres (la primera vez todos los inodos están libres)
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // Leer el bloque de inodos desde el dispositivo virtual
        if (bread(i, buffer) == -1) {
            return FALLO;
        }

        int numInodosPorBloque = BLOCKSIZE / INODOSIZE;

        for (int j = 0; j < numInodosPorBloque; j++) {
            // Marcamos el inodo como libre
            inodos[j].tipo = 'l';

            if (contInodos < SB.totInodos) {
                // Enlazamos al siguiente inodo libre con puntersoDirectos[0]
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                // Último inodo libre tiene que apuntar a UINT_MAX (limite de unsigned int)
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }

        // Escribimos el bloque modificado en el dispositivo virtual
        if (bwrite(i, buffer) == -1) {
            return FALLO;
        }

        // Terminamos si ya hemos inicializado todos los inodos
        if (contInodos >= SB.totInodos) {
            break;
        }
    }
    return EXITO;
}

int escribir_bit(unsigned int nbloque, unsigned int bit){
    unsigned int posbyte,posbit,nbloqueMB,nbloqueabs;
    unsigned char bufferMB[BLOCKSIZE];
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    //Calculamos la posicion del byte del mapa de bits a cambiar
    posbyte=nbloque/8;
    posbit=nbloque%8;
    nbloqueMB=posbyte/BLOCKSIZE;
    nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    if(bread(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }

    posbyte=posbyte%BLOCKSIZE;
    unsigned char mascara = 128;    // 10000000
    mascara >>= posbit;
    if(bit==1){
        bufferMB[posbyte] |= mascara;
    }else if(bit==0){
        bufferMB[posbyte] &= ~mascara;
    }else{
        fprintf(stderr,"Bit leído no valido\n");
        bumount();
        return FALLO;
    }

    if(bwrite(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }
    return EXITO;
}

char leer_bit(unsigned int nbloque){
    unsigned int posbyte,posbit,nbloqueMB,nbloqueabs;
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }


    posbyte=nbloque/8;
    posbit=nbloque%8;
    nbloqueMB=posbyte/BLOCKSIZE;
    nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    if(bread(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }
    
    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;          // desplazamiento de bits a la derecha, los que indique posbit
    mascara &= bufferMB[posbyte]; // operador AND para bits0
    mascara >>= (7 - posbit);     // desplazamiento de bits a la derecha 
                                  // para dejar el 0 o 1 en el extremo derecho y leerlo en decimal
    return mascara;

}

int reservar_bloque(){
    unsigned int nbloqueMB,posbyte,posbit,nbloque;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    

    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    if(SB.cantBloquesLibres<=0){
        fprintf(stderr,"No quedan bloques libres\n");
        
        return FALLO;
    }

    nbloqueMB=0;
    for(;nbloqueMB<SB.posUltimoBloqueMB;nbloqueMB++){
        memset(bufferAux, 255, BLOCKSIZE);
        if(bread(nbloqueMB + SB.posPrimerBloqueMB , bufferMB)==FALLO){
            fprintf(stderr,"Error al leer el mapa de bits.\n");
            return FALLO;
        }
        if(memcmp(bufferMB,bufferAux,BLOCKSIZE)==0){
            //Bloque encontrado,sale del for
            break;
        }

    }
    posbyte=0;
    for(;posbyte!=(BLOCKSIZE/8);posbyte++){
        if(bufferMB[posbyte]!=255){
            unsigned char mascara = 128; // 10000000
            posbit = 0;
            while (bufferMB[posbyte] & mascara) { // operador AND para bits
                bufferMB[posbyte] <<= 1;          // desplazamiento de bits a la izquierda
                posbit++;
            }
            break;
        }
    }
    nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;
    if(escribir_bit(nbloque,1)==FALLO){
        fprintf(stderr,"Error al reservar el bit.\n");
        return FALLO;
    }
    //borrar por si habia basura en el bloque de datos reservado
    unsigned char borrar[BLOCKSIZE];
    memset(borrar,0,BLOCKSIZE);
    if(bwrite(nbloque,borrar)==FALLO){
        fprintf(stderr,"Error en el borrado del bloque basura.\n");
        return FALLO;

    }

    SB.cantBloquesLibres--;
    return nbloque;
}

int liberar_bloque(unsigned int nbloque){
    if(escribir_bit(nbloque,0)==FALLO){
        fprintf(stderr,"Error en la liberacion de bit.\n");
        return FALLO;
    }
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    SB.cantBloquesLibres++;
    return EXITO;
}
int escribir_inodo(unsigned int ninodo, struct inodo *inodo){
    unsigned int nbloqueAI,nbloqueabs,posinodo;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    nbloqueAI=(ninodo*INODOSIZE)/BLOCKSIZE;
    nbloqueabs=nbloqueAI+SB.posPrimerBloqueAI;
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la lectura del inodo.\n");
        return FALLO;
    }
    posinodo=ninodo%(BLOCKSIZE/INODOSIZE);
    inodos[posinodo]=*inodo;
    if(bwrite(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la escritura del inodo.\n");
        return FALLO;
    }
    return EXITO;

}
int leer_inodo(unsigned int ninodo, struct inodo *inodo){
    unsigned int nbloqueAI,nbloqueabs,posinodo;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    nbloqueAI=(ninodo*INODOSIZE)/BLOCKSIZE;
    nbloqueabs=nbloqueAI+SB.posPrimerBloqueAI;
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la lectura del inodo.\n");
        return FALLO;
    }
    posinodo=ninodo%(BLOCKSIZE/INODOSIZE);
    *inodo=inodos[posinodo];
    
    return EXITO;
}
