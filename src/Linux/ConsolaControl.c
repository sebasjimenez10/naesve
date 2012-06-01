
/*
	Consola de Control
	Name: ConsolaControl.c
*/

//Archivos de encabezado
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <semaphore.h>
// #include <ctype.h> Usado para operaciones con caracteres

//Constantes
#define PATH "../src/Common/ArchCfg.txt"
#define TRUE 1
#define TAMANO  1024

//Variable global para llevar la cuenta de los procesos control.
int pCtrlCount = 0;

//Semaforo para controlar la salida mezclada en la terminal
sem_t mutexTerminal;

//Estructura que empaqueta la informacion del proceso suicida
struct SuicideProcessInfo
{
	char * key;
	char * id;
	char * path;
	char * filename;
	char * lifes;
};

//Funcion que obtiene el numero de lineas de un archivo de texto
int getLineCount (  );

//Funcion asigna los datos del proceso suicida en las estructura definida para ello
struct SuicideProcessInfo setSuicideInfo( char * line );

//Hilo que imprime del descriptor de archivo que le asignen a la salida estandar
//void * printStdout( void * file );

//Hilo que imprime del descriptor de archivo que le asignen al error estandar
//void * printStderr( void * file );

//Funcion para leer y escribir multiplexado.
int leerEscribir(fd_set* set, int in, int out);

//Funcion para validar el set de donde se ha leido
int validarError(fd_set* set, int in);

//Funcion que transforma un int a un char
//Extraida de code.google.com/my_itoa/
int my_itoa(int val, char* buf);

//Hilo de consola que lanza el proceso control con la info del proceso suicida que debe controlar
void * hilo_de_consola( void *arg );

//Funcion principal leer el archivos de configuracion, lanzar los hilos y leer de las salidas estandar stdout, stdout
int main( ){

	int processCount = getLineCount();
	FILE * file = fopen(PATH, "r");
	sem_init( &mutexTerminal, 0, 1 );
	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	
	char line[processCount][200];
	struct SuicideProcessInfo suicides[processCount];
	int i = 0;
	
	while( i < processCount ){
		fgets( line[i], sizeof line, file );
		if( line[i][0] != '\n' ){ // Verifica que la linea al menos contenga algo diferente de un \n (Salto de linea)
			suicides[i] = setSuicideInfo( line[i] );
			i++;
		}
	}	
	fclose( file );
	
	pthread_t *threadTable = (pthread_t *) malloc(sizeof(pthread_t) * processCount);	
	for (i = 0; i < processCount; i++)
	{
		if( strcmp( suicides[i].key, "ProcesoSui" ) == 0 ){
			pthread_create((threadTable + i), NULL, hilo_de_consola, (void *) &suicides[i] );
		}
	}	
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
	struct SuicideProcessInfo * pi = (struct SuicideProcessInfo *) arg;
	
	fd_set readfds, execptfds; //Multiplex
	int nFds; //Multiplex
	
	char suiOpt[200] = "--suicidename=";
	char filepathOpt[200] = "--filepath=";
	char filenameOpt[200] = "--filename=";
	char reencOpt[200] = "--reencarnacion=";
	char PctrlId[15];
	
	strcat( suiOpt, pi->id );
	strcat( filepathOpt, pi->path );
	strcat( filenameOpt, pi->filename );
	strcat( reencOpt, pi->lifes );
	
	my_itoa( pCtrlCount, PctrlId );
	pCtrlCount++;
	
	int pipeParentRead[2];
	int pipeParentWrite[2];
	int pipeParentError[2];
	
	if( pipe( pipeParentRead ) || pipe( pipeParentWrite ) || pipe( pipeParentError ) ){
		/* Algo malo ocurrio */
	    printf("Error: La tuberia no pudo ser creada");
	    exit(1);
	}
	
	pid_t pid;	
	pid = fork();
	
	if( pid == (pid_t)(-1) )
	{
		fprintf( stdout, "El hijo del hilo no se pudo crear\n" );
		pthread_exit(NULL);
	}else if ( pid == 0 )
	{	
		dup2( pipeParentWrite[0],0 );
		close( pipeParentWrite[0] );
		close( pipeParentWrite[1] );
		
		dup2( pipeParentRead[1],1 );
		close( pipeParentRead[0] );
		close( pipeParentRead[1] );
		
		dup2( pipeParentError[1],2 );
		close( pipeParentError[0] );
		close( pipeParentError[1] );
		
		execl("procesoctrl",
			"procesoctrl", suiOpt, filepathOpt, filenameOpt, reencOpt, PctrlId, NULL);

		fprintf( stderr, "Error cambiando imagen de proceso\n" );
	}
	
	close( pipeParentRead[1] );	
	close( pipeParentWrite[0] );	
	/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
	//FILE * in = fdopen(pipeParentWrite[1], "w");	
	close( pipeParentError[1] );
	
/*	pthread_t threadOut, threadErr;*/
/*	int retErr, retOut;*/
/*	pthread_create( &threadOut, NULL, printStdout, (void *) pipeParentRead[0] );*/
/*	pthread_create( &threadErr, NULL, printStderr, (void *) pipeParentError[0] );*/
	
	while(TRUE){
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == 0){
			// Still Alive			
			FD_ZERO(&readfds);
			FD_ZERO(&execptfds);

			FD_SET(pipeParentRead[0], &readfds);
			FD_SET(pipeParentError[0], &readfds);
			FD_SET(pipeParentRead[0], &execptfds);
			FD_SET(pipeParentError[0], &execptfds);

			nFds = select((int) pipeParentError[0] + 1, &readfds, NULL, &execptfds, NULL);
			
			sem_wait( &mutexTerminal );
			if (nFds > 0) {
				if (leerEscribir(&readfds, pipeParentRead[0], 0) < 0 && errno != 0) {
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
	 		// Dead
	 		//fflush(stdout);
	 		break;
	 	}
	}
	
	close(pipeParentRead[0]);
	close(pipeParentWrite[1]);
	close(pipeParentError[0]);
	
/*	pthread_join( threadErr, (void **) &retErr );*/
/*	pthread_join( threadOut, (void **) &retOut );*/

	usleep(200 * 1000);
	return 0;
}

int getLineCount (  ){
	FILE * file = fopen(PATH, "r");
	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	int count = 0;
	char line[200];
	while( fgets( line, sizeof(line), file ) != NULL ){
		if ( line[0] != '\n' )
		{
			count++;
		}
	}
	fclose( file );	
	return count;
}

struct SuicideProcessInfo setSuicideInfo( char * line ){
	struct SuicideProcessInfo info;
	char * tokensArray[5];
	char * delim = " {}:";
	char * token;
	
	int index = 0;
	token = strtok( line, delim );
	while ( token != NULL )
	{
		tokensArray[index] = token;
		token = strtok( NULL, delim );
		index++;
	}	
	
	info.key = tokensArray[0];
	info.id = tokensArray[1];
	info.path = strcat( tokensArray[2], "/" );
	info.filename = tokensArray[3];
	info.lifes = tokensArray[4];
	
	return info;
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
      }
      else 
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
