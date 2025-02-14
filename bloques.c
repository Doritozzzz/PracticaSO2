// Bloques.c

#include "bloques.h"

// Descriptor del fichero (dispositivo virtual)
static int descriptor = 0;

/**
 * bmount --> Función para montar el dispositivo virtual
 * @param camino: Ruta del fichero que se va a montar
 * @return Descriptor si se ha montado correctamente, FALLO en caso contrario
 */
int bmount(const char *camino) {
    // Abrir el fichero en modo lectura y escritura
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    // Comprobar si se ha abierto correctamente, devuelve FALLO en caso contrario
    if (descriptor == -1) {
        fprintf(stderr, "Error en la apertura del fichero %s: %s\n", camino, strerror(errno));
        return FALLO;
    }
    // Devolver el descriptor
    return descriptor;
}
/**
 * bumount --> Función para desmontar el dispositivo virtual
 * @return EXITO si se ha desmontado correctamente, FALLO en caso contrario
 */
int bumount(){
    // Cerrar el fichero (dispositivo virtual)
    int cierre = close(descriptor);

    // Comprobar si se ha cerrado correctamente, devuelve FALLO en caso contrario
    if (cierre == -1) {
        fprintf(stderr, "Error en el cierre del fichero: %s\n", strerror(errno));
        return FALLO;
    }
    // Devolver EXITO (se ha cerrado correctamente)
    return EXITO;
}

int bwrite(unsigned int nbloque, const void *buf);

int bread(unsigned int nbloque, void *buf);
