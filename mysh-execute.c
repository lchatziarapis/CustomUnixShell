
#include "mysh-common.h"

int main(void)
{
	char input[MAX + 1];
	setOperation(1,1,1);
	while(1)
	{
		printf("%s","Shell05>");
		if(readInput(input))
		{
			
			tokenizeInput(input," ");

			executePipe();
			
		}
	}
	return 0;
}


