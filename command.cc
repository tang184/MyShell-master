
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <regex.h>

#include "command.h"
extern char ** environ;
char * backgroundPIDs = (char *) malloc(1024 * sizeof(char));
int numBackgroundPIDs = 0;

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	
	//Variable expansion
	if (strstr(argument, "$") != NULL && strstr(argument, "{") != NULL
		&& strstr(argument, "}") != NULL) {

		const char * buffer = "\\${.*}";
		regex_t re;
	
		if (regcomp(&re, buffer, 0)) {
			perror("regcomp");
			exit(0);
		}

		regmatch_t match;
		if (regexec(&re, argument, 1, &match, 0) == 0) {
			//The regex matched ${var}
			char * expandArg = (char *) calloc(1, 1024*sizeof(char));
			int a = 0;	//Position in argument
			int e = 0;	//Position in expandArg

			while (argument[a] != 0 && a < 1024) {
	
				if (strcmp("${$}", argument) == 0) {
					fprintf(stdout, "%d\n", getpid());
					break;
				}
				if (strcmp("${!}", argument) == 0) {
					int lastPID = backgroundPIDs[numBackgroundPIDs - 1];
					fprintf(stdout, "%d\n", lastPID);
					break;
				}
				if (strcmp("${SHELL}", argument) == 0) {
					fprintf(stdout, "%d\n", argv[0]);
					break;
				}
			
				if (argument[a] != '$') {
					expandArg[e] = argument[a];
					expandArg[e + 1] = '\0';  //Add null character
					e++;
					a++;
				} else {
					//Get location of the next { and }	
					char * start = strchr((char*) (argument + a), '{');
					char * end = strchr((char*) (argument + a), '}');
				
					//Copy the env variable to expand w/out including { and }
					char * variable = (char *) calloc(1, strlen(argument) * sizeof(char));
					strncat(variable, start + 1, end - start - 1);
		
					//The value of var comes in getenv() function
					char * value = getenv(variable);

					//If not null, add value to the expanded variable
					if (value == NULL) 
						strcat(expandArg, "");
					else
						strcat(expandArg, value);
				
					//Increment len of variable + ${ and }
					a = a + strlen(variable) + 3;
					e = e + strlen(value);
					free(variable);
				}
			}
			argument = strdup(expandArg);
		}
		regfree(&re);	
	}


	if (argument[0] == '~') {
		if (strlen(argument) == 1)
			argument = strdup(getenv("HOME"));
		else
			argument = strdup(getpwnam(argument+1)->pw_dir);
	}



	_arguments[ _numOfArguments ] = argument;
	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_ambiguous = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_ambiguous = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure	
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	//Save default in, out, and err

	int defaultin = dup(0);	
	int defaultout = dup(1);
	int defaulterr = dup(2);
		
	int fdin;
	int fdout;
	int fderr;

	if (_inFile) {
		fdin = open(_inFile, O_RDONLY);
	} else 
		fdin = dup(defaultin);

	int ret;
	for (int i = 0; i < _numOfSimpleCommands; i++) {
		//Redirect input
		dup2(fdin, 0);
		close(fdin);

		//Set environmental variables
		if (strcmp(_simpleCommands[i]->_arguments[0], "setenv") == 0) {
			int combinedLen = strlen((char*)_simpleCommands[i]->_arguments[1]) + 
				strlen((char*)_simpleCommands[i]->_arguments[2]) + 1;
			char * var = (char *) malloc(combinedLen);
			sprintf(var, "%s=%s", _simpleCommands[i]->_arguments[1],
					_simpleCommands[i]->_arguments[2]);
			
			int setEnv = putenv(var);
			if (setEnv != 0) 
				perror("setenv");
			clear();
			prompt();
			return;
		}
		//Unset environmental variables
		if (strcmp(_simpleCommands[i]->_arguments[0], "unsetenv") == 0) {
			char * ptr = strstr(*environ, 
					_simpleCommands[i]->_arguments[1]); 	
			char * var = (char *) malloc(strlen((char *) ptr) + 1);
			sprintf(var, "%s=", ptr);
			putenv(var);
			clear();
			prompt();
			return;
		}
		//Change the current directory
		if (strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0) {
			int cd = 0;
			if (_simpleCommands[0]->_arguments[1] == NULL) {
				struct passwd * pw = getpwuid(getuid());
				const char * homeDir = pw->pw_dir;
				cd = chdir(homeDir);
			} else 
				cd = chdir(_simpleCommands[0]->_arguments[1]);
			if (cd != 0)
				perror("chdir");

			clear();
			prompt();
			return;
		}
		
		if (_background) {
			backgroundPIDs[numBackgroundPIDs] = getpid();
			numBackgroundPIDs++;
		}

		//Setup output
		if (i == _numOfSimpleCommands - 1) {
			//Last simple command
			if (_outFile) {
				if (_append == 0) {
					fdout = open(_outFile, O_WRONLY | O_CREAT |
						O_TRUNC, 0700);
				}
				else {
					fdout = open(_outFile, O_WRONLY | O_CREAT |
						O_APPEND, 0700);
				}
			} else
				fdout = dup(defaultout);
			
			if (_errFile) {
				if (_append == 0) {
					fderr = open(_errFile, O_WRONLY | O_CREAT |
						O_TRUNC, 0700);
				}
				else {
					fderr = open(_errFile, O_WRONLY | O_CREAT |
						O_APPEND, 0700);
				}
			} else
				fderr = dup(defaulterr);
		}
		else {
			//Not last simple command
			//Create pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}
	
		//Redirect output
		dup2(fdout, 1);
		if (_errFile) {
			dup2(fdout, 2);
		}

		close(fdout);

		if (!strcmp(_simpleCommands[i]->_arguments[0], "exit")) {
	//		printf("Good bye!!\n");
			exit(2);
		}


		//Create child process
		ret = fork();
		if (ret == 0) {		
			//Print environmental variables
			if (strcmp(_simpleCommands[i]->_arguments[0],
						"printenv") == 0) {
				char ** envVars = environ;
				while (*envVars != NULL) {
					printf("%s\n", *envVars);
					envVars++;
				}
				exit(0);
			}
			if (strcmp(_simpleCommands[i]->_arguments[0], "jobs") == 0) {
				//Print info about commands running in background
				if (!_background) {
					printf("PID: %d\n", getpid());
				}
				exit(0);
			}
			if (!strcmp(_simpleCommands[i]->_arguments[0], "exit")) {
		//		printf("Good bye!!\n");
				exit(2);
			}
	
			execvp(_simpleCommands[i]->_arguments[0], 
				_simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);	
		}
		else if (ret < 0) {
			perror("fork");
			return;
		}	
	}	
	//Restore in/out/err defaults
	dup2(defaultin, 0);
	dup2(defaultout, 1);
	dup2(defaulterr, 2);
	close(defaultin);
	close(defaultout);
	close(defaulterr);
	
	//Parent shell continue
	if (!_background) {
		// wait for last process
		waitpid(ret, NULL, 0);
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
	
}

// Shell implementation

void
Command::prompt()
{
	if (isatty(0)) {
		char * prompt = getenv("PROMPT");
		if (prompt != NULL)
			printf("%s", prompt);
		else 
			printf("myshell>");
		fflush(stdout);
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void disp(int sig) {
	printf("\n");
	Command::_currentCommand.prompt();
}

void killzombie(int sig) {
	int pid = wait3(0, 0, NULL);
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void func(int sig) {
	printf("\n");
	kill(sig, SIGSTOP);
	Command::_currentCommand.prompt();
}

main()
{

	struct sigaction sigAction1;
	sigAction1.sa_handler = disp;
    	sigemptyset(&sigAction1.sa_mask);
    	sigAction1.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sigAction1, NULL)){
        	perror("sigaction");
        	exit(2);
    	}

	struct sigaction sigAction2;
	sigAction2.sa_handler = killzombie;
	sigemptyset(&sigAction2.sa_mask);
	sigAction2.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sigAction2, NULL)) {
		perror("sigaction");
		exit(-1);
	}
	
	struct sigaction sigAction3;
	sigAction3.sa_handler = func;
	sigemptyset(&sigAction3.sa_mask);
	sigAction3.sa_flags = SA_RESTART;

	if (sigaction(SIGTSTP, &sigAction3, NULL)) {
		perror("sigaction");
		exit(2);
	}

	Command::_currentCommand.prompt();
	yyparse();
}

