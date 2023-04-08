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

//mycalc variable
int Acc = 0;
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


//handle input redirectionn

//handle standart output redirection
void * handle_st_output(void * arg){
		close(STDOUT_FILENO);
		dup((long)arg);
		close((long)arg);
		pthread_exit(NULL);
	}

//handle standart error redirection
void * handle_st_error(void * arg){
		close(STDERR_FILENO);
		dup((long)arg);
		close((long)arg);
		pthread_exit(NULL);
	}

//handle standart input redirection
void * handle_st_input(void * arg){
		close(STDIN_FILENO);
		dup((long)arg);
		close((long)arg);
		pthread_exit(NULL);
	}


//mycalc

void myCalc(char ***argvv){
	char * operand = argvv[0][2];
	//check myCalc structure, in case is wrong return error
	if((argvv[0][1]==NULL)||(argvv[0][2]==NULL)||(strcmp(operand, "add") != 0 && strcmp(operand, "mul") != 0 && strcmp(operand, "div") != 0 )||(argvv[0][3]==NULL)){
		char * e_message = "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n";
		write(STDOUT_FILENO, e_message, strlen(e_message));
	}
	else{
		//conver strings to integer and set variables
		int op1 = atoi(argvv[0][1]);
		int op2 = atoi(argvv[0][3]);
		int res = 0;
		int reminder=0;
		char message[100];
		//Three ifs for the calculator logic
		if(strcmp(operand, "add") == 0 ){
			res = op1 + op2;
			Acc = Acc + res;
			sprintf(message,"[OK] %d + %d = %d; Acc %d\n",op1,op2,res,Acc);

		}
		else if (strcmp(operand, "mul") == 0 ){
			res = op1 * op2;
			sprintf(message,"[OK] %d * %d = %d\n",op1,op2,res);
		}
		else{
			res = op1 / op2;
			reminder = op1 % op2;
			sprintf(message,"[OK] %d / %d = %d; Resto %d\n",op1,op2,res,reminder);
		}
		write(STDERR_FILENO, message, strlen(message));
	}
}


void getMyTime( ){
	int hours, minutes , seconds = 0;
	unsigned long total_seconds = mytime/1000;
	hours = total_seconds / 3600;
	//los que sobran de las horas
	total_seconds %=3600;

	minutes = total_seconds / 60;
	//los que sobran de los minutos
	seconds = total_seconds % 60;

	char message[50];
	sprintf(message,"%02d:%02d:%02d\n", hours, minutes, seconds);
	write(STDERR_FILENO, message, strlen(message));
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

		//One thread for each type of redirection
		int NUM_THREADS = 3;
		pthread_t threads[NUM_THREADS];

	
		//Save  STDOUT_FILENO STDERR_FILENO STDIN_FILENO so we can restore them later
		int saved_st_out, saved_st_err, saved_st_input;

		if (command_counter > 0)
		{
			if (command_counter > MAX_COMMANDS)
			{
				printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
			}
			else
			{
				// Print command
				//print_command(argvv, filev, in_background);

				//Check for internal command mycalc
				if(strcmp(argvv[0][0], "mycalc") == 0){
						myCalc(argvv);
				}

				else if(strcmp(argvv[0][0],"mytime")==0){
					
					getMyTime();
				}

				else {
					//for single commands
					//for pipes
					
					//pipe manager tells the current pipe in the loop
					int pipe_manager = 0;
					//variation is used to access the pipe of the previus loop in order to connect processes, to do so we do pipe_manager + variation
					int vari = 1;
					//double array for storing two pipes
					int fds[2][2];
					// loop throw all the commands (each command will be a child process)
					for (int i = 0; i < command_counter; i++)
					{
						//standart output redirection (only before creating last child)
						if(i==command_counter-1 && filev[1][0]!='0'){
							saved_st_out = dup(STDOUT_FILENO);
							int fd = creat(filev[1], 0666);
							pthread_create(&threads[1],NULL, handle_st_output,(void *)(long)fd);
							pthread_join(threads[1],NULL);
						}

						//standart error redirection (only before creating last child)
						if(i==command_counter-1 && filev[2][0]!='0'){
							saved_st_err = dup(STDERR_FILENO);
							int fd = creat(filev[2], 0666);
							pthread_create(&threads[2],NULL, handle_st_error,(void *)(long)fd);
							pthread_join(threads[2],NULL);
						}

						//standart input redirection (only in first child)
						if(i==0 && filev[0][0]!='0'){
							saved_st_input = dup(STDIN_FILENO);
							int fd = open(filev[0], O_RDWR);
							pthread_create(&threads[0],NULL, handle_st_input,(void *)(long)fd);
							pthread_join(threads[0],NULL);
						}


						//create the pipe using pipe_manager
						pipe(fds[pipe_manager]);
						int pid = 0;
						pid = fork();

						// padre
						if (pid != 0)
						{
							
							if (i != 0) {
								//only after the first child, because first child does not have previous pipe
								//close the file descriptors of the previous pipe, even though we close them in the child we need to make sure we do the same in the parent so the EOF can reach dependent childs
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);
							}
							
							if (i != command_counter - 1) {
									// change pipe manager and variation for next loop (of course, this does not affect current child)
									vari = -vari;
									pipe_manager = 1 - pipe_manager;
								} 
							else{
								int status;
								// check if the process is a background process, if not we wait
								if (!in_background)
								{
									// SOLAMENTE TIENE QUE ESPERAR EN EL CASO DEL ULTIMO
									while (pid != wait(&status));
								}
								else
								{
									// if background
									printf("[%d]\n", pid);
									fflush(stdout);
								}

								//restore file descriptors
								if(filev[0][0]!='0'){
									close(0);
									dup(saved_st_input);
									close(saved_st_input);
								}
								if(filev[1][0]!='0'){
									close(1);
									dup(saved_st_out);
									close(saved_st_out);
								}
								if(filev[2][0]!='0'){
									close(2);
									dup(saved_st_err);
									close(saved_st_err);
								}
							}
						}
						// hijo
						else
						{
							// first element of the pipe
							if (i == 0)
							{
								//only output
								//this if check is used to also handle single commands
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
								// entrada is the last pipe, not the current pipe that is why we use vari
								close(STDIN_FILENO);
								dup(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);
							
							}
							// elementos del medio
							else
							{
								// entrada is the last pipe, not the current pipe that is why we use vari
								close(STDIN_FILENO);
								dup(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDIN_FILENO]);
								close(fds[pipe_manager + vari][STDOUT_FILENO]);

								// salida is the current pipe
								close(STDOUT_FILENO);
								dup(fds[pipe_manager][STDOUT_FILENO]);
								close(fds[pipe_manager][STDOUT_FILENO]);
								close(fds[pipe_manager][STDIN_FILENO]);	
							}
							
							//get command arguments
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