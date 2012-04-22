
/*
	Proceso Control
	Name: ProcesoControl.c
*/

//Archivos de encabezado
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <Windows.h>

//Constantes
#define TRUE 1

//Hilo para la lectura del stdout
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

//Funcion principal encargada de lanzar el proceso suicida y hacerlo revivir tambien lee de las salidas estandar stdout, stderr
int main( int argc, char *argv[] )
{
	/*fprintf( stdout, "Estoy en el proceso CONTROL\n" );*/
	
	char * id;
	char * path;
	char * filename;
	int lifes;
	int pID = GetCurrentProcessId() / 2;
	char* context = NULL;

	char * token;

	token = strtok_s( argv[1], "=", &context );
	if( strcmp( token, "--suicidename" ) == 0 ){
		id = strtok_s( NULL, "=", &context );		
	}
	token = strtok_s( argv[2], "=", &context  );
	if( strcmp( token, "--filepath" ) == 0 ){
		path = strtok_s( NULL, "=", &context  );		
	}
	token = strtok_s( argv[3], "=", &context  );
	if( strcmp( token, "--filename" ) == 0 ){
		filename = strtok_s( NULL, "=", &context  );		
	}
	token = strtok_s( argv[4], "=", &context );
	if( strcmp( token, "--reencarnacion" ) == 0 ){
		lifes = atoi( strtok_s( NULL, "=", &context  ) );		
	}
	
	if( lifes == 0 )
	{
		lifes = -1;
	}

	while( TRUE ){

		STARTUPINFO startupInfo;
		PROCESS_INFORMATION piProcInfo;
		SECURITY_ATTRIBUTES securAttr;
		DWORD exitCode;
		DWORD exitValue;

		HANDLE parentReadPipe[2];
		HANDLE parentErrorPipe[2];
		HANDLE parentWritePipe[2];

		HANDLE stdoutThread;
		HANDLE stderrThread;

		ZeroMemory(&securAttr,sizeof(securAttr));
		securAttr.nLength = sizeof(securAttr);
		securAttr.bInheritHandle = TRUE;

		CreatePipe(&parentReadPipe[0],&parentReadPipe[1],&securAttr,0);
		SetHandleInformation(parentReadPipe[0], HANDLE_FLAG_INHERIT, 0);

		CreatePipe(&parentErrorPipe[0],&parentErrorPipe[1],&securAttr,0);
		SetHandleInformation(parentErrorPipe[0], HANDLE_FLAG_INHERIT, 0);

		CreatePipe(&parentWritePipe[0],&parentWritePipe[1],&securAttr,0);
		SetHandleInformation(parentWritePipe[1], HANDLE_FLAG_INHERIT, 0);

		ZeroMemory(&piProcInfo,sizeof(piProcInfo));
		ZeroMemory(&startupInfo,sizeof(startupInfo));
		startupInfo.cb=sizeof(startupInfo);
		startupInfo.hStdInput=parentWritePipe[0];
		startupInfo.hStdOutput=parentReadPipe[1];
		startupInfo.hStdError=parentErrorPipe[1];
		startupInfo.dwFlags=STARTF_USESTDHANDLES;

		if (CreateProcess(NULL, "ProcesoSuicida.exe", NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &piProcInfo)) {
			CloseHandle(parentReadPipe[1]);
			CloseHandle(parentErrorPipe[1]);
			CloseHandle(parentWritePipe[0]);

			// Se inicializan los dos hilos de lectura
			stdoutThread = CreateThread( NULL, 0, readStdout, (LPVOID) parentReadPipe[0], 0, NULL);
			stderrThread = CreateThread( NULL, 0, readStderr, (LPVOID) parentErrorPipe[0], 0, NULL);
			while(1){
				exitValue = WaitForSingleObject(piProcInfo.hProcess, 0);
				if (exitValue != 0){
					//Still Alive
				}else{
					//Dead
					GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
					if( lifes > 0 ){
						fprintf( stderr, "El Proceso Suicida %s termino por causa %ld -- Proceso Control %d, vidas restantes: %d\n", id, exitCode, pID, lifes );
					}else{
						fprintf( stderr, "El Proceso Suicida %s termino por causa %ld -- Proceso Control %d, vidas restantes: Infinitas\n", id, exitCode, pID );
					}
					fflush(stderr);
					CloseHandle(parentReadPipe[0]);
					break;
				}
			}

			WaitForSingleObject(stdoutThread, INFINITE);
			WaitForSingleObject(stderrThread, INFINITE);
		}
		else {
			fprintf( stdout, "No se pudo ejecutar el proceso suicida\n" );
		}

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
