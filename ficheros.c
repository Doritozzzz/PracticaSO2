#include "ficheros.h"

/**
 * miWrite_f --> Función para escribir un bloque de datos en un fichero
 * @param ninodo: Número de inodo del fichero
 * @param buf_original: Puntero al buffer de datos a escribir
 * @param offset: Desplazamiento en el fichero
 * @param nbytes: Número de bytes a escribir
 * @return Cantidad de bytes escritos correctamente, deberia ser igual a nbytes
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    mi_waitSem();
    // Definimos las variables necesarias
    struct inodo inodo;
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico;
    char buf_bloque[BLOCKSIZE];
    int bytes_escritos = 0;

    // Comprobamos que el inodo exista
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "El inodo no existe\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Comprobamos que el inodo tenga permisos de escritura
    if ((inodo.permisos & 2) != 2) { 
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET); 
        mi_signalSem();
        return FALLO; 
    }

    // Calculamos el primer y último bloque lógico a escribir
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos los desplazamientos en el primer y último bloque lógico
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // Primer caso: Primer y último bloque lógico son iguales (cabe todo en un solo bloque)
    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1); // Reservamos el bloque
        if (nbfisico == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes); // Copiamos los datos

        if (bwrite(nbfisico, buf_bloque) == FALLO) { // Escribimos el bloque
            mi_signalSem();
            return FALLO;
        }
        bytes_escritos += nbytes;
    } else { // Segundo caso: Primer y último bloque lógico son distintos (hay que escribir en varios bloques)
        unsigned int i;
        unsigned int pos;

        // Escribimos en el primer bloque (hay que leerlo)
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1); // Reservamos el bloque
        if (nbfisico == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) { // Leemos el bloque
            mi_signalSem();
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1); // Copiamos los datos

        if (bwrite(nbfisico, buf_bloque) == FALLO) { // Escribimos el bloque
            mi_signalSem();
            return FALLO;
        }
        bytes_escritos += BLOCKSIZE - desp1;

        // Escribimos en los bloques intermedios (no hace falta leerlos)
        for (i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 1); // Reservamos el bloque
            if (nbfisico == FALLO) {
                mi_signalSem();
                return FALLO;
            }
            // La posición en el buffer original es la cantidad de bytes ya escritos:
            pos = (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE; 
            if (bwrite(nbfisico, (char *)buf_original + pos) == FALLO){
                mi_signalSem();
                return FALLO;
            }
            bytes_escritos += BLOCKSIZE;
        }

        // Escribimos en el último bloque (hay que leerlo)
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == FALLO){
            mi_signalSem();
            return FALLO;
        }

        if (bread(nbfisico, buf_bloque) == FALLO){
            mi_signalSem();
            return FALLO;
        }

        // Calculamos los bytes lógicos del último bloque
        unsigned int bytes_ultimo = desp2 + 1;
        // Copiamos los datos
        memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1); 
        if (bwrite(nbfisico, buf_bloque) == FALLO){
            mi_signalSem();
            return FALLO;
        }
        bytes_escritos += bytes_ultimo;
    }

    // Leemos el inodo para actualizar los campos
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // Actualizamos el tamaño en bytes lógico si hemos escrito más allá del final del fichero (EOF)
    if (offset + nbytes > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nbytes;
    }

    // Actualizamos las marcas de tiempo (mtime y ctime) ya que se ha modificado el fichero
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // Escribir el inodo actualizado en el dispositivo
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // Retornamos la cantidad de bytes escritos
    mi_signalSem();
    return bytes_escritos;
}

/**
 * mi_read_f --> Función para leer un bloque de datos de un fichero
 * @param ninodo: Número de inodo del fichero
 * @param buf_original: Puntero al buffer de datos a leer
 * @param offset: Desplazamiento en el fichero
 * @param nbytes: Número de bytes a leer
 * @return Cantidad de bytes leídos correctamente, debería ser igual a nbytes
 */

 int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    mi_waitSem();
    // Definimos las variables necesarias
    struct inodo inodo;
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico;
    char buf_bloque[BLOCKSIZE];
    int bytes_leidos = 0;
    int inicio, final; // Posiciones de inicio y fin en el bloque lógico a leer

    // Comprobamos que el inodo exista
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "El inodo no existe\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Solo se puede leer si el inodo tiene permisos de lectura
    if ((inodo.permisos & 4) != 4) { 
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET); 
        mi_signalSem();
        return FALLO; 
    }

    // No puede leer más allá del tamaño en bytes lógico del fichero
    if (offset >= inodo.tamEnBytesLog) {
        bytes_leidos = 0;
        mi_signalSem();
        return bytes_leidos;
    }

    // Pretende leer más allá del EOF
    if ((offset + nbytes) >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset;
    }

    // Calculamos el primer y último bloque lógico a escribir
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos los desplazamientos en el primer y último bloque lógico
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // Iteramos sobre los bloques lógicos a leer
    for (unsigned int nblogico = primerBL; nblogico <= ultimoBL; nblogico++) {
        nbfisico = traducir_bloque_inodo(ninodo, nblogico, 0); // No reservamos el bloque
        if (nblogico == primerBL) {
            inicio = desp1;
        } else {
            inicio = 0;
        } 
        if (nblogico == ultimoBL) { // Final - inicio da el número de bytes a leer
            final = desp2 + 1;
        } else {
            final = BLOCKSIZE;
        }

        // Cantida de bytes a leer
        unsigned int bytes_a_leer = final - inicio;
        if (nbfisico == FALLO) { // Si no hay bloque lógico, rellenamos con 0s
            memset((char*)buf_original + bytes_leidos, 0, bytes_a_leer);
            bytes_leidos += bytes_a_leer;
            
        } else {
            if (bread(nbfisico, buf_bloque) == FALLO) { // Leemos el bloque
                mi_signalSem();
                return FALLO;
            }

            // Copiamos los datos al buffer original
            memcpy((char*) buf_original + bytes_leidos, buf_bloque + inicio, bytes_a_leer);
            bytes_leidos += bytes_a_leer; // Actualizamos la cantidad de bytes leidos
        }
    }
    // Actualizamos atime
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr,RED "Error al actualizar el inodo %u\n", ninodo);
        mi_signalSem();
        return FALLO;
    }

    // Retornamos la cantidad de bytes leídos
    mi_signalSem();
    return bytes_leidos;
}

/**
 * mi_stat_f --> Función para obtener la información de un fichero
 * @param ninodo: Número de inodo del fichero
 * @param p_stat: Puntero a la estructura STAT donde se almacenará la información
 * @return EXITO si todo ha ido bien, FALLO en caso de error
 */
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat){
    // Definimos las variables necesarias
    struct inodo inodo;

    // Comprobamos que el inodo exista
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "El inodo no existe\n" RESET);
        return FALLO;
    }

    // Usamos -> para acceder a los campos de la estructura, ya que se pasa por referencia
    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->btime = inodo.btime;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

    return EXITO;
}

/**
 * mi_chmod_f --> Función para cambiar los permisos de un fichero
 * @param ninodo: Número de inodo del fichero
 * @param permisos: Nuevos permisos del fichero
 * @return EXITO si todo ha ido bien, FALLO en caso de error
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos){
    mi_waitSem();
    // Definimos las variables necesarias
    struct inodo inodo;

    // Comprobamos que el inodo exista
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "El inodo no existe\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Actualizamos los permisos del inodo
    inodo.permisos = permisos;

    // Actualizamos la marca de tiempo ctime
    inodo.ctime = time(NULL);

    // Escribimos el inodo actualizado en el dispositivo
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();
    return EXITO;
}