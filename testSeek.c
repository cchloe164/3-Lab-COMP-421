#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include "iolib.c"

/*
 * testing the mkdir
 */
int
main()
{
    char dirname[30];
    memset(dirname, 0, 30);
    int fd = Create("testing");
    int new_offest = Seek(fd, 1, SEEK_END);
    TracePrintf(0, "new_offset is %i", new_offest);
    new_offest = Seek(fd, 566, SEEK_END);
    TracePrintf(0, "new_offset is %i", new_offest);
    return 0;
}