// leer_sf.cS
#include "ficheros_basico.h"

/**
 * Programa para leer el superbloque y la lista enlazada de inodos libres
 * Para usarlo, ejecutar: ./leer_sf <nombre_dispositivo>
 */
int main(int argc, char **argv){
    char *nombre_dispositivo = argv[1];
    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error en el montaje del dispositivo virtual.\n");
        return FALLO;
    }

    
    printf("DATOS DEL SUPERBLOQUE:\n");
    
    struct superbloque SB;
    if(bread(posSB,&SB)==FALLO){
        fprintf(stderr,"Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }
    
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n", SB.totInodos);
    printf("\n");

    printf("sizeof struct superbloque is: %lu\n",sizeof(struct superbloque));
    printf ("sizeof struct inodo is: %lu\n", sizeof(struct inodo));
    printf("\n");


    printf("LISTA ENLAZADA DE INODOS LIBRES:\n");

    unsigned int indice = SB.posPrimerInodoLibre;
    int inodosPorBloque = BLOCKSIZE / INODOSIZE;
    struct inodo inodos[inodosPorBloque];

    while (indice != UINT_MAX) {
        // Determinamos el bloque del AI y el offset dentro de ese bloque
        unsigned int bloqueAI = SB.posPrimerBloqueAI + (indice / inodosPorBloque);
        unsigned int offset = indice % inodosPorBloque;

        if (bread(bloqueAI, (char *)inodos) == FALLO) {
            fprintf(stderr, "Error al leer el bloque de inodos %u.\n", bloqueAI);
            bumount();
            return FALLO;
        }

        printf("Inodo %u -> punterosDirectos[0] = %u\n", indice, inodos[offset].punterosDirectos[0]);
        // El valor en punterosDirectos[0] es el índice del siguiente inodo libre
        indice = inodos[offset].punterosDirectos[0];
    }
    

    if(bumount(nombre_dispositivo)==FALLO){
        fprintf(stderr,"Error en el cierre del dispositivo virtual.\n");
        return FALLO;
    }

    return EXITO;

}