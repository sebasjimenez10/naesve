/*

	Este es el codigo de la consola de control, encargado de crear lo hilos de consola
	con la informacion necesaria para que estos creen los procesos controladores que se
	encargan de creear los procesos suicidas.

*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>


//Estructura que contiene la informacion de un proceso suicida
struct processInfo
{
	char *id;
	char *path;
	char *filename;
	char *num;
};

void *hilo_de_consola( void *arg );

int main (){
	//Usados para Abrir y Leer el archivo
	FILE *fp;
	size_t len = 0;
	ssize_t read;
	char *line = NULL;

	//Usados para hacer los tokens
	char *token = NULL;
	char *saveptr;
	char *separ = " {}:";
	
	//Usado para contar cuantos procesos suicidas hay
	int cantProc = 0;

	//Abre el archivo con un PATH y un modo
	fp = fopen("ArchCfg.txt", "r");
	if( fp == NULL ){
		printf("El archivo no existe");
		exit(EXIT_FAILURE);
	}else{
		//Lee mientras que hayan lineas en el archivo
		while((read = getline(&line, &len, fp)) != EOF){
			cantProc++;
		}
		
		//Arreglo con los tokens de cada linea
		char *arregloTokens[5];
		
		//Se crea la tabla de hilos
		pthread_t *threadTable = (pthread_t *) malloc(sizeof(pthread_t) * cantProc);
		
		//Crea la estructura para almacenar la info del ProcesoSui
		struct processInfo *pi = malloc(sizeof(struct processInfo));
		
		//Cierra el archivo y lo vuelve a abrir para obtener concretamente el contenido de las lineas
		close((int)fp);
		fp = fopen("ArchCfg.txt", "r");
		
		while((read = getline(&line, &len, fp)) != EOF){
			int index = 0;
			token = strtok_r(line, separ, &saveptr);
			arregloTokens[index] = token;
			while(token != NULL){
				token = strtok_r(NULL, separ, &saveptr);
				if( token != NULL ){
					//Si token = NULL pregunta por una dir que no existe. (Segmentation Fault)
					index++;
					arregloTokens[index] = token;
				}
			}
			int k = 0; //Para el arreglo de tokens
			int j = 0; //Para los hilos
			int arrSize = sizeof(arregloTokens)/sizeof(int);
			if( strcmp(arregloTokens[k], "ProcesoSui") == 0 ){
				printf("!Proceso Suicida Detectado!\n");
				k++;
				while( k < arrSize ){
					switch( k ){
						case 1: pi->id = arregloTokens[k];
							break;
						case 2: pi->path = strcat( arregloTokens[k], "/");
							break;
						case 3: pi->filename = arregloTokens[k];
							break;
						case 4: pi->num = arregloTokens[k];
							break;
						default:
							break;
					}
					k++;
				}
			}
			pthread_create((threadTable + j), NULL, hilo_de_consola, (void *) pi);
			pthread_join(*(threadTable + j), NULL);
			j++;
		}
		free(line);
		free(token);
	}
	//Cierra el archivo
	close((int)fp);

	//Termina con exito
	exit(EXIT_SUCCESS);
}

void *hilo_de_consola( void *arg )
{

	struct processInfo *pi = (struct processInfo*) arg;
	int pipeline[2];
	
	if ( pipe(pipeline) < 0) {

	    /* Algo malo ocurrio */
	    printf("Error: La tuberia no pudo ser creada");
	    exit(1);
  	}
	
	pid_t pid;	
	pid = fork();
	
	if( pid == (pid_t)(-1) )
	{
		printf( "El hijo del hilo no se pudo crear" );
		printf( "\n" );
		pthread_exit(NULL);
	}else if ( pid == 0 )
	{
		printf( "El hijo del hilo fue creado" );
		printf( "\n" );
		
		close(1);
		dup2( pipeline[1], 1 );
		close( pipeline[0] );
		
		execl("procesoctrl", 
			"procesoctrl", pi->id, pi->path, pi->filename, pi->num, NULL);
	}
	
	/*
		Problema con la lectura del pipe
	*/
	
	char buf[256 * atoi( pi->num )];
	read( pipeline[0], &buf, sizeof(buf) );
	printf( "%s\n", buf);
	
	
	close(pipeline[0]);
	close(pipeline[1]);
	
	
	usleep(900 * 1000);
	pthread_exit(NULL);
}
/*	
	printf("%s\n", pi->id);
	printf("%s\n", pi->path);
	printf("%s\n", pi->filename);
	printf("%s\n", pi->num);
*/
