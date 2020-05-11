#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define NUM_CHILD 10
#define WITH_SIGNALS

// Global variable signalizing if process was interupted (1 if it was, 0 otherwise)
short interrupted = 0;

// Custom Handler for SIGTERM signal in child process
void childTerminatedHandler() {
	printf("child[%d]: Received SIGTERM signal, terminating process.\n", getpid());
}

// Process run by all created children
void childProcess() {
	#ifdef WITH_SIGNALS
		// ignoring keyboard intrrupt
		signal(SIGINT, SIG_IGN);
		// setting custom sigterm signal handler
		signal(SIGTERM, childTerminatedHandler);
	#endif

	printf("child[%d]: My parent id: %d\n", getpid(), getppid());
	sleep(10);
	printf("child[%d]: Process completed.\n", getpid());
	exit(0);
}

// Function running on keyboard interrupt on parent
void keyboardInterruptHandler() {
	printf("parent[%d]: Received a keyboard interrupt.\n", getpid());
	// Modifying global variable
	interrupted = 1;
}

void restoreAllSignalsToDefaults() {
	for (int i = 1; i < _NSIG; i++) {
		signal(i, SIG_DFL);		
	}
}

void ignoreAllSignals() {
	for (int i = 1; i < _NSIG; i++) {
		signal(i, SIG_IGN);		
	}
}

void killChildrenProcesses(int childrenNum, int *children) {
	for (int j = 0; j < childrenNum; j++) 
		kill(children[j], SIGTERM);
}

int main() {
	printf("=== EOPSY LAB 2 PROJECT ===\n");
	printf("=== Made by Jan Radziminski 293052 ===\n\n");

	// If With signals is defined
	#ifdef WITH_SIGNALS
		// Ignoring all signals
		ignoreAllSignals();
		// Restoring SIGCHLD to default
		signal(SIGCHLD, SIG_DFL);
		signal(SIGINT, keyboardInterruptHandler);
	#endif
	
	// Section 1
	// Array for childern id storage
	pid_t childrenIds[NUM_CHILD];

	for (int i = 0; i < NUM_CHILD; i++) {
			pid_t id = fork();
			if (id == 0) {
				childProcess();
			} else if (id < 0) {
				printf("parent[%d]: There was an error while crating child process, killing all child processes and terminating the program.\n", getpid());
				killChildrenProcesses(i, childrenIds);

				exit(1);
			} else {
				childrenIds[i] = id;			
			}

			sleep(1);

			#ifdef WITH_SIGNALS
				if (interrupted == 1) {
					killChildrenProcesses(i + 1, childrenIds);
					printf("parent[%d]: Sending SIGTERM signal to all children...\n", getpid());
					break;
				}
			#endif
	}
	if (interrupted == 1)
		printf("parent[%d]: Child process creation was interrupted.\n", getpid());	
	else
		printf("parent[%d]: All child processes were succesfully created.\n", getpid());		

	int childrenCounter = 0;
	while (1) {
		if (wait(NULL) == -1) break;
		childrenCounter++;
	}
	printf("parent[%d]: Received %d children exit status codes.\n", getpid(), childrenCounter);

	#ifdef WITH_SIGNALS
		restoreAllSignalsToDefaults();
	#endif

	return 0;
}
