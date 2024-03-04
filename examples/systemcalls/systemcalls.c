#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "fcntl.h"


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

    return system(cmd)!=-1;
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
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    //clear stdout buffer to avoid double printf statment
    fflush(stdout);
    int status;
    pid_t p;
    p = fork();
    if(p==-1) {
        // forking has failed, exit program
        return false;
    } else if(p==0) {
        //the child process will do the work
        execv(command[0], command);
        // execv will never return here if working correctly. 
        // return false to signify an error
        exit(EXIT_FAILURE);
        return false;
    } else {
        //parent process operations

        // wait for child to return, verify no bad return value.
        if(waitpid(p, &status,0)==-1) {
            return false;
        }
        // verify no unexpected exit conditions for the child process
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // no unexpected condition, child completed correctly
            va_end(args);
            return true;
        } else {
            // error in child processing, return a failure
            return false;
        }
    }

    //va_end(args);
    // should be unreachable, left for additional case coverage
    return false;
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
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    


    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if(fd < 0){
        printf("Error in opening of file\n");
        exit(-1);
    }

    //clear stdout buffer to avoid double printf statment
    fflush(stdout);
    int status;
    pid_t p;
    p = fork();
    if(p<0)
    {
        // forking has failed, exit program
        exit(1);
    } else if(p==0) {

        if(dup2(fd,1)<0)
        abort();
        close(fd);
        //the child process will do the work
        execv(command[0], command);
        // this return should never be reached, something failed
        exit(EXIT_FAILURE);

    } else {
        //parent process operations

        // wait for child to return, verify no bad return value.
        if(waitpid(p, &status,0)==-1) {
            return false;
        }
        // verify no unexpected exit conditions for the child process
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // no unexpected condition, child completed correctly
            va_end(args);
            return true;
        } else {
            // error in child processing, return a failure
            return false;
        }
    }

    //va_end(args);

    return false;
}
