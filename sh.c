#define BUFFER_SIZE 1024
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  

/*function declarations*/
int set_redirection(char *prev_tok, int type);
char **parseline(char *cmdline, char **parsed_cmdline);
int cd_helper(char**cmd_args);
int rm_helper(char**cmd_args, int argc);
int ln_helper(char**cmd_args, int argc);
int evaluate(char **cmd_args, int argc);
int clear_buf(char *buf);
int safe_write(int fd, const void *buf, size_t count);
int main(int argc, char** argv);

/*Global Variables*/
char *redir_info[2] = {NULL, NULL};
char *builtin_cmds[8] = {"cd", "/bin/cd", "ln", "/bin/ln", "rm", "/bin/rm", "exit", "/bin/exit"};
int redir_type = NULL;
int redir_okay = 1;
int redir_true = 0;

/*
* parseline()
*
* - Description: parses the commandline, puts each argument 
*   into a new array and finds any redirection symbols. 
*
* -Arguments: the commandline: a pointer to an array of char *s, 
*	parsed commandline: a pointer to an empty aray of char *s where the 
*	arguments from the commandline will be placed
*
* - Return value: the parsed commandline
*/
char **parseline(char *cmdline, char **parsed_cmdline) {
	redir_okay = 1; //denotes whether redirection is allowed
	redir_true = 0; //denotes whether a redirection has been detected
	int pos= 0;
	int redir_now = 0; //denotes whether a redirection is currently being found
	char *tok;
	char *prev_tok = NULL;
	char *redir_symb[3] = {"<", ">", ">>"};
	const char *delim = " \n	";

	//find each token in the commandline
	tok = strtok(cmdline, delim);

	while (tok!=NULL) {
		if (strcmp(tok, redir_symb[0]) == 0) {
			//check for < symbols
			redir_now = set_redirection(prev_tok, 0);
		} else if (strcmp(tok, redir_symb[1]) == 0) {
			//check for > symbols
			redir_now = set_redirection(prev_tok, 1);
		} else if (strcmp(tok, redir_symb[2]) == 0) {
			redir_now = set_redirection(prev_tok, 2);
		} else if (redir_now) {
			redir_info[1] = tok;
			redir_now = 0;
		} else {
			//place all non-redir symbols into a new array of arguments
			parsed_cmdline[pos] = tok;
			pos++;
		}
		//track the previous argument for redirection sake
		prev_tok = tok;
		tok = strtok(NULL, delim);
	}
	parsed_cmdline[pos] = NULL;
	return parsed_cmdline;
}

/*
* set_redirection()
*
* - Description: checks if a redirection symbol has already been
*	found in the command line and sets the redirecion type, input
*	filename, and output filename.
*
* -Arguments: a char * the -input file and an int -the type of redirection
*				0 = > and 1 = << or <
*
* - Return value: an integer; 1 if a redirection is taking place, 0 if not
*/
int set_redirection(char *prev_tok, int type) {
	if(redir_okay) {
		redir_type = type;
		redir_info[0] = prev_tok;
		redir_okay = 0;
		redir_true = 1;
	} else {
		perror("ERROR: multiple redirection symbols");
		redir_true = 0;
		return 0;
	}
	return 1;
}

/*
*  check_builtins()
*
* - Description: checks the first argument of the array of
*	commandline arguments to see if it is a builtin command and
*	calls the command helper if it is a builtin
*
* -Arguments: a char *array of the parsed commandline arguments
*
* - Return value: an int, 0 if the first argument is not a builtin 
*	function, 1 if the builtin command was executed successfully
*/
int check_builtins(char **cmd_args, int argc) {
	if (strcmp(cmd_args[0], builtin_cmds[0]) == 0 || \
	 strcmp(cmd_args[0], builtin_cmds[1]) == 0) {
		return cd_helper(cmd_args);
	
	} else if (strcmp(cmd_args[0], builtin_cmds[2]) == 0 || \
	 strcmp(cmd_args[0], builtin_cmds[3]) == 0) {
		return ln_helper(cmd_args, argc);
	
	} else if (strcmp(cmd_args[0], builtin_cmds[4]) == 0 || \
	 strcmp(cmd_args[0], builtin_cmds[5]) == 0) {
		return rm_helper(cmd_args, argc);
	
	} else if (strcmp(cmd_args[0], builtin_cmds[6]) == 0 || \
	 strcmp(cmd_args[0], builtin_cmds[7]) == 0) {
		exit(0);
	}
	return 0;
}

/*
* cd_helper()
*
* - Description: calls chdir() if the commandline was given 
*	enough arguments
*
* - Arguments: a char *array of the parsed commandline arguments
*
* - Return value: an int; 1 if the system call was executed corrctly
*/
int cd_helper(char**cmd_args) {
	if(cmd_args[1] == NULL) {
		safe_write(STDERR_FILENO, "cd: syntax error", 16); 
	}else if (chdir(cmd_args[1]) == -1) {
		safe_write(STDERR_FILENO, "cd: syntax error", 20);
	}
	return 1;
}

/*
* ln_helper()
*
* - Description: calls link() if the commandline was given 
*	enough arguments
*
* - Arguments: a char *array of the parsed commandline arguments
*
* - Return value: an int; 1 if the system call was executed corrctly
*/
int ln_helper(char**cmd_args, int argc) {
	if(argc < 1 || argc > 4) {
		perror("ln: 3 args expected");
	} else if(cmd_args[1] == NULL) {
		perror("ln: missing file operand");
	} else if(cmd_args[2] == NULL) {
		perror("ln: missing file destination");
	} else {
		if (link(cmd_args[1], cmd_args[2]) == -1) {
			perror("ln");
		}
	}
	return 1;
}

/*
* rm_helper()
*
* - Description: calls unlink() if the commandline was given 
*	enough arguments
*
* - Arguments: a char *array of the parsed commandline arguments
*
* - Return value: an int; 1 if the system call was executed corrctly
*/
int rm_helper(char**cmd_args, int argc) {
	if(argc < 1 || argc > 3) {
		perror("rm: 2 args expected");
	}else if(cmd_args[1] == NULL || cmd_args == '\0') {
		perror("rm: missing file operand");
	} else {
		if (unlink(cmd_args[1]) == -1) {
			perror("rm");
		}
	}
	return 1;
}

/*
* redirection_helper()
*
* - Description: redirects the standard input/output if the user has 
*	indicated redirection
*
* - Arguments: None
*
* - Return value: an int; 1 if the function executed correctly
*/
int redirection_helper() {
	int fd;
	if(redir_info[0] == NULL && redir_info[1] == NULL) {
		perror("ERROR - No Redirection file specified");
	}
	if(redir_type == 0) {
		//input redir <
		if(redir_info[1] == NULL){
			perror("ERROR - No Redirection file specified");
		}
		fd = open(redir_info[1], O_RDWR, 0666);
		if(fd == -1){
			perror("<");
		}
		dup2(fd, STDIN_FILENO);
		if(close(fd) == -1){
			perror("< close");
		}	
	}else if(redir_type == 1) {
		//outpt redir >
		fd = open(redir_info[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
		if(fd == -1){
			perror(">");
		}
		dup2(STDOUT_FILENO, fd);
		if(close(fd) == -1){
			perror("> close");
		}
	}else if(redir_type == 2) {
		fd = open(redir_info[1], O_APPEND | O_CREAT, 0666);
		if(fd == -1){
			perror(">>");
		}
		dup2(STDOUT_FILENO, fd);
		if(close(fd) == -1){
			perror(">> close");
		}
	}
	return 0;
}

/*
* evaluate()
*
* - Description: checks parsed commandline args for builtin functions,
*		if none are found, it calls fork() to create a new process and 
*		then calls execv() on the arguments from the commandline
*
* - Arguments: a char *array of the parsed commandline arguments
*
* - Return value: an int, 0
*/
int evaluate(char **cmd_args, int argc) {
	pid_t pid;

	//if nothing has been entered, return
	if(cmd_args[0] == NULL) {
		return 0;
	}
	//check for redirection
	if(redir_true){
		redirection_helper();
	}
	//check for built-in commands
	if(check_builtins(cmd_args, argc) == 1) {
		return 0;
	}

	pid = fork();
	if(pid == 0) {
		//pid was successful, handoff to child
		if(execv(cmd_args[0], cmd_args) == -1) {
			// printf("%c\n", cmd_args[2]);
			perror("ERROR execv");
		} 
		//if this returns, exit
		exit(0);
	}else if (pid < 0) {
		perror("ERROR");
	} else {
		wait(0);
	}
	return 0;
}

int clear_buf(char *buf) {
	size_t j = 0;
	for(j = 0; j< BUFFER_SIZE; j++){
		buf[j] = '\0';
	}
	return 0;
}
int safe_write(int fd, const void *buf, size_t count) {
	if (write(fd, buf, count) < 0) {
		perror("write");
	}
	return 0;
} 

/*
 * Main function
 *
 * Arguments:
 *	- argc: the number of command line arguments - for this function 9
 *	- **argv: a pointer to the first element in the command line
 *            arguments array 
 * Return value: 0 if program exits correctly, 1 if there is an error
 */
int main(int argc, char** argv) {
	char *parsed_cmdline[BUFFER_SIZE];
	char buf[BUFFER_SIZE];
	char **line;
	ssize_t num_read;

	if(argc == 0 || !argv[0]) {
		return 1;
	}

	//infinite REPL loop
	while(1) {
		#ifdef PROMPT

		//setup prompt for commandline
		const char *prompt = " $ ";
		char full_prompt[100];
		getcwd(full_prompt, 100);
		strcat(full_prompt, prompt);
		size_t len = strlen(full_prompt);

		//write prompt to the commandline
		safe_write(STDOUT_FILENO, full_prompt, len);
		#endif 

		//read command line
		num_read = read(STDIN_FILENO, buf, BUFFER_SIZE);
		if(num_read == 0){
			exit(0);
		}else if(num_read == -1){
			perror("READ ERROR");
		}

		//pase the command line
		line = parseline(buf, parsed_cmdline);

		//evaluate and execute the command line
		evaluate(line, argc);
		clear_buf(buf);
	}
	return 0;
}