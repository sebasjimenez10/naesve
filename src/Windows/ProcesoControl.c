
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
		SECURITY_ATTRIBUTES secattr;
		DWORD exitCode;
		DWORD status;

		HANDLE fatherRead[2];
		HANDLE fatherReadError[2];
		HANDLE fatherWrite[2];

		HANDLE stdoutThread;
		HANDLE stderrThread;

		ZeroMemory(&secattr,sizeof(secattr));
		secattr.nLength = sizeof(secattr);
		secattr.bInheritHandle = TRUE;

		CreatePipe(&fatherRead[0],&fatherRead[1],&secattr,0);
		SetHandleInformation(fatherRead[0], HANDLE_FLAG_INHERIT, 0);

		CreatePipe(&fatherReadError[0],&fatherReadError[1],&secattr,0);
		SetHandleInformation(fatherReadError[0], HANDLE_FLAG_INHERIT, 0);

		CreatePipe(&fatherWrite[0],&fatherWrite[1],&secattr,0);
		SetHandleInformation(fatherWrite[1], HANDLE_FLAG_INHERIT, 0);

		ZeroMemory(&piProcInfo,sizeof(piProcInfo));
		ZeroMemory(&startupInfo,sizeof(startupInfo));
		startupInfo.cb=sizeof(startupInfo);
		startupInfo.hStdInput=fatherWrite[0];
		startupInfo.hStdOutput=fatherRead[1];
		startupInfo.hStdError=fatherReadError[1];
		startupInfo.dwFlags=STARTF_USESTDHANDLES;

		//GetStartupInfo(&startupInfo);
		if (CreateProcess(NULL, "ProcesoSuicida.exe", NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &piProcInfo)) {
			CloseHandle(fatherRead[1]);
			CloseHandle(fatherReadError[1]);
			CloseHandle(fatherWrite[0]);

			// Se inicializan los dos hilos de lectura
			stdoutThread = CreateThread( NULL, 0, readStdout, (LPVOID) fatherRead[0], 0, NULL);
			stderrThread = CreateThread( NULL, 0, readStderr, (LPVOID) fatherReadError[0], 0, NULL);
			while(1){
				status = WaitForSingleObject(piProcInfo.hProcess, 0);
				if (status != 0){
					// No ha muerto
					// Manda un mensaje si quiere al proceso suicida
				}else{
					// sleep(60) // Minuto de silencio para el fallecido
					GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
					if( lifes > 0 ){
						fprintf( stderr, "El Proceso Suicida %s termino por causa %ld -- Proceso Control %d, vidas restantes: %d\n", id, exitCode, pID, lifes );
					}else{
						fprintf( stderr, "El Proceso Suicida %s termino por causa %ld -- Proceso Control %d, vidas restantes: Infinitas\n", id, exitCode, pID );
					}
					fflush(stderr);
					CloseHandle(fatherRead[0]);
					break;
				}
			}

			//CloseHandle(fatherRead[0]); ////////////////
			//CloseHandle(fatherReadError[0]); /////////////
			//CloseHandle(fatherWrite[1]); ///////////////

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
