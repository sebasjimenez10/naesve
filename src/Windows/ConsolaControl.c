
/*
	Consola de Control
	Name: ConsolaControl.c
*/

//Archivos de encabezado
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>

//Constantes
#define PATH "../src/Common/ArchCfg.txt"
#define TRUE 1

//Estructura que empaqueta la informacion del proceso suicida
struct SuicideProcesstartupInfo
{
	char * key;
	char * id;
	char * path;
	char * filename;
	char * lifes;
};

//Funcion que obtiene el numero de lineas de un archivo de texto
int getLineCount (  ){
	FILE * file;
	int count = 0;
	char line[200];

	fopen_s(&file, PATH, "r");
	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	while( fgets( line, sizeof line , file ) != NULL ){
		count++;
	}
	fclose( file );
	return count;
}

//Funcion asigna los datos del proceso suicida en las estructura definida para ello
struct SuicideProcesstartupInfo setSuicideInfo( char * line ){

	struct SuicideProcesstartupInfo info;
	char * tokensArray[5];
	char * delim = " {}:";
	char * token;
	char* context = NULL;
	int index = 0;


	token = strtok_s( line, delim, &context );	
	while ( token != NULL && index < 5 )
	{
		tokensArray[index] = token;
		token = strtok_s( NULL, delim, &context );
		index++;
	}
	info.key = tokensArray[0];
	info.id = tokensArray[1];
	strcat_s( tokensArray[2], 200, "\\" );
	info.path = tokensArray[2];
	info.filename = tokensArray[3];
	info.lifes = tokensArray[4];
	
	return info;
}

DWORD WINAPI readStdout( LPVOID lpParameter ){
	HANDLE pipeStdout = (HANDLE) lpParameter;
	char buffer[200];
	//printf( "Soy un hilo OUT\n" );
	for( ;; ) {
		DWORD ret;
		if( ! ReadFile(pipeStdout, buffer, 200, &ret, 0) )
		{
			break;
		}
		if( ret == 0 )
			break;
		buffer[ret] = '\0';
		fprintf(stdout, buffer);
		fflush( stdout );
	}
	return (DWORD) 0;
}
DWORD WINAPI readStderr( LPVOID lpParameter ){
	HANDLE pipeStderr = (HANDLE) lpParameter;
	char buffer[200];
	//printf( "Soy un hilo ERR\n" );
	for( ;; ) {
		DWORD ret;
		if( ! ReadFile(pipeStderr, buffer, 200, &ret, 0) )
		{
			break;
		}
		if( ret == 0 )
			break;
		buffer[ret] = '\0';
		fprintf(stderr, buffer);
		fflush( stderr );
	}
	return (DWORD) 0;
}

//Hilo de consola que lanza el proceso control con la info del proceso suicida que debe controlar
DWORD WINAPI hilo_de_consola( LPVOID lpParameter ){
	struct SuicideProcesstartupInfo * pi = (struct SuicideProcesstartupInfo *) lpParameter;
	
	char suiOpt[200] = "--suicidename=";
	char filepathOpt[200] = "--filepath=";
	char filenameOpt[200] = "--filename=";
	char reencOpt[200] = "--reencarnacion=";
	char launchArg1[200] = "procesoctrl.exe ";

	SECURITY_ATTRIBUTES securAttr;
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	
	HANDLE parentReadPipe[2];
	HANDLE parentErrorPipe[2];
	HANDLE parentWritePipe[2];

	HANDLE stdoutThread;
	HANDLE stderrThread;

	strcat_s( suiOpt, 200, pi->id );
	strcat_s( filepathOpt, 200, pi->path );
	strcat_s( filenameOpt, 200, pi->filename );
	strcat_s( reencOpt, 200, pi->lifes );

	strcat_s( launchArg1, 200, suiOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, filepathOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, filenameOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, reencOpt );

	ZeroMemory(&securAttr,sizeof(securAttr));
	securAttr.nLength = sizeof(securAttr);
	securAttr.bInheritHandle = TRUE;

	CreatePipe(&parentReadPipe[0],&parentReadPipe[1],&securAttr,0);
	SetHandleInformation(parentReadPipe[0], HANDLE_FLAG_INHERIT, 0);

	CreatePipe(&parentErrorPipe[0],&parentErrorPipe[1],&securAttr,0);
	SetHandleInformation(parentErrorPipe[0], HANDLE_FLAG_INHERIT, 0);

	CreatePipe(&parentWritePipe[0],&parentWritePipe[1],&securAttr,0);
	SetHandleInformation(parentWritePipe[1], HANDLE_FLAG_INHERIT, 0);

	ZeroMemory(&processInfo,sizeof(processInfo));
	ZeroMemory(&startupInfo,sizeof(startupInfo));
	startupInfo.cb=sizeof(startupInfo);
	startupInfo.hStdInput=parentWritePipe[0];
	startupInfo.hStdOutput=parentReadPipe[1];
	startupInfo.hStdError=parentErrorPipe[1];
	startupInfo.dwFlags=STARTF_USESTDHANDLES;


	if (CreateProcess(NULL, launchArg1, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
	
		CloseHandle(parentReadPipe[1]);
		CloseHandle(parentErrorPipe[1]);
		CloseHandle(parentWritePipe[0]);

		// Start reading threads
		stdoutThread = CreateThread( NULL, 0, readStdout, (LPVOID) parentReadPipe[0], 0, NULL);
		stderrThread = CreateThread( NULL, 0, readStderr, (LPVOID) parentErrorPipe[0], 0, NULL);
		
		//Wait until process finish
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		WaitForSingleObject(stdoutThread, INFINITE);
		WaitForSingleObject(stderrThread, INFINITE);
	}
	else {
		fprintf( stdout, "Error creando el proceso papa\n" );
	}
	return 0;
}

//Funcion principal leer el archivos de configuracion, lanzar los hilos y leer de las salidas estandar stdout, stdout
int main( ){
	int processCount = getLineCount();
	FILE * file;
	char line[200][200];
	struct SuicideProcesstartupInfo suicides[200];
	int i = 0;

	DWORD dwResultado;
	HANDLE hThread;
	DWORD *tablaHilos = (LPDWORD) malloc(sizeof(LPDWORD) * processCount);	

	fopen_s(&file, PATH, "r");
	

	if( file == NULL ){
		printf("El archivo no existe\n");
		exit(EXIT_FAILURE);
	}
	while(!feof( file )){
		while( fgets( line[i], 200, file ) != NULL ){
			suicides[i] = setSuicideInfo( line[i] );
			i++;
		}
	}
	fclose( file );
	for (i = 0; i < processCount; i++)
	{
		if( strcmp( suicides[i].key, "ProcesoSui" ) == 0 ){
			 CreateThread(NULL, 0, hilo_de_consola, (LPVOID) &suicides[i], 0, (tablaHilos + i));
		}
	}
	for (i = 0; i < processCount; i++)
	{
		hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, *(tablaHilos + i));
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwResultado);
		fprintf(stdout, "El hilo: %ld termino: %ld\r\n", *(tablaHilos +i), dwResultado);
	}
	return 0;
}
