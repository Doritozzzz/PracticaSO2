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
    // Definimos las variables necesarias
    struct inodo inodo;
    unsigned int primerBL, ultimoBL, desp1, desp2, nbfisico;
    char buf_bloque[BLOCKSIZE];
    int bytes_escritos = 0;

    // Comprobamos que el inodo exista
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    // Comprobamos que el inodo tenga permisos de escritura
    if ((inodo.permisos & 2) != 2) { 
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET); 
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
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, nbytes); // Copiamos los datos

        if (bwrite(nbfisico, buf_bloque) == FALLO) { // Escribimos el bloque
            return FALLO;
        }
        bytes_escritos += nbytes;
    } else { // Segundo caso: Primer y último bloque lógico son distintos (hay que escribir en varios bloques)
        unsigned int i;
        unsigned int pos;

        // Escribimos en el primer bloque (hay que leerlo)
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1); // Reservamos el bloque
        if (nbfisico == FALLO) {
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) { // Leemos el bloque
            return FALLO;
        }
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1); // Copiamos los datos

        if (bwrite(nbfisico, buf_bloque) == FALLO) { // Escribimos el bloque
            return FALLO;
        }
        bytes_escritos += BLOCKSIZE - desp1;

        // Escribimos en los bloques intermedios (no hace falta leerlos)
        for (i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 1); // Reservamos el bloque
            if (nbfisico == FALLO) {
                return FALLO;
            }
            // La posición en el buffer original es la cantidad de bytes ya escritos:
            pos = (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE; 
            if (bwrite(nbfisico, (char *)buf_original + pos) == FALLO){
                return FALLO;
            }
            bytes_escritos += BLOCKSIZE;
        }

        // Escribimos en el último bloque (hay que leerlo)
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == FALLO){
            return FALLO;
        }

        if (bread(nbfisico, buf_bloque) == FALLO){
            return FALLO;
        }

        // Calculamos los bytes lógicos del último bloque
        unsigned int bytes_ultimo = desp2 + 1;
        // Copiamos los datos
        memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1); 
        if (bwrite(nbfisico, buf_bloque) == FALLO)
            return FALLO;
        bytes_escritos += bytes_ultimo;
    }

    // Leemos el inodo para actualizar los campos
    if (leer_inodo(ninodo, &inodo) == FALLO) {
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
        return FALLO;
    }

    // Retornamos la cantidad de bytes escritos
    return bytes_escritos;
}