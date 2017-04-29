# MyShell
Wrote my own shell implementation for my CS252 Systems Programming course. 

##Part 1: Lex and Yacc
Create a scanner and parser for the shell

##Part 2: Executing commands
 - Simple command process creation and execution using fork() and execvp()
 - I/O File redirection
 - Pipes

##Part 3: Ctrl-C, Wildcards, Zombie Elimination, etc.
 - Ctrl-C (Using SIGINT to kill the running command)
 - Builtin Commands: setenv, unsetenv, printenv, cd, jobs
 - Wildcarding
 - Subshells
 - Zombie Elimination
 - Allow Quotes in Commands
 - Escape characters
 - Environment variable expansion
 - Tilde expansion
 - isatty()
 - Implement a line editor (up, down, left, and right arrow keys, delete (ctrl-D), backspace, home key (ctrl-A), end key (ctrl-E)
 - Variable prompt
 - Robustness
