
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

//Entero para la cuenta de los procesos control
int pctrlCount = 0;

//Funcion que obtiene el numero de lineas del archivo de confg
int getLineCount (  );

//Funcion asigna los datos del proceso suicida en las estructura definida para ello
struct SuicideProcesstartupInfo setSuicideInfo( char * line );

//Hilo de consola para leer la salida de los procesos control
DWORD WINAPI readStdout( LPVOID lpParameter );

//Hilo de consola para leer el error de los procesos control
DWORD WINAPI readStderr( LPVOID lpParameter );

//Hilo de consola que lanza el proceso control con la info del proceso suicida que debe controlar
DWORD WINAPI hilo_de_consola( LPVOID lpParameter );

//Funcion que transforma de entero a caracteres.
int my_itoa(int val, char* buf); /*	Extraido de: http://code.google.com/p/my-itoa/  */

//Funcion principal leer el archivos de configuracion, lanzar los hilos y leer de las salidas estandar stdout, stdout
int main( ){
	int processCount = getLineCount(); //Cuenta la cantidad de procesos suicidas.
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
		while( fgets( line[i], 200, file ) != NULL && i < processCount ){ // Obtiene las lineas del archivo obtiene los datos de los suicidas.
			if( line[i][0] != '\n' ){
				suicides[i] = setSuicideInfo( line[i] );
			}
			i++;
		}
	}
	fclose( file );
	for (i = 0; i < processCount; i++) //Lanza un hilo con la informacion de cada suicida.
	{
		if( strcmp( suicides[i].key, "ProcesoSui" ) == 0 ){
			 CreateThread(NULL, 0, hilo_de_consola, (LPVOID) &suicides[i], 0, (tablaHilos + i));
		}
	}
	for (i = 0; i < processCount; i++) //Espera por cada hilo que lanzo
	{
		hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, *(tablaHilos + i));
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwResultado);
		fprintf(stdout, "El hilo: %d termino: %ld\r\n", i, dwResultado);
	}
	return 0;
}

//------------------------------------------------------- DEFINICION DE FUNCIONES ----------------------------------------------
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
		if( line[0] != '\n' ){
			count++;
		}
	}
	fclose( file );
	return count;
}

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

DWORD WINAPI hilo_de_consola( LPVOID lpParameter ){
	struct SuicideProcesstartupInfo * pi = (struct SuicideProcesstartupInfo *) lpParameter;
	
	//Para los parametros de procesos control se debe usar '=' para dar el valor de una opcion ej. --filepath=/Users/Docs/Programa
	//Opciones del proceso control
	char suiOpt[200] = "--suicidename=";
	char filepathOpt[200] = "--filepath=";
	char filenameOpt[200] = "--filename=";
	char reencOpt[200] = "--reencarnacion=";
	char launchArg1[200] = "procesoctrl.exe ";

	char idProcCtrl[100];

	//Variables para el uso de tuberias
	SECURITY_ATTRIBUTES securAttr;
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	
	HANDLE parentReadPipe[2];
	HANDLE parentErrorPipe[2];
	HANDLE parentWritePipe[2];

	HANDLE stdoutThread;
	HANDLE stderrThread;

	my_itoa( pctrlCount, idProcCtrl );
	pctrlCount++;

	//Se concatenan las opciones con su respectiva informacion
	strcat_s( suiOpt, 200, pi->id );
	strcat_s( filepathOpt, 200, pi->path );
	strcat_s( filenameOpt, 200, pi->filename );
	strcat_s( reencOpt, 200, pi->lifes );

	//Se pone todo en un solo lugar para eviarlo como lista de parametros del proceso control.
	strcat_s( launchArg1, 200, suiOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, filepathOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, filenameOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, reencOpt );
	strcat_s( launchArg1, 200, " " );
	strcat_s( launchArg1, 200, idProcCtrl );
	

	ZeroMemory(&securAttr,sizeof(securAttr));
	securAttr.nLength = sizeof(securAttr);
	securAttr.bInheritHandle = TRUE;

	//Creacion de tuberias.
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

	//Creacion del proceso control
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