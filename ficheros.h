// ficheros.h
#include "ficheros_basico.h"

/**
 * Struct que representa un fichero en el sistema de ficheros
 */

 typedef struct STAT {
    char tipo;
    unsigned char permisos;
    unsigned int nlinks;
    unsigned int tamEnBytesLog;
    time_t atime;
    time_t mtime;
    time_t ctime;
    time_t btime;
    unsigned int numBloquesOcupados;
} STAT;


//Nivel 5
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_stat_f(unsigned int ninodo, STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);