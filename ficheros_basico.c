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
 * @param nbloques: Número de bloques del dispositivo
 * @param ninodos: Número de inodos del dispositivo (ninodos=nbloques/4)
 * @return EXITO si se ha inicializado correctamente, FALLO en caso contrario
 */
int initMB(unsigned int nbloques, unsigned int ninodos) {
    // Calculamos el tamaño del mapa de bits en bloques
    int numBloquesMB = tamMB(nbloques);
    int totalBytesMB = numBloquesMB * BLOCKSIZE;

    // Reservamos un buffer para todo el mapa de bits
    unsigned char *MBtotal = malloc(totalBytesMB);
    if (MBtotal == NULL) {
        fprintf(stderr, "Error al reservar memoria para el MB: %s\n", strerror(errno));
        return FALLO;
    }

    memset(MBtotal, 0, totalBytesMB);

    int bloquesMetadatos = tamSB + numBloquesMB + tamAI(ninodos);

    // Marcamos los bits de los bloques ocupados (0 a bloquesMetadatos-1)
    for (int i = 0; i < bloquesMetadatos; i++) {
        // Usando la convención: bit 0 = (128 >> 0), bit 1 = (128 >> 1), etc.
        MBtotal[i / 8] |= (128 >> (i % 8));
    }

    // Escribimos cada bloque del MB en el dispositivo virtual
    for (int i = 0; i < numBloquesMB; i++) {
        if (bwrite(posSB + tamSB + i, MBtotal + i * BLOCKSIZE) == FALLO) {
            fprintf(stderr, "Error al escribir el MB en el bloque %d\n", posSB + tamSB + i);
            free(MBtotal);
            return FALLO;
        }
    }

    // Liberamos la memoria del buffer del MB
    free(MBtotal);
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

/**
 * escribir_bit --> Función para escribir un bit en el mapa de bits
 * @param nbloque: Número de bloque a escribir
 * @param bit: Valor del bit a escribir (0 o 1)
 * @return EXITO si se ha escrito correctamente, FALLO en caso contrario
 */
int escribir_bit(unsigned int nbloque, unsigned int bit){
    // Definimos las variables necesarias
    unsigned int posbyte,posbit,nbloqueMB,nbloqueabs;
    unsigned char bufferMB[BLOCKSIZE];

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    //Calculamos la posicion del byte del mapa de bits a cambiar
    posbyte = nbloque / 8;
    posbit = nbloque % 8;
    nbloqueMB = posbyte / BLOCKSIZE;
    nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el bloque del mapa de bits
    if(bread(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }

    // Calculamos la máscara para modificar el bit
    posbyte = posbyte % BLOCKSIZE;
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

    // Escribimos el bloque del mapa de bits modificado
    if(bwrite(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }
    return EXITO;
}

/**
 * leer_bit --> Función para leer un bit del mapa de bits
 * @param nbloque: Número de bloque a leer
 * @return Valor del bit leído (0 o 1)
 */
char leer_bit(unsigned int nbloque){
    // Definimos las variables necesarias
    unsigned int posbyte,posbit,nbloqueMB,nbloqueabs;
    struct superbloque SB;
    unsigned char bufferMB[BLOCKSIZE];

    // Leemos el superbloque
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Calculamos la posición del byte del mapa de bits a leer
    posbyte = nbloque / 8;
    posbit = nbloque % 8;
    nbloqueMB = posbyte / BLOCKSIZE;
    nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el bloque del mapa de bits
    if(bread(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,"Error en la escritura del Mapa de bits\n");
        bumount();
        return FALLO;
    }

    // Ajustamos posbyte para que sea relativo al bloque leído
    posbyte = posbyte % BLOCKSIZE;
    
    // Calculamos la máscara para leer el bit
    unsigned char mascara = 128; 
    mascara >>= posbit;
    mascara &= bufferMB[posbyte];
    mascara >>= (7 - posbit);

    // Devolvemos el valor del bit leído
    return mascara;
}

/**
 * reservar_bloque --> Función para reservar un bloque en el mapa de bits
 * @return Número de bloque reservado
 */
int reservar_bloque(){
    // Definimos las variables necesarias
    unsigned int nbloqueMB,posbyte,posbit,nbloque;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    
    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Comprobamos si quedan bloques libres
    if(SB.cantBloquesLibres<=0){
        fprintf(stderr,"No quedan bloques libres\n");
        
        return FALLO;
    }

    // Buscamos un bloque libre en el mapa de bits
    nbloqueMB=0;
    for(;nbloqueMB<SB.posUltimoBloqueMB;nbloqueMB++){
        memset(bufferAux, 255, BLOCKSIZE);
        if(bread(nbloqueMB + SB.posPrimerBloqueMB , bufferMB)==FALLO){
            fprintf(stderr,"Error al leer el mapa de bits.\n");
            return FALLO;
        }

        // Comparamos el bufferMB con el bufferAux para encontrar un bloque libre
        if(memcmp(bufferMB,bufferAux,BLOCKSIZE)!=0){
            // Bloque encontrado, sale del for
            break;
        }

    }

    // Calculamos la posición del byte y bit del bloque libre
    posbyte=0;
    for(; posbyte < BLOCKSIZE; posbyte++){
        if(bufferMB[posbyte]!=255){
            unsigned char mascara = 128;
            posbit = 0;
            // Buscamos el bit libre
            while (bufferMB[posbyte] & mascara) {
                bufferMB[posbyte] <<= 1;
                posbit++;
            }
            break;
        }
    }

    // Calculamos el número de bloque reservado
    nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // Escribimos el bit en el mapa de bits
    if(escribir_bit(nbloque,1)==FALLO){
        fprintf(stderr,"Error al reservar el bit.\n");
        return FALLO;
    }

    // Borramos por si habia basura en el bloque de datos reservado
    unsigned char borrar[BLOCKSIZE];
    memset(borrar,0,BLOCKSIZE);
    if(bwrite(nbloque,borrar)==FALLO){
        fprintf(stderr,"Error en el borrado del bloque basura.\n");
        return FALLO;

    }

    // Actualizamos el superbloque
    SB.cantBloquesLibres--;

    // Retornamos el número de bloque reservado
    return nbloque;
}

/**
 * liberar_bloque --> Función para liberar un bloque en el mapa de bits
 * @param nbloque: Número de bloque a liberar
 * @return EXITO si se ha liberado correctamente, FALLO en caso contrario
 */
int liberar_bloque(unsigned int nbloque){
    // Escribimos el bit en el mapa de bits
    if(escribir_bit(nbloque,0)==FALLO){
        fprintf(stderr,"Error en la liberacion de bit.\n");
        return FALLO;
    }

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Sumamos un bloque libre al superbloque
    SB.cantBloquesLibres++;

    // Retornamos EXITO si se ha liberado correctamente
    return EXITO;
}

/**
 * escribir_inodo --> Función para escribir un inodo en el array de inodos
 * @param ninodo: Número de inodo a escribir
 * @param inodo: Puntero al inodo a escribir
 * @return EXITO si se ha escrito correctamente, FALLO en caso contrario
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo){
    // Definimos las variables necesarias
    unsigned int nbloqueAI,nbloqueabs,posinodo;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Calculamos la posición del inodo en el array de inodos
    nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;

    // Leemos el bloque del array de inodos
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la lectura del inodo.\n");
        return FALLO;
    }

    // Calculamos la posición del inodo en el bloque de inodos
    posinodo = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos [posinodo] = *inodo;

    // Escribimos el inodo en el array de inodos
    if(bwrite(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la escritura del inodo.\n");
        return FALLO;
    }

    // Retornamos EXITO si se ha escrito correctamente
    return EXITO;
}

/**
 * leer_inodo --> Función para leer un inodo del array de inodos
 * @param ninodo: Número de inodo a leer
 * @param inodo: Puntero al inodo leído
 * @return EXITO si se ha leído correctamente, FALLO en caso contrario
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo){
    // Definimos las variables necesarias
    unsigned int nbloqueAI,nbloqueabs,posinodo;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Calculamos la posición del inodo en el array de inodos
    nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;

    // Leemos el bloque del array de inodos
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,"Error en la lectura del inodo.\n");
        return FALLO;
    }

    // Calculamos la posición del inodo en el bloque de inodos
    posinodo = ninodo % (BLOCKSIZE / INODOSIZE);

    // Copiamos el inodo leído en el inodo pasado por parámetro
    *inodo = inodos [posinodo];
    
    // Retornamos EXITO si se ha leído correctamente
    return EXITO;
}

/**
 * reservar_inodo --> Función para reservar un inodo en el array de inodos
 * @param tipo: Tipo de inodo a reservar ('f': fichero, 'd': directorio)
 * @param permisos: Permisos del inodo a reservar
 * @return Número de inodo reservado
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    // Definimos las variables necesarias
    unsigned int posInodoReservado;

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Verificamos si hay inodos libres
    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, "Error, no hay inodos libres.\n");
        return FALLO;
    }

    // Guardamos el número del primer inodo libre antes de modificarlo
    posInodoReservado = SB.posPrimerInodoLibre;

    // Leemos el inodo que estaba marcado como libre
    struct inodo inodoLibre;
    if (leer_inodo(posInodoReservado, &inodoLibre) == FALLO) {
        return FALLO;
    }

    // Actualizar el superbloque para que apunte al siguiente inodo libre
    SB.posPrimerInodoLibre = inodoLibre.punterosDirectos[0];
    SB.cantInodosLibres--;

    // Inicializamos el inodo
    struct inodo nuevoInodo;
    memset(&nuevoInodo, 0, sizeof(struct inodo));
    nuevoInodo.tipo = tipo;
    nuevoInodo.permisos = permisos;
    nuevoInodo.nlinks = 1;
    nuevoInodo.tamEnBytesLog = 0;
    nuevoInodo.numBloquesOcupados = 0;

    // Asignar los timestamps
    nuevoInodo.atime = nuevoInodo.mtime = nuevoInodo.ctime = nuevoInodo.btime = time(NULL);

    // Escribir el inodo en la posición reservada
    if (escribir_inodo(posInodoReservado, &nuevoInodo) == FALLO) {
        return FALLO;
    }

    if(bwrite(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la escritura del superbloque\n");
        bumount();
        return FALLO;
    }
    
    // Retornar el número de inodo reservado
    return posInodoReservado;
}