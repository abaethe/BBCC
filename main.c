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