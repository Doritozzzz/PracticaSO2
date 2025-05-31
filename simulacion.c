#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "simulacion.h"


#define REGMAX 500000
#define ESPERA_PROCESOS 150000   // 150ms en microsegundos
#define ESPERA_ESCRITURAS 50000  // 50ms en microsegundos

int acabados = 0;  // Contador de procesos finalizados

// Función enterrador de procesos
void reaper(int signum) {
    signal(SIGCHLD, reaper);
    pid_t ended;
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}

int main(int argc, char **argv) {
    int error;
    if (argc != 2) {
        fprintf(stderr, "Sintaxis: %s <disco>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Montar disco (proceso padre)
    if (bmount(argv[1]) == -1) {
        perror("Error al montar disco");
        exit(EXIT_FAILURE);
    }

    // Crear directorio de simulación (ruta absoluta)
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char nombreSimul[TAMNOMBRE];
    strftime(nombreSimul, TAMNOMBRE, "/simul_%Y%m%d%H%M%S/", tm_info); // Sin barra final
    
    // Crear directorio principal
    if (mi_creat(nombreSimul, 7) < 0) {
        fprintf(stderr, "Error al crear directorio de simulación\n");
        bumount();
        exit(EXIT_FAILURE);
    }

    // Registrar manejador de SIGCHLD
    signal(SIGCHLD, reaper);

    printf("*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n", 
           NUMPROCESOS, NUMESCRITURAS);
    printf("Directorio de simulación: %s\n", nombreSimul);

    // Lanzar procesos hijos
    for (int i = 1; i < NUMPROCESOS+1; i++) {
        pid_t pid = fork();
        if (pid == 0) {  // Proceso hijo
            // Montar disco (cada hijo tiene su propio descriptor)
            if (bmount(argv[1]) == -1) {
                perror("Error en hijo al montar disco");
                exit(EXIT_FAILURE);
            }

            // Crear directorio del proceso (ruta absoluta)
            char nombreDir[TAMNOMBRE];
            snprintf(nombreDir, TAMNOMBRE, "%sproceso_%d/", nombreSimul, getpid());
            if (mi_creat(nombreDir, 7) < 0) {
                perror("Error creando directorio proceso");
                bumount();
                exit(EXIT_FAILURE);
            }

            // Crear fichero prueba.dat (ruta absoluta)
            char nombreFichero[TAMNOMBRE];
            snprintf(nombreFichero, TAMNOMBRE, "%sproceso_%d/prueba.dat", nombreSimul, getpid());
            if (mi_creat(nombreFichero, 7) < 0) {
                perror("Error creando prueba.dat");
                bumount();
                exit(EXIT_FAILURE);
            }

            // Inicializar semilla aleatoria
            srand(time(NULL) + getpid());

            // Realizar escrituras
            for (int j = 0; j < NUMESCRITURAS; j++) {
                struct REGISTRO reg;
                reg.fecha = time(NULL);
                reg.pid = getpid();
                reg.nEscritura = j + 1;
                reg.nRegistro = rand() % REGMAX;

                // Escribir registro (ruta absoluta)
                off_t offset = reg.nRegistro * sizeof(struct REGISTRO);
                error=mi_write(nombreFichero, &reg, offset, sizeof(struct REGISTRO));
                if (error < 0) {
                    perror(RED"Error write proceso\n"RESET);
                }
                
                usleep(ESPERA_ESCRITURAS);
            }
            printf("%s[Proceso %d: Completadas %d escrituras en %sproceso_%d/prueba.dat]%s\n", 
                   GRAY, i, NUMESCRITURAS, nombreSimul, getpid(), RESET);

            bumount();
            exit(0);
        }
        else if (pid < 0) {
            perror("Error en fork");
            continue;
        }

        usleep(ESPERA_PROCESOS);
    }

    while (acabados < NUMPROCESOS) {
        pause();
    }

    bumount();
    printf("Simulación completada. Total procesos: %d\n", NUMPROCESOS);
    return 0;
}