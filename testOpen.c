#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include "iolib.c"

/*
 * 
 */
int main()
{
    // TracePrintf(0, "Making new directory...\n");
    // Dummy("testing");

    // TEST: Create multiple files
    // int fd;
    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // fd = Create("/testing/b");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // fd = Create("c");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // TEST: Create nested directories with files
    // int fd;

    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // MkDir("/testing");

    // fd = Create("/testing/b");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // MkDir("/testing/test");

    // fd = Create("/testing/b1");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // fd = Create("/testing/test/b2");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // fd = Create("c");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // // TEST: Close already closed files
    // int fd;

    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // Close(fd);
    // Close(fd);

    // TEST: Open files
    int fd1;
    int fd2;
    int fd3;
    int fd4;

    fd1 = Create("a");
    TracePrintf(0, "DONE: fd %d\n", fd1);
    
    fd2 = Create("b");
    TracePrintf(0, "DONE: fd %d\n", fd2);
    
    Close(fd);

    fd3 = Create("c");
    TracePrintf(0, "DONE: fd %d\n", fd3);


    fd4 = Open("a");
    TracePrintf(0, "DONE: fd %d\n", fd4);


    // void *buf = malloc(sizeof(char) * 10);
    // Read(0, buf, sizeof(char) * 10);
    // struct Stat *buf = malloc(sizeof(struct Stat));
    // Stat("a", buf);
    // ChDir("q");
    // RmDir("q");

    return (0);
}