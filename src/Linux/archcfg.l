/*
	archcfg.l (Flex)
*/
%{
#include <stdio.h>
#include "y.tab.h" /* GENERADO AUTOMÁTICAMENTE POR BISON */ //->((\/)*[a-zA-Z0-9]+)+
int processCount;
%}
id							[a-zA-Z]+
numero						[0-9]+
%%
[\t\n\s]						;
"ProcesoSui"					{processCount++; return (PROCESOSUI);}
{id}							{yylval = strdup(yytext); return (IDENTIFICADOR);}	
"{"							{return (ALLAVE);}
([a-zA-Z0-9]+)?(\/[a-zA-Z0-9]+)+	{yylval = strdup(yytext); return (PATH);}
"::"							{return (CPUNTOS);}
{numero}						{yylval = strdup(yytext); return (VIDAS);}
"}"							{return (CLLAVE);}
.							;
%%
