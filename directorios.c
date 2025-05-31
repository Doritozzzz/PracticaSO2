#include "directorios.h"
#define PROFUNDIDAD 32
// Estructura para guardar la última entrada buscada (principio de proximidad)
typedef struct {
    char camino[TAMNOMBRE * PROFUNDIDAD];
    unsigned int p_inodo;
} UltimaEntrada;

// Caches separadas para lectura y escritura
static UltimaEntrada UltimaEntradaEscritura = {{""}, 0};
static UltimaEntrada UltimaEntradaLectura  = {{""}, 0};



/**
 *  extraer_camino --> Función para extraer el camino de un fichero
 *  @param camino: Cadena de caracteres que representa el camino del fichero
 *  @param inicial: Cadena de caracteres que representa el inicial
 *  Porción de *camino comprendida entre los dos primeros '/' ⇒ *inicial contendrá el nombre de un directorio.
 *  Si no hay segundo '/': porción de *camino sin el primer '/' ⇒  *inicial contendrá el nombre de un fichero.
 *  @param final: Cadena de caracteres que representa el resto del camino
 *  @param tipo: Cadena de caracteres que representa el tipo de fichero ('d' o 'f')
 *  @return EXITO si se ha extraído correctamente, FALLO en caso contrario
 */

 int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino == NULL || camino[0] != '/') {
        return FALLO;  // Código de error
    }
    const char *segunda_barra = strchr(camino + 1, '/');
    
    if (segunda_barra != NULL) {
        // Caso directorio
        //calculamos lo que mide el nombre del directorio
        size_t len_inicial = segunda_barra - (camino + 1);
        //copiamos desde el primer caracter hasta el ultimo del nombre
        //del directorio
        strncpy(inicial, camino + 1, len_inicial);
        //terminador de strings
        inicial[len_inicial] = '\0';
        *tipo = 'd';
        //recopiamos el final desde la segunda barra
        strcpy(final, segunda_barra);
    } else {
        // Caso fichero
        strcpy(inicial, camino + 1);
        *tipo = 'f';
        final[0] = '\0';  // Cadena vacía
    }

    return EXITO;
}

/**
 * mostrar_error_buscar_entrada: Muestra un mensaje de error según el código de error
 * @param error: Código de error devuelto por buscar_entrada()
 */
void mostrar_error_buscar_entrada(int error)
{
    // Incluir códigos ANSI para color rojo y reset.

    switch (error)
    {
    case -2:
        fprintf(stderr, "%sError: Camino incorrecto.%s\n", RED, RESET);
        break;
    case -3:
        fprintf(stderr, "%sError: Permiso denegado de lectura.%s\n", RED, RESET);
        break;
    case -4:
        fprintf(stderr, "%sError: No existe el archivo o el directorio.%s\n", RED, RESET);
        break;
    case -5:
        fprintf(stderr, "%sError: No existe algún directorio intermedio.%s\n", RED, RESET);
        break;
    case -6:
        fprintf(stderr, "%sError: Permiso denegado de escritura.%s\n", RED, RESET);
        break;
    case -7:
        fprintf(stderr, "%sError: El archivo ya existe.%s\n", RED, RESET);
        break;
    case -8:
        fprintf(stderr, "%sError: No es un directorio.%s\n", RED, RESET);
        break;
    default:
        fprintf(stderr, "%sError: Código de error desconocido: %d%s\n", RED, error, RESET);
        break;
    }
}

/**
 * buscar_entrada: Busca una entrada en un directorio y devuelve su inodo
 * @param camino_parcial: Camino parcial del fichero o directorio a buscar
 * @param p_inodo_dir: Puntero al inodo del directorio padre
 * @param p_inodo: Puntero al inodo del fichero o directorio encontrado
 * @param p_entrada: Puntero a la posición de la entrada en el directorio
 * @param reservar: Si es 1, reserva un nuevo inodo si no se encuentra la entrada
 * @param permisos: Permisos del nuevo inodo (si se reserva)
 * @return EXITO si se encuentra la entrada, FALLO en caso contrario
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    struct superbloque SB;
    char inicial[TAMNOMBRE], final[strlen(camino_parcial) + 1];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo = 0, encontrado = 0;

    

    // Leer superbloque (según tu requerimiento)
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        printf("PAN\n");
        return FALLO;
    }

    if (strcmp(camino_parcial, "/") == 0) {
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    if (extraer_camino(camino_parcial, inicial, final, &tipo) == FALLO){
        return ERROR_CAMINO_INCORRECTO;
    } 


    // Debug: valores extraídos del camino
    #if DEBUGN8
    printf(GRAY"[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n"RESET, inicial, final, reservar);
    #endif

    leer_inodo(*p_inodo_dir, &inodo_dir);
    if ((inodo_dir.permisos & 4) != 4){
        return ERROR_PERMISO_LECTURA;
    }

    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    while (num_entrada_inodo < cant_entradas_inodo && !encontrado) {
        unsigned int offset = num_entrada_inodo * sizeof(struct entrada);
        if (mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada)) == FALLO){
          printf("READ\n");
          return FALLO;  
        } 
        if (strcmp(entrada.nombre, inicial) == 0) {
            encontrado = 1;
        }
        else {
            num_entrada_inodo++;
        }
    }

    if (!encontrado) {
        if (!reservar){
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA; 
        } 
        if (inodo_dir.tipo != 'd') {
            return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
        }
        if ((inodo_dir.permisos & 2) != 2) {
            return ERROR_PERMISO_ESCRITURA;
        }

        struct entrada nueva_entrada;
        strcpy(nueva_entrada.nombre, inicial);

        if (tipo == 'd') {
            if (strcmp(final, "/") != 0) {
                return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
            }
            nueva_entrada.ninodo = reservar_inodo('d', permisos);
            // Debug: reserva de inodo directorio
            #if DEBUGN8
            printf(GRAY"[buscar_entrada()→ reservado inodo %d tipo d con permisos %d para %s]\n"RESET, nueva_entrada.ninodo, permisos, inicial);
            #endif
        } else {
            nueva_entrada.ninodo = reservar_inodo('f', permisos);
            // Debug: reserva de inodo fichero
            #if DEBUGN8
            printf(GRAY"[buscar_entrada()→ reservado inodo %d tipo f con permisos %d para %s]\n"RESET, nueva_entrada.ninodo, permisos, inicial);
            #endif
        }

        if (mi_write_f(*p_inodo_dir, &nueva_entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO) {
            printf("WRITE\n");
            liberar_inodo(nueva_entrada.ninodo);
            return FALLO;
        }
        // Debug: entrada creada
        #if DEBUGN8
        printf(GRAY"[buscar_entrada()→ creada entrada: %s, %d]\n"RESET, inicial, nueva_entrada.ninodo);
        #endif
        *p_inodo = nueva_entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXITO;
    }

    if (strlen(final) == 0 || strcmp(final, "/") == 0) {
        if (reservar && encontrado) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo; 
        *p_entrada = num_entrada_inodo;
         
        return EXITO;
    } else {
        *p_inodo_dir = entrada.ninodo;
        
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}

/**
 * mi_creat: Crea un fichero con los permisos indicados
 * @param camino: Ruta del fichero a crear
 * @param permisos: Permisos del nuevo inodo (0-7)
 * @return EXITO si se crea el fichero, FALLO en caso contrario
 */
int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();
    // Validar permisos (0-7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: Permisos inválidos (0-7)\n");
        mi_signalSem();
        return FALLO;
    }

    // Obtener inodo raíz del superbloque (no haría falta, podemos suponer
    //que el inodo raiz es el 0)
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        mi_signalSem();
        return FALLO;
    }
    unsigned int p_inodo_dir = SB.posInodoRaiz;

    // Variables para buscar_entrada()
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error;

    // Llamar a buscar_entrada() con reservar=1
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    mi_signalSem();
    return error;
}

/**
 * mi_dir: Muestra el contenido de un directorio o los metadatos de un fichero
 * @param camino: Ruta del directorio o fichero a mostrar
 * @param buffer: Buffer donde se almacenará la salida
 * @param tipo: Tipo de entrada ('d' para directorio, 'f' para fichero)
 * @param flag: Modo de salida ('l' para extendido, 's' para simple)
 * @return Número de entradas leídas o FALLO en caso de error
 */
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    mi_waitSem();
    // Inicialización de variables
    struct inodo dir_inodo;  // inodo original del directorio
    struct inodo entrada_inodo;               // inodo de cada entrada
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int nentradas = 0;
    char tmp[TAMFILA], permisos[4];
    struct tm *tm;
    struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    int offset = 0, bytes_leidos;
    unsigned int tam_dir_bytes;               // conservar tamaño original del directorio

    // Limpiar el buffer
    memset(buffer, 0, TAMBUFFER);

    // 1. Buscar la entrada y verificar existencia
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0){
        mi_signalSem();
        return error;
    } 

    // 2. Leer inodo y verificar tipo y permisos (directorio o archivo)
    if (leer_inodo(p_inodo, &dir_inodo) == FALLO) {
        fprintf(stderr, RED "Error al leer el inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Verificar coincidencia entre tipo esperado y real
    if ((tipo == 'd' && dir_inodo.tipo != 'd') || (tipo == 'f' && dir_inodo.tipo != 'f')) {
        sprintf(buffer, RED "Error: Tipo de entrada no coincide\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Verificar permisos de lectura
    if (!(dir_inodo.permisos & 4)) {
        sprintf(buffer, RED "Error: Permiso de lectura denegado\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if (tipo == 'f') {
        
        if (flag == 'l') { // Modo extendido
            printf("Tipo\tPermisos\tmTime\t\tTamaño\tNombre\n--------------------------------------------------\n");
         
            // Formatear permisos
            permisos[0] = (dir_inodo.permisos & 4) ? 'r' : '-';
            permisos[1] = (dir_inodo.permisos & 2) ? 'w' : '-';
            permisos[2] = (dir_inodo.permisos & 1) ? 'x' : '-';
            permisos[3] = '\0';

            // Formatear fecha
            tm = localtime(&dir_inodo.mtime);
            sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d",
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min, tm->tm_sec);

            // Construir línea de salida
            sprintf(buffer, "%c\t%s\t%s\t%d\t%s\n",
                   tipo, permisos, tmp, dir_inodo.tamEnBytesLog, camino);
        } else { // Modo simple
            
            sprintf(buffer, "%s\n", camino);
        }
        mi_signalSem();
        return 1; // Solo una "entrada" (el archivo mismo)
    }
    
    // Si es directorio: guardamos el tamaño original antes de entrar al bucle
    tam_dir_bytes = dir_inodo.tamEnBytesLog;

    // Cabecera modo extendido
    if (flag == 'l' && tam_dir_bytes > 0) {
        sprintf(buffer, "Total: %lu\nTipo\tPermisos\tmTime\t\tTamaño\tNombre\n--------------------------------------------------\n", 
               tam_dir_bytes / sizeof(struct entrada));
    }

    // 4. Leer entradas en bucle sin sobrescribir dir_inodo
    while (offset < tam_dir_bytes) {
        bytes_leidos = mi_read_f(p_inodo, entradas, offset, BLOCKSIZE);
        if (bytes_leidos < 0){
            mi_signalSem();
            return FALLO;
        } 

        int n = bytes_leidos / sizeof(struct entrada);
        for (int i = 0; i < n; i++) {
            // Leemos metadatos de la entrada en una variable distinta
            leer_inodo(entradas[i].ninodo, &entrada_inodo);

            if (flag == 'l') {
                // Tipo
                strcat(buffer, entrada_inodo.tipo == 'd' ? "d\t" : "f\t");
                // Permisos
                strcat(buffer, (entrada_inodo.permisos & 4) ? "r" : "-");
                strcat(buffer, (entrada_inodo.permisos & 2) ? "w" : "-");
                strcat(buffer, (entrada_inodo.permisos & 1) ? "x" : "-");
                strcat(buffer, "\t");
                // Fecha
                tm = localtime(&entrada_inodo.mtime);
                sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
                // Tamaño y nombre
                sprintf(tmp + strlen(tmp), "\t%u\t%s", entrada_inodo.tamEnBytesLog, entradas[i].nombre);
                strcat(buffer, tmp);
                strcat(buffer, "\n");
            } else {
                // Modo simple: nombre + tab
                strcat(buffer, entradas[i].nombre);
                strcat(buffer, "\t");
            }
            nentradas++;
        }
        offset += BLOCKSIZE;
    }
    mi_signalSem();
    return nentradas;
}

/**
 * mi_chmod: Cambia los permisos de un fichero o directorio
 * @param camino: Ruta del fichero o directorio
 * @param permisos: Nuevos permisos (0-7)
 * @return EXITO si se cambian los permisos, FALLO en caso contrario
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    // Validar permisos (0-7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: Permisos inválidos (0-7)\n");
        return FALLO;
    }

    // Obtener inodo raíz
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        return FALLO;
    }
    
    // Buscar la entrada
    unsigned int p_inodo_dir = SB.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    // Cambiar permisos
    if (mi_chmod_f(p_inodo, permisos) == FALLO) {

        fprintf(stderr, "Error al actualizar permisos\n");
        return FALLO;
    }

    return EXITO;
}

/**
 * mi_stat: Devuelve los metadatos de un fichero o directorio
 * @param camino: Ruta del fichero o directorio
 * @param p_stat: Puntero a la estructura STAT donde se almacenarán los metadatos
 * @return Número de inodo si tiene éxito, FALLO en caso contrario
 */
int mi_stat(const char *camino, struct STAT *p_stat) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        return FALLO;
    }

    unsigned int p_inodo_dir = SB.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    
    // Buscar la entrada y obtener el número de inodo (p_inodo)
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    // Llenar la estructura STAT con los metadatos del inodo
    if (mi_stat_f(p_inodo, p_stat) == FALLO) {
        fprintf(stderr, "Error al obtener metadatos\n");
        return FALLO;
    }

    // Devolver el número de inodo como valor positivo
    return p_inodo; 
}

/**
 * mi_write: Escribe datos en un fichero
 * @param camino: Ruta del fichero
 * @param buf: Buffer con los datos a escribir
 * @param offset: Offset donde empezar a escribir
 * @param nbytes: Número de bytes a escribir
 * @return Número de bytes escritos o FALLO en caso de error
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int bytes_escritos;

    // Caché de escritura
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        p_inodo = UltimaEntradaEscritura.p_inodo;
    } else {
        // búsqueda del inodo con permisos 6 (lectura+escritura)
        
        int res = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
        if (res < 0){
            printf("%s\n",camino);
            return res;
        }
        // actualizar caché
        strncpy(UltimaEntradaEscritura.camino, camino, sizeof(UltimaEntradaEscritura.camino)-1);
        UltimaEntradaEscritura.camino[sizeof(UltimaEntradaEscritura.camino)-1] = '\0';
        UltimaEntradaEscritura.p_inodo = p_inodo;
    }

    // escritura en ficheros
    bytes_escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    if (bytes_escritos < 0){
        return bytes_escritos;
    } 

    return bytes_escritos;
}

/**
 * mi_read: Lee datos de un fichero
 * @param camino: Ruta del fichero
 * @param buf: Buffer donde se almacenarán los datos leídos
 * @param offset: Offset donde empezar a leer
 * @param nbytes: Número de bytes a leer
 * @return Número de bytes leídos o FALLO en caso de error
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int bytes_leidos;
    // Caché de lectura
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        p_inodo = UltimaEntradaLectura.p_inodo;
    } else {
        // búsqueda del inodo con permisos 4 (solo lectura)
        int res = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (res < 0){
            mostrar_error_buscar_entrada(res);
            return FALLO;
        } 
        // actualizar caché
        strncpy(UltimaEntradaLectura.camino, camino, sizeof(UltimaEntradaLectura.camino)-1);
        UltimaEntradaLectura.camino[sizeof(UltimaEntradaLectura.camino)-1] = '\0';
        UltimaEntradaLectura.p_inodo = p_inodo;
    }

    // lectura en ficheros
    bytes_leidos = mi_read_f(p_inodo, buf, offset, nbytes);
    if (bytes_leidos < 0) {
        return FALLO;
    }

    return bytes_leidos;
}

/**
 * actualizar_timestamp: Actualiza el timestamp de un inodo
 * @param ninodo: Número de inodo a actualizar
 * @param tipo: Tipo de timestamp a actualizar ('m' para mtime, 'a' para atime)
 * @return EXITO si se actualiza correctamente, FALLO en caso contrario
 */
int actualizar_timestamp(unsigned int ninodo, char tipo) {
    struct inodo in;
    if (leer_inodo(ninodo, &in) == FALLO) return -1;
    time_t t = time(NULL);
    if (t == (time_t)-1) return -1;
    if (tipo == 'm')      in.mtime = t;
    else if (tipo == 'a') in.atime = t;
    else return -1;
    in.ctime = t;
    if (escribir_inodo(ninodo, &in) == FALLO) return -1;
    return 0;
}


int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem();
    unsigned int p_inodo_dir1 = 0, p_inodo_dir2 = 0;
    unsigned int p_inodo1, p_inodo2;
    unsigned int p_entrada1, p_entrada2;
    int error;
    struct inodo inodo;

    // 1. Buscar entrada para camino1 (sin reservar)
    if ((error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0)) < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return FALLO;
    }
    

    // Leer inodo de camino1 y verificar que es fichero
    if (leer_inodo(p_inodo1, &inodo) < 0) return -1;
    if (inodo.tipo == 'd') {
        fprintf(stderr, "Error: camino1 es un directorio\n");
        mi_signalSem();
        return FALLO;
    }

    // 2. Buscar entrada para camino2 (reservando)
    if ((error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return FALLO;
    }
    // 3. Leer entrada creada y modificar su inodo
    struct entrada entrada;
    if (mi_read_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }


    // 4. Actualizar entrada con el inodo de camino1
    entrada.ninodo = p_inodo1;
    if (mi_write_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    // 5. Liberar inodo reservado para camino2
    liberar_inodo(p_inodo2);

    // 6. Actualizar metadatos de camino1
    inodo.nlinks++;
    inodo.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo) < 0) {
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();
    return EXITO;
}

int mi_unlink(const char *camino) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0, p_inodo, p_entrada;
    int error;
    struct inodo inodo;
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    // 1. Buscar la entrada a borrar
    if ((error) < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return error;
    }
    // 2. Leer inodo a borrar
    if (leer_inodo(p_inodo, &inodo) < 0){
        mi_signalSem();
        return FALLO;  
    } 

    // 3. Si es directorio, verificar que está vacío
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        fprintf(stderr, RED"Error: El directorio %s no está vacío\n"RESET, camino);
        mi_signalSem();
        return FALLO;
    }

    // 4. Eliminar entrada del directorio padre
    struct entrada entrada;
    struct inodo inodo_dir;
    
    // Leer entrada y último inodo del directorio padre
    if (leer_inodo(p_inodo_dir, &inodo_dir) < 0){
        mi_signalSem();
        return FALLO;
    } 
    
    // Si no es la última entrada, mover la última
    if (p_entrada != (inodo_dir.tamEnBytesLog / sizeof(struct entrada) - 1)) {
        mi_read_f(p_inodo_dir, &entrada, inodo_dir.tamEnBytesLog - sizeof(struct entrada), sizeof(struct entrada));
        mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada));
    }
    
    // Truncar directorio padre
    mi_truncar_f(p_inodo_dir, inodo_dir.tamEnBytesLog - sizeof(struct entrada));

    // 5. Liberar inodo
    inodo.nlinks--;
    if (inodo.nlinks == 0) {
        liberar_inodo(p_inodo);
    } else {
        inodo.ctime = time(NULL);
        escribir_inodo(p_inodo, &inodo);
    }
    mi_signalSem();
    return EXITO;
}