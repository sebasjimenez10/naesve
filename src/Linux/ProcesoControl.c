
/*
	Proceso Control
	ProcesoControl.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TRUE 1

int main( int argc, char *arg[] )
{
	//Parametros para manejar el proceso suicida
	char * id;
	char * path;
	char * filename;
	int lives;
	int pconId = getpid() - 1;
	
	//Pipes
	int pipeParentRead[2];
	int pipeParentWrite[2];
	int pipeParentError[2];
	
	int cont = 1;
	while ( cont < argc )
	{
		switch( cont )
		{
			case 1: id = arg[cont];
				break;
			case 2: path = arg[cont];
				break;
			case 3: filename = arg[cont];
				break;
			case 4: lives = atoi( arg[cont] );
				break;
			default:
				break;
		}
		cont++;
	}
	char filenameCpy[200];
	strcpy( filenameCpy, filename );
	strcat( path, filenameCpy );
	
	if( lives == 0 )
	{
		lives = -1;
	}
	
	if( pipe( pipeParentRead ) || pipe( pipeParentWrite ) || pipe( pipeParentError ) ){
		/* Algo malo ocurrio */
	    	printf("Error: La tuberia no pudo ser creada");
	     exit(1);
	}
			
	while ( TRUE )
	{		
		/*
			Espacio para crear la tuberia
		*/
		pid_t pid;
		pid = fork();
		if( pid == (pid_t)(-1) )
		{
			//printf( "El hijo suicida del proceso ctrl no se pudo crear\n" );
			exit( 1 );
		}else if ( pid == 0 )
		{
			dup2( pipeParentWrite[0], 0 );
			close( pipeParentWrite[0] );
			close( pipeParentWrite[1] );
			
			dup2( pipeParentRead[1], 1);
			close( pipeParentRead[0] );
			close( pipeParentRead[1] );
			
			dup2( pipeParentError[1], 2 );
			close( pipeParentError[0] );
			close( pipeParentError[1] );
			
			//printf( "El hijo suicida del proceso ctrl fue creado\n" );
			execl( path, filenameCpy, NULL);
		}
		close( pipeParentWrite[0] );
		close( pipeParentRead[1] );
		close( pipeParentError[1] );
		
		FILE * out = fdopen(pipeParentRead[0], "r");
		/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
		//FILE * in = fdopen(pipeParentWrite[1], "w");
		FILE * error = fdopen(pipeParentError[0], "r");
		
		while(TRUE){
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == 0){
				// Still Alive
				char stringOut[100]; //Buffer Out
				char stringErr[100]; //Buffer Err
				while (fgets(stringOut, 100, out) != NULL) {
					fprintf(stdout, "%s", stringOut);
				}
				while (fgets(stringErr, 100, error) != NULL) {
					fprintf(stderr, "%s", stringErr);
				}
		 	}else{
		 		// Dead
		 		if( lives == -1 ){
		 			fprintf(stdout, "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: Infinitas\n",
						id, status, pconId );
				}else{
					fprintf(stdout, "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: %d\n",
						id, status, pconId, lives );
				}
				fflush(stdout);
				break;
		 	}
		}
		if( lives == 1 )
		{
			//Finish
			break;
		}else if( lives > 1 ){
			//Continue
			lives--;
		}
		
		/*
			Captura la razon por la que termino e informar
		*/
	}
	
	close(pipeParentRead[0]);
	close(pipeParentRead[1]);
	
	close(pipeParentWrite[0]);
	close(pipeParentWrite[1]);
	
	close(pipeParentError[0]);
	close(pipeParentError[1]);
	
	exit(EXIT_SUCCESS);
}

/*int status;*/
/*		waitpid(pid, &status, 0);*/
/*		fprintf(stdout, "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: %d\n",*/
/*					id, status, pconId, lives );*/
/*		fflush(stdout);*/
/*		if( lives == 1 )*/
/*		{*/
/*			break;*/
/*		}else if( lives > 1 ){*/
/*			lives--;*/
/*		}*/
