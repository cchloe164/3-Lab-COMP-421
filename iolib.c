#include <comp421/iolib.h>
#include <dirent.h>
#include <comp421/filesystem.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <comp421/yalnix.h>

#define OPEN 0
#define CLOSE 1
#define CREATE 2
#define MKDIR 11
#define NONE -1

struct file *file_arr[MAX_OPEN_FILES];
struct fd *fd_arr[MAX_OPEN_FILES]; // index = fd num
int open_files = 0;

struct file {
    int open;   // is this file open? 0 if no, and 1 if yes.
    char *pathname;
    FILE *fptr;
};

struct fd {
    int used; // 0 if free, 1 if in use
    struct file *file;
    int cur_pos;  // current position inside file
    int inode_num;  // inode number of file
};

// what are all of these parameters used for?
struct msg {    // 32-byte all-purpose message
    int type;   // format type?
    int data;
    char content[16];
    void *ptr;
};

/* Set up file storage.*/
void initFileStorage() {
    TracePrintf(0, "Initializing file storage!\n");
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        struct file *new_file;
        new_file->open = 0;
        file_arr[fd] = new_file;
        struct fd *new_fd;
        new_fd->used = 0;
        fd_arr[fd] = new_fd;
        // resetFile(new_file);
    }
}

/* Return free file descriptor. -1 if storage is full.*/
int findFreeFD() {
    if (open_files < MAX_OPEN_FILES) {
        int fd;
        for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
            if (fd_arr[fd]->used == 0) { //RZW: changed to fd_arr from file_arr
                TracePrintf(0, "findFreeFD: free spot %d found in file storage.\n", fd);
                return fd;
            }
        }
    } else {
        TracePrintf(0, "findFreeFD: ERROR max # open files reached.\n");
        return -1;
    }
}

/**
 * Return pointer to file data structure.
*/
struct file *getFilePtr(char *pathname) {
    int i;
    struct msg *container; //RZW: added this  for the later Sends in here. Maybe we dont' need to send a message
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_arr[i]->pathname == pathname) {//RZW: changed to file_arr from files_arr
            TracePrintf(1, "getFilePtr: file is already opened.\n");
            // return file_arr[fd]; //changed this index from "fd" to "i"
            return file_arr[i]; 
        }
    }

    FILE *new_fptr = fopen(pathname, "r+");
    if (new_fptr == NULL)
    {
        TracePrintf(1, "getFilePtr: file does not exist.\n");
        // container->content = "ERROR";
        strcpy(container->content, "ERROR"); //RZW: commented previous line, added this line to fix "incompatible types when assigning to type 'char[16]' from type 'char *'"
        Send((void *)&container, pid);
        return 0;
    } else {
        // find empty slot on file storage
        // already checked earlier, guaranteed to have at least 1 free slot
        for (i = 0; i < MAX_OPEN_FILES; i ++) {
            if (files_arr[i]->open == 0) {
                files_arr[i]->pathname = pathname;
                files_arr[i]->fptr = new_fptr;
                open_files++;
                return files_arr[i];
            }
        }
    }
}

/**
 * Reset given file descriptor to be free.
*/
void resetFile(struct fd *file) { //RZW: changed all of these from "file_arr" to "file", changed input type to struct fd
    file->used = 0;
    file->pathname = "";
    file->ftpr = 0;
    file->is_file = -1;
    file->cur_pos = 0;
    file->inode_num = -1;
}

/**
 * This request opens the file named by pathname.
*/
int Open(char *pathname) {

    // build message
    struct msg *container;
    container->type = OPEN;

    // check if there is space in storage
    int fd = findFreeFD();
    if (fd == -1) {
        // container->content = "ERROR";
        strcpy(container->content, "ERROR"); //RZW: commented previous line, added this line to fix "incompatible types when assigning to type 'char[16]' from type 'char *'"
        Send((void *)&container, pid);
        return -1;
    }

    // check if file is valid
    // TODO: implement functionality for directories
    struct file *file = getFilePtr(pathname);
    if (fptr == 0) {
        return -1;
    }

    fd_arr[fd]->used = 1; 
    fd_arr[fd]->inode_num = 0; // TODO: where to find
    fd_arr[fd]->cur_pos = 0; 
    fd_arr[fd]->file = file; 
    
    container->data = fd;
    Send((void *)&container, pid);
    TracePrintf(0, "Open: message sent.\n");
    return 0;
}

/**
 * This request closes the open file specified by the file descriptor number fd . If fd is not the descriptor
 * number of a file currently open in this process, this request returns ERROR; otherwise, it returns 0
*/
int Close(int fd) {
    // build message
    struct msg *container;
    container->type = CLOSE;

    if (file_arr[fd]->used == 0) { // file is not open
        TracePrintf(1, "Close: file is not open.\n");
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }

    char *target_file = file_arr[fd]->pathname;
    // ensure that file is closed for all of its fds
    int i;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_arr[i]->pathname == target_file) {    
            fclose(file_arr[i]->fptr); // close file stream
            resetFile(file_arr[i]);
            open_files--;
        }
    }

    container->data = 0;
    Send((void *)&container, pid);
    return 0;
}

/**
 * This request creates and opens the new file named pathname .
 */
int Create(char *pathname) {
    // build message
    struct msg *container;
    container->type = CREATE;

    // TODO: check if pathname directories alr exist
    if (pathname == "." || pathname == "..") {  // TODO: check if name is same as any directory
        TracePrintf(1, "Create: filename cannot be the same as a directory.\n");
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }

    // check if there is space in storage
    int fd = findFreeFile();
    if (fd == -1)
    {
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }

    FILE *new_file;
    new_file = fopen(pathname, "wb+");
    if (new_file == NULL) {
        TracePrintf(1, "Create: unable to open file.\n");
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }
    file_arr[fd]->pathname = pathname;
    file_arr[fd]->fptr = (void *)fptr;
    file_arr[fd]->is_file = 1;
    open_files++;

    container->data = fd;

    Send((void *)&container, pid);
    TracePrintf(0, "Open: message sent.\n");
    return 0;
}

// int Read(int fd, void *buf, int size) {
//     return 0;
// }

// int Write(int fd, void *, int) {
//     return 0;
// }
// int Seek(int, int, int) {
//     return 0;
// }
int MkDir(char *path) { //used to send a dummy message
    struct *msg container;//TODO: malloc here?
    container->type = MKDIR;
    Send((void *)&container, pid);
    return 0;
}
// int RmDir(char *) {
//     return 0;
// }
// int ChDir(char *) {
//     return 0;
// }
// int Stat(char *, struct Stat *) {
//     return 0;
// }
// int Sync(void) {
//     return 0;
// }
// int Shutdown(void) {
//     return 0;
// }