#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mysh-common.h"


static char * io[2];
static struct program ps[255];
int wantArgs = 0;
int wantIOs = 0;
int wantPipes = 0;
// ReadInput
// This method reads a set of sequentia characters and
// initialize some checking process(more than one argument,ctrl+d)
// It returns zero for false,one for true and exit if you press ctrl+d.

int readInput(char * input)
{
	char ch;
	int x = 0;
	pipeNumber = 0;

	while((ch = getchar()) != '\n' && ch != EOF)
	{
		if(ch == '|') pipeNumber++;
		if(ch == ' ' && !wantArgs)
		{
			printf("%s\n", "This shell does not support arguments,nor pipelines.");

			int temp;
			while((temp = getchar()) != '\n');

			return 0;
		}
		else if ((++x) > MAX)
		{
			printf("%s\n", "Your input is too long.");
			return 0;
		}
		input[x-1] = ch;

	}

	if(ch == EOF) exit(1);
	if(x == 0) return 0;
	input[x] = '\0';
	return 1;	
}

// ReadArguments
// Read the input from the stdin and tokenize it
// to obtain the arguments for later use.

void tokenizeInput(char * input,const char * delimeter)
{
	char * tokArray;
	tokArray = strtok(input,delimeter);
	int i=0;
	argc = 0;
	while( tokArray != NULL)
	{
		argv[argc++] = tokArray;
		tokArray = strtok(NULL,delimeter);
		if(tokArray != NULL && tokArray[0] == '|' && wantPipes)
		{
			argv[argc] = '\0';
			argc--;
			initializeIO();
			loadCommands(&ps[i]);
			ps[i].argc = argc;
			ps[i].input = io[0];
			ps[i].output = io[1];

			argc=0;
			i++;
			tokArray = strtok(NULL,delimeter);
		}
	}
	argv[argc] = '\0';
	argc--;
	if(wantPipes)
	{
		initializeIO();
		loadCommands(&ps[i]);
		ps[i].argc = argc;
		ps[i].input = io[0];
		ps[i].output = io[1];
	}
}

void loadCommands(struct program * ps)
{
	int i;
	for(i =0; i<=ps->argc; i++)
		ps->argv[i] = NULL;

	for(i=0; i<=argc; i++)
		ps->argv[i] = argv[i];
}

int hasIO()
{
	int i = 0;
	in = 0 , out = 0;
	char ch;
	while(i < argc)
	{
		ch = *argv[i];

		if(ch == '<')
		{
			if(in > 0)
			{
				printf("%s\n","There is over one input emacs.");
				return 0;
			}
			in++;

			indexInput = i;
		}
		else if(ch == '>')
		{
			if(out > 0)
			{
				printf("%s\n","There is over one output emacs.");
				return 0;
			}
			out++;

			indexOutput = i;
		}
		i++;
	}
	return 1;
}

void initializeIO()
{
	int fin,fout;

	if(!hasIO()) return;

	if(out > 0)
	{
		fout = creat(argv[indexOutput + 1],0644);
		io[1] = argv[indexOutput+1];
		if(fout < 0)
		{
			printf("%s\n","Error with output file!");
			exit(1);
		}
		if(!wantPipes) dup2(fout,STDOUT_FILENO);
		argv[indexOutput] = NULL;
		argv[indexOutput + 1] = NULL;
		argc -= 2;
		close(fout);
	} else io[1] = "void";

	if(in > 0)
	{
		fin = open(argv[indexInput + 1],O_RDONLY,0);
		io[0] = argv[indexInput+1];
		if(fin < 0)
		{
			printf("%s\n","Error with input file");
			exit(1);
		}
		if(!wantPipes) dup2(fin,STDIN_FILENO);
		argv[indexInput] = NULL;
		argv[indexOutput + 1] = NULL;
		argc -= 2;
		close(fin);
	} else io[0] = "void";
}

// ExecuteArguments
// Take the arguments and execute them.
// Everytime the father process create a child process
// to execute the given command.

void executeArguments()
{
	pid_t pid;
	int waitStatus;

	pid = fork();

	if(pid < 0)
	{
		printf("%s\n","Fork Failed!!!");
		return;
	}
	else if(pid == 0)
	{
		if(wantIOs) initializeIO();
		if(execvp(argv[0],argv) < 0)
		{
			printf("ERROR: Execute Failed\n");
			exit(1);
		}
	}
	else
		while(wait(&waitStatus) != pid);
}

void executePipe()
{
	int i;
	int pipeIN = STDIN_FILENO;
	int fd[2];
	int openPipe = 0;

	for(i = 0; i <= pipeNumber - 1; i++)
	{
		// Pipe processing
		if(pipe(fd) != 0)
		{
			printf("Piping error\n");
			return;
		}
		openPipe = 1;

		// Create a process and handle the operations
		// for using the pipe.
		if(!forkPipe(ps[i],pipeIN,fd[1],fd))
		{
			printf("Error during execution!\n");
			return;
		}
		
		// We close this output because we don't want the parent to write.
		close(fd[1]);
		
		// We are keeping the input.The parent will feed it to the next child.
		pipeIN = fd[0];
	}
	if(!forkPipe(ps[i],pipeIN,STDIN_FILENO,fd))
	{
		printf("Error during execution!\n");
		return;
	}
	if(openPipe)
	{
		close(fd[0]);
		close(fd[1]);
	}
	

}

int forkPipe(struct program ps,int pipeInput,int pipeOutput,int fd[])
{
	pid_t pid,waitStatus;
	int fail = 0;
	int input,output;
	pid = fork();
	if(pid < 0) return 0;
	else if(pid == 0) // child process
	{
		// if the child doesn't have to read from the standard io.
		// Must read from pipe
		// We redirect the input to be from the fd[0]
		if(pipeInput != STDIN_FILENO) dup2(pipeInput,0);
		else close(fd[0]); // close the input since we don't read from the pipe

		if(ps.input == NULL)
		{
			input = open(ps.input,O_RDONLY);
			dup2(input,0);
		}

		if(pipeOutput != STDIN_FILENO) dup2(pipeOutput,1);
		else close(fd[1]);

		if(strcmp(ps.output,"void"))
		{
			output = creat(ps.output,0644);
			dup2(output,1);
		}
		
		if(execvp(ps.argv[0],ps.argv) < 0)
		{
			fail = 1;
			exit(1);
		}
	}
	else 
	{
		while(wait(&waitStatus) != pid);
		if (fail == 1) return 0;
	}
	return 1;
}

void setOperation(int wantArg,int wantIO,int wantP)
{
	wantArgs = wantArg;
	wantIOs = wantIO;
	wantPipes = wantP;
}

