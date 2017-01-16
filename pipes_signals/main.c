/************************************************************************************************
 *                                                                                              *
 *                                Ejercicio de pipes y señales                                  *
 *                                                                                              *
 * Autores:                                                                                     *
 *      Mariano González Salazar (100316087)                                                    *
 *      Jorge Peláez González (100315680)                                                       *
 *                                                                                              *
 ************************************************************************************************/
/************************************************************************************************
 *                                                                                              *
 *               Librerías necesarias para el correcto funcionamiento del programa              *
 *                                                                                              *
 ************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>

/************************************************************************************************
 *                                                                                              *
 *                           Declaración de las funciones del programa                          *
 *                                                                                              *
 ************************************************************************************************/


void error();
void close_fds();
void cuatro_caminos();
void metropolitano();
void nuevos_ministerios();
void station_actions();
void train_actions();
static void sig_handle();
static void sig_handle_child();


/************************************************************************************************
 *                                                                                              *
 *                     Declaración de las variables globales del programa                       *
 *                                                                                              *
 ************************************************************************************************/


const char train[] = "Train";           // Texto que se enviará representando el tren
const char change_msg[] = "change";     // Mensaje que se enviará cuando el padre reciba la señal de cambio de sentido
unsigned short change_direction = 0;    // Indica que se ha efectuado la señal de cambio de sentido
int child1_pid=0, child2_pid=0;         // PID de los hijos


/************************************************************************************************
 *                                                                                              *
 *                                      Función principal                                       *
 *                                                                                              *
 * Se encarga de la creación de los pipes de comunicación, asi como de llamar a las funciones   *
 * que crean los procesos hijo.                                                                 *
 *                                                                                              *
 ************************************************************************************************/


int main()
{
    //bindkey -b -c ^G "kill -USR1 2019"
    int PID_parent = getpid();

    // Declaración de los identificadores de los pipes
    int fd_nc_cw[2], fd_nm_cw[2], fd_cm_cw[2], fd_nc_ccw[2], fd_nm_ccw[2], fd_cm_ccw[2];

    // Generación de los 6 pipes. Si la generación de cualquiera de ellas es erronea, se cierra el programa
    if (pipe(fd_nc_cw) < 0 || pipe(fd_nm_cw) < 0 || pipe(fd_cm_cw) < 0 || 
        pipe(fd_nc_ccw) < 0 || pipe(fd_nm_ccw) < 0 || pipe(fd_cm_ccw) < 0)
        error("pipe");

    // Comportamiento al captar la señar SIGTSTP de los hijos (luego se sobreescribirá la del padre)
    if (signal(SIGTSTP, sig_handle_child) == SIG_ERR)
        printf("Impossible catching SIGTSTP\n");

    // Función de creación y funcionamiento del 1er hijo
    cuatro_caminos(fd_nm_cw, fd_nc_cw, fd_cm_cw, fd_nm_ccw, fd_nc_ccw, fd_cm_ccw);
    // Función de creación y funcionamiento del 2o hijo
    metropolitano(fd_nm_cw, fd_nc_cw, fd_cm_cw, fd_nm_ccw, fd_nc_ccw, fd_cm_ccw);
    // Función de funcionamiento del padre
    nuevos_ministerios(fd_nm_cw, fd_nc_cw, fd_cm_cw, fd_nm_ccw, fd_nc_ccw, fd_cm_ccw);

    // Antes de finalizar el proceso padre, espera a la finalización de los procesos hijo
    wait(NULL);
    wait(NULL);
    return 0;
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
    // Si algún proceso hijo ha sido creado, se termina
    if (child1_pid!=0) kill(child1_pid, SIGKILL);
    if (child2_pid!=0) kill(child2_pid, SIGKILL);
    perror(error_name);
    exit(-1);
}


/************************************************************************************************
 *                                                                                              *
 *                                        Función close_fds                                     *
 *                                                                                              *
 * Cierra los extremos de los pipes que no van a ser utilizados por el proceso.                 *
 * Parámetros:                                                                                  *
 *      fd_close_cw[], fd_close_ccw[]: Pipes que deben ser cerrados por ambos extremos          *
 *      resto: Extremos de los pipes que se deben cerrar (de los pipes que si usarán)           *
 *                                                                                              *
 ************************************************************************************************/


void close_fds (int fd_close_cw[], int fd_write_cw, int fd_read_cw, int fd_close_ccw[], 
                int fd_write_ccw, int fd_read_ccw)
{
    // Clockwise direction
    close(fd_close_cw[0]); close(fd_close_cw[1]);
    close(fd_write_cw);
    close(fd_read_cw);

    // Counterclockwise direction
    close(fd_close_ccw[0]); close(fd_close_ccw[1]);
    close(fd_write_ccw);
    close(fd_read_ccw);
}


/************************************************************************************************
 *                                                                                              *
 *                                   Función cuatro_caminos                                     *
 *                                                                                              *
 * Contiene la creación del primer hijo (Cuatro Caminos) y el código que dicho hijo             *
 * debe ejecutar                                                                                *
 * Parámetros:                                                                                  *
 *      Todos los pipes creados por el padre                                                     *
 *                                                                                              *
 ************************************************************************************************/


void cuatro_caminos (int fd_nm_cw[], int fd_nc_cw[], int fd_cm_cw[], 
                     int fd_nm_ccw[], int fd_nc_ccw[], int fd_cm_ccw[])
{
    // Creación del hijo
    child1_pid = fork();

    // Ejecución separada del hijo y el padre
    switch(child1_pid) {
        // Código de ejecución de hijo
        case 0:
            // Cierra los extremos de los pipes que no se usarán
            close_fds(fd_nm_cw, fd_nc_cw[0], fd_cm_cw[1],fd_nm_ccw, fd_nc_ccw[1], fd_cm_ccw[0]);
            printf("\n(Presiona Ctrl^Z para cambio de sentido)\n");
            printf("\n. . . Tren saliendo desde Cuatro Caminos\n");
            // El recorrido del tren se inicia en Cuatro Caminos en dirección Nuevos Ministerios
            write(fd_nc_cw[1], train, sizeof(train));
            // Entra en bucle de recepción y envío del tren
            station_actions(fd_nc_cw[1], fd_cm_cw[0], fd_nc_ccw[0], fd_cm_ccw[1], "Cuatro Caminos");
            exit(0);
            break;

        // El fork puede haber devuelto un error
        case -1:
            error("fork");
            break;

        // El padre no debe hacer nada
        default:
            break;
    }
}


/************************************************************************************************
 *                                                                                              *
 *                                   Función metropolitano                                      *
 *                                                                                              *
 * Contiene la creación del segundo hijo (Metropolitano) y el código que dicho hijo             *
 * debe ejecutar                                                                                *
 * Parámetros:                                                                                  *
 *      Todos los pipes creados por el padre                                                    *
 *                                                                                              *
 ************************************************************************************************/


void metropolitano (int fd_nm_cw[], int fd_nc_cw[], int fd_cm_cw[], 
                    int fd_nm_ccw[], int fd_nc_ccw[], int fd_cm_ccw[])
{
    // Creación del hijo
    child2_pid = fork();

    // Ejecución separada del hijo y el padre
    switch(child2_pid) {
        // Código de ejecución de hijo
        case 0:
            // Cierra los extremos de los pipes que no se usarán
            close_fds(fd_nc_cw, fd_cm_cw[0], fd_nm_cw[1],fd_nc_ccw, fd_nm_ccw[0], fd_cm_ccw[1]);
            // Entra en bucle de recepción y envío del tren
            station_actions(fd_cm_cw[1], fd_nm_cw[0], fd_cm_ccw[0], fd_nm_ccw[1], "Metropolitano");
            exit(0);
            break;

        // El fork puede haber devuelto un error
        case -1:
            error("fork");
            break;

        // El padre no debe hacer nada
        default:
            break;
    }
}


/************************************************************************************************
 *                                                                                              *
 *                                 Función nuevos_ministerios                                   *
 *                                                                                              *
 * Contiene el código de ejecución del padre (Nuevos Ministerios)                               *
 * Parámetros:                                                                                  *
 *      Todos los pipes creados en la función main                                              *
 *                                                                                              *
 ************************************************************************************************/


void nuevos_ministerios(int fd_nm_cw[], int fd_nc_cw[], int fd_cm_cw[], 
                        int fd_nm_ccw[], int fd_nc_ccw[], int fd_cm_ccw[])
{
    // Comportamiento del padre al captar la señar SIGTSTP
    if (signal(SIGTSTP, sig_handle) == SIG_ERR)
        printf("Impossible catching SIGTSTP\n");

    // Cierra los extremos de los pipes que no se usarán
    close_fds(fd_cm_cw, fd_nm_cw[0], fd_nc_cw[1],fd_cm_ccw, fd_nc_ccw[0], fd_nm_ccw[1]);
    // Entra en bucle de recepción y envío del tren
    station_actions(fd_nm_cw[1], fd_nc_cw[0], fd_nm_ccw[0], fd_nc_ccw[1], "Nuevos Ministerios");
}


/************************************************************************************************
 *                                                                                              *
 *                                Función station_actions                                       *
 *                                                                                              *
 * Se encarga de la recepción y envío del tren para todos los procesos según la dirección       *
 * Parámetros:                                                                                  *
 *      char * station: Nombre de la estación                                                   *
 *      resto: Extremos de los pipes que se utilizarán                                          *
 *                                                                                              *
 ************************************************************************************************/


void station_actions(int fd_write_cw, int fd_read_cw, int fd_read_ccw, int fd_write_ccw, char * station)
{
    unsigned char actual_direction = 0;         // Representa el sentido actual del tren
    char buff [sizeof(change_msg)];             // Buffer de recepción del pipe

    while(1){
        // Sentido inicial del tren (ClockWise)
        if(!actual_direction){
            // La estación espera a recibir por el pipe
            read(fd_read_cw, buff, sizeof(buff));
            // Si se recibe "change", se da por enterado del cambio de sentido
            if (!strcmp(buff, "change"))
            {
                actual_direction = !actual_direction;
                if (strcmp(station, "Cuatro Caminos"))
                    write(fd_write_cw, change_msg, sizeof(change_msg));
            }
            // Si recibe un tren, lo envía en el sentido adecuado
            else {
                train_actions(station, "EO");
                if (change_direction)
                {
                    write(fd_write_cw, change_msg, sizeof(change_msg));
                    sleep(1);
                    write(fd_write_ccw, train, sizeof(train));
                    actual_direction = !actual_direction;
                }
                else
                    write(fd_write_cw, train, sizeof(train));
                change_direction = 0;
            }
        }
        // Sentido contrario al inicial (Counter-Clockwise)
        else {
            // La estación espera a recibir por el pipe
            read(fd_read_ccw, buff, sizeof(buff));
            // Si se recibe "change", se da por enterado del cambio de sentido
            if (!strcmp(buff, "change"))
            {
                actual_direction = !actual_direction;
                if (strcmp(station, "Metropolitano"))
                    write(fd_write_ccw, change_msg, sizeof(change_msg));
            }
            // Si recibe un tren, lo envía en el sentido adecuado
            else {
                train_actions(station, "OE");
                if (change_direction)
                {
                    write(fd_write_ccw, change_msg, sizeof(change_msg));
                    sleep(1);
                    write(fd_write_cw, train, sizeof(train));
                    actual_direction = !actual_direction;
                }
                else
                    write(fd_write_ccw, train, sizeof(train));
                change_direction = 0;
            }
        }
    }
}


/************************************************************************************************
 *                                                                                              *
 *                                   Función train_actions                                      *
 *                                                                                              *
 * Muestra un mensaje de error y sale del programa (solo puede entrar el proceso padre)         *
 * Parámetros:                                                                                  *
 *      char * station: Nombre de la estación                                                   *
 *      char * direction: Sentido actual del tren                                               *
 *                                                                                              *
 ************************************************************************************************/


void train_actions(char * station, char * direction)
{
    // Se configura un timer de 2.5 segundos
    struct timespec tim, tim2;
    tim.tv_nsec = 500000000;
    tim.tv_sec = 2;

    // Se esperan los 2.5 segundos (tiempo entre estaciones)
    if(nanosleep(&tim , &tim2) == -1)
        nanosleep(&tim2 , &tim);

    // Se reconfigura el timer a 0.5 segundos
    tim.tv_sec = 0;

    // Se representa la entrada del tren en la estación, y la subida/bajada de pasajeros
    printf("\n%s : Tren %s entrando\n", station, direction);
    if(nanosleep(&tim , &tim2) == -1)
        nanosleep(&tim2 , &tim);

    printf(". . . E/S de pasajeros\n");
    if(nanosleep(&tim , &tim2) == -1)
        nanosleep(&tim2 , &tim);

    // Si se había solicitado un cambio de dirección, se efectúa (change_direction=0 en los hijos)
    if (change_direction)
    {
        printf(". . . Tren cambiado de vías. Cambio de sentido a %c%c.\n", direction[1], direction[0]);
        if(nanosleep(&tim , &tim2) == -1)
            nanosleep(&tim2 , &tim);
    }

    printf(". . . Tren saliendo\n");
}


/************************************************************************************************
 *                                                                                              *
 *                                     Función sig_handle                                       *
 *                                                                                              *
 * Tratamiento de la señal de cambio de sentido por parte del proceso padre.                    *
 * Parámetros:                                                                                  *
 *      int signo: ID de la señal recibida                                                      *
 *                                                                                              *
 ************************************************************************************************/


static void sig_handle (int signo)
{
    printf(" pressed ******** [[Señal de cambio de sentido]]\n");
    change_direction = 1;
}


/************************************************************************************************
 *                                                                                              *
 *                                   Función sig_handle_child                                   *
 *                                                                                              *
 * Ignora la señal de cambio de sentido (necesaria ya que si es una señal de sistema,           *
 * realizaría una acción distinta a la deseada)                                                 *
 * Parámetros:                                                                                  *
 *      int signo: ID de la señal recibida                                                      *
 *                                                                                              *
 ************************************************************************************************/


static void sig_handle_child (int signo){}
