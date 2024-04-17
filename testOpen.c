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
    initFileStorage();
    // Dummy("testing");
    Create("a");
    Open("a");
    // void *buf = malloc(sizeof(char) * 10);
    // Read(0, buf, sizeof(char) * 10);
    // struct Stat *buf = malloc(sizeof(struct Stat));
    // Stat("a", buf);
    // ChDir("q");
    // RmDir("q");

    return (0);
}