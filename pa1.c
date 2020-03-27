/**********************************************************************
 * Copyright (c) 2019
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "types.h"
#include "parser.h"

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
/**
 * String used as the prompt (see @main()). You may change this to
 * change the prompt */
static char __prompt[MAX_TOKEN_LEN] = "$";

/**
 * Time out value. It's OK to read this value, but ** DO NOT CHANGE
 * DIRECTLY **. Instead, use @set_timeout() function below.
 */
static unsigned int __timeout = 2;

static void set_timeout(unsigned int timeout)
{
	__timeout = timeout;

	if (__timeout == 0) {
		fprintf(stderr, "Timeout is disabled\n");
	} else {
		fprintf(stderr, "Timeout is set to %d second%s\n",
				__timeout,
				__timeout >= 2 ? "s" : "");
	}
}
/*          ****** DO NOT MODIFY ANYTHING UP TO THIS LINE ******      */
/*====================================================================*/
/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

static int run_command(int nr_tokens, char *tokens[])
{
	/* This function is all yours. Good luck! */
	if (strncmp(tokens[0], "exit", strlen("exit")) == 0) {
		return 0;
	}
	else if(strncmp(tokens[0], "for",strlen("for")) == 0){
		int size = 0;
		while(tokens[size] != NULL){
			size++;
		}
		size++;
		int forN = 0;
		for(int i=0 ; i < size-1 ; i++){
			if(strncmp(tokens[i],"for",strlen("for"))==0)
				forN++;
		}
		int index = size - 2*forN;
		char* newTokens[index];
		int j=0;
		for(int i = forN*2 ; i < size -1 ; i++){
			newTokens[j] = tokens[i];
			j++;
		}
		newTokens[index-1] = NULL;
		int forLoop=1;
		for(int i=1 ; i < forN*2; i+=2){
			forLoop = forLoop * atoi(tokens[i]);
		}
		if( strncmp(tokens[forN*2],"cd", strlen("cd")) ==0){
			for(int i=0 ; i < forLoop ; i++){
				if(strcmp(newTokens[1], "~") == 0) chdir(getenv("HOME"));
				else chdir(newTokens[1]);
			}
		}
		else{
			for(int i=0 ; i<forLoop ; i++){
				pid_t pid;
				int status;
				pid = fork();
				if(pid==0){
					if(execvp(newTokens[0],newTokens)==-1) 
						fprintf(stderr,"No such file or directory\n");
					close(0);
					exit(0);
				}else
					wait(&status);
			}
			if(execvp(tokens[0],tokens)==-1) 
				fprintf(stderr,"No such file or directory\n");
		}
		return 1;
	}
	else if(strncmp(tokens[0] , "prompt", strlen("prompt")) == 0){
		strcpy(__prompt,tokens[1]);
		return 1;
	}	
	else if( strncmp(tokens[0] , "cd", strlen("cd")) == 0 ){
		if(strcmp(tokens[1], "~") == 0) chdir(getenv("HOME"));
		else chdir(tokens[1]);
		return 1;
	}
	
	else if(strncmp(tokens[0] , "timeout", strlen("timeout")) == 0){
		if(tokens[1]==NULL){
			fprintf(stderr,"Current time out is %d second\n",__timeout);
		}
		else{
			set_timeout(atoi(tokens[1]));
		}
		return 1;
	}
	
	pid_t pid;
	int status=-1;
	pid = fork();

	if(pid==0){		//child process
		close(0);
		alarm(__timeout);
		if(execvp(tokens[0],tokens)==-1){
			fprintf(stderr,"No such file or directory\n");
		}
		exit(0);
		}
	else{	//parent process
		wait(&status);
		if(WIFSIGNALED(status)) 
			fprintf(stderr,"%s is timed out\n", tokens[0]);
	}

	return 1;
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{
}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */

static bool __verbose = true;
static char *__color_start = "[0;31;40m";
static char *__color_end = "[0m";

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;
	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	if (__verbose)
		fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);

	while (fgets(command, sizeof(command), stdin)) {	
		char *tokens[MAX_NR_TOKENS] = { NULL };
		int nr_tokens = 0;

		if (parse_command(command, &nr_tokens, tokens) == 0)
			goto more; /* You may use nested if-than-else, however .. */

		ret = run_command(nr_tokens, tokens);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			fprintf(stderr, "Error in run_command: %d\n", ret);
		}

more:
		if (__verbose)
			fprintf(stderr, "%s%s%s ", __color_start, __prompt, __color_end);
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
