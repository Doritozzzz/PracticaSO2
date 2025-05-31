#include "directorios.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


/**
 * Programa para leer un fichero y mostrar su contenido por pantalla o guardarlo en otro fichero.
 * Uso: mi_cat <nombre_dispositivo> </ruta_fichero> [>fichero_salida.txt]
 */
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> </ruta_fichero> [>fichero_salida.txt]\n", argv[0]);
        return FALLO;
    }

    const char *nombre_dispositivo = argv[1];
    const char *camino = argv[2];
    unsigned int offset = 0;
    int leidos = 0, fd_salida = STDOUT_FILENO;
    unsigned int total_leidos = 0;
    unsigned char buffer[TAMBUFFER];
    struct STAT stat;

    // 1. Manejar redirección si hay 4 argumentos
    if (argc == 4) {
        if (strcmp(argv[3], ">") != 0) {
            fprintf(stderr, "Error: Formato de redirección incorrecto. Use '> fichero'\n");
            return FALLO;
        }
        fd_salida = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_salida == -1) {
            perror("Error al abrir fichero de salida");
            return FALLO;
        }
    }

    // 2. Montar dispositivo
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error al montar %s\n", nombre_dispositivo);
        if (argc == 4) close(fd_salida);
        return FALLO;
    }

    // 3. Validar que no sea un directorio
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, "Error: %s es un directorio\n", camino);
        bumount();
        if (argc == 4) close(fd_salida);
        return FALLO;
    }

    // 4. Obtener metadatos
    if (mi_stat(camino, &stat) == FALLO) {
        fprintf(stderr, "Error: no existe %s\n", camino);
        bumount();
        if (argc == 4) close(fd_salida);
        return FALLO;
    }
    
    // 5. Leer y mostrar contenido
    unsigned int bytes_restantes = stat.tamEnBytesLog;
    while (bytes_restantes > 0) {
        leidos = mi_read(camino, buffer, offset, TAMBUFFER);
        if (leidos < 0) {
            fprintf(stderr, "Error al leer %s\n", camino);
            break;
        }
        write(fd_salida, buffer, leidos);
        total_leidos += leidos;
        offset += leidos;
        bytes_restantes -= leidos;

    }

    // 6. Estadísticas y limpieza
    dprintf(2, "\nTotal_leidos: %u\nTamEnBytesLog: %u\n", total_leidos, stat.tamEnBytesLog);
    
    bumount();
    if (argc == 4) close(fd_salida);

    return (leidos < 0) ? FALLO : EXITO;
}