/************************************************************************************************
 *                                                                                              *
 *                        Hebras POSIX: gestión básica y sincronización                         *
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
 ***********************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

/************************************************************************************************
 *                                                                                              *
 *                      Definición de las constantes usadas por el programa                     *
 *                                                                                              *
 ***********************************************************************************************/

#define DEFAULT_COEFFICIENT_VALUE 1.0
#define MAX_DISCOUNT 10
#define NUMBER_OF_ACCOUNTS 3
#define NUMBER_OF_COEFFICIENTS 5
#define NUMBER_OF_THREADS 4

/************************************************************************************************
 *                                                                                              *
 *                  Declaración de las estructuras y enumeraciones del programa                 *
 *                                                                                              *
 ************************************************************************************************/

typedef enum {NO, ONE, MORE} account_mortage;
typedef enum {UNCOVERED, LOW, HIGH} account_balance;
typedef enum {NO_CARD, CREDIT, CREDIT_DEBIT} account_card;
typedef enum {HOME, HOME_HOUSE, HOME_HOUSE_LIFE} account_insurances;
typedef enum {PHYSICAL, AUTONOMOUS, JURIDIC} account_character;

typedef struct {
  char * owner;                    // Owner of the account
  unsigned long balance;           // Total balance
  account_mortage mort;            // 0-> no mortage; 1-> 1 mortage; 2-> more than 1 mortages
  account_balance bal;             // 0-> ocasionally uncovered; 1-> < 1000 €; 2-> > 1000 €
  account_card card;               // 0-> no credit card (CC); 1-> one CC; 2-> CC and DC
  account_insurances insur;        // INSURANCE: 0-> home; 1-> 0 + house; 2-> 1 + life 
  account_character charact;       // 0-> psysical person; 1-> autonomous; 2-> juridic
  unsigned short comis_prod;       // Commision for the products hired by the customer
  unsigned short comis_rentab;     // Commision depending from the client type and mean balance
  unsigned short comis_total;      // Total commision for the account mainteinance
} account_t;

/************************************************************************************************
 *                                                                                              *
 *                           Declaración de las funciones del programa                          *
 *                                                                                              *
 ************************************************************************************************/

void error(char * error_name);
static void sig_quit (int signo);
void * calculate_comis_prod(void * threadid);
void * calculate_comis_rentab(void * threadid);
void * calculate_comis_total(void * threadid);
void * update(void * threadid);
void new_account(account_t * bank_account, char * name, unsigned long balance, 
                 int mort,  int bal, int card, int insur, int charact);
void print_accounts (account_t * accounts);

/************************************************************************************************
 *                                                                                              *
 *                     Declaración de las variables globales del programa                       *
 *                                                                                              *
 ************************************************************************************************/

float account_coefficients[NUMBER_OF_COEFFICIENTS];       // 0-> mortage coeff; 1-> balance coeff;
                                                          // 2-> card coeff; 3-> insurances coeff;
                                                          // 4-> character coeff
float last_account_coefficients[NUMBER_OF_COEFFICIENTS];
pthread_t tid[NUMBER_OF_THREADS];                         // There are 4 pthreads
account_t bank_accounts [NUMBER_OF_ACCOUNTS];
unsigned short initialize_update = 0, initialize_comis = 0, number_of_updates = 0;
pthread_mutex_t initialize_update_mutex, comis_updates, total_update;
pthread_cond_t sigquit_received, coefficients_updated, comis_updated;

/************************************************************************************************
 *                                                                                              *
 *                                      Función principal                                       *
 *                                                                                              *
 *      Actualmente inicializa el vector account_coefficients y crea tres cuentas bancarias     *
 *                                                                                              *
 ************************************************************************************************/

int main ()
{
  int err, i;

  printf("\nNumber of accounts: 3\nDefault value for coefficients vector: 1\n");
  printf("\nNote: Coefficients are calculated randomly based on time (seconds precision)\n");
  // Comportamiento al captar la señar SIGQUIT
  if (signal(SIGQUIT, sig_quit) == SIG_ERR)
    printf("Impossible catching SIGQUIT\n");

  // Default values initialization for the coefficients
  for (i = 0; i < NUMBER_OF_COEFFICIENTS; i++)
    account_coefficients[i] = DEFAULT_COEFFICIENT_VALUE;
  for (i = 0; i < NUMBER_OF_COEFFICIENTS; i++)
      last_account_coefficients[i] = DEFAULT_COEFFICIENT_VALUE;

  // There is going to be 3 accounts
  new_account(&bank_accounts[0], "Iser Chaminade", 1, 
              NO, UNCOVERED, NO_CARD, HOME, PHYSICAL);
  new_account(&bank_accounts[1], "MariadalIT RegaIT", 666, 
              ONE, LOW, CREDIT, HOME_HOUSE, PHYSICAL);
  new_account(&bank_accounts[2], "Chuck Norris", 50000000, 
              MORE, HIGH, CREDIT_DEBIT, HOME_HOUSE_LIFE, AUTONOMOUS);

  // Pthread creation
  if((err = pthread_create(&(tid[0]), NULL, calculate_comis_prod, NULL)) ||    // h_prod
     (err = pthread_create(&(tid[1]), NULL, calculate_comis_rentab, NULL)) ||  // h_rentab
     (err = pthread_create(&(tid[2]), NULL, calculate_comis_total, NULL)) ||   // h_total
     (err = pthread_create(&(tid[3]), NULL, update, NULL)))                    // h_update
    error("pthread_create");

  while(1);
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
 *                                       Función sig_quit                                       *
 *                                                                                              *
 * Implementa el comortamiento de la interrupción                                               *
 *                                                                                              *
 *                                                                                              *
 ************************************************************************************************/

static void sig_quit (int signo)
{
  printf("\tSIGQUIT PRESSED: \n");
  initialize_update = 1;
  pthread_cond_signal(&sigquit_received);
}

/************************************************************************************************
 *                                                                                              *
 *                                 Función calculate_comis_prod                                 *
 *                                                                                              *
 *                Aplica la comision de los productos contratados por el cliente                *
 *                                                                                              *
 ************************************************************************************************/

void * calculate_comis_prod(void * threadid)                           // Commented on the memory
{
  int i;
  while (1)
  {
    for (i = 0; i < NUMBER_OF_ACCOUNTS; i++) {
      bank_accounts[i].comis_prod = MAX_DISCOUNT*bank_accounts[i].mort/2*account_coefficients[0] +
                                    MAX_DISCOUNT*bank_accounts[i].card/2*account_coefficients[2] +
                                    MAX_DISCOUNT*bank_accounts[i].insur/2*account_coefficients[3];
    }

    number_of_updates++;

    pthread_cond_signal(&comis_updated);  

    pthread_mutex_lock(&comis_updates);

    while (initialize_comis == 0 || initialize_comis == 2)
      pthread_cond_wait(&coefficients_updated, &comis_updates);

    initialize_comis = 2;

    pthread_mutex_unlock(&comis_updates);
  }
}

/************************************************************************************************
 *                                                                                              *
 *                               Función calculate_comis_rentab                                 *
 *                                                                                              *
 *                 Aplica la comision segun el tipo de ciente y su saldo medio                  *
 *                                                                                              *
 ************************************************************************************************/

void * calculate_comis_rentab(void * threadid)                          // Commented on the memory
{
  int i;
  while (1)
  {
    for (i = 0; i < NUMBER_OF_ACCOUNTS; i++) {
        bank_accounts[i].comis_rentab = MAX_DISCOUNT*bank_accounts[i].bal/2*account_coefficients[1] +
                                     MAX_DISCOUNT*bank_accounts[i].charact/2*account_coefficients[4];
    }

    number_of_updates++;
    pthread_cond_signal(&comis_updated);

    pthread_mutex_lock(&comis_updates);

    while (initialize_comis == 0 || initialize_comis == 3)
      pthread_cond_wait(&coefficients_updated, &comis_updates);

    initialize_comis = 3;

    pthread_mutex_unlock(&comis_updates);
  }
}

/************************************************************************************************
 *                                                                                              *
 *                                Función calculate_comis_total                                 *
 *                                                                                              *
 *               Aplica la comision total: suma de las dos comisiones anteriores                *
 *                                                                                              *
 ************************************************************************************************/

void * calculate_comis_total(void * threadid)                           // Commented on the memory
{
  int i;
  while(1)
  {
    pthread_mutex_lock(&total_update);

    while (number_of_updates < 2)
      pthread_cond_wait(&comis_updated, &total_update);

    pthread_mutex_unlock(&total_update);

    for (i = 0; i < NUMBER_OF_ACCOUNTS; i++)
        bank_accounts[i].comis_total = bank_accounts[i].comis_prod + bank_accounts[i].comis_rentab;

    initialize_update = 0;
    number_of_updates = 0;
    initialize_comis = 0;

    print_accounts(bank_accounts);
    for (i = 0; i < NUMBER_OF_COEFFICIENTS; i++)
      last_account_coefficients[i] = account_coefficients[i];
  }
}

/************************************************************************************************
 *                                                                                              *
 *                                        Función update                                        *
 *                                                                                              *
 *               inicializa el proceso de actualización de comisiones y recalcula el            *
 *               valor de los coeficientes                                                      *
 *                                                                                              *
 ************************************************************************************************/

void * update(void * threadid)
{
  int i;
  while(1)
  {
    pthread_mutex_lock(&initialize_update_mutex);

    while (initialize_update == 0)
      pthread_cond_wait(&sigquit_received, &initialize_update_mutex);

    pthread_mutex_unlock(&initialize_update_mutex);
    initialize_comis = 1;                                                   // flag para h_update
    srand(time(NULL));
    for (i = 0; i < NUMBER_OF_COEFFICIENTS; i++)
        account_coefficients[i] = rand()/(double)RAND_MAX;
    pthread_cond_broadcast(&coefficients_updated);                         // señal para h_update
  }
}

/************************************************************************************************
 *                                                                                              *
 *                                     Función new_account                                      *
 *                                                                                              *
 *                Añade una nueva cuenta bancaria al vector de cuentas bancarias                *
 *                                                                                              *
 ************************************************************************************************/

void new_account (account_t * bank_account, char * name, unsigned long balance, int mort, 
                  int bal, int card, int insur, int charact)
{
  bank_account->owner = name;
  bank_account->balance = balance;
  bank_account->mort = mort;
  bank_account->bal = bal;
  bank_account->card = card;
  bank_account->insur = insur;
  bank_account->charact = charact;
}

void print_accounts (account_t * accounts)
{
  int k;
  printf("\n---------------------------------------------------------------------------------------------------------------------------------------------\n");
  printf("COEFFICIENTS      Mortage      Av Balance      Cards      Insurances      Character\n");
  printf("%-18s%-13.2f%-16.2f%-11.2f%-16.2f%-9.2f\n", "Last", last_account_coefficients[0], last_account_coefficients[1], last_account_coefficients[2],
         last_account_coefficients[3], last_account_coefficients[4]);
  printf("%-18s%-13.2f%-16.2f%-11.2f%-16.2f%-9.2f\n\n", "Actual", account_coefficients[0], account_coefficients[1], account_coefficients[2],
         account_coefficients[3], account_coefficients[4]);
  // Impresión de la cabecera de la tabla
  printf("ACCOUNTS   Owner                  Balance     Mortage   Av Balance   Cards   Insurances   Character   Comis Prod   Comis Rentab   Comis Total\n");
  // Filas
  for (k = 0; k < NUMBER_OF_ACCOUNTS; k++)
  {
    printf("Account %d", k + 1);
    printf("  %-23s%-12ld%-10d%-13d%-8d%-13d%-12d%-13d%-15d%-11d\n", accounts[k].owner, accounts[k].balance, accounts[k].mort, accounts[k].bal,
           accounts[k].card, accounts[k].insur, accounts[k].charact, accounts[k].comis_prod, accounts[k].comis_rentab, accounts[k].comis_total);
  }
  printf("---------------------------------------------------------------------------------------------------------------------------------------------\n\n");
}
