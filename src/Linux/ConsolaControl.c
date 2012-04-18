
/*
	Consola de Control
	Name: ConsolaControl.c
*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0

struct SuicideProcessInfo
{
	char * key;
	char * id;
	char * path;
	char * filename;
	char * lives;
};

int getLineCount (  ){
	FILE * file = fopen("./src/Common/ArchCfg.txt", "r");
	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	int count = 0;
	char line[200];
	while( fgets( line, sizeof(line), file ) != NULL ){
		count++;
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
	info.lives = tokensArray[4];
	return info;
}

void * hilo_de_consola( void *arg )
{
	struct SuicideProcessInfo * pi = (struct SuicideProcessInfo *) arg;
	
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
		//fprintf(stdout, "El hijo del hilo ha sido creado\n" );
				
		dup2( pipeParentWrite[0], 0 );
		close( pipeParentWrite[0] );
		close( pipeParentWrite[1] );
		
		dup2( pipeParentRead[1], 1 );
		close( pipeParentRead[0] );
		close( pipeParentRead[1] );
		
		dup2( pipeParentError[1], 2);
		close( pipeParentError[0] );
		close( pipeParentError[1] );
		
		execl("procesoctrl",
			"procesoctrl", pi->id, pi->path, pi->filename, pi->lives, NULL);
		/* No deberia llegar a este punto */
		fprintf( stderr, "Error cambiando imagen de proceso\n" );
	}
	
	close( pipeParentRead[1] );
	FILE * out = fdopen(pipeParentRead[0], "r");
	
	close( pipeParentWrite[0] );	
	/* Para esta primera entrega no esta definido el uso, pero si su declaracion */
	//FILE * in = fdopen(pipeParentWrite[1], "w");
	
	close( pipeParentError[1] );
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
			break;
	 	}
	}
	
	close(pipeParentRead[0]);
	close(pipeParentRead[1]);
	
	close(pipeParentWrite[0]);
	close(pipeParentWrite[1]);
	
	close(pipeParentError[0]);
	close(pipeParentError[1]);
		
	usleep(200 * 1000);
	pthread_exit(NULL);
}

int main( ){

	int processCount = getLineCount();	
	FILE * file = fopen("./src/Common/ArchCfg.txt", "r");
	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	char line[processCount][200];
	struct SuicideProcessInfo suicides[processCount];
	int i = 0;
		
	while( fgets( line[i], sizeof line, file ) != NULL ){
		suicides[i] = setSuicideInfo( line[i] );
		i++;
	}	
	fclose( file );
	
	pthread_t *threadTable = (pthread_t *) malloc(sizeof(pthread_t) * processCount);
	
	for (i = 0; i < processCount; i++)
	{
		if( strcmp( suicides[i].key, "ProcesoSui" ) == 0 ){
			pthread_create((threadTable + i), NULL, hilo_de_consola, (void *) &suicides[i]);
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

//printf( "%s", line[i] );

/*printf( "%s %s %s %s %s \n",*/
/*			suicides[i].key,*/
/*			suicides[i].id,*/
/*			suicides[i].path,*/
/*			suicides[i].filename,*/
/*			suicides[i].lives);*/

/*printf( "%s %s %s %s %s \n",*/
/*			pi->key,*/
/*			pi->id,*/
/*			pi->path,*/
/*			pi->filename,*/
/*			pi->lives);*/