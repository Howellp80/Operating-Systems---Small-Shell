/*******************************************************************************
 * smallsh.c
 * Parker Howell
 * CS 344
 * 11/15/17
 * Description - This is a small shell program with 3 built in commands (exit,
 * cd, and status. Non-built in commands will be forked and exec'd and may be
 * ran in the background by including '&' at the end of your user command.
 * Additionally the shell supports input and output re-direction with the use 
 * of '<' and/or '>' followed by filenames.
 *
 * The general syntax of a command is:
 * command [arg1 arg2 ...] [< input_file] [> outputfile] [&]
 * ... where items in [] are optional.
 *
 * ****************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


#define MAXINPUT 2048    // number of chars a user can enter at prompt
#define MAXARGS 512      // most arguments in that string can be composed of

// needed for signal handler
bool canRunBG = true;      // can user run background process? 


/*******************************************************************************
 * printPrompt
 * prints the prompt for the user shell input. 
 *
 * ****************************************************************************/
void printPrompt(void){
	printf(": ");
	fflush(stdout);
}




/*******************************************************************************
 * tokenizeInput
 * goes through the userInput string and seperates the string into tokens by
 * spaces, saving each token in the userCmds array. Also tracks the amount of 
 * tokens saved in the cmdCount variable.
 *
 * ****************************************************************************/
void tokenizeInput(char* userInput, char** userCmds, int* cmdCount){
	//printf("in tokenize\n");
	char* token;      // stores each token form strtok
	
	// get first token
	token = strtok(userInput, " ");
	// while we havent reached the end of the input
	while (token != NULL){
		// allocate space for the token
		userCmds[*cmdCount] = calloc(50, sizeof(char));
		// copy the token into the array
		strcpy(userCmds[*cmdCount], token);
		// track how many commands we have
		(*cmdCount)++;
		// get the next token
		token = strtok(NULL, " ");
	}
}




/*******************************************************************************
 * printInput
 * loops through userCmds array and prints each command to the console.
 *
 * ****************************************************************************/
void printInput(char** userCmds, int cmdCount){	
	printf("tokenized input is:\n");
	int i;
	for (i = 0; i < cmdCount; i++){
		printf("%s\n", userCmds[i]);
	} 
}




/*******************************************************************************
 * expandPID
 * loops through array of user commands and if any contain '$$', then the 
 * process ID for this program's process is appended in its place. 
 *
 *
 * to do:
 * I changed how this function works to remove all instances of '$$' from any
 * command instead of just the first. With that change, how I allocate and free 
 * the variable post isnt very efficient...   it's late...
 *
 * ****************************************************************************/
void expandPID(char** userCmds, int cmdCount){
	int i;                   // tracks loop
	char* index;             // address of '$$' if found, otherwise is NULL
	char* index2;            // tracks address of just after '$$'
	bool postCheck = false;  // tracks if command hsa anything beyond '$$'
	char* post;              // holds post command if found

	// holds converted pid number as string
	char* pidBuff = calloc(10, sizeof(char));

	// loop through all the commands in array
	for (i = 0; i < cmdCount; i++){
		do {
			// if the current command contains "$$"
			if (index = strstr(userCmds[i], "$$")){
				index2 = index + 2;
				// check if there are chars after the '$$'
				if (strcmp(index2, "\0") != 0){
					// we have a string past the '$$'
					postCheck = true;
					// allocate space to store post string
					post = calloc(40, sizeof(char));
					// copy string after '$$' to post
					strcpy(post, index2);
				}
				// chop off the "$$"
				*index = '\0';
				// get and convert the PID to string
				snprintf(pidBuff, 9, "%d", getpid());
				// append the PID
				strcat(userCmds[i], pidBuff);
				// if post string, 
				// append it back on and clean up post
				if (postCheck){
					strcat(userCmds[i], post);
					free(post);
				}
			}

			// reset flag
			postCheck = false;
		// while we can still find a '$$'
		} while (index);			                         
	}
	// cleanup
	free(pidBuff);
}




/*******************************************************************************
 * changeDirectory
 * checks if a directory path argument was entered and if so changes the 
 * current workiing directory to that location. If a path argument wasn't 
 * provided, the current working directory is set to the users HOME directory.
 *
 * ****************************************************************************/
void changeDirectory(char** userCmds){
	int ret;   // stores returned value from chdir 

	// check if just 'cd' command or if has path
	if (userCmds[1] == NULL){
		// change to home directory
		//Ref:   https://tinyurl.com/yd3yk2ez
		ret = chdir(getenv("HOME"));
		// check if chdir error
		if (ret == -1){
			printf("error changing to HOME dir\n");
			fflush(stdout);
			exit(1);
		}
	}
	// else a path was specified
	else{
		ret = chdir(userCmds[1]);
		// check if chdir error
		if (ret == -1){
			printf("error changing to %s\n", userCmds[1]);
			fflush(stdout);
			exit(1);
		}
	}
}




/*******************************************************************************
 * checkIfBG
 * checks the last element of userCmds array to see if the user wanted to run
 * the process in the background. Updates the wantRunGB bool appropriately.
 *
 * ****************************************************************************/
checkIfBG(char** userCmds, int cmdCount, bool* wantRunBG){
	// if the last command is '&'
	if (strcmp(userCmds[cmdCount - 1], "&") == 0){
		//printf("user wants bg process\n");
		*wantRunBG = true;
		// free and null out the bg command so not passed to execvp
		free(userCmds[cmdCount - 1]);
		userCmds[cmdCount - 1] = NULL;
	}
	else {
		*wantRunBG = false;
	}
}




/*******************************************************************************
 * checkStatus
 * checks if the last process exited or recieved a signal and prints the 
 * cooresponding exit or signal information. Only one of either WIFEXITED or
 * WIFSIGNALED should return a non-zero value.
 *
 * ****************************************************************************/
void checkStatus(int* childExitMethod){
	//printf("in checkStatus\n");
	// check if process exited
	if (WIFEXITED(*childExitMethod) != 0){
		// get and print the exit status
		int exitStatus = WEXITSTATUS(*childExitMethod);
		printf("exit value %d\n", exitStatus);
		fflush(stdout);
	}
	// check if the process recieved a signal
	else if (WIFSIGNALED(*childExitMethod) != 0){
		// get and print the signal number
		int sigStatus = WTERMSIG(*childExitMethod);
		printf("terminated by signal %d\n", sigStatus);
		fflush(stdout);
	}
}




/*******************************************************************************
 * checkReDirect
 * checks if user wants to re-direct stdin and/or stdout. If so, file pointers 
 * are opened and set using dup2.
 *
 * ****************************************************************************/
void checkReDirect(char** userCmds, int cmdCount, bool wantRunBG, 
		int* inFile, int* outFile){
	//printf("checking for input/output re-direction\n");
	int i;                   // for looping
	int ret;                 // stores return val for error check
	bool foundIn = false;    // did we find a input redirect?
	bool foundOut = false;   // did we find a output redirect?

	// loop through all of the user commands
	for (i = 0; i < cmdCount; i++){
		// check value wasn't NULL'd out
		if (userCmds[i]){
			// if we find a input re-direct
			if (strcmp(userCmds[i], "<") == 0){
				//printf("found input re-direct\n");
				foundIn = true;
				// open the file for reading
				*inFile = open(userCmds[i + 1], O_RDONLY);
				// error check
				if (*inFile == -1){
					//perror(userCmds[i + 1]);
					printf("cannot open %s for input\n", 
							userCmds[i + 1]);
					fflush(stdout);
					exit(1);
				}
				// if we opened the file redirect stdin to it 
				ret = dup2(*inFile, 0);
				// check for dup error
				if (ret == -1){
					perror("dup2 - input redirect");
					exit(1);
				}
				//remove '<' from cmds so not passed to execvp
				free(userCmds[i]);
				userCmds[i] = NULL;
			}	
		}
		// else if we're redirecting stdout
		// check value wasn't NULL'd out
		if (userCmds[i]){	
			// if we find a output re-direct
			if (strcmp(userCmds[i], ">") == 0){
				//printf("found output re-direct\n");
				foundOut = true;
				// open the file for writing
				*outFile = open(userCmds[i + 1],
					       	O_WRONLY | O_CREAT | O_TRUNC,
					       	0644);
				// error check
				if (*outFile == -1){
					//perror(userCmds[i+ 1]);
					printf("cannot open %s for output\n",
							userCmds[i + 1]);
					fflush(stdout);
					exit(1);
				}
				// if we opened the file redirect stdout to it
				ret = dup2(*outFile, 1);
				// check for dup error
				if (ret == -1){
					perror("dup2 - output redirect");
					exit(1);
				}
				//remove '>' from cmds so not passed to execvp
				free(userCmds[i]);
				userCmds[i] = NULL;
			}
		}
	}
	// if we're running process in bg we need to redirect input and output
	// from their default values if input or output files are not provided
	if (wantRunBG && canRunBG){
		// if a redirect wasnt set then redirect to dev/null
		if (!foundIn){
			// we will read from dev/null
			*inFile = open("/dev/null", O_RDONLY);
			// error check
			if (*inFile == -1){
				perror("couldnt open dev/null for reading");
				exit(1);
			}
			// if dev/null was opened redirect to it
			ret = dup2(*inFile, 0);
			// check for dup error
			if (ret == -1){
				perror("dup2 - input from dev/null");
				exit(1);
			}
		}
		// if we didn't find any output redirects
		if (!foundOut){
			// we will write to dev/null
			*outFile = open("/dev/null", 
					O_WRONLY | O_CREAT | O_TRUNC, 0644);
			// error check
			if (*outFile == -1){
				perror("couldnt open dev/null for writing");
				exit(1);
			}
			// if dev/null was opened redirect to it
			ret = dup2(*outFile, 1);
			// check for dup error
			if (ret == -1){
				perror("dup2 - output to dev/null");
				exit(1);
			}
		}
	}
}




/*******************************************************************************
 * addPid
 * As background processes are created they are added to the pidArray and the 
 * count is incremented.
 *
 * ****************************************************************************/
void addPid(pid_t* pidArray, int* pidCount, pid_t pid){
	pidArray[*pidCount] = pid;
	(*pidCount)++;
}




/*******************************************************************************
 * removePid
 * searches for (and should find) PID of a recently finished background process.
 * When found the pid is removed and the array count is decremented
 *
 * ****************************************************************************/
int removePid(pid_t* pidArray, int* pidCount, pid_t targetPid){
	int i;
	int index = -1;

	// loop through pidArray looking for index of our target pid
	for (i = 0; i < *pidCount; i++){
		// if we find it record location and break loop
		if (pidArray[i] == targetPid){
			index = i;
			break;
		}
	}
	// if we found the index
	if (index != -1){
		// shift pids down overwriting pid we want to remove
		for (index; index < (*pidCount) - 1; index++){
		pidArray[index] = pidArray[index + 1];
		}	
		// decrement our count of pid's in the array
		(*pidCount)--;
	}
}




/*******************************************************************************
 * reapChildren
 * loops through pidArray and checks if any of the processes within have 
 * finished. If so the process completion information is printed and the 
 * pid is removed from the pidArray.
 *
 * ****************************************************************************/
void reapChildren(pid_t* pidArray, int* pidCount, int* childExitMethod){
	//printf("in reapChildren\n");
	int i;        // for looping
	int ret;      // tracks if removePid succeedes or fails
	pid_t pid;    // holds return from waitpid call
	// for each of the unreaped processes
	for (i = 0; i < *pidCount; i++){
		pid = waitpid(pidArray[i], childExitMethod, WNOHANG);
		// if we reaped a process
		if ((int)pid != 0){
			// check if process exited
			if(WIFEXITED(*childExitMethod)){
				printf("background pid %d is done: exit value %d\n", (int)pid, WEXITSTATUS(*childExitMethod));
				fflush(stdout);
			}
			// or if it was terminated by a signal
			if (WIFSIGNALED(*childExitMethod)){
				int sigStatus = WTERMSIG(*childExitMethod);
				printf("background pid %d is done: terminated by signal %d\n", (int)pid, sigStatus);
				fflush(stdout);
			}
			// remove the reaped child from the pidArray
			removePid(pidArray, pidCount, pid);
			// decrement i so we check the same index again as 
			// removePid shifts remaining pids to lower incices. 
			// we shouldnt segfauld because removePid also decrements
			// pidCount
			i--;
		}
	}
}




/*******************************************************************************
 * forknExec
 * forks off a new process and then executes the proper command. If it is a 
 * background process user access to shell will instantly return. If it is a 
 * foreground process the child process will run until completiion and then 
 * return user access to the shell prompt. 
 *
 * ****************************************************************************/
void forkAndExec(char** userCmds, int cmdCount, int* childExitMethod, 
		bool wantRunBG, pid_t* pidArray, 
		int* pidCount, struct sigaction* normal_action){
	//printf("in forkAndExec\n");
	pid_t spawnPid = -5;     // holds spawned process id
	int inFile;             // input file descriptor
	int outFile;             // output file descriptor

	// fork into a child and parent process
	spawnPid = fork();

	// do different things for parent and child
	switch(spawnPid){
		// if there was an error forking
		case -1:
			perror("Hull Breach! error forking...");
			exit(1);
			break;
	
		// child process
		case 0:
			//printf("in child process\n");
			// check if we are re-directing input/output
			checkReDirect(userCmds, cmdCount, wantRunBG, 
					&inFile, &outFile);

			// change foreground proccs to accept SIGINT signals
			// if user doesnt want to run in bg or cant run in bg
			if (!wantRunBG || !canRunBG){			
				sigaction(SIGINT, normal_action, NULL);
			}

			// have child execute command
			execvp(userCmds[0], userCmds);
			
			// if we get here there was a problem with execvp
			perror(userCmds[0]);
			// set childExitMethod to reflect an error
			*childExitMethod = 1;
			// terminate the child
			exit(1);
			break;

		// parent process
		default:
			//printf("in parent Process\n");
			// if the user wants and can run the process in the bg
			if (wantRunBG && canRunBG){
				printf("background pid is %d\n", spawnPid);
				fflush(stdout);
				// add the process to bgProccArray
				addPid(pidArray, pidCount, spawnPid);
			}
			// else we are waiting for the foreground process
			else {
				// wait until process done
				waitpid(spawnPid, childExitMethod, 0);
				// check if process terminated by signal
				if (WIFSIGNALED(*childExitMethod) != 0){
					int sigStatus = 
						WTERMSIG(*childExitMethod);
					printf("terminated by signal %d\n", 
							sigStatus);
					fflush(stdout);
				}
			}
			break;
	}
}




/*******************************************************************************
 * toggleBG
 * toggles the currently set boolean value of global canRunBG at the receipt of
 * a SIGTSTP signal. So if canRunBG is set to true, and the the SIGTSTP signal 
 * is caught, the value will be toggled to false. Likewise the bool will be 
 * toggled to true if originally set to false when the signal is caught.
 *
 * ****************************************************************************/
void toggleBG(){
	// if we can currently run a background process
	if (canRunBG){
		char* message = "\nEntering foreground-only mode" 
			" (& is now ignored)\n";
		write(STDOUT_FILENO, message, 50);
		canRunBG = false;
	}
	// else we can not run a background process
	else {  // (!canRunBG)
		char* message = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 30);
		canRunBG = true;
	}
}




/*******************************************************************************
 * killBG
 * on program exit loops through the array of background processes and 
 * terminates them.
 *
 * ****************************************************************************/
void killBG(pid_t* pidArray, int pidCount){
	int i;
	for (i = 0; i < pidCount; i++){
		kill(pidArray[i], SIGTERM);
	}
}




/*******************************************************************************
 * reapBG
 * should be used after killBG. Loops through array of background process ids 
 * and because they should all be killed, procededs to reap them.
 *
 * ****************************************************************************/
void reapBG(pid_t* pidArray, int pidCount, int* childExitMethod){	
	int i;
	for (i = 0; i < pidCount; i++){
		waitpid(pidArray[i], childExitMethod, 0);
	}
	//printf("done reaping\n");
}




/*******************************************************************************
 * shellLoop
 * This is the main loop for the program. The loop starts with gathering user 
 * input and then parsing it to run the proper command. 
 *
 * ****************************************************************************/
void shellLoop(struct sigaction* normal_action){
	//printf("in shellLoop\n");
	
	int cmdCount = 0;          // tracks number of commands entered by user
	int bytesEntered;          // tracks bytes read from getline
	
	int childExitMethod = -5;  // tracks how a process exited
	
	bool wantToExit = false;   // user want to exit?
	bool wantRunBG = false;    // want to run a process in background?
	size_t size = MAXINPUT;    // how many bytes to read froom stdin

	pid_t pidArray[50];          // hold unreaped child process id's
	int pidCount = 0;          // tracks how many child PID's in pidArray	
	
	// holds user input string
	char* userInput = calloc(MAXINPUT + 1, sizeof(char));

	// holds tokenized user input
	char** userCmds = calloc(MAXARGS, sizeof(char*));


	// shell loop is here, horray!
	do{
		// until we have cleared any stdinput errors and have some input
		while(1){
			// print the prompt
			printPrompt();

			// get user input from stdin
			bytesEntered = getline(&userInput, &size, stdin);	
			
			// if getline was interrupted by a signal
			if (bytesEntered == -1){
				// clear the error
				clearerr(stdin);
			}
			// else we had good input so remove the trailing
			// newline and break so we can evaluate input
			else{
				userInput[strlen(userInput) - 1] = '\0';
				break;
			}
		}
		//printf("the user entered: %s\n", userInput);

		//check if user wants to exit
		if (strcmp(userInput, "exit") == 0){
			wantToExit = true;
		}

		// check if user entered a blank line
		else if (strcmp(userInput, "") == 0){
			//printf("user entered an empty line\n");
			// check for any finished background processes
			reapChildren(pidArray, &pidCount, &childExitMethod);
			continue;    // to top of do/while loop
		}

		// check if user entered a comment
		else if (userInput[0] == '#'){
			//printf("user entered a comment\n");
			// check for any finished background processes
			reapChildren(pidArray, &pidCount, &childExitMethod);
			continue;    // to top of do/while loop
		}
		// else we can proccess input
		else{
			tokenizeInput(userInput, userCmds, &cmdCount);	
			//printInput(userCmds, cmdCount);
			
			// check for and expand any commands with '$$'
			expandPID(userCmds, cmdCount); 
			//printInput(userCmds, cmdCount);

			// already checked for 'exit' now check for cd or status
			// check for "cd"
			if (strcmp(userCmds[0], "cd") == 0){
				changeDirectory(userCmds);
			}
			
			// else check for "status"
			else if (strcmp(userCmds[0], "status") == 0){
				checkStatus(&childExitMethod);
			}
	
			// else we need to fork and execute a process
			else {
				// user wants to run process in background?
				checkIfBG(userCmds, cmdCount, &wantRunBG);
				// fork and execvp	
				forkAndExec(userCmds, cmdCount,	&childExitMethod,
					       	wantRunBG, pidArray, 
						&pidCount, normal_action);
			}

			// check for any finished background processes
			reapChildren(pidArray, &pidCount, &childExitMethod);
		}

		//printf("clearing array ");
		// free/reset array
		int i;
		for (i = 0; i < cmdCount; i++){
			// check that command wasn't NULL'd out
			if(userCmds[i]){
				free(userCmds[i]);
				userCmds[i] = NULL;
			}
		}
		// reset command count
		cmdCount = 0;
	
	// loop while we dont want to exit
	} while (!wantToExit);

	// clean up the user input
	free(userInput);
	// kill any background processes
	killBG(pidArray, pidCount);
	// reap them
	reapBG(pidArray, pidCount, &childExitMethod);
}




/*******************************************************************************
 * main
 * Creates the sigacton structs and other related signal handlers and then 
 * starts the loop for the shell
 *
 * ****************************************************************************/
int main(){
	//printf("in main\n");

	// signal stuff...
	struct sigaction ignore_action = {0}, 
			 normal_action = {0}, 
			 SIGTSTP_action = {0};

	// set the structs
	// will be used to ignore SIGINT
	ignore_action.sa_handler = SIG_IGN;
	sigfillset(&ignore_action.sa_mask);
	ignore_action.sa_flags = 0;
	
	// used to make foreground proccs respond to SIGTSTP
	normal_action.sa_handler = SIG_DFL;

	// used to toggle canRunBG bool
	SIGTSTP_action.sa_handler = toggleBG;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
		
	// register the signal actions to ignore SIGINT and togglel on SIGTSTP
	sigaction(SIGINT, &ignore_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	// our programs loop
	shellLoop(&normal_action);
	
	printf("\n");
	exit(0);
}









