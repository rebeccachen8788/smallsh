# **Smallsh - A Simple Unix-like Shell in C**
Smallsh is a small shell implementation in C that provides a command line interface similar to well-known shells like bash. It offers basic functionality such as executing commands, managing background processes, and handling signals.

## **Features**
Interactive Prompt: Smallsh provides an interactive command prompt where users can enter commands.

Command Line Parsing: Input from the user is parsed into semantic tokens, allowing for easy processing of commands.

Parameter Expansion: Smallsh supports parameter expansion for certain tokens, such as '$$', '$?', and '$!'.

Built-in Commands: Implemented two built-in commands: exit and cd, enabling users to exit the shell or change the current working directory.

Execution of Non-built-in Commands: Executes non-built-in commands using the appropriate exec(3) function. If the command is not found in the system's PATH, an error message is displayed.

Redirection: Supports input (<) and output (>) redirection for command execution.

Background Execution: Allows commands to be run in the background using the & operator.

Signal Handling: Custom behavior for SIGINT (CTRL-C) and SIGTSTP (CTRL-Z) signals, ensuring correct handling during command execution.

## **Program Functionality**
The smallsh program follows these steps in an infinite loop:

1. Input
2. Word splitting
3. Expansion
4. Parsing
5. Execution
6. Waiting

The loop continues until the built-in exit command is executed, or when an EOF on stdin is encountered, simulating an implied exit $? command.

## **Command Prompt**
Smallsh displays a prompt before each new command line, using the PS1 environment variable for customization.

## **Running Background Processes**
Before displaying the prompt, Smallsh checks for un-waited-for background processes in the same process group ID. It provides informative messages for each child process's status.

## **Signal Handling**
Smallsh handles SIGINT and SIGTSTP signals correctly, allowing for graceful termination of foreground processes and ignoring SIGTSTP (CTRL-Z).

## **Memory Management**
Smallsh properly manages memory and avoids memory leaks, ensuring efficient use of resources.

## **Usage**
To compile and run Smallsh, use the following commands:
``gcc -o smallsh smallsh.c
./smallsh``
Or execute the make file that is included.
