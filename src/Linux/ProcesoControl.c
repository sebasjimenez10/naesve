
/*
	Proceso Control
	Name: ProcesoControl.c
*/

//Archivos de encabezado
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

//Constantes
#define TRUE 1

//Estructura para el manejo de la opciones del programa
struct option long_options[] = {

	{"suicidename",   required_argument, 0, 'i' },
	{"filepath",      required_argument, 0, 'f' },
	{"filename",      required_argument, 0, 'n' },
	{"reencarnacion", required_argument, 0, 'r' },
	{0, 			   0, 			  0,  0  }
	
};

//Hilo que imprime del descriptor de archivo que le asignen al error estandar
void * printStderr( void * file );

//Hilo que imprime del descriptor de archivo que le asignen a la salida estandar
void * printStdout( void * file );

//Funcion principal encargada de lanzar el proceso suicida y hacerlo revivir tambien lee de las salidas estandar stdout, stderr
int main( int argc, char *argv[] )
{
	//Parametros para manejar el proceso suicida
	char * id;
	char * path;
	char * filename;
	int lifes = -1;
	int pConId = atoi( argv[5] );
	
	int c;
	int opt_ind = 0;
	
	while ( (c = getopt_long( argc, argv, "", long_options, &opt_ind )) != -1 )
	{
		switch( c ){
			case 'i': id = optarg;
				break;
			case 'f': path = optarg;
				break;
			case 'n' : filename = optarg;
				break;
			case 'r' : lifes = atoi( optarg );
				break;
			default:
				return 0;
		}
	}
	if( id != NULL && path != NULL && filename != NULL && lifes != -1  ){
		//Todos los args estan bien
		//fprintf( stdout, "Suicide ID: %s\nSuicide Path: %s\nSuicide Filename: %s\nSuicide Lifes: %d\n", id, path, filename, lifes);
	}else{
		fprintf( stderr, "Error en los argumentos\n");
		exit( EXIT_FAILURE );
	}
	strcat( path, filename );
	
	if( lifes == 0 )
	{
		lifes = -1;
	}
			
	while ( TRUE )
	{
		//Pipes
		int pipeParentRead[2];
		int pipeParentWrite[2];
		int pipeParentError[2];

		if( pipe( pipeParentRead ) || pipe( pipeParentWrite ) || pipe( pipeParentError ) ){
		    	printf("Error: La tuberia no pudo ser creada");
			exit(1);
		}
		
		pid_t pid;
		pid = fork();
		if( pid == (pid_t)(-1) )
		{
			exit( 1 );
		}else if ( pid == 0 )
		{
			dup2( pipeParentRead[1],1 );
			close( pipeParentRead[0] );
			close( pipeParentRead[1] );
			
			dup2( pipeParentError[1],2 );
			close( pipeParentError[0] );
			close( pipeParentError[1] );			
			
			dup2( pipeParentWrite[0],0 );
			close( pipeParentWrite[0] );
			close( pipeParentWrite[1] );
			
			execl( path, filename, NULL);
			
			fprintf( stderr, "Hubo un error al ejecutar el suicida\n");
		}
		
		close( pipeParentRead[1] );
		close( pipeParentWrite[0] );	
		/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
		//FILE * in = fdopen(pipeParentWrite[1], "w");
		close( pipeParentError[1] );
		
		pthread_t threadOut, threadErr;
		int retOut, retErr;
		pthread_create( &threadErr, NULL, printStderr, (void *) pipeParentError[0] );
		pthread_create( &threadOut, NULL, printStdout, (void *) pipeParentRead[0] );
		
		while(TRUE){
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == 0){
				// Still Alive
		 	}else if( result > 0 ){
		 		// Dead
		 		if( lifes == -1 ){
		 			fprintf(stdout, "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: Infinitas\n",
						id, status, pConId );
				}else{
					fprintf(stdout, "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: %d\n",
						id, status, pConId, lifes );
				}
		 		fflush(stdout);
				break;
		 	}
		}
		
		close(pipeParentRead[0]);
		close(pipeParentWrite[1]);
		close(pipeParentError[0]);

		pthread_join( threadErr, (void **) &retErr );
		pthread_join( threadOut, (void **) &retOut );
		
		if( lifes == 1 )	
		{
			//Finish
			break;
		}else if( lifes > 1 ){
			//Continue
			lifes--;
		}
	}
	return 0;
}

void * printStderr( void * file ){
	
	FILE * err;
	int fid = (int)file;
	while( TRUE ){
		err = fdopen (fid, "r");
		if( err == NULL ) break;
		char stringErr[100]; //Buffer Err
		while (fgets(stringErr, 100, err) != NULL) {
			fprintf(stderr, "%s", stringErr);
		}
		fflush( stderr );
		fclose( err );
	}
	return 0;
}

void * printStdout( void * file ){
	FILE * out;
	int fid = (int)file;
	while( TRUE ){
		out = fdopen (fid, "r");
		if( out == NULL ) break;
		char stringOut[100]; //Buffer Err
		while (fgets(stringOut, 100, out) != NULL) {
			fprintf(stdout, "%s", stringOut);
		}
		fflush( stdout );
		fclose( out );
	}
	return 0;
}
