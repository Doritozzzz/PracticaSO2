#include "ficheros_basico.h"

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


    if(bumount(nombre_dispositivo)==FALLO){
        fprintf(stderr,"Error en el cierre del dispositivo virtual.\n");
        return FALLO;
    }

    return EXITO;

}