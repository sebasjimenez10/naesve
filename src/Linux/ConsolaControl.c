
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
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

//Constantes
#define PATH "../src/Common/ArchCfg.txt"
#define TRUE 1
#define SEM_NAME "/sem_valSeq"

//Estructura que empaqueta la informacion del proceso suicida
struct SuicideProcessInfo
{
	char * key;
	char * id;
	char * path;
	char * filename;
	char * lifes;
};

//Estructura que almacena la informacion para la estadistica de cada proceso suicida
struct InfoMuerte {
	long int seq;
	int nDecesos;
};

//Estructura para la memoria compartida
struct MemoriaCompartida {
	int n; // Numero de procesos controladores
	long int valSeq;
	struct InfoMuerte muertes[254]; // Cada entrada identifica la informacion de cada proceso suicida.
};

//Mutexes
sem_t * mutexValSeq;
sem_t mutexOut;
sem_t mutexErr;

//Funcion que obtiene el numero de lineas de un archivo de texto
int getLineCount (  );

//Funcion asigna los datos del proceso suicida en las estructura definida para ello
struct SuicideProcessInfo setSuicideInfo( char * line );

//Hilo que imprime del descriptor de archivo que le asignen a la salida estandar
void * printStdout( void * file );

//Hilo que imprime del descriptor de archivo que le asignen al error estandar
void * printStderr( void * file );

//Hilo de consola que lanza el proceso control con la info del proceso suicida que debe controlar
void * hilo_de_consola( void *arg );

//Funcion para pasar un entero a un string
int my_itoa(int val, char* buf);	/*	Extraido de: http://code.google.com/p/my-itoa/  */

//Id del segmento de memoria
int idSegmento;

//Lleva la cuenta de los procesos control que lanza
int proCtrlCount = 0;

//Funcion principal leer el archivos de configuracion, lanzar los hilos y leer de las salidas estandar stdout, stdout
int main( ){
	int processCount = getLineCount();	//Cuenta los procesos suicidas que hay en el archivo de configuracion.
	sem_init( &mutexOut, 0, 1 );	// Inicializa el semaforo usado para la salida.
	sem_init( &mutexErr, 0, 1 );	// Inicializa el semaforo usado para el error.
	mutexValSeq = sem_open( SEM_NAME,
						O_CREAT,
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
						0 ); // Crea el semaforo nombrado como un semaforo de Sinc
	sem_post( mutexValSeq ); // Pone el semaforo nombrado en 1
	key_t key = 5677; //Llave para el segmento de memoria compartida
	struct MemoriaCompartida * varInfo; // Variable para la memoria compartida
	
	//Crea la memoria compartida
	if ((idSegmento = shmget(key, (size_t )getpagesize(), IPC_CREAT | 0660 )) < 0)
	{
		fprintf(stderr, "Fallo al crear el segmento de memoria debido a: %d %s\n", errno, strerror(errno));
		exit(1);
	}
	if ((varInfo = (struct MemoriaCompartida *) shmat(idSegmento, 0, 0)) == (void *) 0)
	{
		fprintf(stderr, "No pudo ser asignado el segmento de memoria: %d %s\n", errno, strerror(errno));
		exit(1);
	}
	
	//Inicializa la memoria compartida
	varInfo->n = processCount;
	varInfo->valSeq = 0;	
	int j;
	for (j = 0; j < processCount; j++)
	{
		varInfo->muertes[j].seq = 0;
		varInfo->muertes[j].nDecesos = 0;
	}
	
	//Abre el archivo para obtener cada linea y obtener la informacion de cada linea.
	FILE * file = fopen(PATH, "r");
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
	pthread_t *threadTable = (pthread_t *) malloc(sizeof(pthread_t) * processCount);	// Tabla de hilos
	
	// Lanza un hilo con la informacion de cada proceso suicida identificado.
	for (i = 0; i < processCount; i++)
	{
		if( strcmp( suicides[i].key, "ProcesoSui" ) == 0 ){
			pthread_create((threadTable + i), NULL, hilo_de_consola, (void *) &suicides[i] );
		}
	}	
	
	//Espera por cada hilo
	int returnValue;	
	for (i = 0; i < processCount; i++)
	{
		pthread_join(*(threadTable + i), (void **) &returnValue);
		fprintf( stdout, "El hilo %d termino en estado: %d\n", i, returnValue );

	}
	
	//Muestra en la terminal la informacion final de la memoria compartida
	for (j = 0; j < processCount; j++)
	{
		printf("Estadistica del proceso control %d:\nSecuencia Final: %ld -- Decesos Totales: %d\n",
			j, varInfo->muertes[j].seq, varInfo->muertes[j].nDecesos);
	}
	
	//Libera el segmento de memoria compartida
	if (shmctl(idSegmento, IPC_RMID, NULL) < 0) {
		fprintf(stderr, "Fallo al borrar el segmento de memoria debido a: %d %s\n", errno, strerror(errno));
		exit(1);
	}	
	return 0;
}

//-------------------------------------------------------- DEFINICION DE FUNCIONES -----------------------------------------------------

int getLineCount (  )
{
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

struct SuicideProcessInfo setSuicideInfo( char * line )
{
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

void * printStdout( void * file )
{
	
	FILE * out;
	int fid = (int) file;
	
	while( TRUE ){
		out = fdopen (fid, "r");
		if( out == NULL ) break;
		char stringOut[100]; //Buffer Out
		
		while (fgets(stringOut, 100, out) != NULL) {
			sem_wait(&mutexOut);
			fprintf(stdout, "%s", stringOut);
			sem_post(&mutexOut);
		}
		fflush( stdout );
		fclose( out );
	}
	return 0;
}

void * printStderr( void * file )
{
	FILE * err;
	int fid = (int) file;
	
	while( TRUE ){
		err = fdopen (fid, "r");
		if( err == NULL ) break;
		char stringErr[100]; //Buffer Err
		while (fgets(stringErr, 100, err) != NULL) {
			sem_wait(&mutexErr);
			fprintf(stderr, "%s", stringErr);
			sem_post(&mutexErr);
		}
		fflush( stderr );
		fclose( err );
	}
	return 0;
}

void * hilo_de_consola( void *arg )
{
	struct SuicideProcessInfo * pi = (struct SuicideProcessInfo *) arg;
	
	char suiOpt[200] = "--suicidename=";
	char filepathOpt[200] = "--filepath=";
	char filenameOpt[200] = "--filename=";
	char reencOpt[200] = "--reencarnacion=";
	char idMemoria[200] = "--idMemoria=";
	char idSemaphore[200] = "--idSemaforoMemoria=";
	
	char bufferID[15];
	my_itoa( idSegmento, bufferID);
	
	char idProCtrl[15];
	my_itoa( proCtrlCount, idProCtrl);
	proCtrlCount++;
	
	strcat( suiOpt, pi->id );
	strcat( filepathOpt, pi->path );
	strcat( filenameOpt, pi->filename );
	strcat( reencOpt, pi->lifes );	
	strcat( idMemoria, bufferID);
	strcat( idSemaphore, SEM_NAME );
	
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
		
		execl("../bin/procesoctrl",
			"procesoctrl", suiOpt, filepathOpt, filenameOpt, reencOpt, idMemoria, idSemaphore, idProCtrl, NULL);

		fprintf( stderr, "Error cambiando imagen de proceso\n" );
	}
	
	close( pipeParentRead[1] );	
	close( pipeParentWrite[0] );	
	/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
	//FILE * in = fdopen(pipeParentWrite[1], "w");	
	close( pipeParentError[1] );
	
	pthread_t threadOut, threadErr;
	int retErr, retOut;
	pthread_create( &threadOut, NULL, printStdout, (void *) pipeParentRead[0] );
	pthread_create( &threadErr, NULL, printStderr, (void *) pipeParentError[0] );
	
	int status;
	waitpid(pid, &status, 0);
	
	close(pipeParentRead[0]);
	close(pipeParentWrite[1]);
	close(pipeParentError[0]);
	
	pthread_join( threadErr, (void **) &retErr );
	pthread_join( threadOut, (void **) &retOut );

	usleep(200 * 1000);
	return 0;
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
