// leer_sf.c
#include "ficheros_basico.h"
#include <time.h>

/**
 * Programa para leer el superbloque, mostrar el mapa de bits, reservar/liberar bloques
 * y mostrar información del inodo raíz.
 * Uso: ./leer_sf <nombre_dispositivo>
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];

    // Montar el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error en el montaje del dispositivo virtual.\n");
        return FALLO;
    }

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error en la lectura del superbloque\n");
        bumount();
        return FALLO;
    }

    printf("\n===== DATOS DEL SUPERBLOQUE =====\n");
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

    printf("sizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));
    printf("\n");

    #if DEBUG_N4
    printf("===== MAPA DE BITS =====\n");
    unsigned int bloques[] = {SB.posPrimerBloqueMB, SB.posUltimoBloqueMB, 
                              SB.posPrimerBloqueAI, SB.posUltimoBloqueAI, 
                              SB.posPrimerBloqueDatos, SB.posUltimoBloqueDatos};
    
    for (int i = 0; i < 6; i++) {
        int bit = leer_bit(bloques[i]);
        printf("[leer_bit(%u)] → %d\n", bloques[i], bit);
    }
    printf("\n");

    printf("===== RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS =====\n");
    printf("SB.cantBloquesLibres antes = %d\n", SB.cantBloquesLibres);

    int bloqueReservado = reservar_bloque();
    if (bloqueReservado == FALLO) {
        fprintf(stderr, "Error al reservar un bloque.\n");
    } else {
        // Actualizar SB desde disco
        if (bread(posSB, &SB) == FALLO) {
            fprintf(stderr, "Error al refrescar superbloque\n");
            bumount();
            return FALLO;
        }
        printf("Se ha reservado el bloque físico nº %d\n", bloqueReservado);
        printf("SB.cantBloquesLibres después de reservar = %d\n", SB.cantBloquesLibres);

        if (liberar_bloque(bloqueReservado) == FALLO) {
            fprintf(stderr, "Error al liberar el bloque %d.\n", bloqueReservado);
        } else {
            // Actualizar SB desde disco
            if (bread(posSB, &SB) == FALLO) {
                fprintf(stderr, "Error al refrescar superbloque\n");
                bumount();
                return FALLO;
            }
            printf("Bloque %d liberado correctamente.\n", bloqueReservado);
            printf("SB.cantBloquesLibres después de liberar = %d\n", SB.cantBloquesLibres);
        }
    }
    printf("\n");

    printf("===== RESERVAMOS UN INODO =====\n");
    printf("SB.cantInodosLibres antes = %d\n", SB.cantInodosLibres);
    int inodoReservado = reservar_inodo('d', 6); // Directorio con permisos 6
    if (inodoReservado == FALLO) {
        fprintf(stderr, "Error al reservar inodo.\n");
    } else {
        // Actualizar SB desde disco
        if (bread(posSB, &SB) == FALLO) {
            fprintf(stderr, "Error al refrescar superbloque\n");
            bumount();
            return FALLO;
        }
        printf("Inodo reservado: %d\n", inodoReservado);
        printf("SB.cantInodosLibres después = %d\n", SB.cantInodosLibres);
    }
    printf("\n");

    printf("===== DATOS DEL INODO DEL DIRECTORIO RAÍZ =====\n");
    struct inodo inodoRaiz;
    if (leer_inodo(SB.posInodoRaiz, &inodoRaiz) == FALLO) {
        fprintf(stderr, "Error al leer el inodo raíz.\n");
    } else {
        printf("tipo: %c\n", inodoRaiz.tipo);
        printf("permisos: %d\n", inodoRaiz.permisos);

        char atime[80], mtime[80], ctime[80], btime[80];
        struct tm *ts;
        
        ts = localtime(&inodoRaiz.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoRaiz.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoRaiz.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoRaiz.btime);
        strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

        printf("atime: %s\n", atime);
        printf("mtime: %s\n", mtime);
        printf("ctime: %s\n", ctime);
        printf("btime: %s\n", btime);
        printf("nlinks: %d\n", inodoRaiz.nlinks);
        printf("tamEnBytesLog: %d\n", inodoRaiz.tamEnBytesLog);
        printf("numBloquesOcupados: %d\n", inodoRaiz.numBloquesOcupados);
    }
    printf("\n");
    #endif

    #if DEBUG_N5
    // Prueba de traducción de bloques lógicos
    printf("===== PRUEBA DE TRADUCCIÓN DE BLOQUES LÓGICOS =====\n");

    // Reservamos un inodo para usar en las pruebas
    int inodoReservado = reservar_inodo('d', 6);
    if (inodoReservado == FALLO) {
        fprintf(stderr, "Error al reservar inodo.\n");
        bumount();
        return FALLO;
    }

    // Traducimos bloques lógicos a bloques físicos
    unsigned int bloques_logicos[] = {8, 204, 30004, 400004, 468750};
    for (int i = 0; i < 5; i++) {
        int bloque_fisico = traducir_bloque_inodo(inodoReservado, bloques_logicos[i], 1);
        if (bloque_fisico == FALLO) {
            fprintf(stderr, "Error en la traducción del bloque lógico %u\n", bloques_logicos[i]);
        } else {
            printf("Bloque lógico %u → Bloque físico %d\n\n", bloques_logicos[i], bloque_fisico);
        }
    }

    // Se lee y muestra la información del inodo reservado
    printf("===== DATOS DEL INODO RESERVADO %d =====\n", inodoReservado);
    struct inodo inodoReservadoEstruct;
    if (leer_inodo(inodoReservado, &inodoReservadoEstruct) == FALLO) {
        fprintf(stderr, "Error al leer el inodo reservado.\n");
    } else {
        printf("tipo: %c\n", inodoReservadoEstruct.tipo);
        printf("permisos: %d\n", inodoReservadoEstruct.permisos);

        char atime[80], mtime[80], ctime[80], btime[80];
        struct tm *ts;

        ts = localtime(&inodoReservadoEstruct.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoReservadoEstruct.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoReservadoEstruct.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoReservadoEstruct.btime);
        strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

        printf("atime: %s\n", atime);
        printf("mtime: %s\n", mtime);
        printf("ctime: %s\n", ctime);
        printf("btime: %s\n", btime);
        printf("nlinks: %d\n", inodoReservadoEstruct.nlinks);
        printf("tamEnBytesLog: %d\n", inodoReservadoEstruct.tamEnBytesLog);
        printf("numBloquesOcupados: %d\n", inodoReservadoEstruct.numBloquesOcupados);
    }

    // Actualizar SB desde disco
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al refrescar superbloque\n");
        bumount();
        return FALLO;
    }
    
    // Mostramos el valor actualizado de posPrimerInodoLibre
    printf("SB.posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("\n");
#endif

    // Desmontar el dispositivo
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error en el cierre del dispositivo virtual.\n");
        return FALLO;
    }

    return EXITO;
}