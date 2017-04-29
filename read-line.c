/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include "tty-raw-mode.c"
#include <termios.h>
#include "read-line.h"

#define MAX_BUFFER_LINE 2048

void addToHistory(char * newline);

// Buffer where line is stored
int line_length;;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.

int history_pos = 0;   //Holds the position of the last added line 
int max = 2;		//Holds the max history size
char ** history = NULL;
int history_length = 0; 
int line_pos = 0;

void initHistory() {
  history = (char **) malloc(sizeof(char *) * max);
  char * blank = "";
  addToHistory(blank);
}

void addToHistory(char * newline) {
  char * line = strdup(newline);

  //Check if history size is at its max
  if (history_length == max + 1) {
      max *= 2;
      history = (char **) realloc(history, max * sizeof(char *));
  }
  //Add null character
  line[strlen(line)] = '\0';
  history[history_length] = line;
  history_length++;
  history_pos++;
}

void resetBuffer() {
  for (int i = 0; i < MAX_BUFFER_LINE; i++)
    line_buffer[i] = '\0';   
}

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n"
    " left arrow   Moves cursor to the left and allow insertion\n"
    " right arrow  Moves cursor to the right and allow insertion\n"
    " down arrow   See the next command in the history\n"
    " delete key   Removes character at cursor and shifts chars to the left\n"
    " home key     Moves cursor to the beginning of the line\n"
    " end key      Moves cursor to the end of the line\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  //Store terminal settings
  struct termios tty_attr;
  tcgetattr(0, &tty_attr);

  // Set terminal in raw mode
  tty_raw_mode();

  // Set variables
  line_length = 0;
  int len = 0;
  line_pos = 0;

  //Call resetBuffer
  resetBuffer();

  // Read one line until enter is typed
  while (1) {

    //Initialize the history
    if (history == NULL)
        initHistory();

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch >= 32 && ch < 127) {
      // It is a printable character. 
 
      //Check if we're at the end of the line
      if (line_pos == line_length) {
          // Do echo
          write(1,&ch,1);

          // If max number of character reached return.
          if (line_length==MAX_BUFFER_LINE-2) {
	      break; 
	  }
	  // add char to buffer.
          line_buffer[line_pos]=ch;
          line_length++;
          line_pos++;   
      }
      else {
          char * holdChars = (char *) malloc(sizeof(char) * MAX_BUFFER_LINE);
  	  for (int i = 0; i < MAX_BUFFER_LINE; i++) {
	 
	      if (line_buffer[i + line_pos] == '\0')
	          break;   //Finished copying
	      else
	          holdChars[i] = line_buffer[line_pos + i];
	  }

	write(1,&ch,1);
    
        // If max number of character reached return.
        if (line_length==MAX_BUFFER_LINE-2) {
            break; 
	}
      
        //Add char to the buffer and increment variables
	line_buffer[line_pos] = ch;
	line_pos++;
	line_length++;

        //Print the remaining chars
	int i = 0;
	int count = 0;
	while (i < MAX_BUFFER_LINE) {
	    write(1, &holdChars[i], 1);
	    count++;
	    //Check if we have reached null character
	    if (line_buffer[line_pos + i] == '\0')
	        break;   
	    i++;
	}
	
	//Backspace "count" number of times
	ch = 8;
	for (int j = 0; j < count; j++) {
	    write(1, &ch, 1);
	}
      }	
    }
    
    else if (ch==10) {
      // <Enter> was typed. Return line 
      if (strlen(line_buffer) != 0)
          addToHistory(line_buffer);

      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    
    // Home key
    else if (ch == 1) {
        if (line_pos > 0) {
		for (int i = 0; i < line_length; i++) {
	    		ch = 8;
	   		write(1,&ch,1);
		}
		//Move to beginning of the line
		line_pos = 0;
    	}
    }
    
    else if (ch == 127 || ch == 8) {
      // <backspace> was typed. Remove previous character read.

      // If already at beginning, don't go back
      if (line_pos > 0) {
          
	  //Shift chars
	  for (int i = line_pos; i < line_length; i++) {	
	      char temp = line_buffer[i];
	      line_buffer[i] = line_buffer[i + 1];
	      line_buffer[i - 1] = temp;
	  }

  	//Move cursor right, which is 27, 91, 67
	int distance = line_length - line_pos;
	for (int i = 0; i < distance; i++) {
	    char x = 27;
	    char y = 91;
	    char z = 67;
	    write(1,&x,1);
	    write(1,&y,1);
	    write(1,&z,1);
    	}
	    
	//Print backspaces
	for (int i = 0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	}
	
	// Print spaces on top
	for (int i = 0; i < line_length; i++) {
  	    ch = ' ';
  	    write(1,&ch,1);
	}
		
	//Print backspaces
	for (int i = 0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	}

	// echo line
	line_length--;
	write(1, line_buffer, line_length);
     
	//Print backspaces
	for (int i = 0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	}

	line_buffer[line_length] = '\0';
	if (line_pos - 1 >= 0)
	    line_pos--;

	for (int i = 0; i < line_pos; i++) {
	    char x = 27;
	    char y = 91;
	    char z = 67;
	    write(1,&x,1);
	    write(1,&y,1);
	    write(1,&z,1);    
	}
    }
    }

    // Delete key
    else if (ch == 4) {
	if (line_pos != -1) {
	    if (line_buffer[line_pos] != NULL) {
	        int i = line_pos + 1;
		for (i; i < line_length; i++) {
		    //Shift characters left
		    char temp = line_buffer[i];
		    line_buffer[i] = line_buffer[i + 1];
		    line_buffer[i - 1] = temp;
		}
	    	//Move cursor right, which is 27, 91, 67
		int distance = line_length - line_pos;
		for (i = 0; i < distance/*line_length*/; i++) {
		    char x = 27;
		    char y = 91;
		    char z = 67;
		    write(1,&x,1);
		    write(1,&y,1);
		    write(1,&z,1);
	    	}
	    
		//Print backspaces
		for (i = 0; i < line_length; i++) {
		    ch = 8;
		    write(1,&ch,1);
		}
	
		// Print spaces on top
		for (i = 0; i < line_length; i++) {
	  	    ch = ' ';
	  	    write(1,&ch,1);
		}
		
		//Print backspaces
		for (i = 0; i < line_length; i++) {
		    ch = 8;
		    write(1,&ch,1);
		}
	
		// echo line
		line_length--;
		write(1, line_buffer, line_length);
     
     		//Print backspaces
		for (i = 0; i < line_length; i++) {
		    ch = 8;
		    write(1,&ch,1);
		}
		line_buffer[line_length] = '\0';
		if (line_pos - 1 >= 0)
			line_pos--;

		for (i = 0; i < line_pos; i++) {
		    char x = 27;
		    char y = 91;
		    char z = 67;
		    write(1,&x,1);
		    write(1,&y,1);
		    write(1,&z,1);
	    	}
	    }
	}
    }

    // End key
    else if (ch == 5) {
	for (int i = 0; i < line_length - line_pos; i++) {
	    //Need to move right, which is 27 91 67
	    char x = 27; 
	    char y = 91;
	    char z = 67;
	    write(1,&x,1);
	    write(1,&y,1);
	    write(1,&z,1);
	}
	line_pos = line_length;
    }

    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.

	// Erase old line
	// Print backspaces	 
	for (int i = 0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (int i = 0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (int i = 0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	if (history_pos > 0)
		history_pos--;
	if (history_pos <= 0)
		history_pos = history_length - 1;

	// Copy line from history
	strcpy(line_buffer, history[history_pos]);
	line_length = strlen(line_buffer);

	// echo line
	write(1, line_buffer, line_length);
      }
      
      // Left arrow key	
      if (ch1 == 91 && ch2 == 68) {
     	if (line_length > 0 && line_pos > 0) {
	  //Go back one character
	  line_pos--;
	  ch = 8;
	  write(1,&ch,1);
	}
      }

      // Right arrow key
      if (ch1 == 91 && ch2 == 67) {
	if (line_length > line_pos) {
	    char x = 27;
	    char y = 91;
	    char z = 67;
	    write(1,&x,1);
	    write(1,&y,1);
	    write(1,&z,1);
	
	    line_pos++;
	}
      }

      // Down arrow key
      if (ch1 == 91 && ch2 == 66) {
	
      	    if (history_pos <= 0)
	        history_pos = 0;
            else {
	        history_pos++;
	        if (history_pos >= history_length) {
	            history_pos = 0;
	        }
	    }
	    // Print backspaces
	    for (int i = 0; i < line_length; i++) {
	        ch = 8;
		write(1,&ch,1);
	    }
	    // Print spaces on top
	    for (int i = 0; i < line_length; i++) {
	  	ch = ' ';
	  	write(1,&ch,1);
	    }
	    // Print backspaces
	    for (int i =0; i < line_length; i++) {
	  	ch = 8;
	  	write(1,&ch,1);
	    }	
	    // Copy line from history
	    strcpy(line_buffer, history[history_pos]);
	    line_length = strlen(line_buffer);

	    // Echo line
	    write(1, line_buffer, line_length);
	}

      }
   }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

   //Restore original terminal settings
  tcsetattr(0, TCSANOW, &tty_attr);

  return line_buffer;
}

