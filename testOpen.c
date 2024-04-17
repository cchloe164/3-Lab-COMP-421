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
    int fd;

    fd = Create("\\a");
    TracePrintf(0, "DONE: fd %d\n", fd);
    fd = Create("b");
    TracePrintf(0, "DONE: fd %d\n", fd);
    fd = Create("\\c");
    TracePrintf(0, "DONE: fd %d\n", fd);
    // void *buf = malloc(sizeof(char) * 10);
    // Read(0, buf, sizeof(char) * 10);
    // struct Stat *buf = malloc(sizeof(struct Stat));
    // Stat("a", buf);
    // ChDir("q");
    // RmDir("q");

    return (0);
}