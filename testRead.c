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
    // char dirname[30];
    struct dir_entry *entry = malloc(sizeof(struct dir_entry));
    // memset(dirname, 0, 30);
    MkDir("/testing");
    MkDir("/testing/test2");
    MkDir("/testing/test3");
    int fd = Open("/testing/test3");
    Read(fd, entry, sizeof(struct dir_entry));
    TracePrintf(0, "First entry is %s\n", entry->inum);
    Read(fd, entry, sizeof(struct dir_entry));
    TracePrintf(0, "Second entry is %s\n", entry->inum);
    Read(fd, entry, sizeof(struct dir_entry));
    TracePrintf(0, "Third entry is %s\n", entry->inum);
    Create("testing3");
    ChDir("/testing");
    return 0;
}