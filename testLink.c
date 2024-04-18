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
    ChDir("/testing/test2");
    Create("fil");
    // Create("asdf");
    ChDir("/testing/test3");
    Link("/testing/test2/fil", "/testing/test3/string2"); //should work
    Unlink("/testing/test3/string2");


    ChDir("/testing/test3");
    Link("string1", "string2");

    //linking real directories
    Link("/testing/test3", "string2"); //should fail (test3 is a directory)
    Link("/testing/test2/fil", "string2"); //should work

    Link("/testing/test2/fil", "string3");
    Link("/testing/test2/fil", "string3");

    Link("/testing/test2/asdf", "string2");

    Unlink("asdf/asdf"); //should fail
    Unlink("/testing/test2"); //should fail
    Unlink("/testing/test2/fil");
    Unlink("/testing/test2/asdf");
    
    Unlink("/testing/test3/string2");


    return 0;
}