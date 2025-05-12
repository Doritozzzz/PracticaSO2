//Función main con parámetros que recogerán los valores introducidos por 
//teclado junto a la ejecución del programa ./mi_mkdir <disco> <permisos>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "directorios.h"

int main(int argc, char **argv) {
    // Verificar sintaxis correcta
    if (argc != 4) {
        fprintf(stderr, RED"Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta>\n"RESET);
        return FALLO;
    }

    // Validar permisos (0-7)
    int permisos = atoi(argv[2]); //entero
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED"Error: Permisos inválidos (0-7)\n"RESET);
        return FALLO;
    }

    // Montar dispositivo
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED"Error al montar el dispositivo\n"RESET);
        return FALLO;
    }

    // Llamar a mi_creat() para crear el directorio/fichero
    int resultado = mi_creat(argv[3], permisos);

    //Muestra de posibles errores en la creación del directorio/fichero
    if(resultado!=EXITO){
        mostrar_error_buscar_entrada(resultado);
    }

    // Desmontar dispositivo
    if (bumount() == FALLO) {
        fprintf(stderr, RED"Error al desmontar el dispositivo\n"RESET);
        return FALLO;
    }

    return resultado;
}