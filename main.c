#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define H_BUF_SIZE 10	/*	10 recently used commands	*/
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

typedef struct
{
	char arr[H_BUF_SIZE][MAX_LINE];
	int sz;
} h_buf_t;

h_buf_t h_buf = { {"\0"}, 0 };

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */

int setup(char *args[],int *background)
{
	static char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
		exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++)
    {
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t' :               /* argument separators */
			if(start != -1)
			{
				args[ct] = &inputBuffer[start];    /* set up pointer */
				ct++;
			}
			inputBuffer[i] = '\0'; /* add a null char; make a C string */
			start = -1;
			break;

		case '\n':                 /* should be the final char examined */
			if (start != -1){
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; /* no more arguments to this command */
			break;

		default :             /* some other character */
			if (start == -1)
				start = i;
			if (inputBuffer[i] == '&')
			{
				*background  = 1;
				inputBuffer[i] = '\0';
			}
		}
	}
	args[ct] = NULL; /* just in case the input line was > 80 */

	if (!ct || (ct >= 1 && iscntrl(args[0][0])))
		return 0;
	return 1;
}

int main(void)
{
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2+1];/* command line (of 80) has max of 40 arguments */
    char prompt[MAX_LINE];	/*	prompt string	*/
    pid_t pid;				/*	pid of child	*/
    char out[MAX_LINE];

    strcpy(prompt, "rsh->");
	while (1)            /* Program terminates normally inside setup */
    {
		background = 0;
		write(STDOUT_FILENO, prompt, strlen(prompt));

        if (setup(args,&background))       /* get next command */
		{
		    if (!strcmp(args[0], "l"))
			{
				int i = h_buf.sz > H_BUF_SIZE ? h_buf.sz%H_BUF_SIZE : 0;
				int c = h_buf.sz > H_BUF_SIZE ? H_BUF_SIZE : h_buf.sz;
				int j = 0;

				while (c--)
				{
					sprintf(out, "[%d] %-10s  (%d)\n", h_buf.sz-c, h_buf.arr[i], j++);
					write(STDOUT_FILENO, out, strlen(out));
					i = (i+1)%H_BUF_SIZE;
				}
				continue;
			}
			else if (strlen(args[0])==2 && args[0][0] == 'l' && isdigit(args[0][1]))
			{
				int c = args[0][1]-'0';
				if (h_buf.sz <= c)
				{
					fprintf(stderr, "rsh: %s shortcut not valid!!\n", args[0]);
					continue;
				}
				else
				{
					int i = h_buf.sz > H_BUF_SIZE ? h_buf.sz%H_BUF_SIZE : 0;
					i = (i+c)%H_BUF_SIZE;
					strcpy(args[0], h_buf.arr[i]);
				}
			}
			pid = fork();

			if (pid < 0)
			{
				perror("Fork failed!!\n");
			}
			else if (!pid)
			{
				if (execvp(args[0], args) == -1)	//	error
				{
					fprintf(stderr, "rsh: %s not found!!\n", args[0]);
					exit(-1);
				}
			}
			if (!background)
			{
				wait(NULL);
				strcpy(h_buf.arr[h_buf.sz++%H_BUF_SIZE], args[0]);		/*	save for history	*/
			}
		}
    }
    return 0;
}
