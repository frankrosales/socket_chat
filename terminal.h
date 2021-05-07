
/*
Initializes the terminal.
It receives as a parameter a pointer to an array fds[2], it is filled with two file descriptors referring to the
write and read ends of the terminal.
-> fds[0] refers to the read end of the terminal. This file descriptor blocks until the user writes something and presses the enter key
-> fds[1] refers to the write end of the terminal. Everything written to this file descriptor is printed on the upper side of the terminal
*/
void init_terminal(int *fds);

/*
Call this on program exit, it restores the terminal to it's original state.
I you don't the terminal will be left all messed up.
*/
int end_terminal();