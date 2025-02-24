#include "ficheros.h"

/**
 * Programa para escribir en un fichero--> Es para pruebas no se usa en la práctica
 * Uso: escribir ./escribir <nombre_dispositivo> <"$(cat fichero)"> <diferentes_inodos>
 * Si diferentes_inodos = 0, se escribirá en el mismo inodo
 * Si diferentes_inodos = 1, se escribirá en diferentes inodos
 */

 int main(int argc, char **argv) {
     // Definimos las variables necesarias
     char *nombre_dispositivo = argv[1];
     char *texto = argv[2];
     int diferentes_inodos = atoi(argv[3]);
     int longitud_texto = strlen(texto);
 
     // Comprobamos los argumentos
     if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombre_dispositivo> <\"texto a escribir\"> <diferentes_inodos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

     // Montar el dispositivo virtual
     if (bmount(nombre_dispositivo) == FALLO) {
         fprintf(stderr,RED "Error al montar el dispositivo virtual.\n");
         exit(EXIT_FAILURE);
     }
 
     // Pruebas de escritura
     unsigned int offsets[5] = {9000, 209000, 30725000, 409605000, 480000000};
     int total_offsets = 5;
     int ninodo, bytes_escritos;
 
     printf("Longitud texto: %d\n", longitud_texto);
 
     if (diferentes_inodos == 0) {
         // Reservar un único inodo para todos los offsets
         ninodo = reservar_inodo('f', 6);
         if (ninodo == FALLO) {
             fprintf(stderr, "Error al reservar inodo.\n");
             bumount();
             exit(EXIT_FAILURE);
         }
         printf("Nº inodo reservado: %d\n", ninodo);
         for (int i = 0; i < total_offsets; i++) {
             printf("Offset: %u\n", offsets[i]);
             bytes_escritos = mi_write_f(ninodo, texto, offsets[i], longitud_texto);
             if (bytes_escritos == FALLO) {
                 fprintf(stderr,RED "Error al escribir en el inodo %d\n", ninodo);
                 break;
             }
             printf("Bytes escritos: %d\n", bytes_escritos);
         }
     } else {
         // Reservar un inodo distinto para cada offset
         for (int i = 0; i < total_offsets; i++) {
             ninodo = reservar_inodo('f', 6);
             if (ninodo == FALLO) {
                 fprintf(stderr,RED "Error al reservar inodo.\n");
                 bumount();
                 exit(EXIT_FAILURE);
             }
             printf("Nº inodo reservado: %d\n", ninodo);
             printf("Offset: %u\n", offsets[i]);
             bytes_escritos = mi_write_f(ninodo, texto, offsets[i], longitud_texto);
             if (bytes_escritos == FALLO) {
                 fprintf(stderr,RED "Error al escribir en el inodo %d\n", ninodo);
                 break;
             }
             printf("Bytes escritos: %d\n", bytes_escritos);
         }
     }
 
     // Desmontar el dispositivo virtual
     if (bumount(nombre_dispositivo) == FALLO) {
         fprintf(stderr, "Error al desmontar el dispositivo virtual.\n");
         exit(EXIT_FAILURE);
     }
 
     return EXIT_SUCCESS;
 }
 