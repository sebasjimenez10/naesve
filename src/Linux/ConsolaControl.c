
/*	
	Practica de Sistemas Operativos 2012-1
	
	Natalia Cano Escobar.
	Sebastian Jimenez Velez.	
	
	Descripcion:
	La Consola de Control es la encargada de crear todos los Procesos de Control
	que se encargan de monitorear y de "revivir" los Procesos Suicidas.
	Por medio de hilos de consola, esta, crea los Procesos de Control.
	
	Consola de Control
	Nombre: ConsolaControl.c
*/

//Archivos de encabezado
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <semaphore.h>

//Constantes
#define PATH "../src/Common/ArchCfg.txt"
#define TRUE 1
#define TAMANO  1024

//Para la validacion del archivo de configuracion (Parsing)
extern int processCount;
extern int yyparse();
extern FILE * yyin;

//Variable global para llevar la cuenta de los procesos control.
int pCtrlCount = 0;

//Semaforo para controlar la salida mezclada en la terminal
sem_t mutexTerminal;

//Estructura que empaqueta la informacion del proceso suicida
struct SuicideProcessInfo
{
	char * id;
	char * path;
	char * filename;
	char * lifes;
};

//Aqui se depositan los suicidas que va reconociendo el parser
struct SuicideProcessInfo suicidas[1024];

//Cuenta para el vector de suicidas
int count = 0;

//Funcion utilizada desde el parser cuando se termina de validar un suicida, esta regresa los tokens.
void * getTokens( char * id, char * path, char * filename, char * lifes );

//Hilo que imprime del descriptor de archivo que le asignen a la salida estandar
//void * printStdout( void * file );

//Hilo que imprime del descriptor de archivo que le asignen al error estandar
//void * printStderr( void * file );

//Funcion para leer y escribir de manera multiplexada
int leerEscribir(fd_set* set, int in, int out);

//Funcion para validar el set de donde se ha leido
int validarError(fd_set* set, int in);

//Funcion que transforma un int a un char
//Extraida de http://code.google.com/my_itoa/
int my_itoa(int val, char* buf);

//Hilo de consola que lanza el proceso control con la info del proceso suicida que debe controlar
void * hilo_de_consola( void *arg );

//Funcion principal leer el archivos de configuracion, lanzar los hilos y leer de las salidas estandar stdout, stdout
int main( ){
	//Se realiza la validacion del archivo de configuracion y se obtienen los tokens
	yyin = fopen( PATH, "r" );
	if( yyin == NULL ){
		printf( "Error al abrir el archivo de configuracion.\n" );
		exit(1);
	}
	int parseStatus = yyparse(); //Llama a la funcion que valida el archivo y almacena el resultado de la validacion en in int
	if( parseStatus == 1 ){ //Si hubo un error no se puede continuar y se termina el programa.
		//El archivo contiene errores de sintaxis
		exit(1);
	}
	//Se inicia la consola control
	sem_init( &mutexTerminal, 0, 1 ); //Semaforo utilizado para controlar la salida combinada en la terminal
	
	//Lanzamiento de los hilos de consola que inician los procesos control
	int i = 0;	
	pthread_t *threadTable = (pthread_t *) malloc(sizeof(pthread_t) * processCount);	
	for (i = 0; i < processCount; i++)
	{
		pthread_create((threadTable + i), NULL, hilo_de_consola, (void *) &suicidas[i] );
	}	
	//Se espera por cada hilo lanzado
	int returnValue;	
	for (i = 0; i < processCount; i++)
	{
		pthread_join(*(threadTable + i), (void **) &returnValue);
		fprintf( stdout, "El hilo %d termino en estado: %d\n", i, returnValue );
	}	
	return 0;
}

void * hilo_de_consola( void *arg )
{
	//Se recibe la estructura pasada como argumento
	struct SuicideProcessInfo * pi = (struct SuicideProcessInfo *) arg;
	
	fd_set readfds, execptfds; //Conjuntos para los descriptores a leer (Multiplex)
	int nFds; //Variable que almacena el numero de descriptores de archivo que se pueden leer
	
	//Creacion de las opciones
	char suiOpt[200] = "--suicidename=";
	char filepathOpt[200] = "--filepath=";
	char filenameOpt[200] = "--filename=";
	char reencOpt[200] = "--reencarnacion=";
	char PctrlId[15];
	
	//Concatenacion de las opciones con los parametros de los suicidas
	strcat( suiOpt, pi->id );
	strcat( filepathOpt, pi->path );
	strcat( filenameOpt, pi->filename );
	strcat( reencOpt, pi->lifes );
	
	//Por medio de la funcion my_itoa se convierte el entero id del proceso control a char para pasarlo por parametro
	my_itoa( pCtrlCount, PctrlId );
	pCtrlCount++;
	
	//Declaracion de las tuberias
	int pipeParentRead[2];
	int pipeParentWrite[2];
	int pipeParentError[2];
	
	//Se inician las tuberias, si alguna no pudo ser creada el programa termina
	if( pipe( pipeParentRead ) || pipe( pipeParentWrite ) || pipe( pipeParentError ) ){
		/* Algo malo ocurrio */
	    printf("Error: La tuberia no pudo ser creada");
	    exit(1);
	}
	
	//Declaracion y creacion del hijo (Proceso Control)
	pid_t pid;	
	pid = fork();
	
	//Se verifica que el hijo se haya podido crear
	if( pid == (pid_t)(-1) )
	{
		fprintf( stdout, "El hijo del hilo no se pudo crear\n" );
		pthread_exit(NULL);
	}else if ( pid == 0 )
	{
		//Se cambian los descriptores de archivo del hijo
		dup2( pipeParentWrite[0],0 );
		close( pipeParentWrite[0] );
		close( pipeParentWrite[1] );
		
		dup2( pipeParentRead[1],1 );
		close( pipeParentRead[0] );
		close( pipeParentRead[1] );
		
		dup2( pipeParentError[1],2 );
		close( pipeParentError[0] );
		close( pipeParentError[1] );
		
		//Se cambia la imagen del proceso del hijo
		execl("procesoctrl",
			"procesoctrl", suiOpt, filepathOpt, filenameOpt, reencOpt, PctrlId, NULL);
		
		//Si este codigo se ejecuta hubo un error cambiando la imagen
		fprintf( stderr, "Error cambiando imagen de proceso\n" );
		exit(1);
	}
	//Codigo del padre
	//Se cierran los extremos de las tuberias que no se van a utilizar por el padre
	close( pipeParentRead[1] );	
	close( pipeParentWrite[0] );	
	/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
	//FILE * in = fdopen(pipeParentWrite[1], "w");
	close( pipeParentError[1] );
	
/*	pthread_t threadOut, threadErr;*/
/*	int retErr, retOut;*/
/*	pthread_create( &threadOut, NULL, printStdout, (void *) pipeParentRead[0] );*/
/*	pthread_create( &threadErr, NULL, printStderr, (void *) pipeParentError[0] );*/
	
	//Se verifica el estado del hijo hasta que este muera, para leer lo que este escribiendo en la salida o el error
	while(TRUE){
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == 0){
			// El hijo todavia esta vivo
			//Se limpian los conjuntos			
			FD_ZERO(&readfds);
			FD_ZERO(&execptfds);
			
			//Se agregan los extremos correspondientes de las tuberias a los conjuntos de descriptores
			FD_SET(pipeParentRead[0], &readfds);
			FD_SET(pipeParentError[0], &readfds);
			FD_SET(pipeParentRead[0], &execptfds);
			FD_SET(pipeParentError[0], &execptfds);

			//Obtiene la cantidad de descriptores de los cuales se puede leer
			nFds = select((int) pipeParentError[0] + 1, &readfds, NULL, &execptfds, NULL);
			
			//Para solucionar la salida combinada en la consola se utiliza un semaforo mutex
			sem_wait( &mutexTerminal ); // Ingreso a la zona critica
			
			//Si hay algun descriptor disponible para la operacion de E/S, esta se realiza
			if (nFds > 0) {
				if (leerEscribir(&readfds, pipeParentRead[0], 1) < 0 && errno != 0) {
					fprintf(stderr, "Error en la lectura: %d %s\n", errno, strerror(errno));
					exit(1);
				}

				if (leerEscribir(&readfds, pipeParentError[0], 2) < 0 && errno != 0) {
					fprintf(stderr, "Error en la lectura: %d %s\n", errno, strerror(errno));
					exit(1);
				}

				if (validarError(&execptfds, pipeParentRead[0]) < 0) {
					fprintf(stderr, "Error en la lectura: %d %s\n", errno, strerror(errno));
					exit(1);
				}

				if (validarError(&execptfds, pipeParentError[0]) < 0) {
					fprintf(stderr, "Error en la lectura: %d %s\n", errno, strerror(errno));
					exit(1);
				}	
			}
			sem_post( &mutexTerminal );		
	 	}else if( result > 0 ){
	 		// El hijo ha muerto
	 		break;
	 	}
	}
	
	//Se cierran los extremos de las tuberias usadas por el padre
	close(pipeParentRead[0]);
	close(pipeParentWrite[1]);
	close(pipeParentError[0]);
	
/*	pthread_join( threadErr, (void **) &retErr );*/
/*	pthread_join( threadOut, (void **) &retOut );*/

	usleep(200 * 1000);
	return 0;
}

void * getTokens( char * id, char * path, char * filename, char * lifes ){
	struct SuicideProcessInfo suicida;
	
	suicida.id = id;
	strcat( path, "/");
	suicida.path = path;
	suicida.filename = filename;
	suicida.lifes = lifes;	
	suicidas[count] = suicida;	
	count++;
	
	return NULL;
}

int my_itoa(int val, char* buf)
{
    const unsigned int radix = 10;
    char* p;
    unsigned int a;      //every digit
    int len;
    char* b;            	//start of the digit char
    char temp;
    unsigned int u;

    p = buf;

    if (val < 0)
    {
        *p++ = '-';
        val = 0 - val;
    }
    u = (unsigned int)val;
    b = p;
    do
    {
        a = u % radix;
        u /= radix;
        *p++ = a + '0';

    } while (u > 0);

    len = (int)(p - buf);
    *p-- = 0;
    //swap
    do
    {
        temp = *p;
        *p = *b;
        *b = temp;
        --p;
        ++b;

    } while (b < p);

    return len;
}

int leerEscribir(fd_set* set, int in, int out) {
  static char *buffer = NULL;
  int nCLeidos;

  if (!buffer) {
    buffer = (char *) malloc(TAMANO);
    if (!buffer) {
      return -1;
    }
  }

  if (FD_ISSET(in, set)) {
    
    do {

      nCLeidos = read(in, buffer, TAMANO);
      if (nCLeidos > 0) {
		if (write(out, buffer, nCLeidos) < 0) {
		  return -1;
		}
      } else 
		return -1;
    } while (nCLeidos == TAMANO);
  }
  return 0;
}

int validarError(fd_set* set, int in) {
  char buffer[1];

  if (FD_ISSET(in, set)) {
    if (read(in, buffer, 1) < 0) {
      return -1;
    }
  }
  return 0;
}

/*void * printStderr( void * file ){*/
/*	FILE * err;*/
/*	int fid = (int) file;*/
/*	*/
/*	while( TRUE ){*/
/*		err = fdopen (fid, "r");*/
/*		if( err == NULL ) break;*/
/*		char stringErr[100]; //Buffer Err*/
/*		while (fgets(stringErr, 100, err) != NULL) {*/
/*			fprintf(stderr, "%s", stringErr);*/
/*		}*/
/*		fflush( stderr );*/
/*		fclose( err );*/
/*	}*/
/*	return 0;*/
/*}*/

/*void * printStdout( void * file ){*/
/*	*/
/*	FILE * out;*/
/*	int fid = (int) file;*/
/*	*/
/*	while( TRUE ){*/
/*		out = fdopen (fid, "r");*/
/*		if( out == NULL ) break;*/
/*		char stringOut[100]; //Buffer Out*/
/*		while (fgets(stringOut, 100, out) != NULL) {*/
/*			fprintf(stdout, "%s", stringOut);*/
/*		}*/
/*		fflush( stdout );*/
/*		fclose( out );*/
/*	}*/
/*	return 0;*/
/*}*/
