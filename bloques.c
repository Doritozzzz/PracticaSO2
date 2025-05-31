// bloques.c

#include "bloques.h"

// Descriptor del fichero (dispositivo virtual)
static int descriptor = 0;

//Semáforo global del fs
static sem_t *mutex;
static unsigned int inside_sc = 0;

//Funciones inicio y parada de semaforos
void mi_waitSem() {
   if (!inside_sc) { // inside_sc==0, no se ha hecho ya un wait
       waitSem(mutex);
   }
   inside_sc++;
}


void mi_signalSem() {
   inside_sc--;
   if (!inside_sc) {
       signalSem(mutex);
   }
}


/**
 * bmount --> Función para montar el dispositivo virtual
 * @param camino: Ruta del fichero que se va a montar
 * @return Descriptor si se ha montado correctamente, FALLO en caso contrario
 */
int bmount(const char *camino) {
    if(descriptor>0){
        close(descriptor);
    }
    // Cambiar la máscara de creación de ficheros para evitar problemas de permisos
    umask(000);

    // Abrir el fichero en modo lectura y escritura
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    
    // Comprobar si se ha abierto correctamente, devuelve FALLO en caso contrario
    if (descriptor == -1) {
        fprintf(stderr,RED "Error en la apertura del fichero %s: %s\n", camino, strerror(errno));
        return FALLO;
    }
    if (!mutex) { // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
       mutex = initSem(); 
       if (mutex == SEM_FAILED) {
           return -1;
       }
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
        fprintf(stderr,RED "Error en el cierre del fichero: %s\n", strerror(errno));
        return FALLO;
    }
    deleteSem();
    // Devolver EXITO (se ha cerrado correctamente)
    return EXITO;
}
/**
 * bwrite --> Función para escribir en un bloque
 * @param nbloque: Número de bloque físico donde se va a escribir
 * @param buf: Buffer con los datos a escribir (con tamaño BLOCKSIZE)
 * 
 * Calcula el desplazamiento en el fichero, se desplaza a esa posición y 
 * escribe en el fichero. Se usa off_t para el desplazamiento y ssize_t (permite +2Gb)
 * 
 * @return Numero de bytes escritos, FALLO en caso contrario
 */
int bwrite(unsigned int nbloque, const void *buf){
    // Calculamos desplazamiento en el fichero
    off_t desplazamiento = nbloque * BLOCKSIZE;

    // Movemos el puntero a la posición calculada
    int posicion = lseek(descriptor, desplazamiento, SEEK_SET);

    // Comprobar si se ha desplazado correctamente, devuelve FALLO en caso contrario
    if (posicion == -1) {
        fprintf(stderr,RED "Error en el desplazamiento del fichero: %s\n", strerror(errno));
        return FALLO;
    }

    // Escribir en el fichero
    ssize_t escritura = write(descriptor, buf, BLOCKSIZE);

    // Comprobar si se ha escrito correctamente, devuelve FALLO en caso contrario
    if (escritura == -1) {
        fprintf(stderr,RED "Error en la escritura del fichero: %s\n", strerror(errno));
        return FALLO;
    }

    // Devolver el número de bytes escritos (BLOCKSIZE)--> 1024
    return escritura;
}

/**
 * bread --> Función para leer un bloque
 * @param nbloque: Número de bloque físico que se va a leer
 * @param buf: Buffer donde se van a almacenar los datos leídos (con tamaño BLOCKSIZE)
 * 
 * Calcula el desplazamiento en el fichero, se desplaza a esa posición y 
 * lee del fichero. Se usa off_t para el desplazamiento y ssize_t (permite +2Gb)
 * 
 * @return Numero de bytes leidos, FALLO en caso contrario
 */
int bread(unsigned int nbloque, void *buf){
    // Calculamos desplazamiento en el fichero
    off_t desplazamiento = nbloque * BLOCKSIZE;

    // Movemos el puntero a la posición calculada
    int posicion = lseek(descriptor, desplazamiento, SEEK_SET);

    // Comprobar si se ha desplazado correctamente, devuelve FALLO en caso contrario
    if (posicion == -1) {
        fprintf(stderr,RED "Error en el desplazamiento del fichero: %s\n", strerror(errno));
        return FALLO;
    }

    // Leer del fichero
    ssize_t lectura = read(descriptor, buf, BLOCKSIZE);

    // Comprobar si se ha leído correctamente, devuelve FALLO en caso contrario
    if (lectura == -1) {
        fprintf(stderr,RED "Error en la lectura del fichero: %s\n", strerror(errno));
        return FALLO;
    }

    // Devolver el número de bytes leidos (BLOCKSIZE)--> 1024
    return lectura;
}
