# smallsh - Basic Unix Shell Implementation

## Project title and brief description
"smallsh" is a basic implementation of a Unix shell that supports executing programs, handling command line arguments, managing background processes and environment variables, as well as dealing with input and output redirection.

## Prerequisites
- C compiler (like gcc)

## Installation or Setup
There is no installation required for "smallsh", but you need to compile the code before you can run it. To compile the "smallsh" shell, use a C compiler like gcc.

`gcc smallsh.c -o smallsh`

## Running the Application
To run the shell, use the following command:

`./smallsh`

You can now type in commands at the prompt as you would in a regular shell. 

The shell supports the redirection operators '<', '>', '>>', and '&'. These can be used to redirect input and output to and from files and to run processes in the background.

`command > output.txt`

`command < input.txt`

`command >> append_output.txt`

`command &`

## Usage
In "smallsh", you can execute programs, manage background processes, handle environment variables, and redirect input and output just like in a standard Unix shell. The shell recognizes the commands entered, processes them, and executes the corresponding operations.

## Project Structure and Implementation Details
The codebase is organized around the following key functions:

- The <b>'main'</b> function contains the primary read-evaluate-print loop (REPL) of the shell, which prompts for and reads input, parses and executes the input commands.

- The <b>'wordsplit'</b> function is responsible for splitting a string into words, recognizing comments, and handling backslash escapes.

- The <b>'expand'</b> function is used for parameter expansion in the shell, substituting instances of '$' with appropriate values.

- The <b>'param_scan'</b> function finds the next instance of a parameter within a word and sets the start and end pointers to the parameter token.

- The <b>'build_str'</b> function builds up a base string by appending supplied strings or character ranges to it.

## Limitations
While "smallsh" supports several key shell features, it does not provide some of the more advanced features found in many modern Unix shells, such as command history, job control, tab completion, etc.
