
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND LESS GREATGREATAMPERSAND GREATAMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include "command.h"
#include "string.h"
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
extern char **environ;
void expandWildcard(char * prefix, char * suffix);
void sortArrayStrings(char ** array, int nEntries);
int compareFunc(char * s1, char * s2);
void yyerror(const char * s);
int yylex();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
	simple_command
        ;

simple_command:	
	command_and_args iomodifier_opt NEWLINE {
/*		printf("   Yacc: Execute command\n");
*/		Command::_currentCommand.execute();
	} 
	| pipe_list iomodifier_list background_optional NEWLINE {
/*		printf("   Yacc: Execute command\n");
*/		Command::_currentCommand.execute();
		
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word argument_list {
	Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

argument_list:
	argument_list argument
	| /* can be empty */
	;

argument:
	WORD {
 /*             printf("   Yacc: insert argument \"%s\"\n", $1);
*/
		if (strchr($1, '*') == NULL && strchr($1, '?') == NULL)
			Command::_currentSimpleCommand->insertArgument( $1 );
		else {
			expandWildcard("", $1);
		}
	}
	;

command_word:
	WORD {
 /*              printf("   Yacc: insert command \"%s\"\n", $1);
*/	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	|
	;

/* Add rule pipe_list */ 
pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;


iomodifier_opt:	

	GREAT WORD GREAT {
		yyerror("Ambiguous output redirect.\n");
		exit(0);
	}
	|
	GREATGREAT WORD GREAT {
		yyerror("Ambiguous output redirect.\n");
		exit(0);
	}
	|
	GREATGREATAMPERSAND WORD {
		if (Command::_currentCommand._outFile) { 
			yyerror("Ambiguous output redirect.\n");		
			exit(0);
		}
/*		printf("   Yacc: insert output \"%s\"\n", $2);
*/		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = strdup($2);
		Command::_currentCommand._append = 1;
	}
	|
	GREATGREAT WORD {
		if (Command::_currentCommand._outFile) {	
			yyerror("Ambiguous output redirect.\n");		
			exit(0);
		}
	
/*		printf("   Yacc: insert output \"%s\"\n", $2);
*/		Command::_currentCommand._outFile = $2;	
		Command::_currentCommand._append = 1;
	}
	|
	GREATAMPERSAND WORD {
		if (Command::_currentCommand._outFile) { 
			yyerror("Ambiguous output redirect.\n");	
			exit(0);
		}
/*		printf("   Yacc: insert output \"%s\"\n", $2);
*/		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = strdup($2);
	}
	|	
	LESS WORD {
/*		printf("   Yacc: insert output \"%s\"\n", $2);
*/		Command::_currentCommand._inFile = $2;
	}
	|
	GREAT WORD {
		if (Command::_currentCommand._outFile) {
			yyerror("Ambiguous output redirect.\n");	
			exit(0);
		}

/*		printf("   Yacc: insert output \"%s\"\n", $2);
*/		Command::_currentCommand._outFile = $2;
	}	
	;

/* Add rule for iomodifier_list */ 
iomodifier_list:
	iomodifier_list iomodifier_opt
	| iomodifier_opt
	| /* empty */
	;

/* Add rule for background_optional */
background_optional:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	| /* empty */	{
	Command::_currentCommand._background = 0;
	}
	; 

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

void expandWildcard(char * prefix, char * suffix) {
	if (suffix[0] == 0) {
		//Suffix is empty. Put prefix in argument
		Command::_currentSimpleCommand->insertArgument(strdup(++prefix));	
		prefix = NULL;
		return;
	}

	// Get the next component in the suffix and advance suffix
	char * s = strchr(suffix, '/');
	char component[2000];
	
	if (s != NULL) {
		// Copy up to the first "/"
		strncpy(component, suffix, s-suffix);
		suffix = s + 1;
	} else {
		//Last part of path. Copy the whole thing
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	s = NULL;

	//Expand the component
	char newPrefix[2000];
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
		//Does not have wildcards
		sprintf(newPrefix, "%s/%s", prefix, component);		
		expandWildcard(newPrefix, suffix);
		return;
	}

	//Has wildcards
	//Convert component to regular expression
	char * reg = (char *) malloc(10 * strlen(component) + 10);
	char * a = component;
	char * r = reg;
	*r = '^';
	r++;
	
	//Set flag for hidden files if dot is the first thing
	int dotAtFront = 0;
	if (a[0] == '.') {
		dotAtFront = 1;
	}
	while (*a) {
		if (*a == '*') {
			*r = '.';
			r++;
			*r = '*';
			r++;
		} else if (*a == '?') {
			*r = '.';
			r++;
		} else if (*a == '.') {
			*r = '\\';
			r++;
			*r = '.';
			r++;
		} else {
			*r = *a;
			r++;
		}
		a++;
	}
	*r = '$';
	r++;
	*r = '\0';  //Add null character

	//Compile regular expression
	regex_t re;
	if (regcomp(&re, reg, REG_EXTENDED | REG_NOSUB)) {
		perror("regcomp");
		return;
	}
	
	//Reset variables no longer needed
	for (int i = 0; i < strlen(component); i++)
		component[i] = '\0';
	a = NULL;
	r = NULL;

	// Begin scanning directory process
	struct dirent ** directoryList;
	int n;
	
	if (prefix[0] == 0) {
		n = scandir(".", &directoryList, 0, alphasort);
	} else {
		n = scandir(prefix, &directoryList, 0, alphasort);
	} 
	if (n < 0) {	
		if (dotAtFront) {
			//Scan files beginning with "."
			n = scandir(".", &directoryList, 0, alphasort);
		}
	}

	int count = 0;
	while (count < n) {
		if (regexec(&re, directoryList[count]->d_name, 0, NULL, 0) == 0) {		
			if (component[0] == '.') {
				sprintf(newPrefix, "%s", directoryList[count]->d_name);
			}
			else {
				sprintf(newPrefix, "%s/%s", prefix, directoryList[count]->d_name);
			}
			if (directoryList[count]->d_name[0] == '.') {
				//Only do something if we want to get the files beginning with "."
				if (dotAtFront) {
					sprintf(newPrefix, ".%s", directoryList[count]->d_name);
					expandWildcard(newPrefix, suffix);	
				}
			}
			else {
				sprintf(newPrefix, "%s/%s", prefix, directoryList[count]->d_name);
				expandWildcard(newPrefix, suffix);	
			}
		}
		count++;
	}		
	//Reset variables
	for (int j = 0; j < strlen(newPrefix); j++)
		newPrefix[j] = '\0';
	prefix = NULL;
	suffix = NULL;
	directoryList = NULL;
}

/*This function compares two strings.
  Returns 0 if strings are equal
*/
int compareFunc(const void * s1, const void * s2) {
	char * string1 = *(char**)s1;
	char * string2 = *(char**)s2;
	return strcmp(string1, string2); //(char*) s1, (char*) s2);
}

void sortArrayStrings(char ** array, int nEntries) {
	qsort(array, nEntries, sizeof(*array), compareFunc);
}

#if 0
main()
{	

	yyparse();
}
#endif
