%{
/*
	archcfg.y (Bison)
*/
#define YYSTYPE char*
#include <stdio.h>
#include <errno.h>
#include <string.h>
int yylex(void);
void yyerror(char *s);
extern void * getTokens();
%}

%token PROCESOSUI IDENTIFICADOR ALLAVE PATH CPUNTOS VIDAS CLLAVE
%start archcfg
%%
archcfg : procesosui archcfg
	   |	/* epsilon */
	   ;
procesosui : PROCESOSUI IDENTIFICADOR ALLAVE PATH CPUNTOS IDENTIFICADOR VIDAS CLLAVE {getTokens($2, $4, $6, $7);};
%%
void yyerror (s) /* Llamada por yyparse ante un error */
char *s;
{
	printf ("%s\n", s); /* Esta implementación por defecto nos valdrá */
	/* Si no creamos esta función, habrá que enlazar con –ly en el
	momento de compilar para usar una implementación por defecto */
	errno = -1;
}
