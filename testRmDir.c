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

    Dummy("testing");
    MkDir("/testing");
    MkDir("/testing/test2");
    MkDir("/testing/test3");
    RmDir("/testing/test3");
    MkDir("/testing/test3");
    ChDir("/testing/test2");

    RmDir("/testing/test3");
    RmDir("/testing/test3"); //should fail
    RmDir("/testing"); //should fail
    ChDir("/testing/test3"); //should also fail



    return 0;
}