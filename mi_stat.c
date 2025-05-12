#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED"Sintaxis: ./mi_stat <nombre_dispositivo> </ruta>\n"RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    struct STAT stat;
    int num_inodo = mi_stat(argv[2], &stat); // num_inodo es el valor devuelto

    if (num_inodo < 0) {
        bumount();
        return FALLO;
    }

    // Formatear fechas
    struct tm *tm;
    char atime[80], mtime[80], ctime[80], btime[80];
    
    tm = localtime(&stat.atime);
    strftime(atime, 80, "%a %Y-%m-%d %H:%M:%S", tm);
    
    tm = localtime(&stat.mtime);
    strftime(mtime, 80, "%a %Y-%m-%d %H:%M:%S", tm);
    
    tm = localtime(&stat.ctime);
    strftime(ctime, 80, "%a %Y-%m-%d %H:%M:%S", tm);
    
    tm = localtime(&stat.btime);
    strftime(btime, 80, "%a %Y-%m-%d %H:%M:%S", tm);

    // Mostrar resultados
    printf("Nº de inodo: %d\n", num_inodo); // Usar el valor devuelto por mi_stat()
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %o\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks: %d\n", stat.nlinks);
    printf("tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    bumount();
    return EXITO;
}