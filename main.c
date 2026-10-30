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

/* Parte Benja Grez culo */

struct NodoTitulo* agregarTituloABB(struct NodoTitulo *raiz, struct Titulo *nuevoTitulo) {
    /* En caso que el arbol este vacío */
    if (raiz == NULL) {
        struct NodoTitulo *nuevoNodo = (struct NodoTitulo *)malloc(sizeof(struct NodoTitulo));

        /* Asignación de datos */
        nuevoNodo->datos = nuevoTitulo;
        nuevoNodo->izq = NULL;
        nuevoNodo->der = NULL;

        return nuevoNodo;
    }

    /* Caso en que arbol no este vacío */
    if (nuevoTitulo->codigo < raiz->datos->codigo) {
        /* Llama recursivamente por la izquierda y actualiza el puntero */
        raiz->izq = agregarTituloABB(raiz->izq, nuevoTitulo);
    }
    else if (nuevoTitulo->codigo > raiz->datos->codigo) {
        /* Llama recursivamente por la derecha y actualiza el puntero */
        raiz->der = agregarTituloABB(raiz->der, nuevoTitulo);
    }
    else {
        /* caso en que el título ya existe en el árbol */
        printf("El titulo con codigo %d ya existe en la sala.\n", nuevoTitulo->codigo);
    }

    return raiz;
}

struct Titulo* buscarTituloABB(struct NodoTitulo *raiz, int codigo) {
    /* Si arbol esta vacío o el titulo no existe */
    if (raiz == NULL) {
        return NULL;
    }

    /* Caso base donde se encuentra el titulo */
    if (codigo == raiz->datos->codigo) {
        return raiz->datos;
    }

    /* Si el código que busco es menor que el del nodo actual */
    if (codigo < raiz->datos->codigo) {
        return buscarTituloABB(raiz->izq, codigo);
    }
    /* Si el código que busco es mayor que el del nodo actual */
    else {
        return buscarTituloABB(raiz->der, codigo);
    }
}

int modificarTitulo(struct Titulo *titulo) {
    int opcion;
    int nuevoStock; 

    if (titulo == NULL) {
        return 0; /* En caso que puntero sea NULL */
    }

    printf("¿Qué desea modificar de %s?\n", titulo->nombre);
    printf("1. Sumar stock (ingreso de despacho)\n");
    printf("2. Cambiar estado a Descatalogado (TIT_DESC)\n");
    printf("Opcion: ");
    scanf("%d", &opcion);

    if (opcion == 1) {
        printf("Ingrese la cantidad de stock a sumar: ");
        scanf("%d", &nuevoStock);
        titulo->stock += nuevoStock;

        /* Proceso de entrega de titulos mientras haya stock */
        while (titulo->listaEspera->frente != NULL && titulo->stock > 0) {
            /* Para simplificar, si hay stock suficiente para la primera reserva, se la entregamos y lo sacamos */
            if (titulo->stock >= titulo->listaEspera->frente->datos->cantidad) {
                titulo->stock -= titulo->listaEspera->frente->datos->cantidad;
                printf("Entregando %d ejemplares de la reserva %d.\n", titulo->listaEspera->frente->datos->cantidad, titulo->listaEspera->frente->datos->numero);

                /* Se llama a eliminarReservaCola pasando la cola y el numero de la reserva del frente */
                eliminarReservaCola(titulo->listaEspera, titulo->listaEspera->frente->datos->numero);
            } else {
                 printf("No alcanza el stock para cubrir la primera reserva por completo.\n");
                 break; /* No hay más stock suficiente para el primero de la fila */
            }
        }

        /* Si el stock se volvió mayor a 0 y estaba agotado, lo ponemos disponible */
        if (titulo->stock > 0 && titulo->estado == TIT_AGOTADO) {
            titulo->estado = TIT_DISP;
        }

    } else if (opcion == 2) {
        titulo->estado = TIT_DESC;

        /* Se vacía toda la cola debido que see descatalogó */
        while (titulo->listaEspera->frente != NULL) {
             printf("Cancelando reserva %d porque el título se descatalogó.\n", titulo->listaEspera->frente->datos->numero);
             eliminarReservaCola(titulo->listaEspera, titulo->listaEspera->frente->datos->numero);
        }
    } else {
        printf("Opción inválida.\n");
    }

    return 1; /* Éxito al modificar */
}

/* Funcion auxiliar para eliminarTituloABB */
void reemplazar(struct NodoTitulo **abb, struct NodoTitulo **aux) {
    /* Si no tiene hijo derecho, significa que llegamos al más grande de la rama izquierda */
    if ((*abb)->der == NULL) {
        /* Copiamos el puntero de los datos del reemplazo al nodo original */
        (*aux)->datos = (*abb)->datos;

        /* Hacemos que 'aux' apunte al nodo de reemplazo y conectamos al 'padre' con el 'hijo' */
        *aux = *abb;
        *abb = (*abb)->izq;
    } else {
        /* Si tiene hijo derecho, sigue bajando hacia la derecha recursivamente */
        reemplazar(&(*abb)->der, aux);
    }
}

struct NodoTitulo* eliminarTituloABB(struct NodoTitulo *raiz, int codigo) {
    /* Caso base si el árbol está vacío o no encontramos el código */
    if (raiz == NULL) {
        return NULL;
    }

    /* Recorrido recursivo buscando el código */
    if (codigo < raiz->datos->codigo) {
        raiz->izq = eliminarTituloABB(raiz->izq, codigo);
    } else if (codigo > raiz->datos->codigo) {
        raiz->der = eliminarTituloABB(raiz->der, codigo);
    }
    /* Se encontró el código */
    else {
        struct NodoTitulo *aux = raiz; /* Guardamos el nodo a borrar en aux */
        int numReserva; 

        /* VACIAR LA COLA DE RESERVAS */
        /* Llamamos a eliminarReservaCola en un ciclo hasta que la fila quede vacía */
        if (raiz->datos->listaEspera != NULL) {
            while (raiz->datos->listaEspera->frente != NULL) {
                numReserva = raiz->datos->listaEspera->frente->datos->numero;
                printf("Eliminando reserva %d (el titulo sera borrado del sistema).\n", numReserva);
                eliminarReservaCola(raiz->datos->listaEspera, numReserva);
            }
            /* Liberamos también el struct de la cola en sí */
            free(raiz->datos->listaEspera);
        }

        /* REESTRUCTURACIÓN DEL ÁRBOL Y LIBERACIÓN DE MEMORIA */
        /* Caso donde no tiene 'hijo' izquierdo  */
        if (raiz->izq == NULL) {
            raiz = raiz->der;
            free(aux->datos); /* Liberamos el struct Titulo */
            free(aux);        /* Liberamos el Nodo del árbol */
        }
        /* Caso donde no tiene 'hijo' derecho */
        else if (raiz->der == NULL) {
            raiz = raiz->izq;
            free(aux->datos); /* Liberamos el struct Titulo */
            free(aux);        /* Liberamos el Nodo del árbol */
        }
        /* Caso donde tiene ambos 'hijos' */
        else {
            /* Como vamos a sobrescribir los datos con el reemplazo, */
            /* liberamos la memoria del Titulo original ahora */
            free(aux->datos);

            /* Llamamos a reemplazar pasándole la dirección de memoria de su 'hijo' izquierdo */
            reemplazar(&(raiz->izq), &aux);

            /* Liberamos el nodo de reemplazo que quedó vacío en aux */
            free(aux);
        }
    }

    /* Retornamos la raíz actualizada para que las recursiones no se rompan */
    return raiz;
}

void listarTitulosABB(struct NodoTitulo *raiz) {
    /* Caso base donde el nodo no es nulo */
    if (raiz != NULL) {

        listarTitulosABB(raiz->izq);

        printf("Codigo: %d | Titulo: %s | Precio: $%ld | Stock: %d | Estado: %d\n",
               raiz->datos->codigo,
               raiz->datos->nombre,
               raiz->datos->precio,
               raiz->datos->stock,
               raiz->datos->estado);

        listarTitulosABB(raiz->der);
    }
}

int calcularTiempoEsperaTitulo(struct Titulo *titulo) {
    int despachosNecesarios; 
    int diasEspera;

    if (titulo == NULL || titulo->listaEspera == NULL || titulo->listaEspera->frente == NULL) {
        return -1;
    }

    if (titulo->ejemplaresPorDespacho <= 0 || titulo->diasEntreDespachos <= 0) {
        return -1;
    }

    despachosNecesarios = titulo->listaEspera->totalEjemplares / titulo->ejemplaresPorDespacho;

    if (titulo->listaEspera->totalEjemplares % titulo->ejemplaresPorDespacho != 0) {
        despachosNecesarios++;
    }

    diasEspera = despachosNecesarios * titulo->diasEntreDespachos;

    return diasEspera;
}

/* Función Auxiliar para reporteTituloMasEsperado */
void buscarMasEsperadoABB(struct NodoTitulo *nodo, struct Titulo **mejor, int *maxTiempo) {
    if (nodo != NULL) {
        int tiempoActual; 
        
        buscarMasEsperadoABB(nodo->izq, mejor, maxTiempo);

        /* Se el tiempo usando la función que se hizo anteriormente */
        tiempoActual = calcularTiempoEsperaTitulo(nodo->datos);
        if (tiempoActual > *maxTiempo) {
            *maxTiempo = tiempoActual;
            *mejor = nodo->datos;
        }

        buscarMasEsperadoABB(nodo->der, mejor, maxTiempo);
    }
}

void reporteTituloMasEsperado(struct Tienda *tienda) {
    struct Titulo *masEsperado = NULL; 
    int maxTiempo = -1;
    int i;

    if (tienda == NULL) return;

    for (i = 0; i < tienda->pLibreSalas; i++) {
        if (tienda->salas[i] != NULL) {
            buscarMasEsperadoABB(tienda->salas[i]->raizTitulos, &masEsperado, &maxTiempo);
        }
    }

    if (masEsperado != NULL && maxTiempo >= 0) {
        printf("El titulo mas esperado es: '%s' con %d dias estimados de espera.\n", masEsperado->nombre, maxTiempo);
    } else {
        printf("Actualmente no hay titulos con lista de espera en la tienda.\n");
    }
}

/* Función Auxiliar para reporteTituloMasSolicitado */
void buscarMasSolicitadoABB(struct NodoTitulo *nodo, struct Titulo **mejor, int *maxSol) {
    if (nodo != NULL) {
        buscarMasSolicitadoABB(nodo->izq, mejor, maxSol);

        if (nodo->datos->solicitudesDia > *maxSol) {
            *maxSol = nodo->datos->solicitudesDia;
            *mejor = nodo->datos;
        }

        buscarMasSolicitadoABB(nodo->der, mejor, maxSol);
    }
}

void reporteTituloMasSolicitado(struct Tienda *tienda) {
    struct Titulo *masSolicitado = NULL; 
    int maxSol = -1;
    int i;

    if (tienda == NULL) return;

    for (i = 0; i < tienda->pLibreSalas; i++) {
        if (tienda->salas[i] != NULL) {
            buscarMasSolicitadoABB(tienda->salas[i]->raizTitulos, &masSolicitado, &maxSol);
        }
    }

    if (masSolicitado != NULL && maxSol > 0) {
        printf("El titulo mas solicitado es: '%s' con %d solicitudes.\n", masSolicitado->nombre, maxSol);
    } else {
        printf("Aun no hay solicitudes registradas para ningun titulo.\n");
    }
}

/* Función Auxiliar para listarTitulosAgotados */
void imprimirAgotadosABB(struct NodoTitulo *nodo, int *encontrados) {
    if (nodo != NULL) {
        imprimirAgotadosABB(nodo->izq, encontrados);

        if (nodo->datos->estado == TIT_AGOTADO) {
            printf("- [%d] %s (Dias agotado: %d)\n", nodo->datos->codigo, nodo->datos->nombre, nodo->datos->diasAgotado);
            (*encontrados)++;
        }

        imprimirAgotadosABB(nodo->der, encontrados);
    }
}

void listarTitulosAgotados(struct Tienda *tienda) {
    int encontrados = 0; 
    int i;

    if (tienda == NULL) return;

    printf("\n--- REPORTE DE TITULOS AGOTADOS ---\n");
    for (i = 0; i < tienda->pLibreSalas; i++) {
        if (tienda->salas[i] != NULL) {
            imprimirAgotadosABB(tienda->salas[i]->raizTitulos, &encontrados);
        }
    }

    if (encontrados == 0) {
        printf("¡Excelente! No hay ningun titulo agotado en la tienda.\n");
    }
}
