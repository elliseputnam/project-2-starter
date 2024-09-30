#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"

// FROM RECITATION
// int program_pipe_1[2];

// program_pipe_1[0];
// program_pipe_1[1];

// pipe(program_pipe_1);

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
// ONLY EDIT THIS FUNCTION FOR DELIVERABLE 1!!!
static int dispatch_external_command(struct command *pipeline)
{
	/*
	 * Note: this is where you'll start implementing the project.
	 *
	 * It's the only function with a "TODO".  However, if you try
	 * and squeeze your entire external command logic into a
	 * single routine with no helper functions, you'll quickly
	 * find your code becomes sloppy and unmaintainable.
	 *
	 * It's up to *you* to structure your software cleanly.  Write
	 * plenty of helper functions, and even start making yourself
	 * new files if you need.
	 *
	 * For D1: you only need to support running a single command
	 * (not a chain of commands in a pipeline), with no input or
	 * output files (output to stdout only).  In other words, you
	 * may live with the assumption that the "input_file" field in
	 * the pipeline struct you are given is NULL, and that
	 * "output_type" will always be COMMAND_OUTPUT_STDOUT.
	 *
	 * For D2: you'll extend this function to support input and
	 * output files, as well as pipeline functionality.
	 *
	 * Good luck!
	 */
	//fprintf(stderr, "TODO: handle external commands\n");

	//pipeline->argv[0]; // ex) "echo" 

// DELIVERABLE 1 CODE !!!
// comment out
	// // alway guarenteed to be NULL!
	// pipeline->pipe_to = NULL;
	// pipeline->input_filename = NULL;
	// pipeline->output_filename = NULL;

	// // fork() creates copy of current program
	// int process = fork();

	// int child_status = 0;

	// // if current executing process is child process
	// if (process == 0) {
    //     // printf("I AM THE CHILD PROCESS.");
    //     // we forked outselves so we could start this...

    //     if (execvp(pipeline->argv[0], pipeline->argv) == -1) {
    //         // execvp fails, print an error message
    //         perror("Error executing command");
    //         exit(EXIT_FAILURE);
    //     }
    // } else if (process > 0) {
    //     // printf("I AM THE MAIN PROCESS.");
        
    //     wait(&child_status);

    // } else {
    //     // if fork fails, handle the error
    //     perror("Error forking process");
    //     return -1;
    // }

    // return child_status;

// DELIVERABLE 2 CODE !!!

	// 2 pipes needed, each with 2 places, read/write end
	// one for current process
	int currentPipe[2];
	// one for previous process
	int prevPipe[2];

	bool firstArg = true;
	int childStatus = 0;

	// loop to parse pipeline, enter loop if pipe to next command
	while(true) {
		// file descriptors
		// for piping in, STDIN
		int fd_in = STDIN_FILENO; 
		// for piping out, STDOUT
        int fd_out = STDOUT_FILENO; 

		// check previous command output to see if we need to pipe into next command
		if (!firstArg) {
			// read output from previous pipe
			fd_in = prevPipe[0];
		}

		// check if input file
		if (firstArg && pipeline->input_filename != NULL) {
			fd_in = open(pipeline->input_filename, O_RDONLY);

			if (fd_in < 0) {
				perror("Error opening input file");
				return -1;
			}
		}

		// output handling, writing to file or STDOUT
		if(pipeline->output_type == COMMAND_OUTPUT_PIPE) {
			// create a new pipe
			pipe(currentPipe);
			// write end of pipe?
			fd_out = currentPipe[1];
		}

		else if (pipeline->output_type == COMMAND_OUTPUT_FILE_TRUNCATE) {
			fd_out = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		}

		else if (pipeline->output_type == COMMAND_OUTPUT_FILE_APPEND) {
            fd_out = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
        }

		// now we fork() !
		int process = fork();

		if (process < 0) {
			perror("fork() failed");
			return -1;
		}
		// child process
		else if (process == 0) {
			// STDIN
			if (fd_in != STDIN_FILENO) {
				dup2(fd_in, STDIN_FILENO);
				close(fd_in);
			}
			// STDOUT
			if (fd_out != STDOUT_FILENO) {
				dup2(fd_out, STDOUT_FILENO);
				close(fd_out);
			}

			if (execvp(pipeline->argv[0], pipeline->argv) == -1) {
				perror("execvp() failed");
				exit(EXIT_FAILURE);
			}
		}
		// parent process
		else {
			if (fd_in != STDIN_FILENO) {
				close(fd_in);
			}
			if (fd_out != STDOUT_FILENO) {
				close(fd_out);
			}

			// wait for teh child process to finish
			wait(&childStatus);

			// update our pipes for the next command
			prevPipe[0] = currentPipe[0];
			prevPipe[1] = currentPipe[1];

			// prep next command
			firstArg = false;

			// check if another command in pipeline
			if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
				// continue to next command
				pipeline = pipeline->pipe_to;
			}
			else {
				// no more commands
				break;
			}
		}
	}
	// returning status of the last executed command
	return childStatus;
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
