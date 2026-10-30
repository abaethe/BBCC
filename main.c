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
/*Tipo de criterio:
que no sea 2 (puede ser 1)= ordenar por codigo
2 = ordenar por ocupacion
*/
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

/* Retorna: 1 si se agrego con exito, 0 si el arreglo esta lleno o posea cualquier otro inconveniente. */
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

/* Retorna: 1 si se modifico con exito, 0 si no se encontro la sala.
Llama a: buscarSala con el objetivo de encontrar la posicion exacta de la sala que se quiere editar */
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
#define
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

/*Llama a: ordenarSalas con el objetivo de ordenar el arreglo de salas de mayor a menor ocupacion antes de mostrarlas. */
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

/*===============================================================================================================
 * CLAUDIO
 =============================================================================================================== */

/*Llama a: Todas las funciones
anteriores segun la opcion elegida con el objetivo de permitir al usuario operar todo el sistema desde la consola. */
void menuPrincipal(struct Tienda *tienda);

int main(void);



