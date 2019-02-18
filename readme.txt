To compile use:
gcc -o smallsh smallsh.c

This is a small shell program with 3 built in commands - exit,
 cd, and status. Non-built in commands will be forked and exec'd and may be
 ran in the background by including '&' at the end of your user command.
 Additionally the shell supports input and output re-direction with the use 
 of '<' and/or '>' followed by filenames.
 
 The general syntax of a command is:
 command [arg1 arg2 ...] [< input_file] [> outputfile] [&]
 ... where items in [] are optional.
