#include <stdio.h>
#define MAX 255

char * argv[MAX+1];
int indexInput,indexOutput;
int in , out;
int argc;
static int pipeNumber = 0;

struct program
{
char * argv[MAX+1];
int argc;
char * input;
char * output;
};

int readInput(char * input);
int hasIO();
int forkPipe(struct program ps,int pipeInput,int pipeOutput,int fd[]);
void tokenizeInput(char * input,const char * delimeter);
void initializeIO();
void executeArguments();
void executePipe();
void loadCommands();
void setOperation();

