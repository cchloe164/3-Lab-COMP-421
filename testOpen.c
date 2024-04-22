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
    // Create("c");

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
    // int fd3;

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
    // fd3 = Open("/testing/test/basdf");
    // TracePrintf (0, "DONE: fd %d\n", fd3);

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

    // // TEST: write to a large file
    // int fd;
    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // char buf[] = "Generating long and coherent text is an important but challenging task, particularly for open-ended language generation tasks such as story generation. Despite the success in modeling intra-sentence coherence, existing generation models (e.g., BART) still struggle to maintain a coherent event sequence throughout the generated text.";
    // int size = sizeof(buf);
    // int written = Write(fd, (void *)&buf, size);
    // TracePrintf(0, "DONE: written %d\n", written);

    // TEST: read overloaded file memory
    int fd = Create("a");
    TracePrintf(0, "DONE: fd %d\n", fd);

    // // text is size of a block
    // char buf[] = "Receied shutters expenses ye he pleasant. Drift as blind above at up. No up simple county stairs do should praise as. Drawings sir gay together landlord had law smallest. Formerly welcomed attended declared met say unlocked. Jennings outlived no dwelling denoting in peculiar as he believed. Behaviour excellent middleton be as it curiosity departure ourselves. Received shutters expenses ye he pleasant. Drift as blind above at up. No up simple county stairs do should praise as. Drawings sir gay together land";
    // int size = sizeof(buf);

    // int i;
    // int written;
    // for (i = 0; i < 145; i++) { // 145 blocks are written
    //     written = Write(fd, (void *)&buf, size);
    //     TracePrintf(0, "DONE: written %d\n", written);
    // }

    char buf2[] = "WE'RE DONE.";   // should appear at end of file
    int size = sizeof(buf2);
    int written = Write(fd, (void *)&buf2, size);
    TracePrintf(0, "DONE: written %d\n", written);

    // char *res_buf = malloc(512 * 145 + size);
    char *res_buf = malloc(size);
    // Read(fd, res_buf, 512 * 145 + size);
    Read(fd, res_buf, size);
    TracePrintf(0, "FINAL OUTPUT: %s\n", res_buf);


    // // TEST: write nothing to file
    // int fd;
    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);

    // char buf[] = "";
    // int size = sizeof(buf);
    // int written = Write(fd, (void *)&buf, size);
    // TracePrintf(0, "DONE: written %d\n", written);

    // // TEST: create, close, open, write 
    // int fd;
    // fd = Create("a");
    // TracePrintf(0, "DONE: fd %d\n", fd);
    // Close(fd);
    // Open("a");
    // char buf[] = "string";
    // int size = sizeof(buf);
    // int written = Write(fd, (void *)&buf, size);
    // TracePrintf(0, "DONE: written %d\n", written);

    return (0);
}