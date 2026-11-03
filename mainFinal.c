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

/*esta funcion aplica fifo primero que entra primero que sale*/
int agregarReservaCola(struct Titulo *titulo, struct Cliente *cliente, int cantidad, int numReserva, int hora) {
    struct Reserva *nuevaReserva;
    struct NodoReserva *nuevoNodo;

    /*verificar que no sea null */
    if (titulo == NULL || cliente == NULL) {
      return 0;
    }
    if (titulo->estado==TIT_DESC) { /*que el titulo si este en el catalogo*/
        return 0;
    }
    if (cantidad <= 0) {
        return 0;
    }
    if (titulo->limitePorCliente > 0 && cantidad > titulo->limitePorCliente) {
        return 0;
    }
    if (cliente->edad < titulo->edadMinima) { /*validando edad del cliente*/
        return 0;
    }

    /*si la lista de espera no se existe se crea*/
    if (titulo->listaEspera == NULL) {
        titulo->listaEspera = (struct ColaReservas *)malloc(sizeof(struct ColaReservas));

        titulo->listaEspera->frente = NULL;
        titulo->listaEspera->final = NULL;
        titulo->listaEspera->cantidadClientes = 0;
        titulo->listaEspera->totalEjemplares = 0;
    }

    /*reserva de memoria*/
    nuevaReserva = (struct Reserva *)malloc(sizeof(struct Reserva));

    /*datos del struct Reserva para la nueva reserva*/
    nuevaReserva->numero = numReserva;
    nuevaReserva->cliente = cliente;
    nuevaReserva->cantidad = cantidad;
    nuevaReserva->horaRegistro = hora;

    nuevoNodo = (struct NodoReserva *)malloc(sizeof(struct NodoReserva)); /*reserva de memoria */

    /*enlace struct NodoReserva*/
    nuevoNodo->datos = nuevaReserva;
    nuevoNodo->sig = NULL;

    /*FIFO:el primero que entra el primero que sale*/
    if (titulo->listaEspera->frente == NULL) {
        titulo->listaEspera->frente = nuevoNodo;
        titulo->listaEspera->final = nuevoNodo;
    } else {
        titulo->listaEspera->final->sig = nuevoNodo;
        titulo->listaEspera->final = nuevoNodo;
    }

    /* actualiza los datos*/
    titulo->listaEspera->cantidadClientes++;
    titulo->listaEspera->totalEjemplares += cantidad;
    titulo->solicitudesDia++;

    return 1;
}

struct Reserva* buscarReservaCola(struct ColaReservas *cola, int numeroReserva) {
    struct NodoReserva *actual;

    if (cola == NULL || cola->frente == NULL) { /*verificar que la cola exista con elementos*/
        return NULL;
    }
    actual = cola->frente;

    /*recorrer la fila buscando el numero de reserva*/
    while (actual != NULL) {
        if (actual->datos != NULL && actual->datos->numero == numeroReserva) {
            return actual->datos; /*Retorna el "struct Reserva" encontrado*/
        }
        actual = actual->sig;
    }

    /*si no encuentra nada retorna null*/
    return NULL;
}


int modificarReservaCola(struct ColaReservas *cola, int numeroReserva, int nuevaCantidad) {
    struct Reserva *reservaEncontrada;

    if (cola == NULL || nuevaCantidad <= 0) {
        return 0;
    }

    reservaEncontrada = buscarReservaCola(cola, numeroReserva); /*buscar la reserva*/

    if (reservaEncontrada == NULL) { /*no existe reserva*/
        return 0;
    }

    /*ajustar el total de ejemplares de la cola*/
    cola->totalEjemplares = cola->totalEjemplares - reservaEncontrada->cantidad + nuevaCantidad;

    reservaEncontrada->cantidad = nuevaCantidad; /*actualizar la cantidad en la reserva*/

    return 1;
}

/*desengachar el nodo de reserva  */
int eliminarReservaCola(struct ColaReservas *cola, int numeroReserva) {
    struct NodoReserva *actual;
    struct NodoReserva *anterior;

    /*ve que la cola exista y no este vacia */
    if (cola == NULL || cola->frente == NULL) {
        return 0;
    }

    actual = cola->frente;
    anterior = NULL;

    /*buscar el nodo que tiene el numeroReserva */
    while (actual != NULL && actual->datos->numero != numeroReserva) {
        anterior = actual;
        actual = actual->sig;
    }

    /* Si actual llego a NULL el numeroReserva no estaba en la fila */
    if (actual == NULL) {
        return 0;
    }

    /*desengachar el nodo segun donde este */
    if (actual == cola->frente) {
        cola->frente = actual->sig; /*si es el primero de la fila el frente avanza */
    } else {
        anterior->sig = actual->sig; /* si esta al medio o al final el anterior se salta al actual */
    }

    if (actual == cola->final) {
        cola->final = anterior; /*si era el ultimo o unico el final retrocede*/
    }

    /*antualizar los contadores */
    cola->cantidadClientes--;
    cola->totalEjemplares -= actual->datos->cantidad;

    /* liberar memoria*/
    free(actual->datos);
    free(actual);

    return 1;
}

/* Recorre e imprime todas las reservas de la lista de espera en orden de llegada */
void listarReservasCola(struct ColaReservas *cola) {
    struct NodoReserva *actual;
    int turno;

    if (cola == NULL || cola->frente == NULL) {
        printf("\nLa lista de espera esta vacia.\n");
        return;
    }

    printf("\n--- LISTA DE ESPERA ---\n");
    printf("Clientes en fila: %d | Total ejemplares reservados: %d\n", cola->cantidadClientes, cola->totalEjemplares);
    printf("------------------------------------------------------------\n");

    actual = cola->frente;
    turno = 1;

    while (actual != NULL) {
        if (actual->datos != NULL && actual->datos->cliente != NULL) {
            printf("Turno %d | Reserva #%d | Cliente: %s (RUT: %s) | Cantidad: %d | Hora: %04d\n",
                   turno,
                   actual->datos->numero,
                   actual->datos->cliente->nombre,
                   actual->datos->cliente->rut,
                   actual->datos->cantidad,
                   actual->datos->horaRegistro);
        }
        actual = actual->sig;
        turno++;
    }
    printf("------------------------------------------------------------\n");
}


int buscarSala(struct Tienda *tienda, int codigoBuscado){
   int i;
   if(tienda == NULL){
      return -1;
   }
   for(i = 0;i<tienda->pLibreSalas;i++){
      if(tienda->salas[i]->codigo == codigoBuscado){
         return i;
      }
   }
   return -1;
}

void ordenarSalas(struct Tienda *tienda, int criterio){
   int i,j;
   int pos;
   struct Sala *aux;
   if(tienda == NULL){
      return;
   }
   for(i = 0; i<tienda->pLibreSalas-1;i++){
      pos = i;
      for(j = i + 1;j<tienda->pLibreSalas;j++){
         if(criterio == 2){
            if(tienda->salas[j]->ocupacionActual > tienda->salas[pos]->ocupacionActual){
               pos = j;
            }

         }
         else{
            if(tienda->salas[j]->codigo < tienda->salas[pos]->codigo){
               pos = j;
            }
         }

      }
      if(pos != i){
         aux = tienda->salas[i];
         tienda->salas[i] = tienda->salas[pos];
         tienda->salas[pos] = aux;
      }
   }

}


int agregarSala(struct Tienda *tienda, struct Sala *nuevaSala){
   if (tienda == NULL || nuevaSala == NULL){
      return 0;
   }
   if(tienda->pLibreSalas >= MAX_SALAS){
      printf("No hay espacio para mas salas.\n");
      return 0;
   }
   if(buscarSala(tienda,nuevaSala->codigo)!= -1){
      printf("Ya existe una sala con ese codigo \n");
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
   /*aqui se definen la cantidad de datos que hay dentro de la sala nueva para que despues se vayan rellenando*/
   nuevaSala->ocupacionActual = 0;
   nuevaSala->cantidadTitulos = 0;
   nuevaSala->raizTitulos = NULL; /*Esta es el arbol*/

   /*aqui se guarda en la primera poscion libre que seria la que sigue, y pLibre se agrega una para contar la nueva sala que agregamos*/
   tienda->salas[tienda->pLibreSalas] = nuevaSala;
   tienda->pLibreSalas++;
   return 1;
}


int modificarSala(struct Tienda *tienda, int codigo){
   int pos;
   int opcion;
   int nuevaApertura,nuevoCierre,nuevoAforo;
   struct Sala *s;
   if(tienda ==NULL){
      return 0;
   }
   pos= buscarSala(tienda,codigo);
   if(pos == -1 ){
      printf("No existe una sala con ese codigo");
      return 0;
   }
   s = tienda->salas[pos];
   do{
      printf("-------------------------------------\n");
      printf("Selecciona lo que quieres modificar\n");
      printf("Nombre = 1\n");
      printf("Tematica = 2\n");
      printf("Encargado = 3 \n");
      printf("horario = 4\n");
      printf("Aforo = 5\n");
      printf("-------------------------------------\n");
      printf("Para salir = 0\n");
      scanf("%d",&opcion);
      if (opcion == 1){
         printf("Nuevo nombre : ");
         scanf("%s",s->nombre);
      }
      else if (opcion == 2 ){
         printf("Nueva Tematica : ");
         scanf("%s",s->tematica);
      }
      else if (opcion == 3 ){
         printf("Nuevo encargado : ");
         scanf("%s",s->encargado);
      }
      else if (opcion == 4 ){
         printf("Nueva hora de apertura (HHMM) : ");
         scanf("%d",&nuevaApertura);
         printf("Nueva hora de cierre (HHMM) : ");
         scanf("%d",&nuevoCierre);

         if(nuevaApertura < 0 || nuevaApertura > 2359 || nuevaApertura % 100 >= 60){
            printf("Hora de apertura invalida, no se modifico el horario.\n");
         }
         else if(nuevoCierre < 0 || nuevoCierre >2359 || nuevoCierre %100 >=60){
            printf("Hora de cierre invalida, no se modifico el horario\n");
         }
         else if(nuevaApertura >=nuevoCierre){
            printf("LA apertura debe ser anterior al cierre , no se modifico el horario \n");
         }
         else{
            s->horaApertura = nuevaApertura;
            s->horaCierre = nuevoCierre;
            printf("Horario Actualizado \n");
         }

      }
      else if(opcion == 5){
         printf("Nuevo aforo maximo : ");
         scanf("%d",&nuevoAforo);
         if (nuevoAforo <= 0 ){
            printf("El aforo debe ser mayor a 0 por lo que no se modifico\n");
         }
         else if (nuevoAforo < s->ocupacionActual){
            printf("Hay %d personas en la sala, el aforo no puede ser menor\n",s->ocupacionActual);
         }
         else{
            s->aforoMaximo = nuevoAforo;
            printf("Aforo Actualizado \n");
         }
      }
      else if(opcion != 0){
         printf("Opcion invalida \n");
      }

   } while(opcion != 0);
   return 1;
}

void listarSalas(struct Tienda *tienda){
   int i;
   if(tienda ==NULL){
      return;
   }
   if(tienda->pLibreSalas == 0){
      printf("No hay salas registradas en la tienda\n");
      return;
   }

   for(i = 0; i<tienda->pLibreSalas;i++){
      printf("---------------Sala n_%d---------------\n",i+1);
      printf("Codigo : %d\n",tienda->salas[i]->codigo);
      printf("Nombre : %s\n",tienda->salas[i]->nombre);
      printf("Tematica : %s\n",tienda->salas[i]->tematica);
      printf("Encargado : %s\n",tienda->salas[i]->encargado);
      printf("Horario desde/hasta : %02d:%02d - %02d:%02d\n",tienda->salas[i]->horaApertura /100,tienda->salas[i]->horaApertura %100,tienda->salas[i]->horaCierre / 100, tienda->salas[i]->horaCierre %100);
      printf("Aforo Maximo de personas : %d\n",tienda->salas[i]->aforoMaximo);
      printf("Ocupacion : %d\n",tienda->salas[i]->ocupacionActual);
      printf("Cantidad de titulos : %d\n",tienda->salas[i]->cantidadTitulos);
      printf("------------------------------\n");
   }

}


struct NodoTitulo* agregarTituloABB(struct NodoTitulo *raiz, struct Titulo *nuevoTitulo) {
    struct NodoTitulo *nuevoNodo; /* nuevo: variable declarada al inicio de la funcion por ANSI C */

    /* En caso que el arbol este vacío */
    if (raiz == NULL) {
        nuevoNodo = (struct NodoTitulo *)malloc(sizeof(struct NodoTitulo)); /*asdoibaskdjbasjdasd*/

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
    }else if (nuevoTitulo->codigo > raiz->datos->codigo) {
        /* Llama recursivamente por la derecha y actualiza el puntero */
        raiz->der = agregarTituloABB(raiz->der, nuevoTitulo);
    }else {
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
    int opcion = 0;
    int nuevoStock = 0;

    if (titulo == NULL) {
        return 0; /*En caso que puntero sea NULL*/
    }

    printf("¿Qué desea modificar de %s?\n", titulo->nombre);
    printf("1. Sumar stock (ingreso de despacho)\n");
    printf("2. Cambiar estado a Descatalogado (TIT_DESC)\n");
    printf("Opcion: ");
    scanf("%d", &opcion);

    if (opcion == 1) {
        if (titulo->estado == TIT_DESC) { /*por si el titulo estaba descatalogado*/
            printf("No se puede sumar stock porque el titulo esta descatalogado.\n");
            return 0;
        }
        printf("Ingrese la cantidad de stock a sumar: ");
        scanf("%d", &nuevoStock);
        if (nuevoStock <= 0) {
            printf("La cantidad de stock a sumar debe ser mayor a 0.\n");
            return 0;
        }
        titulo->stock += nuevoStock;

        /*Proceso de entrega de titulos mientras haya stock*/
        if (titulo->listaEspera != NULL) {
            while (titulo->listaEspera->frente != NULL && titulo->stock > 0) {
                /*Para simplificar, si hay stock suficiente para la primera reserva, se la entregamos y lo sacamos*/
                if (titulo->stock >= titulo->listaEspera->frente->datos->cantidad) {
                    titulo->stock -= titulo->listaEspera->frente->datos->cantidad;
                    printf("Entregando %d ejemplares de la reserva %d.\n", titulo->listaEspera->frente->datos->cantidad, titulo->listaEspera->frente->datos->numero);

                    /*Se llama a eliminarReservaCola pasando la cola y el numero de la reserva del frente*/
                    eliminarReservaCola(titulo->listaEspera, titulo->listaEspera->frente->datos->numero);
                } else {
                    printf("No alcanza el stock para cubrir la primera reserva por completo.\n");
                    break; /* No hay más stock suficiente para el primero de la fila */
                }
            }
        }

        /*Solo pasa a disponible si sobra stock y NO queda nadie esperando en la fila*/
        if (titulo->stock > 0 && (titulo->listaEspera == NULL || titulo->listaEspera->frente == NULL)) {
            if (titulo->estado == TIT_AGOTADO || titulo->estado == TIT_PREVENTA) {
                titulo->estado = TIT_DISP;
                titulo->diasAgotado = 0;
            }
        } else {
            /*Si el stock quedo en 0 o fue insuficiente para cubrir la primera reserva pendiente*/
            if (titulo->estado == TIT_DISP || titulo->estado == TIT_PREVENTA) {
                titulo->estado = TIT_AGOTADO;
            }
        }

    } else if (opcion == 2) {
        titulo->estado = TIT_DESC;

        /*Se vacía toda la cola debido que see descatalogó*/
        if (titulo->listaEspera != NULL) {
            while (titulo->listaEspera->frente != NULL) {
                printf("Cancelando reserva %d porque el título se descatalogó.\n", titulo->listaEspera->frente->datos->numero);
                eliminarReservaCola(titulo->listaEspera, titulo->listaEspera->frente->datos->numero);
            }
        }
    } else {
        printf("Opción inválida.\n");
        return 0;
    }

    return 1;
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
    struct NodoTitulo *aux;
    int numReserva;

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
        aux = raiz; /* Guardamos el nodo a borrar en aux */ /* nuevo: se deja solo la asignacion */

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
    int tiempoActual;
    if (nodo != NULL) {


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


struct Pase* crearPase(int codigo, int tipo, long valor) {
    struct Pase *nuevo;

    nuevo = (struct Pase *)malloc(sizeof(struct Pase));
    if (nuevo == NULL) {
        return NULL;
    }

    nuevo->codigo = codigo;
    nuevo->tipo = tipo;
    nuevo->valor = valor;
    nuevo->estado = PASE_ACTIVO;

    return nuevo;
}

struct Cliente* buscarClientePorRut(struct NodoCliente *head, char *rut) {
    struct NodoCliente *rec;

    rec = head;
    while (rec != NULL) {
        if (strcmp(rec->datos->rut, rut) == 0) {
            return rec->datos;
        }
        rec = rec->sig;
    }
    return NULL;
}

int agregarCliente(struct Tienda *tienda, char *rut, char *nombre, int edad, int tipoPase, long valorPase) {
    struct Cliente *nuevoCliente;
    struct NodoCliente *nuevoNodo, *rec;

    if (tienda == NULL || rut == NULL || nombre == NULL) {
        return 0;
    }
    if (tipoPase < PASE_GEN || tipoPase > PASE_PRIO || valorPase < 0 || edad < 0) {
        return 0;
    }
    if (buscarClientePorRut(tienda->headClientes, rut) != NULL) {
        return 0;
    }

    nuevoCliente = (struct Cliente *)malloc(sizeof(struct Cliente));
    if (nuevoCliente == NULL) {
        return 0;
    }

    nuevoNodo = (struct NodoCliente *)malloc(sizeof(struct NodoCliente));
    if (nuevoNodo == NULL) {
        free(nuevoCliente);
        return 0;
    }

    nuevoCliente->pase = crearPase(tienda->siguienteCodigoPase, tipoPase, valorPase);
    if (nuevoCliente->pase == NULL) {
        free(nuevoCliente);
        free(nuevoNodo);
        return 0;
    }

    strncpy(nuevoCliente->rut, rut, MAX_TXT - 1);
    nuevoCliente->rut[MAX_TXT - 1] = '\0';
    strncpy(nuevoCliente->nombre, nombre, MAX_TXT - 1);
    nuevoCliente->nombre[MAX_TXT - 1] = '\0';
    nuevoCliente->edad = edad;
    nuevoCliente->dentroTienda = 0;
    nuevoCliente->codigoSalaActual = 0;
    nuevoCliente->horaIngreso = 0;
    nuevoCliente->horaSalida = 0;

    nuevoNodo->datos = nuevoCliente;
    nuevoNodo->sig = NULL;
    nuevoNodo->ant = NULL;

    if (tienda->headClientes == NULL) {
        tienda->headClientes = nuevoNodo;
    } else {
        rec = tienda->headClientes;
        while (rec->sig != NULL) {
            rec = rec->sig;
        }
        rec->sig = nuevoNodo;
        nuevoNodo->ant = rec;
    }

    tienda->siguienteCodigoPase++;
    tienda->pasesVendidos++;
    tienda->recaudacionPases += valorPase;

    return 1;
}

int modificarClienteYPase(struct Tienda *tienda, char *rut) {
    struct Cliente *cliente;
    struct Sala *salaNueva, *salaActual;
    int opcion, hora, codigoSala, nuevoEstado, posSala;

    if (tienda == NULL || rut == NULL) {
        return 0;
    }

    cliente = buscarClientePorRut(tienda->headClientes, rut);
    if (cliente == NULL) {
        printf("No existe un cliente con RUT %s\n", rut);
        return 0;
    }

    printf("\n--- Modificar cliente %s (%s) ---\n", cliente->nombre, cliente->rut);
    printf("1. Modificar nombre y edad\n");
    printf("2. Registrar ingreso a la tienda\n");
    printf("3. Cambiar de sala\n");
    printf("4. Registrar salida de la tienda\n");
    printf("5. Cambiar estado del pase\n");
    printf("Opcion: ");
    if (scanf("%d", &opcion) != 1) {
        return 0;
    }

    switch (opcion) {
        case 1:
            printf("Nuevo nombre: ");
            scanf(" %59[^\n]", cliente->nombre);
            printf("Nueva edad: ");
            scanf("%d", &cliente->edad);
            return 1;

        case 2:
            if (cliente->dentroTienda == 1) {
                printf("El cliente ya esta dentro de la tienda.\n");
                return 0;
            }
            if (cliente->pase->estado != PASE_ACTIVO) {
                printf("El pase no esta activo, no puede ingresar.\n");
                return 0;
            }
            if (tienda->clientesDentro >= tienda->aforoMaximo) {
                printf("La tienda alcanzo su aforo maximo.\n");
                return 0;
            }
            printf("Hora de ingreso (HHMM): ");
            scanf("%d", &hora);
            printf("Codigo de la sala a la que entra: ");
            scanf("%d", &codigoSala);

            posSala = buscarSala(tienda, codigoSala);
            if (posSala == -1) {
                printf("La sala no existe.\n");
                return 0;
            }
            salaNueva = tienda->salas[posSala];

            if (salaNueva->ocupacionActual >= salaNueva->aforoMaximo) {
                printf("La sala %s esta llena.\n", salaNueva->nombre);
                return 0;
            }
            if (hora < salaNueva->horaApertura || hora > salaNueva->horaCierre) {
                printf("La sala %s esta cerrada a esa hora.\n", salaNueva->nombre);
                return 0;
            }

            salaNueva->ocupacionActual++;
            cliente->codigoSalaActual = codigoSala;
            cliente->dentroTienda = 1;
            cliente->horaIngreso = hora;
            cliente->horaSalida = 0;
            cliente->pase->estado = PASE_USADO;

            tienda->clientesDentro++;
            tienda->ingresosDia++;
            tienda->pasesUtilizados++;
            return 1;

        case 3:
            if (cliente->dentroTienda == 0) {
                printf("El cliente no esta dentro de la tienda.\n");
                return 0;
            }
            printf("Codigo de la nueva sala: ");
            scanf("%d", &codigoSala);
            if (codigoSala == cliente->codigoSalaActual) {
                printf("El cliente ya esta en esa sala.\n");
                return 0;
            }

            posSala = buscarSala(tienda, codigoSala);
            if (posSala == -1) {
                printf("La sala no existe.\n");
                return 0;
            }
            salaNueva = tienda->salas[posSala];

            if (salaNueva->ocupacionActual >= salaNueva->aforoMaximo) {
                printf("La sala %s esta llena.\n", salaNueva->nombre);
                return 0;
            }

            posSala = buscarSala(tienda, cliente->codigoSalaActual);
            if (posSala != -1) {
                salaActual = tienda->salas[posSala];
                if (salaActual != NULL && salaActual->ocupacionActual > 0) {
                    salaActual->ocupacionActual--;
                }
            }
            salaNueva->ocupacionActual++;
            cliente->codigoSalaActual = codigoSala;
            return 1;

        case 4:
            if (cliente->dentroTienda == 0) {
                printf("El cliente no esta dentro de la tienda.\n");
                return 0;
            }
            printf("Hora de salida (HHMM): ");
            scanf("%d", &hora);
            if (hora < cliente->horaIngreso) {
                printf("La hora de salida no puede ser anterior al ingreso.\n");
                return 0;
            }

            posSala = buscarSala(tienda, cliente->codigoSalaActual);
            if (posSala != -1) {
                salaActual = tienda->salas[posSala];
                if (salaActual != NULL && salaActual->ocupacionActual > 0) {
                    salaActual->ocupacionActual--;
                }
            }
            cliente->codigoSalaActual = 0;
            cliente->dentroTienda = 0;
            cliente->horaSalida = hora;
            if (tienda->clientesDentro > 0) {
                tienda->clientesDentro--;
            }
            return 1;

        case 5:
            printf("Nuevo estado (1: Activo, 2: Usado, 3: Anulado, 4: Vencido): ");
            scanf("%d", &nuevoEstado);
            if (nuevoEstado < PASE_ACTIVO || nuevoEstado > PASE_VENCIDO) {
                printf("Estado invalido.\n");
                return 0;
            }
            cliente->pase->estado = nuevoEstado;
            return 1;

        default:
            printf("Opcion invalida.\n");
            return 0;
    }
}

int eliminarClienteYPase(struct Tienda *tienda, char *rut) {
    struct Cliente *cliente;
    struct NodoCliente *rec;
    struct Sala *salaActual;
    struct NodoTitulo *pila[200];
    struct NodoTitulo *nodo;
    struct NodoReserva *resActual, *resSig;
    int i, tope, posSala;

    if (tienda == NULL || rut == NULL) {
        return 0;
    }

    cliente = buscarClientePorRut(tienda->headClientes, rut);
    if (cliente == NULL) {
        return 0;
    }

    if (cliente->dentroTienda == 1) {
        posSala = buscarSala(tienda, cliente->codigoSalaActual);
        if (posSala != -1) {
            salaActual = tienda->salas[posSala];
            if (salaActual != NULL && salaActual->ocupacionActual > 0) {
                salaActual->ocupacionActual--;
            }
        }
        if (tienda->clientesDentro > 0) {
            tienda->clientesDentro--;
        }
    }

    for (i = 0; i < tienda->pLibreSalas; i++) {
        if (tienda->salas[i] == NULL || tienda->salas[i]->raizTitulos == NULL) {
            continue;
        }
        tope = 0;
        pila[tope++] = tienda->salas[i]->raizTitulos;
        while (tope > 0) {
            nodo = pila[--tope];
            if (nodo->datos != NULL && nodo->datos->listaEspera != NULL) {
                resActual = nodo->datos->listaEspera->frente;
                while (resActual != NULL) {
                    resSig = resActual->sig;
                    if (resActual->datos != NULL && resActual->datos->cliente == cliente) {
                        eliminarReservaCola(nodo->datos->listaEspera, resActual->datos->numero);
                    }
                    resActual = resSig;
                }
            }
            if (nodo->izq != NULL && tope < 200) {
                pila[tope++] = nodo->izq;
            }
            if (nodo->der != NULL && tope < 200) {
                pila[tope++] = nodo->der;
            }
        }
    }

    rec = tienda->headClientes;
    while (rec != NULL && rec->datos != cliente) {
        rec = rec->sig;
    }
    if (rec == NULL) {
        return 0;
    }

    if (rec->ant != NULL) {
        rec->ant->sig = rec->sig;
    } else {
        tienda->headClientes = rec->sig;
    }
    if (rec->sig != NULL) {
        rec->sig->ant = rec->ant;
    }

    free(cliente->pase);
    free(cliente);
    free(rec);

    return 1;
}

void listarClientesYPases(struct NodoCliente *head) {
    struct NodoCliente *rec;
    struct Cliente *c;
    char *tipos[] = {"Desconocido", "General", "Infantil", "Familiar", "Prioritario"};
    char *estados[] = {"Desconocido", "Activo", "Usado", "Anulado", "Vencido"};
    int t, e;

    if (head == NULL) {
        printf("No hay clientes registrados.\n");
        return;
    }

    printf("\n===== CLIENTES Y PASES =====\n");
    rec = head;
    while (rec != NULL) {
        c = rec->datos;
        printf("RUT: %s | Nombre: %s | Edad: %d | %s\n",
               c->rut, c->nombre, c->edad,
               c->dentroTienda ? "Dentro de la tienda" : "Fuera de la tienda");
        if (c->pase != NULL) {
            t = (c->pase->tipo >= PASE_GEN && c->pase->tipo <= PASE_PRIO) ? c->pase->tipo : 0;
            e = (c->pase->estado >= PASE_ACTIVO && c->pase->estado <= PASE_VENCIDO) ? c->pase->estado : 0;
            printf("   Pase #%d | Tipo: %s | Valor: $%ld | Estado: %s\n",
                   c->pase->codigo, tipos[t], c->pase->valor, estados[e]);
        }
        rec = rec->sig;
    }
}

void listarClientesEnTienda(struct NodoCliente *head) {
    struct NodoCliente *rec;
    struct Cliente *c;
    int cantidad = 0;

    printf("\n===== CLIENTES DENTRO DE LA TIENDA =====\n");
    rec = head;
    while (rec != NULL) {
        c = rec->datos;
        if (c->dentroTienda == 1) {
            printf("RUT: %s | Nombre: %s | Sala: %d | Ingreso: %04d\n",
                   c->rut, c->nombre, c->codigoSalaActual, c->horaIngreso);
            cantidad++;
        }
        rec = rec->sig;
    }

    if (cantidad == 0) {
        printf("No hay clientes dentro de la tienda.\n");
    } else {
        printf("Total: %d cliente(s)\n", cantidad);
    }
}

int eliminarSala(struct Tienda *tienda, int codigo){
   int pos;
   int i;
   struct Sala *s;

   if(tienda == NULL){
      return 0;
   }
   pos = buscarSala(tienda,codigo);
   if(pos == -1 ){
      printf("No existe una sala con ese codigo\n");
      return 0;
   }
   s = tienda->salas[pos];
   if(s->ocupacionActual > 0){
      printf("hay clientes en la sala\n");
      return 0;
   }
   /*mientras haya arbol, borra el titulo que estsa en la raiz y quedate con la nueva raiz*/
   while(s->raizTitulos !=NULL){

      s->raizTitulos = eliminarTituloABB(s->raizTitulos,s->raizTitulos->datos->codigo);
   }
   free(s);
   for(i = pos ; i<tienda->pLibreSalas -1 ; i++){
      tienda->salas[i] = tienda->salas[i+1];
   }
   tienda->pLibreSalas--;
   tienda->salas[tienda->pLibreSalas] = NULL;

   return 1;
}

void reporteEstadisticasGenerales(struct Tienda *tienda){
   int porcentaje;
   if(tienda == NULL){
      return;
   }
   printf("================ESTADISTICAS GENERALES================\n");
   printf("Tienda : %s / Ciudad de la tienda : %s\n",tienda->nombre,tienda->ciudad);

   printf("Hoy Ingresaron %d Clientes\n",tienda->ingresosDia);
   printf("Ahora hay %d Clientes dentro\n",tienda->clientesDentro);
   printf("El aforo maximo de la tienda es de %d persona(s)\n",tienda->aforoMaximo);
   printf("Se vendieron un total de %d pases\n",tienda->pasesVendidos);
   printf("Se utilizaron un total de %d pases\n",tienda->pasesUtilizados);
   printf("Se recaudo un total de %ld pesos\n",tienda->recaudacionPases);
   printf("Hay un total de %d sala(s)\n",tienda->pLibreSalas);

   /*Paso opcional:Mostrar el porcentaje de ocupacion de la tienda*/
   if(tienda->aforoMaximo > 0){
      porcentaje = tienda->clientesDentro * 100 / tienda->aforoMaximo;
      printf("Ocupacion de la tienda : %d%%\n",porcentaje);
   }
   printf("======================================================\n");
}

void reporteSalasMasOcupadas(struct Tienda *tienda){
   int i;
   int porcentaje;
   struct Sala *s;
   if(tienda == NULL){
      return ;
   }
   if(tienda->pLibreSalas == 0){
      printf("No hay salas Registradas\n");
      return;
   }
   ordenarSalas(tienda,2);/*aqui se ordena por ocupacion*/
   printf("================SALAS MAS OCUPADAS================\n");
   for(i = 0;i<tienda->pLibreSalas;i++){
      s = tienda->salas[i];
      porcentaje = s->ocupacionActual * 100 / s->aforoMaximo;
      printf("Puesto %d\n", i + 1);
      printf("Sala: [%d] %s\n", s->codigo, s->nombre);
      printf("Ocupacion: %d / %d personas\n", s->ocupacionActual, s->aforoMaximo);
      printf("Porcentaje de ocupacion: %d%%\n", porcentaje);
      if(porcentaje >= 80){
         printf("ALERTA : sala cerca de su aforo maximo\n");
      }
   }
   ordenarSalas(tienda,1);/*se vuelve a ordenar para que quede como estaba antes*/
}


/*todo el sistema desde la consola. */
void menuPrincipal(struct Tienda *tienda) {
    int opcion = -1;
    int subOpcion = -1;
    int codSala, codTitulo, posSala;
    int numReserva, cantidad, hora, criterio, tiempoEspera;
    int edad, tipoPase;
    long valorPase;
    char rut[MAX_TXT];
    char nombre[MAX_TXT];
    struct Sala *nuevaSala;
    struct Titulo *nuevoTitulo;
    struct Titulo *tituloEncontrado;
    struct Cliente *clienteEncontrado;
    struct Reserva *reservaEncontrada;

    if (tienda == NULL) {
        return;
    }

    do {
        printf("\n================ MENU PRINCIPAL ================\n");
        printf("1. Gestionar Salas\n");
        printf("2. Gestionar Titulos (Comics)\n");
        printf("3. Gestionar Clientes y Pases\n");
        printf("4. Gestionar Reservas (Cola de espera)\n");
        printf("5. Reportes y Estadisticas\n");
        printf("0. Salir\n");
        printf("Seleccione una opcion: ");

        if (scanf("%d", &opcion) != 1) {
            while (getchar() != '\n');
            opcion = -1;
        }

        /* ---------------- 1. GESTION DE SALAS ---------------- */
        if (opcion == 1) {
            printf("\n--- GESTION DE SALAS ---\n");
            printf("1. Agregar Sala\n");
            printf("2. Buscar Sala\n");
            printf("3. Modificar Sala\n");
            printf("4. Eliminar Sala\n");
            printf("5. Listar Salas\n");
            printf("6. Ordenar Salas\n");
            printf("Seleccione: ");

            if (scanf("%d", &subOpcion) != 1) {
                while (getchar() != '\n');
                subOpcion = -1;
            }

            if (subOpcion == 1) {
                nuevaSala = (struct Sala *)malloc(sizeof(struct Sala));
                nuevaSala->ocupacionActual = 0;
                nuevaSala->cantidadTitulos = 0;
                nuevaSala->raizTitulos = NULL;

                printf("Codigo de sala: "); scanf("%d", &nuevaSala->codigo);
                printf("Nombre (sin espacios): "); scanf("%59s", nuevaSala->nombre);
                printf("Tematica (sin espacios): "); scanf("%59s", nuevaSala->tematica);
                printf("Encargado (sin espacios): "); scanf("%59s", nuevaSala->encargado);
                printf("Hora apertura (HHMM): "); scanf("%d", &nuevaSala->horaApertura);
                printf("Hora cierre (HHMM): "); scanf("%d", &nuevaSala->horaCierre);
                printf("Aforo maximo: "); scanf("%d", &nuevaSala->aforoMaximo);

                if (agregarSala(tienda, nuevaSala) == 1) {
                    printf("Sala agregada con exito.\n");
                } else {
                    free(nuevaSala);
                }
            } else if (subOpcion == 2) {
                printf("Codigo de sala a buscar: "); scanf("%d", &codSala);
                posSala = buscarSala(tienda, codSala);
                if (posSala != -1) {
                    printf("Sala encontrada en posicion %d: [%d] %s | Tematica: %s | Ocupacion: %d/%d\n",
                           posSala, tienda->salas[posSala]->codigo,
                           tienda->salas[posSala]->nombre,
                           tienda->salas[posSala]->tematica,
                           tienda->salas[posSala]->ocupacionActual,
                           tienda->salas[posSala]->aforoMaximo);
                } else {
                    printf("No existe una sala con ese codigo.\n");
                }
            } else if (subOpcion == 3) {
                printf("Codigo de sala a modificar: "); scanf("%d", &codSala);
                if (modificarSala(tienda, codSala) == 1) {
                    printf("Fin de modificacion de sala.\n");
                }
            } else if (subOpcion == 4) {
                printf("Codigo de sala a eliminar: "); scanf("%d", &codSala);
                if (eliminarSala(tienda, codSala) == 1) {
                    printf("Sala eliminada correctamente.\n");
                }
            } else if (subOpcion == 5) {
                listarSalas(tienda);
            } else if (subOpcion == 6) {
                printf("Criterio (1: Por codigo ascendente, 2: Por ocupacion descendente): ");
                scanf("%d", &criterio);
                ordenarSalas(tienda, criterio);
                printf("Salas ordenadas correctamente.\n");
            } else {
                printf("Opcion invalida.\n");
            }
        }

        /* ---------------- 2. GESTION DE TITULOS ---------------- */
        else if (opcion == 2) {
            if (tienda->pLibreSalas == 0) {
                printf("Primero debe registrar al menos una sala en la tienda.\n");
            } else {
                printf("\n--- GESTION DE TITULOS ---\n");
                printf("1. Agregar Titulo a una Sala\n");
                printf("2. Buscar Titulo en una Sala\n");
                printf("3. Modificar Titulo (Stock / Descatalogar)\n");
                printf("4. Eliminar Titulo de una Sala\n");
                printf("5. Listar Titulos de una Sala\n");
                printf("6. Calcular Tiempo de Espera de un Titulo\n");
                printf("Seleccione: ");

                if (scanf("%d", &subOpcion) != 1) {
                    while (getchar() != '\n');
                    subOpcion = -1;
                }

                if (subOpcion < 1 || subOpcion > 6) {
                    printf("Opcion invalida.\n");
                } else {
                    printf("Ingrese el codigo de la sala: ");
                    scanf("%d", &codSala);
                    posSala = buscarSala(tienda, codSala);

                    if (posSala == -1) {
                        printf("No existe una sala con ese codigo.\n");
                    } else {
                        if (subOpcion == 1) {
                            printf("Codigo del nuevo titulo: ");
                            scanf("%d", &codTitulo);

                            if (codTitulo <= 0) {
                                printf("El codigo del titulo debe ser mayor a 0.\n");
                            } else if (buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo) != NULL) {
                                printf("El titulo con codigo %d ya existe en esta sala.\n", codTitulo);
                            } else {
                                nuevoTitulo = (struct Titulo *)malloc(sizeof(struct Titulo));
                                nuevoTitulo->codigo = codTitulo;
                                nuevoTitulo->solicitudesDia = 0;
                                nuevoTitulo->diasAgotado = 0;
                                nuevoTitulo->listaEspera = NULL;

                                printf("Nombre (sin espacios): "); scanf("%59s", nuevoTitulo->nombre);
                                printf("Tipo publicacion (1:Serie, 2:Mini, 3:Tomo, 4:Novela, 5:Tira, 6:Fanzine): ");
                                scanf("%d", &nuevoTitulo->tipoPublicacion);
                                printf("Edad minima: "); scanf("%d", &nuevoTitulo->edadMinima);
                                printf("Precio: "); scanf("%ld", &nuevoTitulo->precio);
                                printf("Stock inicial: "); scanf("%d", &nuevoTitulo->stock);
                                printf("Estado (1:Disp, 2:Agotado, 3:Preventa, 4:Desc): ");
                                scanf("%d", &nuevoTitulo->estado);
                                printf("Ejemplares por despacho: "); scanf("%d", &nuevoTitulo->ejemplaresPorDespacho);
                                printf("Dias entre despachos: "); scanf("%d", &nuevoTitulo->diasEntreDespachos);
                                printf("Limite por cliente: "); scanf("%d", &nuevoTitulo->limitePorCliente);

                                if (nuevoTitulo->tipoPublicacion < 1 || nuevoTitulo->tipoPublicacion > 6 ||
                                    nuevoTitulo->estado < 1 || nuevoTitulo->estado > 4 ||
                                    nuevoTitulo->edadMinima < 0 || nuevoTitulo->precio < 0 ||
                                    nuevoTitulo->stock < 0 || nuevoTitulo->ejemplaresPorDespacho <= 0 ||
                                    nuevoTitulo->diasEntreDespachos <= 0 || nuevoTitulo->limitePorCliente < 0) {
                                    printf("Error: Datos invalidos. No se agrego el titulo.\n");
                                    free(nuevoTitulo);
                                } else {
                                    /* Coherencia entre stock inicial y estado */
                                    if (nuevoTitulo->stock == 0 && nuevoTitulo->estado == TIT_DISP) {
                                        nuevoTitulo->estado = TIT_AGOTADO;
                                    } else if (nuevoTitulo->stock > 0 && nuevoTitulo->estado == TIT_AGOTADO) {
                                        nuevoTitulo->estado = TIT_DISP;
                                    }

                                    if (nuevoTitulo->estado == TIT_AGOTADO) {
                                        printf("Dias que lleva agotado: ");
                                        scanf("%d", &nuevoTitulo->diasAgotado);
                                        if (nuevoTitulo->diasAgotado < 0) {
                                            nuevoTitulo->diasAgotado = 0;
                                        }
                                    }

                                    tienda->salas[posSala]->raizTitulos = agregarTituloABB(tienda->salas[posSala]->raizTitulos, nuevoTitulo);
                                    tienda->salas[posSala]->cantidadTitulos++;
                                    printf("Titulo agregado con exito.\n");
                                }
                            }
                        } else if (subOpcion == 2) {
                            printf("Codigo del titulo a buscar: "); scanf("%d", &codTitulo);
                            tituloEncontrado = buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo);
                            if (tituloEncontrado != NULL) {
                                printf("Codigo: %d | Titulo: %s | Precio: $%ld | Stock: %d | Estado: %d | Limite/Cliente: %d\n",
                                       tituloEncontrado->codigo, tituloEncontrado->nombre,
                                       tituloEncontrado->precio, tituloEncontrado->stock,
                                       tituloEncontrado->estado, tituloEncontrado->limitePorCliente);
                            } else {
                                printf("Titulo no encontrado en esta sala.\n");
                            }
                        } else if (subOpcion == 3) {
                            printf("Codigo del titulo a modificar: "); scanf("%d", &codTitulo);
                            tituloEncontrado = buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo);
                            if (tituloEncontrado != NULL) {
                                if (modificarTitulo(tituloEncontrado) == 1) {
                                    printf("Titulo actualizado con exito.\n");
                                }
                            } else {
                                printf("Titulo no encontrado en esta sala.\n");
                            }
                        } else if (subOpcion == 4) {
                            printf("Codigo del titulo a eliminar: "); scanf("%d", &codTitulo);
                            if (buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo) != NULL) {
                                tienda->salas[posSala]->raizTitulos = eliminarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo);
                                tienda->salas[posSala]->cantidadTitulos--;
                                printf("Titulo eliminado correctamente.\n");
                            } else {
                                printf("Titulo no encontrado en esta sala.\n");
                            }
                        } else if (subOpcion == 5) {
                            if (tienda->salas[posSala]->raizTitulos == NULL) {
                                printf("No hay titulos registrados en esta sala.\n");
                            } else {
                                listarTitulosABB(tienda->salas[posSala]->raizTitulos);
                            }
                        } else if (subOpcion == 6) {
                            printf("Codigo del titulo: "); scanf("%d", &codTitulo);
                            tituloEncontrado = buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo);
                            if (tituloEncontrado == NULL) {
                                printf("Titulo no encontrado en esta sala.\n");
                            } else {
                                tiempoEspera = calcularTiempoEsperaTitulo(tituloEncontrado);
                                if (tiempoEspera != -1) {
                                    printf("Tiempo estimado de espera para '%s': %d dias.\n", tituloEncontrado->nombre, tiempoEspera);
                                } else {
                                    printf("El titulo no tiene lista de espera activa o sus datos de despacho no son validos.\n");
                                }
                            }
                        }
                    }
                }
            }
        }

        /* ---------------- 3. GESTION DE CLIENTES Y PASES ---------------- */
        else if (opcion == 3) {
            printf("\n--- GESTION DE CLIENTES Y PASES ---\n");
            printf("1. Agregar Cliente y Pase\n");
            printf("2. Buscar Cliente por RUT\n");
            printf("3. Modificar Cliente y Pase (Ingreso/Salida/Cambio de sala)\n");
            printf("4. Eliminar Cliente y Pase\n");
            printf("5. Listar Todos los Clientes y Pases\n");
            printf("6. Listar Clientes Actualmente en la Tienda\n");
            printf("Seleccione: ");

            if (scanf("%d", &subOpcion) != 1) {
                while (getchar() != '\n');
                subOpcion = -1;
            }

            if (subOpcion == 1) {
                printf("RUT del cliente: "); scanf("%59s", rut);
                printf("Nombre (sin espacios): "); scanf("%59s", nombre);
                printf("Edad: "); scanf("%d", &edad);
                printf("Tipo de pase (1:Gen, 2:Inf, 3:Fam, 4:Prio): "); scanf("%d", &tipoPase);
                printf("Valor del pase: "); scanf("%ld", &valorPase);

                if (agregarCliente(tienda, rut, nombre, edad, tipoPase, valorPase) == 1) {
                    printf("Cliente y pase registrados con exito.\n");
                } else {
                    printf("No se pudo registrar el cliente (RUT duplicado o datos invalidos).\n");
                }
            } else if (subOpcion == 2) {
                printf("RUT a buscar: "); scanf("%59s", rut);
                clienteEncontrado = buscarClientePorRut(tienda->headClientes, rut);
                if (clienteEncontrado != NULL) {
                    printf("Cliente: %s (RUT: %s) | Edad: %d | En tienda: %d | Sala actual: %d\n",
                           clienteEncontrado->nombre, clienteEncontrado->rut,
                           clienteEncontrado->edad, clienteEncontrado->dentroTienda,
                           clienteEncontrado->codigoSalaActual);
                    if (clienteEncontrado->pase != NULL) {
                        printf("Pase #%d | Tipo: %d | Valor: $%ld | Estado: %d\n",
                               clienteEncontrado->pase->codigo, clienteEncontrado->pase->tipo,
                               clienteEncontrado->pase->valor, clienteEncontrado->pase->estado);
                    }
                } else {
                    printf("Cliente no encontrado.\n");
                }
            } else if (subOpcion == 3) {
                printf("RUT del cliente a modificar: "); scanf("%59s", rut);
                if (modificarClienteYPase(tienda, rut) == 1) {
                    printf("Datos del cliente actualizados.\n");
                } else {
                    printf("No se pudo modificar el cliente.\n");
                }
            } else if (subOpcion == 4) {
                printf("RUT del cliente a eliminar: "); scanf("%59s", rut);
                if (eliminarClienteYPase(tienda, rut) == 1) {
                    printf("Cliente eliminado con exito.\n");
                } else {
                    printf("No se encontro el cliente.\n");
                }
            } else if (subOpcion == 5) {
                listarClientesYPases(tienda->headClientes);
            } else if (subOpcion == 6) {
                listarClientesEnTienda(tienda->headClientes);
            } else {
                printf("Opcion invalida.\n");
            }
        }

        /* ---------------- 4. GESTION DE RESERVAS (COLA) ---------------- */
        else if (opcion == 4) {
            if (tienda->pLibreSalas == 0) {
                printf("No hay salas registradas en la tienda.\n");
            } else {
                printf("\n--- GESTION DE RESERVAS ---\n");
                printf("1. Agregar Reserva a la Cola\n");
                printf("2. Buscar Reserva en la Cola\n");
                printf("3. Modificar Cantidad de una Reserva\n");
                printf("4. Eliminar Reserva de la Cola\n");
                printf("5. Listar Reservas de un Titulo\n");
                printf("Seleccione: ");

                if (scanf("%d", &subOpcion) != 1) {
                    while (getchar() != '\n');
                    subOpcion = -1;
                }

                if (subOpcion < 1 || subOpcion > 5) {
                    printf("Opcion invalida.\n");
                } else {
                    printf("Codigo de la sala: "); scanf("%d", &codSala);
                    posSala = buscarSala(tienda, codSala);

                    if (posSala == -1) {
                        printf("Sala no encontrada.\n");
                    } else {
                        printf("Codigo del titulo: "); scanf("%d", &codTitulo);
                        tituloEncontrado = buscarTituloABB(tienda->salas[posSala]->raizTitulos, codTitulo);

                        if (tituloEncontrado == NULL) {
                            printf("Titulo no encontrado en esta sala.\n");
                        } else {
                            if (subOpcion == 1) {
                                printf("RUT del cliente que reserva: "); scanf("%59s", rut);
                                clienteEncontrado = buscarClientePorRut(tienda->headClientes, rut);
                                if (clienteEncontrado == NULL) {
                                    printf("El cliente no esta registrado en el sistema.\n");
                                } else {
                                    printf("Cantidad de ejemplares: "); scanf("%d", &cantidad);
                                    printf("Hora de registro (HHMM): "); scanf("%d", &hora);

                                    if (hora < 0 || hora > 2359 || hora % 100 >= 60) {
                                        printf("Hora de registro invalida.\n");
                                    } else if (agregarReservaCola(tituloEncontrado, clienteEncontrado, cantidad, tienda->siguienteNumeroReserva, hora) == 1) {
                                        printf("Reserva #%d agregada con exito.\n", tienda->siguienteNumeroReserva);
                                        tienda->siguienteNumeroReserva++;
                                    } else {
                                        printf("No se pudo agregar la reserva (verifique edad minima, limite por cliente o estado del titulo).\n");
                                    }
                                }
                            } else if (subOpcion == 2) {
                                if (tituloEncontrado->listaEspera == NULL || tituloEncontrado->listaEspera->frente == NULL) {
                                    printf("La lista de espera de este titulo esta vacia.\n");
                                } else {
                                    printf("Numero de reserva a buscar: "); scanf("%d", &numReserva);
                                    reservaEncontrada = buscarReservaCola(tituloEncontrado->listaEspera, numReserva);
                                    if (reservaEncontrada != NULL && reservaEncontrada->cliente != NULL) {
                                        printf("Reserva #%d | Cliente: %s (RUT: %s) | Cantidad: %d | Hora: %04d\n",
                                               reservaEncontrada->numero, reservaEncontrada->cliente->nombre,
                                               reservaEncontrada->cliente->rut, reservaEncontrada->cantidad,
                                               reservaEncontrada->horaRegistro);
                                    } else {
                                        printf("Reserva no encontrada en la cola de este titulo.\n");
                                    }
                                }
                            } else if (subOpcion == 3) {
                                if (tituloEncontrado->listaEspera == NULL || tituloEncontrado->listaEspera->frente == NULL) {
                                    printf("La lista de espera de este titulo esta vacia.\n");
                                } else {
                                    printf("Numero de reserva a modificar: "); scanf("%d", &numReserva);
                                    printf("Nueva cantidad de ejemplares: "); scanf("%d", &cantidad);

                                    if (tituloEncontrado->limitePorCliente > 0 && cantidad > tituloEncontrado->limitePorCliente) {
                                        printf("Error: La cantidad supera el limite permitido por cliente (%d).\n", tituloEncontrado->limitePorCliente);
                                    } else if (modificarReservaCola(tituloEncontrado->listaEspera, numReserva, cantidad) == 1) {
                                        printf("Reserva modificada con exito.\n");
                                    } else {
                                        printf("No se pudo modificar la reserva (numero no encontrado o cantidad invalida).\n");
                                    }
                                }
                            } else if (subOpcion == 4) {
                                if (tituloEncontrado->listaEspera == NULL || tituloEncontrado->listaEspera->frente == NULL) {
                                    printf("La lista de espera de este titulo esta vacia.\n");
                                } else {
                                    printf("Numero de reserva a eliminar: "); scanf("%d", &numReserva);
                                    if (eliminarReservaCola(tituloEncontrado->listaEspera, numReserva) == 1) {
                                        printf("Reserva eliminada con exito.\n");
                                    } else {
                                        printf("No se encontro la reserva.\n");
                                    }
                                }
                            } else if (subOpcion == 5) {
                                listarReservasCola(tituloEncontrado->listaEspera);
                            }
                        }
                    }
                }
            }
        }

        /* ---------------- 5. REPORTES Y ESTADISTICAS ---------------- */
        else if (opcion == 5) {
            printf("\n--- REPORTES Y ESTADISTICAS ---\n");
            printf("1. Estadisticas Generales de la Tienda\n");
            printf("2. Ranking de Salas Mas Ocupadas\n");
            printf("3. Titulo con Mayor Tiempo Estimado de Espera\n");
            printf("4. Titulo Mas Solicitado del Dia\n");
            printf("5. Listado de Titulos Agotados\n");
            printf("Seleccione: ");

            if (scanf("%d", &subOpcion) != 1) {
                while (getchar() != '\n');
                subOpcion = -1;
            }

            if (subOpcion == 1) {
                reporteEstadisticasGenerales(tienda);
            } else if (subOpcion == 2) {
                reporteSalasMasOcupadas(tienda);
            } else if (subOpcion == 3) {
                reporteTituloMasEsperado(tienda);
            } else if (subOpcion == 4) {
                reporteTituloMasSolicitado(tienda);
            } else if (subOpcion == 5) {
                listarTitulosAgotados(tienda);
            } else {
                printf("Opcion invalida.\n");
            }
        }

        else if (opcion != 0) {
            printf("Opcion invalida.\n");
        }

    } while (opcion != 0);

    printf("Saliendo del sistema...\n");
}

int main(void) {
    struct Tienda tienda;
    int i;

    /* Inicializacion de la estructura Tienda */
    strcpy(tienda.nombre, "Tienda_Comics");
    strcpy(tienda.ciudad, "Valparaiso");
    tienda.aforoMaximo = 100;
    tienda.clientesDentro = 0;
    tienda.ingresosDia = 0;
    tienda.pasesVendidos = 0;
    tienda.pasesUtilizados = 0;
    tienda.recaudacionPases = 0;

    tienda.siguienteCodigoPase = 1;
    tienda.siguienteNumeroReserva = 1;

    tienda.pLibreSalas = 0;
    for (i = 0; i < MAX_SALAS; i++) {
        tienda.salas[i] = NULL;
    }

    tienda.headClientes = NULL;

    menuPrincipal(&tienda);

    /* Liberacion de memoria antes de finalizar */
    while (tienda.headClientes != NULL) {
        eliminarClienteYPase(&tienda, tienda.headClientes->datos->rut);
    }
    while (tienda.pLibreSalas > 0) {
        tienda.salas[0]->ocupacionActual = 0;
        eliminarSala(&tienda, tienda.salas[0]->codigo);
    }

    return 0;
}