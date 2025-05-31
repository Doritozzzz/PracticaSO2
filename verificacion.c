#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "verificacion.h"

#define REGMAX 500000
#define BUFFER_SIZE 256

// Función para formatear timestamp
char* formatear_fecha(time_t fecha) {
    static char buffer[30];
    struct tm *tm_info = localtime(&fecha);
    strftime(buffer, 30, "%a %d-%m-%Y %H:%M:%S", tm_info);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <directorio_simulacion>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Asegurar que el directorio termine con '/'
    char dir_simul[256];
    snprintf(dir_simul, sizeof(dir_simul), "%s%c", 
             argv[2], (argv[2][strlen(argv[2])-1] == '/' ? '\0' : '/'));

    // Montar dispositivo
    if (bmount(argv[1]) == -1) {
        perror("Error al montar disco");
        exit(EXIT_FAILURE);
    }

    struct STAT stat_dir;
    struct INFORMACION info;
    struct entrada entrada;
    char ruta_fichero[512];
    char ruta_informe[512];
    char buffer_informe[2000];
    
    // Obtener información del directorio de simulación
    if (mi_stat(dir_simul, &stat_dir) < 0) {
        perror("Error en mi_stat directorio");
        bumount();
        exit(EXIT_FAILURE);
    }

    int num_procesos = stat_dir.tamEnBytesLog / sizeof(struct entrada);
    if (num_procesos != NUMPROCESOS) {
        fprintf(stderr, "Error: Se esperaban %d procesos, encontrados %d\n", 
                NUMPROCESOS, num_procesos);
        bumount();
        exit(EXIT_FAILURE);
    }

    // Crear fichero de informe
    snprintf(ruta_informe, sizeof(ruta_informe), "%sinforme.txt", dir_simul);
    if (mi_creat(ruta_informe, 6) < 0) {
        perror("Error creando informe.txt");
        bumount();
        exit(EXIT_FAILURE);
    }

    // Obtener tamaño actual del informe (para escritura incremental)
    struct STAT stat_informe;
    off_t offset_escritura = 0;
    if (mi_stat(ruta_informe, &stat_informe) == 0) {
        offset_escritura = stat_informe.tamEnBytesLog;
    }

    printf("*** VERIFICACIÓN DE ESCRITURAS EN %s ***\n", dir_simul);
    printf("Procesos: %d, Escrituras esperadas: %d\n\n", NUMPROCESOS, NUMPROCESOS * NUMESCRITURAS);

    // Buffer para lectura de registros
    struct REGISTRO buffer_registros[BUFFER_SIZE];
    
    // Recorrer directorios de procesos
    for (int i = 0; i < num_procesos; i++) {
        memset(buffer_registros, 0, sizeof(buffer_registros));
        memset(&info, 0, sizeof(info));
        
        // Leer entrada de directorio
        if (mi_read(dir_simul, &entrada, i * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            perror("Error leyendo entrada directorio");
            continue;
        }

        // Extraer PID del nombre del directorio
        char *pid_str = strchr(entrada.nombre, '_');
        if (!pid_str) {
            fprintf(stderr, "Formato nombre incorrecto: %s\n", entrada.nombre);
            continue;
        }
        info.pid = atoi(pid_str + 1);
        
        // Construir ruta al fichero prueba.dat
        snprintf(ruta_fichero, sizeof(ruta_fichero), "%s%s/prueba.dat", dir_simul, entrada.nombre);
        
        // Recorrer fichero secuencialmente
        off_t offset = 0;
        int bytes_leidos;
        
        while ((bytes_leidos = mi_read(ruta_fichero, buffer_registros, offset, sizeof(buffer_registros))) > 0) {
            int num_registros_bloque = bytes_leidos / sizeof(struct REGISTRO);
            
            for (int j = 0; j < num_registros_bloque; j++) {
                // Validar registro (PID debe coincidir)
                if (buffer_registros[j].pid == info.pid) {
                    // Primera escritura válida
                    if (info.nEscrituras == 0) {
                        info.primera_escritura = buffer_registros[j];
                        info.ultima_escritura = buffer_registros[j];
                        info.menor_posicion = buffer_registros[j];
                        info.mayor_posicion = buffer_registros[j];
                    } else {
                        // Actualizar primera/última escritura (por número)
                        if (buffer_registros[j].nEscritura < info.primera_escritura.nEscritura) {
                            info.primera_escritura = buffer_registros[j];
                        }
                        if (buffer_registros[j].nEscritura > info.ultima_escritura.nEscritura) {
                            info.ultima_escritura = buffer_registros[j];
                        }
                        
                        // Actualizar menor/mayor posición
                        if (buffer_registros[j].nRegistro < info.menor_posicion.nRegistro) {
                            info.menor_posicion = buffer_registros[j];
                        }
                        if (buffer_registros[j].nRegistro > info.mayor_posicion.nRegistro) {
                            info.mayor_posicion = buffer_registros[j];
                        }
                    }
                    info.nEscrituras++;
                }
            }
            offset += bytes_leidos;
            memset(buffer_registros, 0, sizeof(buffer_registros));
        }

        // Generar línea de informe
        int len = snprintf(buffer_informe, sizeof(buffer_informe), 
            "PID: %d\n"
            "Numero de escrituras: %u\n"
            "Primera Escritura\t%d\t%d\t%s\n"
            "Ultima Escritura\t%d\t%d\t%s\n"
            "Menor Posicion\t\t%d\t%d\t%s\n"
            "Mayor Posicion\t\t%d\t%d\t%s\n\n",
            info.pid, info.nEscrituras,
            info.primera_escritura.nEscritura, info.primera_escritura.nRegistro, 
            formatear_fecha(info.primera_escritura.fecha),
            info.ultima_escritura.nEscritura, info.ultima_escritura.nRegistro, 
            formatear_fecha(info.ultima_escritura.fecha),
            info.menor_posicion.nEscritura, info.menor_posicion.nRegistro, 
            formatear_fecha(info.menor_posicion.fecha),
            info.mayor_posicion.nEscritura, info.mayor_posicion.nRegistro, 
            formatear_fecha(info.mayor_posicion.fecha));
        
        // Escribir en informe.txt (append)
        int escritos = mi_write(ruta_informe, buffer_informe, offset_escritura, len);
        if (escritos < 0) {
            fprintf(stderr, "Error escribiendo en informe: %d\n", escritos);
        } else {
            offset_escritura += escritos;
        }
        
        printf("[%d] %d escrituras validadas en %s\n", 
               i+1, info.nEscrituras, ruta_fichero);
    }

    bumount();
    printf("\nVerificacion completada. Informe generado en %s\n", ruta_informe);
    return 0;
}