#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>
#include "iolib.c"

/*
 * 
 */
int main()
{
    TracePrintf(0, "Making new directory...\n");
    Dummy("testing");

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

    // // TEST: Open files
    // int fd1;
    // int fd2;
    // int fd3;

    // fd1 = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd1);
    // fd2 = Create("b");
    // TracePrintf(0, "DONE: fd %d\n", fd2);
    // fd3 = Create("c");
    // TracePrintf(0, "DONE: fd %d\n", fd3);

    // Close(fd1);
    // Close(fd2);
    // Close(fd3);

    // fd1 = Open("a");
    // TracePrintf(0, "DONE: fd %d\n", fd1);
    // fd1 = Open("b");
    // TracePrintf(0, "DONE: fd %d\n", fd1);
    // fd1 = Open("c");
    // TracePrintf(0, "DONE: fd %d\n", fd1);

    // // TEST: Open files
    // int fd1;
    // int fd2;

    // MkDir("/testing");
    // MkDir("/testing/test");

    // fd1 = Create("/testing/a");
    // TracePrintf(0, "DONE: fd %d\n", fd1);

    // fd2 = Create("/testing/test/b");
    // TracePrintf(0, "DONE: fd %d\n", fd2);

    // Close(fd1);
    // Close(fd2);

    // fd1 = Open("/testing/a");
    // TracePrintf(0, "DONE: fd %d\n", fd1);
    // fd2 = Open("/testing/test/b");
    // TracePrintf(0, "DONE: fd %d\n", fd2);

    // // TEST: Stat one file
    // int fd;

    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // struct Stat *buf = malloc(sizeof(struct Stat));
    // int res = Stat("a", buf);
    // TracePrintf(0, "res %d\n", res);
    // TracePrintf(0, "STATS\tinum=%d\ttype=%d\tsize=%d\tnlink=%d\n", buf->inum, buf->type, buf->size, buf->nlink);

    // TEST: Stat nested file.
    // MkDir("/testing");

    // int fd;
    // fd = Create("/testing/a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // struct Stat *buf = malloc(sizeof(struct Stat));
    // int res = Stat("/testing/a", buf);
    // TracePrintf(0, "res %d\n", res);
    // TracePrintf(0, "STATS\tinum=%d\ttype=%d\tsize=%d\tnlink=%d\n", buf->inum, buf->type, buf->size, buf->nlink);

    // // TEST: change to child directory
    // // TEST: change to parent directory
    // // TEST: change to grandchild directory
    // // TEST: change to grandparent directory
    // MkDir("/a");
    // ChDir("/a");
    // ChDir("..");
    // MkDir("/a/b");
    // ChDir("/a/b");
    // ChDir("../..");

    // // TEST: duplicate directories after changing directories
    // MkDir("/a");
    // ChDir("/a");
    // MkDir("b");
    // ChDir("..");
    // MkDir("/a/b");
    // ChDir(".");

    // TEST: write to a file
    int fd;
    fd = Create("a");
    TracePrintf(0, "DONE: fd %d\n", fd);

    char *buf = "string";
    int size = sizeof("string");
    int written = Write(fd, (void *)buf, size);
    TracePrintf(0, "DONE: written %d\n", written);

    // void *buf = malloc(sizeof(char) * 10);
    // Read(0, buf, sizeof(char) * 10);

    return (0);
}