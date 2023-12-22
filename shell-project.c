#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFSIZE 4096

char current_directory[BUFFSIZE]; // Variable to store the current working directory

/* Retrieve the hostname and make sure that this program is not being run on the main odin server.
 * It must be run on one of the vcf cluster nodes (vcf0 - vcf3).
 */
void check()
{
        char hostname[10];
        gethostname(hostname, 9);
        hostname[9] = '\0';
        if (strcmp(hostname, "csci-odin") == 0) {
                fprintf(stderr, "WARNING: TO MINIMIZE THE RISK OF FORK BOMBING THE ODIN SERVER,\nYOU MUST RUN THIS PROGRAM ON ONE OF THE VCF CLUSTER NODES!!!\n");
                exit(EXIT_FAILURE);
        } // if
} // check

/* Function to set the current working directory to the user home directory */
void set_initial_directory()
{
    char *home_directory = getenv("HOME");
    if (home_directory == NULL)
    {
        perror("getenv");
        exit(EXIT_FAILURE);
    }

    if (chdir(home_directory) == -1)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    // Copy the home directory path to the current_directory variable
    strncpy(current_directory, home_directory, BUFFSIZE - 1);
    current_directory[BUFFSIZE - 1] = '\0';
} // set_initial_directory

/* Function to update the current working directory */
void update_current_directory()
{
    if (getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
} // update_current_directory

int main()
{
	check();
	setbuf(stdout, NULL); // makes printf() unbuffered
	int n;
	char cmd[BUFFSIZE];

	// Project 3 TODO: set the current working directory to the user home directory upon initial launch of the shell
	// You may use getenv("HOME") to retrive the user home directory

	set_initial_directory();

	// inifite loop that repeated prompts the user to enter a command
	while (1) {
		printf("1730sh:");
		// Project 3 TODO: display the current working directory as part of the prompt
		update_current_directory();

		// Check if the current directory is a subdirectory of the home directory
		if (strstr(current_directory, getenv("HOME")) == current_directory)
		{
			// If yes, replace the home directory path with "~"
			printf("~%s", current_directory + strlen(getenv("HOME")));
		}
		else
		{
			// If no, print the current directory as is
			printf("%s", current_directory);
		}
		printf("$ ");
		n = read(STDIN_FILENO, cmd, BUFFSIZE);

		// if user enters a non-empty command
		if (n > 1) {
			cmd[n-1] = '\0'; // replaces the final '\n' character with '\0' to make a proper C string

			// Lab 06 TODO: parse/tokenize cmd by space to prepare the
			// command line argument array that is required by execvp().
			// For example, if cmd is "head -n 1 file.txt", then the
			// command line argument array needs to be
			// ["head", "-n", "1", "file.txt", NULL].

			char *token;
			char *args[BUFFSIZE]; // Assuming a maximum of BUFFSIZE arguments

			token = strtok(cmd, " ");
			int i = 0;

			while (token != NULL)
			{
				args[i] = token;
				token = strtok(NULL, " ");
				i++;
			}

			args[i] = NULL; // NULL-terminate the array

			// Lab 07 TODO: if the command contains input/output direction operators
			// such as "head -n 1 < input.txt > output.txt", then the command
			// line argument array required by execvp() needs to be
			// ["head", "-n", "1", NULL], while the "< input.txt > output.txt" portion
			// needs to be parsed properly to be used with dup2(2) inside the child process

			int redirect_input = 0; // Flag to check if input redirection is present
            int redirect_output = 0; // Flag to check if output redirection is present
			int redirect_output_append = 0; // Flag to check if output redirection appending is present

			for (int j = 0; args[j] != NULL; j++)
			{
				if (strcmp(args[j], "<") == 0)
				{
					redirect_input = j;
					args[j] = NULL; // Exclude "<" from the arguments array
				}
				else if (strcmp(args[j], ">") == 0)
				{
					redirect_output = j;
					args[j] = NULL; // Exclude ">" from the arguments array
				}
				else if (strcmp(args[j], ">>") == 0)
				{
					redirect_output_append = j;
					args[j] = NULL; // Exclude ">>" from the arguments array
				}
			}

			// Lab 06 TODO: if the command is "exit", quit the program

			if (strcmp(args[0], "exit") == 0)
			{
				exit(EXIT_SUCCESS);
			}

			// Project 3 TODO: else if the command is "cd", then use chdir(2) to
			// to support change directory functionalities

			else if (strcmp(args[0], "cd") == 0)
			{
				// Check the number of arguments for cd
				if (i == 1)
				{
					// Change to the home directory
					set_initial_directory();
				}
				else if (i == 2)
				{
					// Change to the specified directory
					if (strcmp(args[1], "~") == 0)
					{
						// Change to the home directory
						set_initial_directory();
					}
					else if (args[1][0] == '~' && (args[1][1] == '\0' || args[1][1] == '/'))
					{
						// Expand tilde and change to the specified directory
						char *home = getenv("HOME");
						if (home == NULL)
						{
							perror("getenv");
							exit(EXIT_FAILURE);
						}

						char expanded_path[BUFFSIZE];
						snprintf(expanded_path, sizeof(expanded_path), "%s%s", home, args[1] + 1);

						if (chdir(expanded_path) == -1)
						{
							perror("chdir");
						}
					}
					else
					{
						if (chdir(args[1]) == -1)
						{
							perror("chdir");
						}
					}
				}
				else
				{
					fprintf(stderr, "cd: too many arguments\n");
				}
			}

			// Lab 06 TODO: for all other commands, fork a child process and let
			// the child process execute user-specified command with its options/arguments.
			// NOTE: fork() only needs to be called once. DO NOT CALL fork() more than one time.

			else
			{
				pid_t pid = fork();

				if (pid == -1)
				{
					perror("fork");
					exit(EXIT_FAILURE);
				}
				else if (pid == 0)
				{
					// This is the child process

					// Lab 07 TODO: inside the child process, use dup2(2) to redirect
					// standard input and output as specified by the user command

					if (redirect_input > 0)
					{
						int fd_in = open(args[redirect_input + 1], O_RDONLY);
						if (fd_in == -1)
						{
							perror("open");
							exit(EXIT_FAILURE);
						}

						// Redirect standard input to the specified file
						dup2(fd_in, STDIN_FILENO);
						close(fd_in); // Close the file descriptor after redirection
					}

					if (redirect_output > 0)
					{
						int fd_out = open(args[redirect_output + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd_out == -1)
						{
							perror("open");
							exit(EXIT_FAILURE);
						}

						// Redirect standard output to the specified file
						dup2(fd_out, STDOUT_FILENO);
						close(fd_out); // Close the file descriptor after redirection
					}

					if (redirect_output_append > 0)
					{
						int fd_out = open(args[redirect_output_append + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
						if (fd_out == -1)
						{
							perror("open");
							exit(EXIT_FAILURE);
						}

						// Redirect standard output to the specified file
						dup2(fd_out, STDOUT_FILENO);
						close(fd_out); // Close the file descriptor after redirection
					}

					// Lab 06 TODO: inside the child process, invoke execvp().
					// if execvp() returns -1, be sure to use exit(EXIT_FAILURE);
					// to terminate the child process

					if (execvp(args[0], args) == -1)
					{
						perror("execvp");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					// This is the parent process

					// Lab 06 TODO: inside the parent process, wait for the child process
					// You are not required to do anything special with the child's
					// termination status

					waitpid(pid, NULL, 0);
				}
			} // else
		} // if
	} // while
	return 0;
} // main