#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TXT 60
#define MAX_SALAS 20

/* Tipos de pase */
#define PASE_GEN 1  /* pase general */
#define PASE_INF 2  /* pase infantil */
#define PASE_FAM 3  /* pase familiar */
#define PASE_PRIO 4 /* pase prioritario */

/* Estados del pase */
#define PASE_ACTIVO 1
#define PASE_USADO 2
#define PASE_ANULADO 3
#define PASE_VENCIDO 4

/* Tipos de publicacion */
#define PUB_SERIE 1
#define PUB_MINI 2   /* publicacion MiniSerie */
#define PUB_TOMO 3
#define PUB_NOVELA 4
#define PUB_TIRA 5   /* publicacion TiraComica */
#define PUB_FANZINE 6

/* Estados del titulo */
#define TIT_DISP 1
#define TIT_AGOTADO 2
#define TIT_PREVENTA 3
#define TIT_DESC 4

struct Pase {
    int codigo;
    int tipo;
    long valor;
    int estado;
};

/* Datos del cliente y su estado en la tienda  */
struct Cliente {
    char rut[MAX_TXT];
    char nombre[MAX_TXT];
    int edad;
    int dentroTienda;     /* 1: Sigue en la tienda, 0: Se retiro */
    int codigoSalaActual; /* Sala donde se encuentra actualmente */
    int horaIngreso;
    int horaSalida;
    struct Pase *pase;
};

/* Lista doblemente enlazada general de clientes */
struct NodoCliente {
    struct Cliente *datos;
    struct NodoCliente *ant, *sig;
};

/* Reserva individual en una fila  */
struct Reserva {
    int numero;              /* Numero unico de la reserva */
    struct Cliente *cliente; /* Puntero directo al cliente */
    int cantidad;            /* Ejemplares solicitados */
    int horaRegistro;        /* Hora en HHMM  */
};

/* Nodo simple para la cola de reservas */
struct NodoReserva {
    struct Reserva *datos;
    struct NodoReserva *sig;
};

/* Cola de espera estructurada */
struct ColaReservas {
    struct NodoReserva *frente;
    struct NodoReserva *final;
    int cantidadClientes; /* Cuantas personas hay en la fila */
    int totalEjemplares;  /* Suma real de comics pedidos en la fila */
};

/* Informacion de cada titulo/comic  */
struct Titulo {
    int codigo;
    char nombre[MAX_TXT];
    int tipoPublicacion;
    int edadMinima;
    long precio;
    int stock;
    int estado;
    int ejemplaresPorDespacho;
    int diasEntreDespachos;
    int limitePorCliente;
    int solicitudesDia;
    int diasAgotado;
    struct ColaReservas *listaEspera;
};

/* Arbol Binario de Busqueda - ABB  */
struct NodoTitulo {
    struct Titulo *datos;
    struct NodoTitulo *izq, *der;
};

/* Estructura de cada sala tematica  */
struct Sala {
    int codigo;
    char nombre[MAX_TXT];
    char tematica[MAX_TXT];
    char encargado[MAX_TXT];
    int horaApertura;
    int horaCierre;
    int aforoMaximo;
    int ocupacionActual;
    int cantidadTitulos;
    struct NodoTitulo *raizTitulos; /* ABB */
};

/* Estructura central que administra todo el sistema */
struct Tienda {
    char nombre[MAX_TXT];
    char ciudad[MAX_TXT];
    int aforoMaximo;
    int clientesDentro;
    int ingresosDia;
    int pasesVendidos;
    int pasesUtilizados;
    long recaudacionPases;

    int siguienteCodigoPase;    /* proximo codigo unico de pase */
    int siguienteNumeroReserva; /* proximo numero unico de reserva */

    struct Sala *salas[MAX_SALAS]; /* Arreglo estatico con pLibre */
    int pLibreSalas;

    struct NodoCliente *headClientes; /* Lista doblemente enlazada */
};

/*===============================================================================================================
 *  CLAUDIO
 =============================================================================================================== */

/* Retorna: 1 si se encolo al final con exito, 0 si fallo (ej. si el titulo esta en TIT_DESC).
   Llama a: Ninguna funcion. */
int agregarReservaCola(struct Titulo *titulo, struct Cliente *cliente, int cantidad, int numReserva, int hora);

/* Retorna: Puntero al struct Reserva encontrado, o NULL si no existe en la fila.
   Llama a: Ninguna funcion. */
struct Reserva* buscarReservaCola(struct ColaReservas *cola, int numeroReserva);

/* Retorna: 1 si se modifico la cantidad y se ajusto totalEjemplares, 0 si no se encontro.
   Llama a: buscarReservaCola con el objetivo de encontrar el nodo de la reserva que se quiere editar. */
int modificarReservaCola(struct ColaReservas *cola, int numeroReserva, int nuevaCantidad);

/* Retorna: 1 si se desencolo/elimino la reserva y se libero su memoria, 0 si no se encontro o la cola estaba vacia. */
int eliminarReservaCola(struct ColaReservas *cola, int numeroReserva);


void listarReservasCola(struct ColaReservas *cola);

/*===============================================================================================================
 * Cristopher
 =============================================================================================================== */

void ordenarSalas(struct Tienda *tienda, int criterio);

/* Retorna: 1 si se agrego con exito, 0 si el arreglo esta lleno. */
int agregarSala(struct Tienda *tienda, struct Sala *nuevaSala){
   if (tienda == NULL || nuevaSala == NULL){
      return 0;
   }
   if(tienda->pLibreSalas == MAX_SALAS){
      return 0;
   }
   if(nuevaSala->aforoMaximo <=0){
      printf("el aforo debe ser mayor a 0 \n");
      return 0;
   }
   
   if(nuevaSala->horaApertura < 0 || nuevaSala->horaApertura > 2359 || nuevaSala->horaApertura %100 >= 60){
      printf("Hora de apertura invalida\n");
      return 0;

   }
   if(nuevaSala->horaCierre < 0 || nuevaSala->horaCierre > 2359 || nuevaSala->horaCierre %100 >= 60){
      printf("hora de cierre invalida\n");
      return 0;
   }
   if(nuevaSala->horaApertura >=nuevaSala->horaCierre){
      printf("los datos de hora no coinciden\n");
      return 0;
   }
   
}

/* Retorna: El indice (posicion >= 0) de la sala en el arreglo, o -1 si no existe.
Llama a: ordenarSalas con el objetivo de dejar el arreglo ordenado por codigo antes de aplicar la Busqueda Binaria */
int buscarSala(struct Tienda *tienda, int codigoBuscado);

/* Retorna: 1 si se modifico con exito, 0 si no se encontro la sala.
Llama a: buscarSala con el objetivo de encontrar la posicion exacta de la sala que se quiere editar */
int modificarSala(struct Tienda *tienda, int codigo);

void listarSalas(struct Tienda *tienda);

/*===============================================================================================================
 * Benja Grez culo
 =============================================================================================================== */

/* Retorna: Puntero a la raiz del arbol actualizada con el nuevo nodo insertado.
   Llama a: agregarTituloABB (recursivamente)
   con el objetivo de recorrer el arbol por izquierda o derecha hasta encontrar donde insertar el titulo. */
struct NodoTitulo* agregarTituloABB(struct NodoTitulo *raiz, struct Titulo *nuevoTitulo);

/* Retorna: Puntero al struct Titulo encontrado, o NULL si no existe.
   Llama a: buscarTituloABB (recursivamente) con el objetivo de bajar por el arbol buscando el codigo. */
struct Titulo* buscarTituloABB(struct NodoTitulo *raiz, int codigo);

/* Retorna: 1 si se modifico con exito, 0 si el puntero es NULL.
   Llama a: eliminarReservaCola en un ciclo con el objetivo de entregar comics
   y sacar de la fila a los primeros clientes si subio el stock (despacho), o para vaciar toda la cola si el estado cambio a TIT_DESC. */
int modificarTitulo(struct Titulo *titulo);

/* Retorna: Puntero a la raiz del arbol actualizada tras borrar el nodo.
   Llama a: eliminarTituloABB (recursivamente) para reestructurar el arbol,
   y a eliminarReservaCola en ciclo con el objetivo de liberar toda la lista de espera del comic antes de hacerle free. */
struct NodoTitulo* eliminarTituloABB(struct NodoTitulo *raiz, int codigo);

/* Retorna: Nada (void).
   Llama a: listarTitulosABB (recursivamente) con el objetivo de recorrer e imprimir todo el arbol en In-Orden. */
void listarTitulosABB(struct NodoTitulo *raiz);

/* Retorna: La cantidad de dias estimados de espera (>= 0), o -1 si no hay lista de espera.*/
int calcularTiempoEsperaTitulo(struct Titulo *titulo);

void reporteTituloMasEsperado(struct Tienda *tienda);

void reporteTituloMasSolicitado(struct Tienda *tienda);

void listarTitulosAgotados(struct Tienda *tienda);

/*===============================================================================================================
 * Benja Garcia pene
 =============================================================================================================== */

/* Retorna: Puntero al nuevo struct Pase creado en memoria dinamica, o NULL si falla malloc.
   Llama a: Ninguna funcion. */
struct Pase* crearPase(int codigo, int tipo, long valor);

/* Retorna: Puntero al struct Cliente encontrado (con su pase adentro), o NULL si no existe.
   Llama a: Ninguna funcion. */
struct Cliente* buscarClientePorRut(struct NodoCliente *head, char *rut);

/* Retorna: 1 si el cliente se inserto en headClientes, 0 si el RUT ya existia o hubo error.
   Llama a: buscarClientePorRut para validar que no este repetido,
   y a crearPase con el objetivo de crearle y enlazarle su ticket de entrada. */
int agregarCliente(struct Tienda *tienda, char *rut, char *nombre, int edad, int tipoPase, long valorPase);

/* Retorna: 1 si se modificaron los datos o estados (ingreso/salida/cambio de sala), 0 si fallo o no se encontro.
   Llama a: buscarClientePorRut para ubicar al cliente a
   modificar, y a buscarSala con el objetivo de verificar el aforo y actualizar la ocupacionActual si el cliente se mueve de sala. */
int modificarClienteYPase(struct Tienda *tienda, char *rut);

/* Retorna: 1 si se desconecto al cliente y se libero su memoria y la de su pase, 0 si no se encontro.
   Llama a: buscarClientePorRut para ubicarlo, y a
   eliminarReservaCola (recorriendo las salas) con el objetivo de borrar cualquier reserva activa de ese cliente antes de hacerle free. */
int eliminarClienteYPase(struct Tienda *tienda, char *rut);

void listarClientesYPases(struct NodoCliente *head);

void listarClientesEnTienda(struct NodoCliente *head);

/*===============================================================================================================
 * Chhristopher
 =============================================================================================================== */

/* Retorna: 1 si se elimino la sala y se compacto el arreglo, 0 si no se encontro.
   Llama a: buscarSala para ubicar la sala, y a
   eliminarTituloABB en ciclo con el objetivo de borrar todos los comics de su arbol y liberar memoria antes de borrar la sala. */
int eliminarSala(struct Tienda *tienda, int codigo);

void reporteEstadisticasGenerales(struct Tienda *tienda);

/*Llama a: ordenarSalas con el objetivo de ordenar el arreglo de salas de mayor a menor ocupacion antes de mostrarlas. */
void reporteSalasMasOcupadas(struct Tienda *tienda);

/*===============================================================================================================
 * CLAUDIO
 =============================================================================================================== */

/*Llama a: Todas las funciones
anteriores segun la opcion elegida con el objetivo de permitir al usuario operar todo el sistema desde la consola. */
void menuPrincipal(struct Tienda *tienda);

int main(void);



