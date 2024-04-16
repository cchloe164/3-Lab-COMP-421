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
    Dummy("testing");
    MkDir("/testing");
    return 0;
}