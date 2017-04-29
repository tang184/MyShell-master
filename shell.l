
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"
#include <unistd.h>
#include <sys/wait.h>
//#include "read-line.c"
#include "read-line.h"

//extern "C" char * read_line();

int mygetc(FILE * f) {
	static char * p;
	char ch;

	if (!isatty(0)) {
		// stdin is not a tty. Call real getc
		return getc(f);
	}

	//stdin is a tty. Call our read_line.
	if (p == NULL || *p == 0) {
		char * s = read_line();
		p = s;
	}
	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)


static void yyunput (int c,char *buf_ptr  );
//#define unput(c) yyunput( c, (yytext_ptr))
void myunputc(int c) {
    unput(c);
}

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

"&"	{
		return AMPERSAND;
	}

">>&"	{
		return GREATGREATAMPERSAND;
	}

">>"	{
		return GREATGREAT;
	}


">&"	{
		return GREATAMPERSAND;
	}

"<" 	{
		return LESS;
	}

">"	{
		return GREAT;
	}	

"|"	{
		return PIPE;
	}

\`[^\n\`]*\` {
	//Handle backticks
	//Create array to hold the command without backticks
		
	char * noTicArray = strdup(yytext + 1);
	*(noTicArray + strlen(noTicArray) - 1) = '\0';
	strcat(noTicArray, "\nexit\n");
	
	//Initialize buffer to be used later
	char buffer[2048];

	int in[2];
	int out[2];

	if (pipe(in) == -1) {
		perror("pipe");
		exit(2);
	}
	if (pipe(out) == -1) {
		perror("pipe");
		exit(2);
	}
	int ret = fork();
	if (ret < 0) {
		perror("fork");
		exit(1);
	}
	if (ret == 0) {
		//Assign write end of the read pipe to stdin
		dup2(in[0], 0);
	
		//Assign read end of the write pipe to stdout
		dup2(out[1], 1);
		
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);

		execlp("/proc/self/exe", "/proc/self/exe",  NULL);
		perror("execlp");
		exit(1);
	}
	
	close(in[0]);
	close(out[1]);

	write(in[1], noTicArray, strlen(noTicArray));
	close(in[1]);

	int bufferlen = read(out[0], buffer, 2048);		
	close(out[0]);

	buffer[bufferlen] = '\0';

	char * newString = (char *) malloc(sizeof(char) * strlen(buffer));
	int i = -1; 
	int j = 0;
	while (buffer[++i] != '\0');
	i--;
	while (i >= 0) {
		if (buffer[i] == '\n') {
			newString[j++] = ' ';
		} else {
			newString[j++] = buffer[i];
		}
		i--;
	}
	newString[j] = '\0';

	for (int x = 0; x < strlen(newString); x++) {
		unput(newString[x]);
	}
}	


\"[^\n\"]*\"	{
		//Remove quotes
		yylval.string_val = (char *) calloc(strlen(yytext) - 1, sizeof(char));
		strncpy(yylval.string_val, &yytext[1], strlen(yytext) - 2);
		yylval.string_val[strlen(yytext)-1] = '\0';
		return WORD;
	}

[^ \t\n&<>\|]*\\[^ \t\n]* {
		//Handle escape characters		
		char * newWord = (char *) malloc(1024 * sizeof(char));
		int pos = 0;
		for (int i = 0; i != strlen(yytext); i++) {
			if (yytext[i] == 92) {	
				if (yytext[i + 1] == 92) {
					newWord[pos] = yytext[i + 1]; //Remove escape char
					i++;
					pos++;			
			 	}
			 } else {
			 	//No escape char, continue as normal
			 	newWord[pos] = yytext[i];
				pos++;
			}
		}
		yylval.string_val = strdup(newWord);
		return WORD;
	}

[^ \t\n|><&][^ \t\n|><&]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}
.	{
		return NOTOKEN;
	}
%%
