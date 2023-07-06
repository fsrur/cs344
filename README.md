# smallsh

This codebase is a basic implementation of a Unix shell. It provides support for executing programs, handling command line arguments, background processes, managing environment variables, and handling input and output redirection.

## Key Features

- **Program Execution**: The shell allows execution of programs present in the system's PATH environment variable. It handles command-line arguments for these programs.

- **Background Process Management**: The shell can run processes in the background, allowing concurrent execution of tasks. Status updates of these background tasks are managed and provided to the user.

- **Environment Variables**: The shell handles environment variables in commands. This allows access and use of global system information in commands.

- **Input and Output Redirection**: This implementation provides support for basic input and output redirection using '>' and '<' symbols.

- **Signal Handling**: The shell correctly handles signals like SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z).

- **Built-in Commands**: This shell supports basic built-in commands such as cd for changing directories and exit for terminating the shell session.

## Code Structure

The <b>'main'</b> function contains the primary read-evaluate-print loop (REPL) of the shell. This includes prompting for and reading input, parsing and executing the input commands.

The <b>'wordsplit'</b> function is responsible for splitting a string into words, recognizing comments, and handling backslash escapes.

The <b>'expand'</b> function is used for parameter expansion in the shell, substituting instances of '$' with appropriate values.

The <b>'param_scan'</b> function finds the next instance of a parameter within a word and sets the start and end pointers to the parameter token.

The <b>'build_str'</b> function builds up a base string by appending supplied strings or character ranges to it.

## Build and Usage

To build this shell, use a C compiler like gcc.

`gcc smallsh.c -o smallsh`

To run the shell, use the following command.

`./smallsh`

You can now type in commands at the prompt as you would in a regular shell. Press Ctrl+C to stop a running process and Ctrl+Z to pause a running process.

The shell supports the redirection operators '<', '>', '>>', and '&'. These can be used to redirect input and output to and from files and to run processes in the background.

`command > output.txt`

`command < input.txt`

`command >> append_output.txt`

`command &`

## Limitations

The shell is basic and does not provide advanced features like command history, job control, tab completion, etc.
