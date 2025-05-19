#include "ficheros_basico.h"
#include <math.h>

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
        fprintf(stderr,RED "Error: nbloques o ninodos no pueden ser cero.\n");
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
        fprintf(stderr,RED "Error en la escritura del SB en el bloque %d: %d\n", posSB, resultado);
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
        fprintf(stderr,RED "Error al reservar memoria para el MB: %s\n", strerror(errno));
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
            fprintf(stderr,RED "Error al escribir el MB en el bloque %d\n", posSB + tamSB + i);
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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
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
        fprintf(stderr,RED "Error en la escritura del Mapa de bits\n");
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
        fprintf(stderr,RED "Bit leído no valido\n");
        bumount();
        return FALLO;
    }

    // Escribimos el bloque del mapa de bits modificado
    if(bwrite(nbloqueabs,bufferMB)==FALLO){
        fprintf(stderr,RED "Error en la escritura del Mapa de bits\n");
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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
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
        fprintf(stderr,RED "Error en la escritura del Mapa de bits\n");
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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Comprobamos si quedan bloques libres
    if(SB.cantBloquesLibres<=0){
        fprintf(stderr,RED "No quedan bloques libres\n");
        
        return FALLO;
    }

    // Buscamos un bloque libre en el mapa de bits
    nbloqueMB=0;
    for(;nbloqueMB<SB.posUltimoBloqueMB;nbloqueMB++){
        memset(bufferAux, 255, BLOCKSIZE);
        if(bread(nbloqueMB + SB.posPrimerBloqueMB , bufferMB)==FALLO){
            fprintf(stderr,RED "Error al leer el mapa de bits.\n");
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
        fprintf(stderr,RED "Error al reservar el bit.\n");
        return FALLO;
    }

    // Borramos por si habia basura en el bloque de datos reservado
    unsigned char borrar[BLOCKSIZE];
    memset(borrar,0,BLOCKSIZE);
    if(bwrite(nbloque,borrar)==FALLO){
        fprintf(stderr,RED "Error en el borrado del bloque basura.\n");
        return FALLO;

    }

    // Actualizamos el superbloque
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr,RED "Error al escribir el superbloque.\n");
        return FALLO;
    }

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
        fprintf(stderr,RED "Error en la liberacion de bit.\n");
        return FALLO;
    }

    // Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Sumamos un bloque libre al superbloque
    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr,RED "Error al escribir el superbloque\n");
        return FALLO;
    }

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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Calculamos la posición del inodo en el array de inodos
    nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;

    // Leemos el bloque del array de inodos
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,RED "Error en la lectura del inodo.\n");
        return FALLO;
    }

    // Calculamos la posición del inodo en el bloque de inodos
    posinodo = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos [posinodo] = *inodo;

    // Escribimos el inodo en el array de inodos
    if(bwrite(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,RED "Error en la escritura del inodo.\n");
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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Calculamos la posición del inodo en el array de inodos
    nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    nbloqueabs = nbloqueAI + SB.posPrimerBloqueAI;

    // Leemos el bloque del array de inodos
    if(bread(nbloqueabs,inodos)==FALLO){
        fprintf(stderr,RED "Error en la lectura del inodo.\n");
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
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    // Verificamos si hay inodos libres
    if (SB.cantInodosLibres == 0) {
        fprintf(stderr,RED "Error, no hay inodos libres.\n");
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
        fprintf(stderr,RED "Error en la escritura del superbloque\n");
        bumount();
        return FALLO;
    }
    
    // Retornar el número de inodo reservado
    return posInodoReservado;
}

/**
 * obtener_nRangoBL --> Función para obtener el rango de bloques lógicos de un inodo
 * @param inodo: Inodo del que obtener el rango de bloques lógicos
 * @param nblogico: Número de bloque lógico a obtener
 * @param ptr: Puntero al bloque lógico obtenido
 * @return nRangoBL: Número de rango de bloque lógico obtenido
 */
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr){
    // Definimos las variables necesarias
    unsigned int nRangoBL;

    // Comprobamos si el bloque lógico está en un puntero directo
    if (nblogico < DIRECTOS) {
        nRangoBL = 0;
        *ptr = inodo->punterosDirectos[nblogico];
    } else if (nblogico < INDIRECTOS0) {
        nRangoBL = 1;
        *ptr = inodo->punterosIndirectos[0];
    } else if (nblogico < INDIRECTOS1) {
        nRangoBL = 2;
        *ptr = inodo->punterosIndirectos[1];
    } else if (nblogico < INDIRECTOS2) {
        nRangoBL = 3;
        *ptr = inodo->punterosIndirectos[2];
    } else {
        fprintf(stderr,RED "Error: número de bloque lógico fuera de rango\n");
        return FALLO;
    }

    // Retornamos el rango de bloque lógico obtenido
    return nRangoBL;
}

/**
 * obtener_indice --> Función para obtener el índice de un bloque lógico en un bloque físico
 * @param nblogico: Número de bloque lógico a obtener
 * @param nivel_punteros: Nivel de punteros a obtener
 * @return Índice del bloque lógico en el bloque físico
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    // Comprobamos el nivel de punteros
    if (nblogico < DIRECTOS) {
        return nblogico;
    } else if (nblogico < INDIRECTOS0) {
        return nblogico - DIRECTOS;
    } else if (nblogico < INDIRECTOS1) {
        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        } else {
            fprintf(stderr,RED "Error: nivel_punteros incorrecto (%d) para nblogico %u en rango INDIRECTOS0\n", nivel_punteros, nblogico);
            return FALLO;
        }
    } else if (nblogico < INDIRECTOS2) {
        if (nivel_punteros == 3) {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        } else {
            fprintf(stderr,RED "Error: nivel_punteros incorrecto (%d) para nblogico %u en rango INDIRECTOS1\n", nivel_punteros, nblogico);
            return FALLO;
        }
    } else {
        fprintf(stderr,RED "El bloque lógico %u está fuera de rango\n", nblogico);
        return FALLO;
    }
}

/**
 * traducir_bloque_inodo --> Función para traducir un bloque lógico de un inodo a un bloque físico
 * @param inodo: Número de inodo a traducir
 * @param nblogico: Número de bloque lógico a traducir
 * @param reservar: Indica si se debe reservar el bloque físico
 * @return Número de bloque físico traducido
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, char reservar){
    // Definimos las variables necesarias
    struct inodo inodo;
    unsigned int ptr, ptr_ant, salvar_inodo;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS];

    // Inicializamos variables
    ptr = 0;
    ptr_ant = 0;
    salvar_inodo = 0;
    indice = 0;

    // Leemos el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    // Obtenemos el rango de bloque lógico
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);

    // El nivel de punteros depende del rango de bloque lógico
    nivel_punteros = nRangoBL;

    // Iteramos para cada nivel de punteros indirectos
    while (nivel_punteros > 0) {

        // Comprobamos si el puntero es nulo
        if (ptr == 0) {

            // Reservamos un bloque si es necesario
            if (reservar == 0) {
                return FALLO;
            } else {

                // Guardamos el inodo si es necesario
                salvar_inodo = 1;
                ptr = reservar_bloque(); // Reservamos un bloque
                inodo.numBloquesOcupados++; // Incrementamos el número de bloques ocupados
                inodo.ctime = time(NULL); // Actualizamos la fecha de modificación

                // Actualizamos el puntero directo o indirecto
                if (nivel_punteros == nRangoBL) {
                    inodo.punterosIndirectos[nRangoBL - 1] = ptr; // Actualizamos el puntero indirecto
                    #if DEBUGN6
                    printf(GRAY "[traducir_bloque_inodo()→ inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n"RESET, nRangoBL - 1, ptr, ptr, nRangoBL);
                    #endif
                } else {
                    buffer[indice] = ptr; 
                    if (bwrite(ptr_ant, buffer) == FALLO) { // Escribimos el bloque de punteros
                        return FALLO;
                    }
                    #if DEBUGN6
                    printf(GRAY"[traducir_bloque_inodo()→ punteros_nivel%d[%d] = %d (reservado BF %d para punteros_nivel%d)]\n"RESET, nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                    #endif
                }
                memset(buffer, 0, BLOCKSIZE);
            }      
        }
        else {
            // Leemos el bloque de punteros
            if (bread(ptr, buffer) == FALLO) {
                return FALLO;
            }
        }
        // Calculamos el índice del bloque lógico en el bloque de punteros
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr; // Guardamos el puntero anterior
        ptr = buffer[indice]; // Actualizamos el puntero
        nivel_punteros--; // Decrementamos el nivel de punteros
    }

    // No existe el bloque lógico
    if (ptr == 0) {
        if (reservar == 0) {
            return FALLO;
        } else {
            salvar_inodo = 1; // Guardamos el inodo
            ptr = reservar_bloque(); // Reservamos un bloque
            inodo.numBloquesOcupados++; // Incrementamos el número de bloques ocupados
            inodo.ctime = time(NULL); // Actualizamos la fecha de modificación
            if (nRangoBL == 0) { // Actualizamos el puntero directo
                inodo.punterosDirectos[nblogico] = ptr;
                #if DEBUGN6
                printf(GRAY"[traducir_bloque_inodo()→ inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %u)]\n"RESET, nblogico, ptr, ptr, nblogico);
                #endif
            } else {
                buffer[indice] = ptr;
                if (bwrite(ptr_ant, buffer) == FALLO) { // Escribimos el bloque de punteros
                    return FALLO;
                }
                #if DEBUGN6
                printf(GRAY "[traducir_bloque_inodo()→ punteros_nivel1[%d] = %d (reservado BF %d para BL %u)]\n"RESET, indice, ptr, ptr, nblogico);
                #endif
            }
        }
    }

    // Guardamos el inodo si es necesario
    if (salvar_inodo == 1) {
        if (escribir_inodo(ninodo, &inodo) == FALLO) {
            return FALLO;
        }
    }

    // Retornamos el número de bloque físico traducido
    return ptr;
}

/**
 * liberar_inodo  --> Libera un inodo del dispositivo virtual
 * @param ninodo: Número de inodo a liberar
 * @return Número de inodo liberado, FALLO en caso contrario
 */
int liberar_inodo(unsigned int ninodo) {
    // Definimos las variables necesarias
    struct inodo inodo;
    struct superbloque SB;
    int bloquesLiberados;

    // Leemos el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr,RED "Error en la leectura del inodo %u\n", ninodo);
        return FALLO;
    }

    // Liberamos los bloques de datos del inodo
    bloquesLiberados = liberar_bloques_inodo(0, &inodo);
    if (bloquesLiberados == FALLO) {
        fprintf(stderr,RED "Error al liberar bloques del inodo %u\n", ninodo);
        return FALLO;
    }

    // Liberamos el inodo
    inodo.numBloquesOcupados -= bloquesLiberados;

    // Actualizamos el inodo
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;

    // Leemos el superbloque
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr,RED "Error en la lectura del superbloque\n");
        return FALLO;
    }

    // Actualizamos el inodo
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre = ninodo;
    SB.cantInodosLibres++;

    // Escribimos el SB
    if(bwrite(posSB, &SB) == FALLO){
        fprintf(stderr,RED "Error en la escritura del superbloque\n");
        return FALLO;
    }

    // Escribimos el inodo
    inodo.ctime = time(NULL);

    // Escribimos el inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr,RED "Error en la escritura del inodo %u\n", ninodo);
        return FALLO;
    }

    return ninodo;
}

//#ifdef COMPACTACODIGO
/**
 * liberar_bloques_inodo --> Libera los bloques de datos de un inodo
 * @param primerBL: Número de primer bloque lógico a liberar
 * @param inodo: Inodo del que liberar los bloques
 * @return Número de bloques liberados, FALLO en caso contrario
 */
int total_breads=0;
int total_bwrites=0;
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo){
    // Definimos las variables necesarias
    unsigned int ultimoBL; 
    unsigned int nivel_punteros = 0;
    unsigned int nBL = primerBL;
    unsigned int ptr = 0;
    int nRangoBL = 0;
    int liberados = 0;
    int eof = 0;

    // Fichero vacío
    if (inodo->tamEnBytesLog == 0) {
        return liberados;
    }

    // Obtenemos el último bloque lógico
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }
    #if DEBUGN6
    printf(BLUE"[liberar_bloques_inodo()\u2192 primer BL: %u, último BL: %u]\n"RESET, primerBL, ultimoBL);
    #endif
    // Obtenemos el rango de bloque lógico
    nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
    if (nRangoBL == FALLO) {
        fprintf(stderr,RED "Error al obtener el rango de bloque lógico\n");
        return FALLO;
    }
    // Liberamos los punteros directos
    if (nRangoBL == 0){
        liberados += liberar_directos(&nBL,ultimoBL,inodo,&eof);
    }

    
    
    // Liberamos los punteros indirectos
    while (!eof){
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);
        nivel_punteros = nRangoBL;
        liberados+= liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros, &ptr, &eof);
    }
    #if DEBUGN6
    printf(BLUE "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados,total_breads,total_bwrites);
    #endif
    return liberados;
}

/**
 * liberar_directos --> Libera los bloques de datos directos de un inodo
 * @param nBL: Número de bloque lógico a liberar
 * @param ultimoBL: Último bloque lógico a liberar
 * @param inodo: Inodo del que liberar los bloques
 * @param eof: Indica si se ha llegado al final del fichero
 * @return Número de bloques liberados, FALLO en caso contrario
 */
int liberar_directos(unsigned int *nBL, unsigned int ultimoBL, struct inodo *inodo, int *eof){
    // Definimos las variables necesarias
    int liberados = 0;

    // Liberamos los bloques directos
    while ((*nBL) < DIRECTOS && !(*eof)) {

        // Comprobamos si el bloque lógico está ocupado
        if (inodo->punterosDirectos[*nBL] != 0) {
            #if DEBUGN6
            printf(GRAY"[liberar_bloques_inodo()\u2192 liberado BF %u de datos para BL %u]\n"RESET, inodo->punterosDirectos[*nBL], *nBL);
            #endif
            // Liberamos el bloque lógico
            liberar_bloque(inodo->punterosDirectos[*nBL]);
            inodo->punterosDirectos[*nBL] = 0;
            liberados++;
        }
        *nBL = *nBL + 1; // Incrementamos el número de bloque lógico
        // Comprobamos si se ha llegado al final del fichero
        if (*nBL > ultimoBL){
            *eof = 1;
        }
        
    }

    // Devolvemos el número de bloques liberados
    return liberados;
}

/**
 * liberar_indirectos --> Libera los bloques de datos indirectos de un inodo
 * @param nBL: Número de bloque lógico a liberar
 * @param primerBL: Número de primer bloque lógico a liberar
 * @param ultimoBL: Último bloque lógico a liberar
 * @param inodo: Inodo del que liberar los bloques
 * @param nRangoBL: Número de rango de bloque lógico a liberar
 * @param nivel_punteros: Nivel de punteros a liberar
 * @param ptr: Puntero al bloque lógico a liberar
 * @param eof: Indica si se ha llegado al final del fichero
 * @return Número de bloques liberados, FALLO en caso contrario
 */

int liberar_indirectos_recursivo(unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL, struct inodo *inodo, int nRangoBL,unsigned int nivel_punteros, unsigned int *ptr, int *eof){
    // Definimos las variables necesarias
    int liberados = 0;
    int indice_inicial;
    unsigned int bloquePunteros[NPUNTEROS];
    unsigned int bloquePunteros_Aux[NPUNTEROS];
    unsigned int bufferCeros[NPUNTEROS];
    unsigned int ult_cero=*nBL;

    memset(bufferCeros, 0, BLOCKSIZE);

    // Si cuelga de un puntero indirrecto
    if (*ptr != 0){
        // Asignamos el índice inicial
        indice_inicial = obtener_indice(*nBL, nivel_punteros);
        if (indice_inicial == 0 || *nBL == primerBL){ // Si es el primer bloque lógico
            if (bread(*ptr, bloquePunteros) == FALLO){
                fprintf(stderr,RED "Error al leer el bloque de punteros\n");
                return FALLO;
            }
            total_breads++;
            // Copiamos el bloque de punteros
            memcpy(bloquePunteros_Aux, bloquePunteros, BLOCKSIZE);
        }

        // Recorrer los bloques de punteros
        for(int i = indice_inicial; i < NPUNTEROS && !(*eof); i++){
            if (bloquePunteros[i] != 0){ // Si el bloque de punteros no está vacío
                // Imprimir salto si hubo un salto previo
                if (*nBL > ult_cero) {
                    #if DEBUGN6
                    printf(BLUE "[liberar_bloques_inodo()→ Saltamos del BL %u al BL %u]\n" RESET, ult_cero,*nBL - 1);
                    #endif
                }

                if (nivel_punteros == 1){
                    #if DEBUGN6
                    printf(WHITE"[liberar_bloques_inodo()\u2192 liberado BF %u de datos para BL %u]\n"RESET, bloquePunteros[i], *nBL);
                    #endif
                    liberar_bloque(bloquePunteros[i]); // Liberamos el bloque de datos
                    bloquePunteros[i] = 0; // Ponemos el puntero a 0
                    liberados++; 
                    *nBL = *nBL + 1; // Incrementamos el número de bloque lógico
                    ult_cero=*nBL;
                } else {
                    // Liberamos los bloques de datos indirectos, ya ue no es el último nivel
                    
                    liberados+=liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros - 1, &bloquePunteros[i], eof);    
                    ult_cero=*nBL;
                    
                }
                
            } else {
                unsigned int salto = 0;
                // Cuantos bloques/posiciones de punteros hay que avanzar segun el nivel de punteros
                switch (nivel_punteros) {
                    case 1:
                        salto = 1;
                        break;
                    case 2: 
                        salto = NPUNTEROS;
                        break;
                    case 3:
                        salto = NPUNTEROS * NPUNTEROS;
                        break;
                    default:
                        break;
                }

                // Verificamos si estamos realmente saltando bloques
                if (salto > 0) {
                    *nBL += salto;
                }
            }

            // Si se ha llegado al final del fichero
            if (*nBL > ultimoBL){ 
                *eof = 1;
            }
        }
        // Si el bloque de punteros es distinto al original
        if (memcmp(bloquePunteros, bloquePunteros_Aux, BLOCKSIZE) != 0){

            // Si quedan punteros != 0 en el bloque lo salvamos
            if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) != 0){
                if (bwrite(*ptr, bloquePunteros) == FALLO){
                    fprintf(stderr,RED "Error al escribir el bloque de punteros\n");
                    return FALLO;
                }           
                //Imprimir que se ha salvado el bloque (color naranja)
                #if DEBUGN6
                printf(ORANGE "[liberar_bloques_inodo()\u2192 salvado BF %u de punteros_nivel %d correspondiente al BL %u]\n"RESET, *ptr, nivel_punteros, ult_cero-1);
                #endif
                total_bwrites++;
            } else { // Si no hay punteros != 0 en el bloque lo liberamos
                liberar_bloque(*ptr);
                #if DEBUGN6
                printf(WHITE"[liberar_bloques_inodo()\u2192 liberado BF %u de punteros_nivel %d correspondiente al BL %u]\n"RESET, *ptr, nivel_punteros, *nBL-1);
                #endif
                *ptr = 0;
                liberados++;
                
            }      
        }

        // Imprimir último salto si es necesario
        if (*nBL > ult_cero && !(*eof)) {
            #if DEBUGN6
            printf(BLUE "[liberar_bloques_inodo()→ Saltamos del BL %u al BL %u]\n" RESET, ult_cero,*nBL - 1);
            #endif
        
        }
        
    }  else { // Si el puntero es 0 es que tiene que ir a otro nivel
        switch (nRangoBL) {
            case 1:
                *nBL = INDIRECTOS0;
                break;
            case 2:
                *nBL = INDIRECTOS1;
                break;
            case 3:
                *nBL = INDIRECTOS2;
                break;
            default:
                break;
        }
    }

    // Devolvemos el número de bloques liberados
    return liberados;
}
//#endif

/**
 * mi_truncar_f --> Trunca un fichero a los bytes indicados como nbytes, liberando los bloques necesarios.
 * @param ninodo: Número de inodo a truncar
 * @param nbytes: Número de bytes a truncar
 * @return Número de bloques liberados, FALLO en caso contrario
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes){
    // Definimos las variables necesarias
    struct inodo inodo;
    unsigned int primerBL = 0;
    int liberados;

    //Leemos el inodo
    if(leer_inodo(ninodo,&inodo)==FALLO){
        fprintf(stderr,RED "Error en la lectura del inodo\n");
        return FALLO;
    }

    // Comprobamos permisos de escritura
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr,RED "Error: el inodo %u no tiene permisos de escritura\n", ninodo);
        return FALLO;
    }

    // No se puede truncar más allá del tamaño del fichero
    if (nbytes > inodo.tamEnBytesLog) {
        fprintf(stderr,RED "Error: no se puede truncar más allá del tamaño del fichero\n");
        return FALLO;
    }

    // Calculamos el primer bloque lógico necesario
    if (nbytes % BLOCKSIZE == 0) {
        primerBL = nbytes / BLOCKSIZE;
    } else {
        primerBL = nbytes / BLOCKSIZE + 1;
    }

    // Liberamos los bloques de datos
    liberados = liberar_bloques_inodo(primerBL, &inodo);

    // Actualizamos mtime y ctime
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // Actualizamos el tamaño en bytes lógicos
    inodo.tamEnBytesLog = nbytes;

    // Actualizamos el número de bloques ocupados
    inodo.numBloquesOcupados -= liberados;

    // Escribimos el inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr,RED "Error en la escritura del inodo %u\n", ninodo);
        return FALLO;
    }

    return liberados;
}

