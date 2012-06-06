Introduccion:
En este repositorio se incluyen la version de Linux que implementa el control de procesos reencarnantes, es decir, que se ha creado un proyecto que permite que los procesos vuelvan a su vida
despues de haber terminado su ejecucion (por diferentes razones) y ademas puedan revivir tantas veces como este establecido para cada uno de los procesos.

General:
La practica se realizo en el lenguaje de programacion C.

Estructura:
- En la carpeta bin se encuentran los archivos ejecutables
- En la carpeta src se encuentran la carpeta Linux con el codigo correspondiente y la capeta Common que contiene el archivo de configuracion.
- Tambien se encuentra el Makefile del proyecto.

Compilacion:
Para la compilacion de todos los archivos fuentes se debe ejecutar el Makefile y este sera el encargado de generar los archivos ejecutables y guardarlos en la carpeta bin.

Ejecucion:
Para realizar la ejecucion, se debe ingresar desde la linea de comandos a la carpeta bin y ejecutar la consola control

NOTA: Para saber como realizar la ejecucion mas detalladamente, abrir el archivo Install.
NOTA: Para esta version se incluye un parser que valida el archivo de configuracion. En el archivo Install esta como se deben instalar bison y flex, necesarios para la compilacion del proyecto.
