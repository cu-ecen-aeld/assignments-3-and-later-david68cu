#include "systemcalls.h"
#include <stdlib.h>  
#include <stdbool.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/  
    // If command is NULL, system() will  then return  nonzero value if a shell is available, or 0 if no shell is available.
    // No matter what we return false
    if (cmd == NULL) {
        return false; 
    }

    int ret = system(cmd);

    // If a child process could not be created, or its status could not be retrieved, the return value is -1 and errno is set to indicate the error.
    if (ret == -1) {
        return false; 
    }

    // If system() returns 127, the shell itself could not be executed
    if (ret == 127) {
        return false; // The shell execution failed
    }

     // If execution succeeds, check termination status
    if (WIFEXITED(ret)) {
        return WEXITSTATUS(ret) == 0; 
    }
    
    // If we are here something happened
    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{   

    // This function is expected to be call like shown below form examples in the test case  https://github.com/cu-ecen-aeld/assignment-autotest/blob/master/test/assignment3/Test_systemcalls.c
    // do_exec(3, "/usr/bin/test","-f","echo") 
    // do_exec(3, "/usr/bin/test","-f","/bin/echo")  
    // do_exec(2, "echo", "Testing execv implementation with echo")
    //
    // Lets declare the variable args of type va_list type
    // va_list is a type (technically a typedef) defined in stdarg.h, used to handle variable arguments in C functions.
    // It is an opaque type that stores information about variable arguments passed to a function.
    va_list args;
    // args is a va_list type, which is an opaque data structure that holds information about variable arguments.
    // count is the last known fixed parameter in the function.
    // va_start(args, count); initializes args by:
    // Setting up an internal pointer inside args to the first unnamed argument that appears after count in memory.
    // Allowing va_arg(args, type); to retrieve the next argument correctly.
    va_start(args, count);

    char * command[count+1];
    int i;
    // Here we use va_args() another macro that do the following
    // Extracts the next argument from args.
    // Casts it to the specified type (char * in this case).
    // Moves the internal pointer of args to the next argument.
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // For example for do_exec(2, "/bin/echo", "Testing execv implementation with echo");
    // command = ["bin/echo", "Testing execv implementation with echo", NULL ]
    // It is because (execv requires NULL termination)
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    // Like va_list() and va_start() , va_end is also found in  <stdarg.h> and is used to 
    // clean up a va_list that was previously initialized with va_start()
    va_end(args);

    // Inspired in From page 161 of LSP programming book 
 
    int status;
    pid_t pid = fork(); // Create a child process

    if (pid == -1) {
        perror("fork failed");
        return false; // Fork failed
    }

    if (pid == 0) { // Child process
        execv(command[0], command); // Execute the command
        perror("execv failed"); // If execv fails, print error
        exit(EXIT_FAILURE); // Ensure the child exits on failure
    }

    // The Parent process needs to wait for the child to complete
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
        return false;
    }

    // Check if the child process terminated normally
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0; // Return true if exit code is 0
    }

    return false; // The process did not exit normally

}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   Here we need to take into account that redirect with > and other are not part of the kernel but part of the shell
 *   So if we want redirection we need to do in other way 
 *   execv() is a system call in Linux/Unix that replaces the current process with a new program. 
 *   It does not return if successful; instead, the new program starts executing immediately.
*/

    va_end(args);
    pid_t pid = fork(); // Create child process

    if (pid == -1) {
        perror("fork failed");
        return false;
    }

    if (pid == 0) { // Child process
        // Open output file for writing (create if not exists, truncate)
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to the file (fd becomes stdout)
        // dup2() is a system call in Linux and Unix-like operating systems that duplicates a file descriptor, 
        // allowing you to redirect input (stdin), output (stdout), or error (stderr) to another file or process.
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 failed");
            exit(EXIT_FAILURE);
        }
        
        // Close the file descriptor (it's now linked to stdout)
        close(fd);

        // Execute the command using execv
        execv(command[0], command);
        perror("execv failed"); // If execv fails, print error
        exit(EXIT_FAILURE); // Ensure the child exits on failure
    }

    // Parent process waits for the child to complete
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
        return false;
    }

    // Check if the child process terminated normally
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0; // Returns true if exit code is 0
    }


    return false;
}
