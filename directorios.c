#include "directorios.h"

#define PROFUNDIDAD 32

struct UltimaEntrada {
    char camino[TAMNOMBRE * PROFUNDIDAD];
    int p_inodo;
};

static struct UltimaEntrada UltimaEntradaLectura;
static struct UltimaEntrada UltimaEntradaEscritura;


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
    printf(GRAY"[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n"RESET, inicial, final, reservar);

    leer_inodo(*p_inodo_dir, &inodo_dir);
    if ((inodo_dir.permisos & 4) != 4){
        return ERROR_PERMISO_LECTURA;
    }

    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    while (num_entrada_inodo < cant_entradas_inodo && !encontrado) {
        unsigned int offset = num_entrada_inodo * sizeof(struct entrada);
        if (mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada)) == FALLO){
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
            printf(GRAY"[buscar_entrada()→ reservado inodo %d tipo d con permisos %d para %s]\n"RESET, nueva_entrada.ninodo, permisos, inicial);
        } else {
            nueva_entrada.ninodo = reservar_inodo('f', permisos);
            // Debug: reserva de inodo fichero
            printf(GRAY"[buscar_entrada()→ reservado inodo %d tipo f con permisos %d para %s]\n"RESET, nueva_entrada.ninodo, permisos, inicial);
        }

        if (mi_write_f(*p_inodo_dir, &nueva_entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO) {
            liberar_inodo(nueva_entrada.ninodo);
            return FALLO;
        }
        // Debug: entrada creada
        printf(GRAY"[buscar_entrada()→ creada entrada: %s, %d]\n"RESET, inicial, nueva_entrada.ninodo);
        *p_inodo = nueva_entrada.ninodo;
        *p_entrada = cant_entradas_inodo;
    }

    if (strlen(final) == 0 || strcmp(final, "/") == 0) {
        if (reservar && encontrado) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo; 
        *p_entrada = num_entrada_inodo;
        return EXITO;
    } else {
        unsigned int nuevo_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, &nuevo_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}


int mi_creat(const char *camino, unsigned char permisos) {
    // Validar permisos (0-7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: Permisos inválidos (0-7)\n");
        return FALLO;
    }

    // Obtener inodo raíz del superbloque (no haría falta, podemos suponer
    //que el inodo raiz es el 0)
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, "Error al leer el superbloque\n");
        return FALLO;
    }
    unsigned int p_inodo_dir = SB.posInodoRaiz;

    // Variables para buscar_entrada()
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error;

    // Llamar a buscar_entrada() con reservar=1
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    return error;
}

int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    // Inicialización de variables
    struct inodo inodo;
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int nentradas = 0;
    char tmp[TAMFILA], permisos[4];
    struct tm *tm;
    struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    int offset = 0, bytes_leidos;

    // Limpiar el buffer
    memset(buffer, 0, TAMBUFFER);

    // 1. Buscar la entrada y verificar existencia
    int error= buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        return error; // Error ya manejado por buscar_entrada()
    }

    // 2. Leer inodo y verificar tipo y permisos
    if(leer_inodo(p_inodo, &inodo)==FALLO){
        fprintf(stderr, RED"Error al leer el inodo\n"RESET);
        return FALLO;
    }
    
    
    // Verificar coincidencia entre tipo esperado y real
    if ((tipo == 'd' && inodo.tipo != 'd') || (tipo == 'f' && inodo.tipo != 'f')) {
        sprintf(buffer, RED"Error: Tipo de entrada no coincide\n"RESET);
        return FALLO;
    }

    // Verificar permisos de lectura
    if (!(inodo.permisos & 4)) {

        sprintf(buffer, RED"Error: Permiso de lectura denegado\n"RESET);
        return FALLO;
    }

    // 3. Caso 1: Es un archivo (mostrar metadatos)
    if (tipo == 'f') {
        
        if (flag == 'l') { // Modo extendido
            printf("Tipo\tPermisos\tmTime\t\tTamaño\tNombre\n--------------------------------------------------\n");
         
            // Formatear permisos
            permisos[0] = (inodo.permisos & 4) ? 'r' : '-';
            permisos[1] = (inodo.permisos & 2) ? 'w' : '-';
            permisos[2] = (inodo.permisos & 1) ? 'x' : '-';
            permisos[3] = '\0';

            // Formatear fecha
            tm = localtime(&inodo.mtime);
            sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d",
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min, tm->tm_sec);

            // Construir línea de salida

            sprintf(buffer, "%c\t%s\t%s\t%d\t%s\n",
                   tipo, permisos, tmp, inodo.tamEnBytesLog, camino);
        } else { // Modo simple
            
            sprintf(buffer, "%s\n", camino);
        }
        return 1; // Solo una "entrada" (el archivo mismo)
    }

    // 4. Caso 2: Es un directorio (listar entradas)
    // Cabecera para modo extendido
    if (flag == 'l' && inodo.tamEnBytesLog > 0) {
        sprintf(buffer, "Total: %ld\nTipo\tPermisos\tmTime\t\tTamaño\tNombre\n--------------------------------------------------\n", 
               inodo.tamEnBytesLog / sizeof(struct entrada));
    }

    // Leer entradas del directorio por bloques
    
    
    
    while (offset < inodo.tamEnBytesLog) {
        bytes_leidos = mi_read_f(p_inodo, entradas, offset, BLOCKSIZE);
        if (bytes_leidos < 0) return -1;

        // Procesar cada entrada del bloque actual
        for (int i = 0; i < bytes_leidos / sizeof(struct entrada); i++) {
            leer_inodo(entradas[i].ninodo, &inodo);

            if (flag == 'l') { // Modo extendido
                if(inodo.tipo=='f'){
                    strcat(buffer, "f\t");
                }else strcat(buffer, "d\t");

                // Permisos
                if (inodo.permisos & 4) strcat(buffer, "r"); else strcat(buffer, "-\t");
                if (inodo.permisos & 2) strcat(buffer, "w"); else strcat(buffer, "-\t");
                if (inodo.permisos & 1) strcat(buffer, "x"); else strcat(buffer, "-\t");

                // Fecha
                tm = localtime(&inodo.mtime);
                sprintf(tmp, "%04d-%02d-%02d %02d:%02d:%02d",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);

                // Concatenar al buffer
                sprintf(tmp + strlen(tmp), "\t%d\t%s", inodo.tamEnBytesLog, entradas[i].nombre);
                strcat(buffer, tmp);
                strcat(buffer, "\n");
            } else { // Modo simple
                strcat(buffer, entradas[i].nombre);
                strcat(buffer, "\t");
            }
            nentradas++;
        }
        offset += BLOCKSIZE;
    }

    return nentradas;
}

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

int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int bytes_escritos;
    int resultado;

    // Check if the path matches the cached entry
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        p_inodo = (unsigned int)UltimaEntradaEscritura.p_inodo;
    } else {
        // Cache miss: call buscar_entrada with necessary parameters
        resultado = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (resultado < 0) {
            return resultado; // Return error (e.g., ENOENT)
        }

        // Update the cache with the new path and inode (cast to int)
        strncpy(UltimaEntradaEscritura.camino, camino, sizeof(UltimaEntradaEscritura.camino) - 1);
        UltimaEntradaEscritura.camino[sizeof(UltimaEntradaEscritura.camino) - 1] = '\0';
        UltimaEntradaEscritura.p_inodo = (int)p_inodo;
    }

    // Perform the write operation using the obtained inode
    bytes_escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    return bytes_escritos;
}

int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int bytes_leidos;
    int resultado;

    // 1. Comprobar si el camino está en la caché de lectura
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        p_inodo = (unsigned int)UltimaEntradaLectura.p_inodo; // Conversión de tipos
    } else {
        // 2. Llamar a buscar_entrada si no hay coincidencia en la caché
        resultado = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (resultado < 0) {
            return resultado; // Devolver error (ej: ENOENT, EACCES)
        }

        // 3. Actualizar la caché de lectura
        strncpy(UltimaEntradaLectura.camino, camino, sizeof(UltimaEntradaLectura.camino) - 1);
        UltimaEntradaLectura.camino[sizeof(UltimaEntradaLectura.camino) - 1] = '\0'; // Asegurar fin de cadena
        UltimaEntradaLectura.p_inodo = (int)p_inodo; // Almacenar como int
    }

    // 4. Leer del inodo y devolver bytes leídos
    bytes_leidos = mi_read_f(p_inodo, buf, offset, nbytes);
    return bytes_leidos;
}