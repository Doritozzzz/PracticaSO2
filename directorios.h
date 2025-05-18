// directorios.h
#include "ficheros.h" 


#define TAMNOMBRE 60 //tamaño del nombre de directorio o fichero, en Ext2 = 256
#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000) // Buffer para 1000 entradas

struct entrada {
  char nombre[TAMNOMBRE];
  unsigned int ninodo;
};

// Prototipo de actualizar_timestamp para evitar declaración implícita
typedef struct inodo inodo_t;

//Nivel 7
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);
//Nivel 8
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char tipo, char flag);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);
//Nivel 9
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
int actualizar_timestamp(unsigned int ninodo, char tipo);
//Nivel 10
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);