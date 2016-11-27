/************************************************************************************************
 *                                                                                              *
 *                 Comunicación de procesos mediante colas de mensajes en POSIX                 *
 *                                                                                              *
 * Autores:                                                                                     *
 *      Mariano González Salazar (100316087)                                                    *
 *      Jorge Peláez González (100315680)                                                       *
 *                                                                                              *
 ***********************************************************************************************/
/************************************************************************************************
 *                                                                                              *
 *               Librerías necesarias para el correcto funcionamiento del programa              *
 *                                                                                              *
 ***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include "sys/wait.h"
#include <signal.h>

/************************************************************************************************
 *                                                                                              *
 *                      Definición de las constantes usadas por el programa                     *
 *                                                                                              *
 ***********************************************************************************************/

#define MAX_SIZE 50
#define MAX_MESSAGES 10
#define BUFFER_SIZE 60

/************************************************************************************************
 *                                                                                              *
 *                           Declaración de las funciones del programa                          *
 *                                                                                              *
 ************************************************************************************************/

void operator1 ();
void operator2 ();
void sleep_secs (int secs);
void error (char * error_name);
void *listener_operator1();
void *listener_operator2();
void op2_handler1(int signo);
void op2_handler2(int signo);

/************************************************************************************************
 *                                                                                              *
 *                     Declaración de las variables globales del programa                       *
 *                                                                                              *
 ************************************************************************************************/

char ** quejas;
int op1_state, end;
int pid2;

/************************************************************************************************
 *                                                                                              *
 *                                      Función principal                                       *
 *                                                                                              *
 *      Actualmente inicializa el vector account_coefficients y crea tres cuentas bancarias     *
 *                                                                                              *
 ************************************************************************************************/

int main (int argc, char *argv[])
{
    int i;
    mqd_t queue;


////////////////////////////                ARGS CHECKING            ////////////////////////////

    if (argc < 2)
    {
        printf("\n\tUso: ./a.out <Queja 1> <Queja 2> ...\n");
        printf("\t\tQueja N:\tqueja a elección tuya.\n");
        printf("\n\tSaliendo del programa...\n\n");
        exit(-1);
    }


////////////////////////////               ARRAY QUEJAS            ////////////////////////////
    
    // Memory allocation for array of strings 'quejas'
    quejas = (char **) malloc (sizeof(char *) * (argc - 1));
    for (i = 0; i < (argc - 1); i++)
    {
        quejas[i] = (char *) malloc (sizeof(char));
        strcpy(quejas[i], argv[i + 1]);
    }

    // Print the array of strings 'quejas'
    printf("\nQuejas = {");
    for (i = 0; i < argc - 1; i++)
        printf(" %s,", quejas[i]);
    printf("\b }\n\n");

////////////////////////////           ESPERA DEL CLIENTE            ////////////////////////////
    
    for (i=5;i>0;i--){
        printf("Cliente en espera... %i segundos...\n",i);
        sleep_secs(1);
        fputs("\033[A\033[2K",stdout);
    }
    
    printf("Cliente en espera... 5 segundos... \n\n");


////////////////////////////               STRUCT ATTR                ////////////////////////////

    struct mq_attr attr;  
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

////////////////////////////           CREACIÓN COLA E HIJOS           ////////////////////////////

    queue = mq_open("/ColaQuejas", (O_WRONLY | O_CREAT | O_EXCL), (S_IRWXU | S_IRWXG | S_IRWXO), &attr);
    if (!(pid2=fork())) operator2();
    if (!fork()) operator1();

////////////////////////////             ENVIAR DATOS              ////////////////////////////

    printf("Cliente: %s\n", quejas[0]);
    mq_send(queue, quejas[0], strlen(quejas[0]), 0);
    for (i = 1; i < argc - 1; i++)
    {
        // Espera aleatoria entre 0 y 3 segundos
        sleep_secs(1+(int) (3.0*rand()/(RAND_MAX+1.0)));
        // Manda la queja
        printf("Cliente: %s\n", quejas[i]);
        mq_send(queue, quejas[i], strlen(quejas[i]), 0);
    }

////////////////////////////             FINALIZACIÓN              ////////////////////////////
   
    wait(NULL);
    wait(NULL);
    
    mq_close(queue);
    mq_unlink("/ColaQuejas");

    // Memory release
    for (i = 0; i < argc - 1; i++)
            free(quejas[i]);
    free(quejas);

    exit(0);
}

/************************************************************************************************
 *                                                                                              *
 *                                        Función error                                         *
 *                                                                                              *
 * Muestra un mensaje de error y sale del programa (solo puede entrar el proceso padre)         *
 * Parámetros:                                                                                  *
 *      char * error_name: Nombre del error                                                     *
 *                                                                                              *
 ************************************************************************************************/

void error (char * error_name)
{
    perror(error_name);
    exit(-1);
}

/************************************************************************************************
 *                                                                                              *
 *                                      Función operator1                                       *
 *                                                                                              *
 * Funcion que ejecuta el proceso del Operador1                                                 *
 *                                                                                              *
 ************************************************************************************************/

void operator1 ()
{
    // Se crea la hebra de comunicación
    pthread_t listener_t;
    pthread_create(&listener_t, NULL, listener_operator1, NULL);
    
    // Se espera a la finalización de la hebra de comunicación
    pthread_join(listener_t, NULL);
    
    exit(0);
}

/************************************************************************************************
 *                                                                                              *
 *                                      Función operator2                                       *
 *                                                                                              *
 * Funcion que ejecuta el proceso del Operador2                                                 *
 *                                                                                              *
 ************************************************************************************************/

void operator2 ()
{
    // Si op_state=0, operador2 interpretará que operador1 está libre y debe dejarle prioridad
    // Si end=1, operador2 interpretará que han pasado 5 segundos desde que operador1 procesó la última queja
    op1_state = 0;
    end = 0;
    
    // Se indica cómo procesar las notificaciones de operador1
    signal(SIGUSR1, op2_handler1);
    signal(SIGUSR2, op2_handler2);
    
    // Se crea la hebra de comunicación
    pthread_t listener_t;
    pthread_create(&listener_t, NULL, listener_operator2, NULL);
    
    // Se espera a la finalización de la hebra de comunicación
    pthread_join(listener_t, NULL);
    
    exit(0);
}

/************************************************************************************************
 *                                                                                              *
 *                                  Función listener_operator1                                  *
 *                                                                                              *
 * Función de la hebra de comunicación del Operador 1. Escucha quejas hasta que pasan 5         *
 * segundos sin ninguna queja.                                                                  *
 *                                                                                              *
 ************************************************************************************************/

void * listener_operator1()
{
    char buffer[BUFFER_SIZE];
    mqd_t queue = mq_open("/ColaQuejas", O_RDONLY);
    
    // Se configura el temporizador
    struct timespec tim;
    clock_gettime(CLOCK_REALTIME, &tim);
    tim.tv_sec += 5;
    
    // Mientras no pasen 5 segundos entre queja y queja, se procesa mediante el interior del while
    while (mq_timedreceive(queue, buffer, BUFFER_SIZE, NULL, &tim) != -1) {
        printf("---- Operador1: %s recibida. Atendiendo...\n",buffer);
        // Cambia el estado de operador1 a ocupado
        kill(pid2, SIGUSR1);
        sleep_secs(5);
        // Cambia el estado de operador1 a libre
        kill(pid2, SIGUSR1);
        printf("---- Operador1: %s servida. ***\n",buffer);
        
        // Actualiza el temporizador
        clock_gettime(CLOCK_REALTIME, &tim);
        tim.tv_sec += 5;
    }
    
    // Si han pasado 5 segundos sin quejas, se notifica a operador2 y se finaliza
    printf("\n---- Operador1: 5 segundos sin quejas\n");
    kill(pid2, SIGUSR2);
    // Se espera un tiempo prudencial por si operador2 aún no ha finalizado
    sleep_secs(5);
    printf("\n---- Operador1: CALL CENTER CERRADO POR HOY.\n\n");
    mq_close(queue);

    pthread_exit(NULL);
}

/************************************************************************************************
 *                                                                                              *
 *                                  Función listener_operator2                                  *
 *                                                                                              *
 * Función de la hebra de comunicación del Operador 2. Escucha quejas hasta que operador1 le    *
 * dice que han pasado 5 segundos sin ninguna queja.                                            *
 *                                                                                              *
 ************************************************************************************************/

void * listener_operator2()
{
    char buffer[BUFFER_SIZE];
    mqd_t queue = mq_open("/ColaQuejas", O_RDONLY);
    struct timespec tim;
    
    // Mientras end!=1 se procesan quejas
    while (end==0){
        // Se espera a que operador1 esté ocupado
        while(op1_state==0 && end == 0);
        clock_gettime(CLOCK_REALTIME, &tim);
        tim.tv_sec += 1;
        if (end==0 && mq_timedreceive(queue, buffer, BUFFER_SIZE, NULL, &tim) != -1){
            printf("---- Operador2: %s recibida. Atendiendo...\n",buffer);
            sleep_secs(5);
            printf("---- Operador2: %s servida. ***\n",buffer);
        }
    }
    
    // Si han pasado 5 segundos sin quejas, se finaliza la ejecución
    printf("---- Operador2: 5 segundos sin quejas. FIN DE LA JORNADA.\n");
    
    mq_close(queue);

    pthread_exit(NULL);
}

/************************************************************************************************
 *                                                                                              *
 *                            Funciónes  op2_handler1 y op2_handler2                            *
 *                                                                                              *
 * Funciones encargadas de procesar las señales enviadas por operador 1 a operador2             *
 * Parámetros:                                                                                  *
 *      int signo: código de la señal que capta                                                 *
 *                                                                                              *
 ************************************************************************************************/

void op2_handler1(int signo){
    // Cambia el estado de operador1 de ocupado a libre o de libre a ocupado
    if (op1_state == 0) op1_state = 1;
    else op1_state = 0;
}

void op2_handler2(int signo){
    // Se notifica a operador2 que se han estado 5 segundos sin quejas
    end = 1;
}

/************************************************************************************************
 *                                                                                              *
 *                                     Función sleep_secs                                       *
 *                                                                                              *
 * Funcion que espera un número de segundos especificado                                        *
 * Parámetros:                                                                                  *
 *      int secs: segundos a esperar                                                            *
 *                                                                                              *
 ************************************************************************************************/

void sleep_secs (int secs)
{
    struct timespec tim, tim2;
    tim.tv_nsec = 0;
    tim.tv_sec = secs;
    nanosleep(&tim, &tim2);
}
