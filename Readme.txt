Introduccion:
En este repositorio se incluyen las versiones de Windows y Linux que implementan el control de procesos reencarnantes, es decir, que se ha creado un proyecto que permite que los procesos vuelvan a su vida 
despues de haber terminado su ejecucion (por diferentes razones) y ademas puedan revivir tantas veces como este establecido para cada uno de los procesos. 

General:
La practica se realizo en el lenguaje de programacion C.

Estructura:
- En la carpeta bin se encuentran los archivos ejecutables
- En la carpeta src se encuentran las carpetas Windows y Linux con el codigo correspondiente y la capeta Common que contiene el archivo de configuracion.
- Tambien se encuentran los Makefile de ambos sistemas operativos para la compilacion. 

Compilacion:
Para la compilacion de todos los archivos fuentes se debe ejecutar el Makefile del sistema operativos que se desea ejecutar y este sera el encargado de generar los archivos ejecutables y guardarlos en la carpeta bin. 

Ejecucion:
Para realizar la ejecucion, se debe ingresar desde la linea de comandos a la carpeta bin y ejecutar la consola control
Este procedimiento se debe hacer para la ejecucion en ambos sistemas operativos. 


NOTA: Para saber como realizar la ejecucion mas detalladamente, abrir el archivo Install

NOTA: Se debe tener en cuenta que el archivo de configuracion no debe contener lineas vacias ('\n')

NOTA: En el archivo de configuracion, el path del proceso suicida, no debe contener espacios ('\s')