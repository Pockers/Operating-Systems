/**
 * Name: Michaela Hay
 * ID: 1001649623
 * Date of Submission: 2/17/2020
 * Lab 1 -- Shell Assignment
 * Professor: Bakker, Trevor
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  char ** user_history = malloc(sizeof(char*)*15);
  int histIncrement = 0;

  while( 1 )
  {
    // Concatenation String for User History insertion
    char * catStr = (char*)malloc(MAX_COMMAND_SIZE);

    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;
    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    
    // Start of my own code-- all code above is the professor's.
    // Check to see if the token array holds an element -- if not then don't fork().
    if( token[0]=='\0')
    {
         continue;
    }
    else
    {
        // Increemnter for pid_array
        static int x = 0;
        static pid_t pid_array [15];
        // Incrementer for the user_history array
        int i;

        // Prioritize checking for exclamation point so that repeated commands do not 
        // interefere with the fork() command.            
        if (token[0][0] == '!')
        {   
            int userNumber = atoi(strtok(token[0], "!"));
            if (userNumber >= 1 && userNumber <= 15)
            {
                char * tempString = (char*)malloc(MAX_COMMAND_SIZE); 
                // Subtract because it has to be the correct user_history index.
                int index = userNumber - 1;
                strcpy(tempString, user_history[index]);

                // Put code here that replaces everything in token string with the specified user_history command!
                token_count = 0;
                while ( ( (arg_ptr = strsep(&tempString, WHITESPACE ) ) != NULL) && 
                        (token_count<MAX_NUM_ARGUMENTS))
                {
                    token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
                    if( strlen( token[token_count] ) == 0 )
                    {
                        token[token_count] = NULL;
                    }
                    token_count++;
                }

            }
            else
            {
                // Print error message
                printf("Command not in history.\n");
            }
            
        }


        // Code for User History array
        for (i = 0; i < token_count-1; i++)
        {
            strcat(catStr, (strndup(token[i], MAX_COMMAND_SIZE)));
            strcat(catStr, " ");
        }

        // Check the size of user_history to see if it's too full.
        if (histIncrement >= 15)
        {
            histIncrement = 0;
            user_history[histIncrement] = strndup(catStr, MAX_COMMAND_SIZE);
            histIncrement++;
        }
        else if (histIncrement < 15)
        {
            user_history[histIncrement] = strndup(catStr, MAX_COMMAND_SIZE);
            histIncrement++;
        }
            
         pid_t pid = fork();
         int child_status;

        // Inside the child process
        if (pid == 0)
        {

            if (strcmp(token[0], "history") == 0)
            {
                int x = 0;
                while (user_history[x] != '\0')
                {
                    printf("[%d]: %s\n", (x+1), user_history[x]);
                    x++;
                }
            }
            
            if (execvp(token[0], token) < 0 
            && strcmp(token[0], "cd") != 0 
            && strcmp(token[0], "exit") != 0 
            && strcmp(token[0], "quit") != 0 
            && strcmp(token[0], "showpids") != 0
            && strcmp(token[0], "history") != 0)
            {
                printf("%s: Command not found\n", token[0]);
            }
            return 0;
        }

        // Inside the parent process
        else if (pid > 0)
        {
            wait(&child_status);
            // While in parent, if the "cd" command is passed, then change directory
            if (strcmp(token[0], "cd") == 0)
            {
                chdir(token[1]);
            }

            // While in parent, quit the shell when user specifies
            if ((strcmp(token[0], "exit") == 0) || strcmp(token[0], "quit") == 0)
            {
                exit(0);
            }

            // Print the child pids of the parent process
            if (strcmp(token[0], "showpids") == 0)
            {
                int j = 0;
                while (pid_array[j] != '\0' && j != 15)
                {
                    printf("[%d]: %d\n", (j+1), pid_array[j]);
                    j++;
                }
            }

            // Check if the pid array is too full-- if not then continue adding on.
             if (x <= 14)
            {
                pid_array[x] = pid;
                x++;
            }
            // If array hit max, set increment var to 0
            else if (x >= 14)
            {
                x = 0;
                pid_array[x] = pid;
                x++;
            }

        }
    }

    free( working_root );

  }

  return 0;
}
