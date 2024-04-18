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
    MkDir("/testing/test2");
    MkDir("/testing/test3");
    MkDir("/testing/test2");
    MkDir("/testinga/test2");
    MkDir("test4");
    ChDir("/testing");
    MkDir("test4");
    ChDir("/testing/test4");

    return 0;
}