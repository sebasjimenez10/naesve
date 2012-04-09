#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main( int argc, char *arg[] )
{
	//Parametros para manejar el proceso suicida
	char * id;
	char * path;
	char * filename;
	int num;
	
	//Variables para la creacion del proceso suicida
	pid_t pid;
	int childValReturn;
	
	//Informacion del proceso control
	int idProcesoCtrl = (int) getpid() + 1;
	
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
			case 4: num = atoi( arg[cont] );
				break;
			default:
				break;
		}
		cont++;
	}	
	strcat( path, filename );
	if( num == 0 )
	{
		num = -1;
	}
	while ( 1 )
	{		
		/*
			Espacio para crear la tuberia
		*/
		pid = fork();
	
		if( pid == (pid_t)(-1) )
		{
			//printf( "El hijo suicida del proceso ctrl no se pudo crear\n" );
			exit( 1 );
		}else if ( pid == 0 )
		{
			//printf( "El hijo suicida del proceso ctrl fue creado\n" );
			execl( path, filename, NULL);
		}
		waitpid( pid, &childValReturn, 0  );
		
		printf( "Proceso suicida %s termino por causa %d -- Proceso Control %d, vidas restantes: %d\n", 
			id, childValReturn, idProcesoCtrl, num );
		
		if( num == 1 )
		{
			break;
		}else if( num > 1 ){
			num--;
		}
		
		/*
			Captura la razon por la que termino e informar
		*/
	}
	
	exit(EXIT_SUCCESS);
}
