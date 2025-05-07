#include "directorios.h"

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
        return EXITO;
    } else {
        unsigned int nuevo_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, &nuevo_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}