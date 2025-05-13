#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directorios.h"


int main(int argc, char **argv) {
    // Verificar sintaxis correcta (mi_ls [-l] <disco> </ruta>)
    if (argc != 3 && argc != 4) {
        fprintf(stderr, RED"Sintaxis: ./mi_ls [-l] <nombre_dispositivo> </ruta>\n"RESET);
        return FALLO;
    }

    // Determinar si está el flag -l
    char flag = 's'; // Modo simple por defecto
    char *disco, *ruta;

    if (argc == 4 && strcmp(argv[1], "-l") == 0) {
        flag = 'l'; // Modo extendido
        disco = argv[2];
        ruta = argv[3];
    } else {
        disco = argv[1];
        ruta = argv[2];
    }

    // Montar dispositivo
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED"Error al montar el dispositivo\n"RESET);
        return FALLO;
    }

    // Determinar tipo (directorio o fichero)
    char tipo;
    if (ruta[strlen(ruta) - 1] == '/') {
        tipo = 'd'; // Directorio
    } else {
        tipo = 'f'; // Fichero
    }

    // Llamar a mi_dir()
    char buffer[TAMBUFFER];
    int nentradas = mi_dir(ruta, buffer, tipo, flag);

    // Manejar errores
    if (nentradas < 0) {
        mostrar_error_buscar_entrada(nentradas);
        // Desmontar dispositivo
        if (bumount() == FALLO) {
            fprintf(stderr, RED"Error al desmontar el dispositivo\n"RESET);
        }
        return FALLO;
    }

    // Imprimir resultados
    if (tipo == 'd' && nentradas > 0) {
        if (flag == 'l') {
            // Modo extendido: imprimir cabeceras y buffer completo
            printf("%s", buffer);
        } else {
            // Modo simple: eliminar última tabulación y añadir salto de línea
            buffer[strlen(buffer) - 1] = '\0'; 
            printf("Total: %d\n%s\n", nentradas, buffer);
        }
    } else if (tipo == 'f') {
        // Mostrar metadatos del fichero (buffer ya formateado)
        
        printf("%s", buffer);
    }

    // Desmontar dispositivo
    if (bumount() == FALLO) {
        fprintf(stderr, RED"Error al desmontar el dispositivo\n"RESET);
        return FALLO;
    }

    return EXITO;
}