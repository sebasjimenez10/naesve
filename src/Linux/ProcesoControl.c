
/*
	Practica de Sistemas Operativos 2012-1
	
	Natalia Cano Escobar.
	Sebastian Jimenez Velez.
	
	Descripcion:
	El Proceso de Control es el que se encarga de "revivir" a los Procesos Suicidas
	que tienen una cantidad de vidas establecida.
	Esta cantidad puede ser un entero positivo o cero (Infinito).
		* Lo anterior se define en el archivo de configuracion.

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

//Constantes
#define TRUE 1
#define TAMANO  1024

//Estructura para el manejo de la opciones del programa
struct option long_options[] = {

	{"suicidename",   required_argument, 0, 'i' },
	{"filepath",      required_argument, 0, 'f' },
	{"filename",      required_argument, 0, 'n' },
	{"reencarnacion", required_argument, 0, 'r' },
	{0, 			   0, 			  0,  0  }
	
};

//Hilo que imprime del descriptor de archivo que le asignen al error estandar
//void * printStderr( void * file );

//Hilo que imprime del descriptor de archivo que le asignen a la salida estandar
//void * printStdout( void * file );

//Funcion para leer y escribir de manera multiplexada
int leerEscribir(fd_set* set, int in, int out);

//Funcion para validar el set de donde se ha leido
int validarError(fd_set* set, int in);

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
	
	//Si las vidas son 0 es porque vive infinito
	if( lifes == 0 )
	{
		lifes = -1;
	}
	
	//Ciclo en el cual se ejecutan los suicidas dado el numero de vidas o vidas infinitas
	while ( TRUE )
	{
		fd_set readfds, execptfds; //Conjuntos para los descriptores a leer (Multiplex)
		int nFds; //Variable que almacena el numero de descriptores de archivo que se pueden leer
		
		//Declaracion de las tuberias
		int pipeParentRead[2];
		int pipeParentWrite[2];
		int pipeParentError[2];

		//Se inician las tuberias, si alguna no pudo ser creada el programa termina
		if( pipe( pipeParentRead ) || pipe( pipeParentWrite ) || pipe( pipeParentError ) ){
		    	printf("Error: La tuberia no pudo ser creada");
			exit(1);
		}
		
		//Declaracion y creacion del hijo (Proceso Control)
		pid_t pid;
		pid = fork();
		
		//Se verifica que el hijo se haya podido crear
		if( pid == (pid_t)(-1) )
		{
			exit( 1 );
		}else if ( pid == 0 )
		{
			//Se cambian los descriptores de archivo del hijo
			dup2( pipeParentRead[1],1 );
			close( pipeParentRead[0] );
			close( pipeParentRead[1] );
			
			dup2( pipeParentError[1],2 );
			close( pipeParentError[0] );
			close( pipeParentError[1] );			
			
			dup2( pipeParentWrite[0],0 );
			close( pipeParentWrite[0] );
			close( pipeParentWrite[1] );
			
			//Se cambia la imagen del proceso del hijo
			execl( path, filename, NULL);
			
			//Si este codigo se ejecuta hubo un error cambiando la imagen
			fprintf( stderr, "Hubo un error al ejecutar el suicida\n");
		}
		//Codigo del padre
		//Se cierran los extremos de las tuberias que no se van a utilizar por el padre
		close( pipeParentRead[1] );
		close( pipeParentWrite[0] );	
		/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
		//FILE * in = fdopen(pipeParentWrite[1], "w");
		close( pipeParentError[1] );
		
/*		pthread_t threadOut, threadErr;*/
/*		int retOut, retErr;*/
/*		pthread_create( &threadErr, NULL, printStderr, (void *) pipeParentError[0] );*/
/*		pthread_create( &threadOut, NULL, printStdout, (void *) pipeParentRead[0] );*/

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

				//Para solucionar la salida combinada en la consola se utiliza un semaforo mutex
				nFds = select((int) pipeParentError[0] + 1, &readfds, NULL, &execptfds, NULL);
				
				//Si hay algun descriptor disponible para la operacion de E/S, esta se realiza
				if (nFds > 0) {
					if (leerEscribir(&readfds, pipeParentRead[0], 1) < 0 && errno != 0) {
						fprintf(stderr, "Error en la lectura pipeRead: %d %s\n", errno, strerror(errno));
						exit(1);
					}

					if (leerEscribir(&readfds, pipeParentError[0], 2) < 0 && errno != 0) {
						fprintf(stderr, "Error en la lectura pipeErr: %d %s\n", errno, strerror(errno));
						exit(1);
					}

					if (validarError(&execptfds, pipeParentRead[0]) < 0) {
						fprintf(stderr, "Error en la lectura validarError: %d %s\n", errno, strerror(errno));
						exit(1);
					}

					if (validarError(&execptfds, pipeParentError[0]) < 0) {
						fprintf(stderr, "Error en la lectura validarError: %d %s\n", errno, strerror(errno));
						exit(1);
					}	
				}
		 	}else if( result > 0 ){
		 		// El hijo ha muerto
		 		//Escribe en la tuberia la razon por la cual murio el suicida
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
		
		//Se cierran los extremos de las tuberias usadas por el padre
		close(pipeParentRead[0]);
		close(pipeParentWrite[1]);
		close(pipeParentError[0]);

/*		pthread_join( threadErr, (void **) &retErr );*/
/*		pthread_join( threadOut, (void **) &retOut );*/
		
		if( lifes == 1 )	
		{
			//Termina cuando vive las vidas que tenia
			break;
		}else if( lifes > 1 ){
			//Aun faltan vidas por vivir, continua viviendo
			lifes--;
		}
	}
	return 0;
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
/*	*/
/*	FILE * err;*/
/*	int fid = (int)file;*/
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
/*	FILE * out;*/
/*	int fid = (int)file;*/
/*	while( TRUE ){*/
/*		out = fdopen (fid, "r");*/
/*		if( out == NULL ) break;*/
/*		char stringOut[100]; //Buffer Err*/
/*		while (fgets(stringOut, 100, out) != NULL) {*/
/*			fprintf(stdout, "%s", stringOut);*/
/*		}*/
/*		fflush( stdout );*/
/*		fclose( out );*/
/*	}*/
/*	return 0;*/
/*}*/
