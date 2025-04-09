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

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{
    // Caso especial: camino es "/" (directorio raíz)
    if (strcmp(camino, "/") == 0)
    {
        strcpy(inicial, "");
        strcpy(final, "");
        *tipo = 'd';
        return EXITO;
    }

    // Verificar que el camino empieza con '/'
    if (camino[0] != '/')
    {
        return FALLO;
    }

    // Saltar la '/' inicial
    const char *p = camino + 1;
    char *slash = strchr(p, '/');

    if (slash)
    {
        // Copiar el primer componente (inicial)
        int len = slash - p;
        strncpy(inicial, p, len);
        inicial[len] = '\0';

        // El resto es final (incluye '/')
        strcpy(final, slash);
        *tipo = 'd'; // Hay más componentes ⇒ directorio
    }
    else
    {
        // No hay más '/', es un fichero
        strcpy(inicial, p);
        strcpy(final, "");
        *tipo = 'f';
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
/*
 * buscar_entrada --> Busca una entrada en un directorio
 * @param camino_parcial: Cadena de caracteres que representa el camino parcial
 * @param p_inodo_dir: Puntero al inodo del directorio
 * @param p_inodo: Puntero al número de inodo al que está asociado el nombre de la entrada buscada.
 * @param p_entrada: Puntero al número de entrada   dentro del inodo *p_inodo_dir que lo contiene (empezando por 0)
 * @param reservar: Indica si se debe reservar espacio para la entrada
 * @param permisos: Permisos del inodo
 * @return EXITO si se ha encontrado la entrada, ERROR_NO_EXISTE_ENTRADA_CONSULTA si no existe, ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO si no existe el directorio intermedio
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos)
{
    // Definimos las variables necesarias
    struct entrada entrada;
    struct inodo inodo_dir;
    struct superbloque SB;
    unsigned int ninodo;
    struct entrada nueva_entrada;
    char inicial[TAMNOMBRE];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;
    cant_entradas_inodo = sizeof(struct inodo) / sizeof(struct entrada);
    // Leemos el superbloque
    if (bread(posSB, &SB) == FALLO)
    {
        fprintf(stderr, "Error al leer el superbloque\n");
        return FALLO;
    }

    // Si es el directorio raíz
    if (*p_inodo_dir == 0)
    {
        *p_inodo_dir = SB.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    // Extraemos el camino
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == ERROR_CAMINO_INCORRECTO){
        printf("Error al extraer el camino\n");
        mostrar_error_buscar_entrada(ERROR_CAMINO_INCORRECTO);
        return ERROR_CAMINO_INCORRECTO;
    }

    // Buscamos entrada cuyo nombre sea igual a inicial
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
    {
        mostrar_error_buscar_entrada(ERROR_PERMISO_LECTURA);
        return ERROR_PERMISO_LECTURA;
    }

    // Inicializamos el buffer de lectura a ceros
    memset(&entrada, 0, sizeof(struct entrada));
    // Calculamos el número de entradas por inodo
    num_entrada_inodo = 0;
    if (cant_entradas_inodo > 0)
    {
        // Leer entradas del inodo
        if (bread(inodo_dir.punterosDirectos[0], &entrada) == FALLO)
        {
            mostrar_error_buscar_entrada(ERROR_NO_EXISTE_ENTRADA_CONSULTA);
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        }
        // Buscamos la entrada
        while (num_entrada_inodo < cant_entradas_inodo)
        {
            if (strcmp(entrada.nombre, inicial) == 0)
            {
                // Si se encuentra la entrada
                *p_inodo = entrada.ninodo;
                *p_entrada = num_entrada_inodo;
                return EXITO;
            }
            num_entrada_inodo++;
            // Leer siguiente entrada
            if (bread(inodo_dir.punterosDirectos[0] + num_entrada_inodo * sizeof(struct entrada), &entrada) == FALLO)
            {
                mostrar_error_buscar_entrada(ERROR_NO_EXISTE_ENTRADA_CONSULTA);
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            }
        }
    }
    // Si la entrada no existe
    if (strcmp(entrada.nombre, inicial) != 0)
    {
        switch (reservar)
        {
        case 0: // Modo consulta
            mostrar_error_buscar_entrada(ERROR_NO_EXISTE_ENTRADA_CONSULTA);
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        case 1: // Modo escritura
            if (inodo_dir.tipo == 'f')
            {
                mostrar_error_buscar_entrada(ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO);
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            if ((inodo_dir.permisos & 2) != 2)
            {
                mostrar_error_buscar_entrada(ERROR_PERMISO_ESCRITURA);
                return ERROR_PERMISO_ESCRITURA;
            }
            else
            {
                // Copiar inicial en entrada
                strcpy(entrada.nombre, inicial);

                if (tipo == 'd')
                {
                    if (strcmp(final, "/") != 0)
                    {
                        mostrar_error_buscar_entrada(ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO);
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                    ninodo = reservar_inodo('d', permisos);
                }
                else
                {
                    ninodo = reservar_inodo('f', permisos);
                }
                nueva_entrada.ninodo = ninodo;
            }
            // Escribir entrada al final
            if (mi_write_f(*p_inodo_dir, &nueva_entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO)
            {
                if (reservar == 1)
                {
                    liberar_inodo(ninodo);
                }
                return FALLO;
            }
            *p_inodo = ninodo;
            *p_entrada = cant_entradas_inodo;
            break;
        }
    }
    // Si hemos llegado al final
    if (strlen(final) > 0)
    {
        if (num_entrada_inodo < cant_entradas_inodo && reservar == 1)
        {
            mostrar_error_buscar_entrada(ERROR_ENTRADA_YA_EXISTENTE);
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXITO;
    }
    else
    {
        *p_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
    return EXITO;
}