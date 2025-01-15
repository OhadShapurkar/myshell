Unix Shell Emulator
This project implements a custom Unix-like shell with features including process management, command history, signal handling, and pipeline support. The shell is built in C, leveraging Linux system calls and basic data structures for process tracking.

Features:
Process Management: Tracks running, suspended, and terminated processes.
Command History: Maintains a history of the last 10 commands with support for execution via history indices.
Signal Handling: Allows stopping, resuming, and terminating processes with commands like stop, wake, and term.
Pipeline Execution: Supports command pipelines (e.g., ls | grep txt).
Input/Output Redirection: Handles redirection for input and output streams.
Debug Mode: Provides detailed output for debugging purposes.
Usage:
Launch the shell with ./shell [-d] for debug mode.
Use quit to exit the shell, cd to change directories, procs to view processes, and history to display command history.
Execute commands directly or rerun historical commands using !<index>.
Implementation Highlights:
Dynamic Process List: Uses linked lists to manage processes and history.
Signal Integration: Sends and handles SIGINT, SIGTSTP, and SIGCONT for process control.
Error Handling: Provides informative error messages for invalid commands or system call failures.
