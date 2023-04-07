// P2-SSOO-22/23

// MSH main file
// Write your msh source code here

// #include "parser.h"
#include <stddef.h> /* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define MAX_COMMANDS 8

// ficheros por si hay redirección
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Saliendo del MSH **** \n");
	// signal(SIGINT, siginthandler);
	exit(0);
}

/* Timer */
pthread_t timer_thread;
unsigned long mytime = 0;

void *timer_run()
{
	while (1)
	{
		usleep(1000);
		mytime++;
	}
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char ***argvv, int num_command)
{
	// reset first
	for (int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for (i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}

/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[])
{

	/**** Do not delete this code.****/
	int end = 0;
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO))
	{
		cmd_line = (char *)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF)
		{
			if (strlen(cmd_line) <= 0)
				return 0;
			cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush(stdin);
			fflush(stdout);
		}
	}

	pthread_create(&timer_thread, NULL, timer_run, NULL);

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	while (1)
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
		//********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
		executed_cmd_lines++;
		if (end != 0 && executed_cmd_lines < end)
		{
			command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
		}
		else if (end != 0 && executed_cmd_lines == end)
		{
			return 0;
		}
		else
		{
			command_counter = read_command(&argvv, filev, &in_background); // NORMAL MODE
			// printf("%d\n",command_counter);
		}
		//************************************************************************************************

		/************************ STUDENTS CODE ********************************/
		if (command_counter > 0)
		{
			if (command_counter > MAX_COMMANDS)
			{
				printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
			}
			else
			{
				// Print command
				print_command(argvv, filev, in_background);

				//for single commands
			
				//for pipes
				if(1)
				{

					// needed pipes for all cases, also need to know which pipe to use in each iteration
					int pipe_manager = 0;
					int vari = 1;
					int fds[2][2];
					// loop throw all the commands
					for (int i = 0; i < command_counter; i++)
					{
						pipe(fds[pipe_manager]);
						int pid1 = 0;
						pid1 = fork();

						// padre
						if (pid1 != 0)
						{
							
							if (i != 0) {
								//close the file descriptors of the previous pipe, even though we close them in the child we need to make sure we do the same in the parent so the EOF can reach dependent childs
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);
							}
							
							if (i != command_counter - 1) {
									// change pipe manager and variation
									vari = -vari;
									pipe_manager = 1 - pipe_manager;
								} 
							else{
								int status;
								// check if the process is a background process, if not we wait
								if (!in_background)
								{
									// SOLAMENTE TIENE QUE ESPERAR EN EL CASO DEL ULTIMO
									while (pid1 != wait(&status));
								}
								else
								{
									// if background
									printf("[%d]\n", getpid());
									fflush(stdout);
								}
								
							}

						}
						// hijo
						else
						{
							// primer elemento
							if (i == 0)
							{
								//only output
								if(command_counter>1){
									close(STDOUT_FILENO);
									dup(fds[pipe_manager][STDOUT_FILENO]);
									close(fds[pipe_manager][STDOUT_FILENO]);
									close(fds[pipe_manager][STDIN_FILENO]);
								}
							
							}
							// ultimo elemento (la salida es la estandar)
							else if (i == command_counter - 1)
							{
								// entrada
								
								close(STDIN_FILENO);
								dup(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);
							
							}
							// elementos del medio
							else
							{
								// entrada
								close(STDIN_FILENO);
								dup(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);

								// salida
								close(STDOUT_FILENO);
								dup(fds[pipe_manager][STDOUT_FILENO]);
								close(fds[pipe_manager][STDOUT_FILENO]);
								close(fds[pipe_manager][STDIN_FILENO]);

								
							}
							
							getCompleteCommand(argvv, i);
								int exec_result = execlp(argv_execvp[0], argv_execvp[0], argv_execvp[1], argv_execvp[2], argv_execvp[3], argv_execvp[4], argv_execvp[5], argv_execvp[6], argv_execvp[7], NULL);
								// check for errors
								if (exec_result == -1)
								{
									perror("COMMAND NOT FOUND");
								}
								// by closing the child we are also closing its file descriptors
								exit(EXIT_SUCCESS);
						}
					
					
					}
				}
			}
		}
	}

	return 0;
}