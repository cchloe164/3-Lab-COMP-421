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
#define DUMMY 50

// struct file *file_arr[MAX_OPEN_FILES];
struct fd *fd_arr[MAX_OPEN_FILES]; // index = fd num
int open_files = 0;

int current_directory = ROOTINODE;
// struct file {
//     int open;   // is this file open? 0 if no, and 1 if yes.
//     char *pathname;
//     FILE *fptr;
// };

struct fd {
    int used; // 0 if free, 1 if in use
    // struct file *file;
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
        // struct file new_file;
        // new_file.open = 0;
        // file_arr[fd] = &new_file;

        struct fd new_fd;
        new_fd.used = 0;
        new_fd.cur_pos = 0;
        fd_arr[fd] = &new_fd;
    }
}

/* Return free file descriptor. -1 if storage is full.*/
int findFreeFD() {
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (fd_arr[fd]->used == 0) { 
            TracePrintf(0, "findFreeFD: free spot %d found in file storage.\n", fd);
            return fd;
        }
    }
    TracePrintf(0, "findFreeFD: max # file descriptors reached.\n");
    return -1;
}

// /**
//  * Return pointer to file data structure.
// */
// struct file *getFilePtr(char *pathname) {
//     int i;
//     for (i = 0; i < MAX_OPEN_FILES; i++) {
//         if (file_arr[i]->pathname == pathname && file_arr[i]->open == 1)
//         { 
//             TracePrintf(1, "getFilePtr: file is already opened.\n");
//             // return file_arr[fd]; //changed this index from "fd" to "i"
//             return file_arr[i];
//         }
//     }

//     if (open_files >= MAX_OPEN_FILES) {
//         TracePrintf(1, "getFilePtr: max # of open files reached.\n");
//         return NULL;
//     }
    
//     FILE *new_fptr = fopen(pathname, "r+");
//     if (new_fptr == NULL)
//     {
//         TracePrintf(1, "getFilePtr: file does not exist.\n");
//         return NULL;
//     } else {
//         // find empty slot on file storage
//         // already checked earlier, guaranteed to have at least 1 free slot
//         for (i = 0; i < MAX_OPEN_FILES; i ++) {
//             if (file_arr[i]->open == 0) {
//                 file_arr[i]->pathname = pathname;
//                 file_arr[i]->fptr = new_fptr;
//                 open_files++;
//                 return file_arr[i];
//             }
//         }

//         TracePrintf(1, "getFilePtr: logic should not reach here.\n");
//         return NULL;
//     }
// }

// /**
//  * Reset given file descriptor to be free.
// */
// void resetFile(struct file *file) {
//     file_arr->used = 0;
//     file_arr->pathname = "";
//     file_arr->ftpr = 0;
//     file_arr->is_file = -1;
//     file_arr->cur_pos = 0;
//     file_arr->inode_num = -1;
// }

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
        return ERROR;
    }

    fd_arr[fd]->used = 1; 
    fd_arr[fd]->cur_pos = 0; 
    
    container->ptr = (void *) pathname;
    container->data = current_directory;
    Send((void *)&container, -FILE_SERVER); // blocked here waiting for reply
    fd_arr[fd]->inode_num = container->data;
    TracePrintf(5, "Open: inode num %d set.\n", fd_arr[fd]->inode_num = container->data);

    TracePrintf(5, "Open: message sent.\n");
    return 0;
}

/**
 * This request closes the open file specified by the file descriptor number fd . If fd is not the descriptor
 * number of a file currently open in this process, this request returns ERROR; otherwise, it returns 0
*/
// int Close(int fd) {
//     // free up fd
//     fd_arr[fd]->used = 0;

//     // find target file
//     struct file *target_file = fd_arr[fd]->file;
//     if (target_file->open == 1) { // file is not open
//         TracePrintf(1, "Close: file is not open.\n");
//         return ERROR;
//     } else { // close file
//         target_file->open = 0;
//         fclose(target_file->fptr);
//         open_files--;

//         return 0;
//     }
// }

/**
 * This request creates and opens the new file named pathname .
 */
// int Create(char *pathname) {
//     TracePrintf(0, "Create function called from iolib!\n");
//     // build message
//     struct msg *container;
//     container->type = CREATE;

//     // TODO: check if pathname directories alr exist
//     if (strcmp(pathname, ".") == 0 || strcmp(pathname, "..") == 0)
//     { // TODO: check if name is same as any directory
//         TracePrintf(1, "Create: filename cannot be the same as a directory.\n");
//         return ERROR;
//     }

//     // check if there is space in storage
//     int fd = findFreeFD();
//     if (fd == -1)
//     {
//         return ERROR;
//     }

//     struct file *new_file;
//     new_file = getFilePtr(pathname);
//     if (new_file == NULL) {
//         TracePrintf(1, "Create: unable to open file.\n");
//         return ERROR
//     }
//     fd_arr[fd]->used = 1;
//     fd_arr[fd]->file = new_file;
//     fd_arr[fd]->cur_pos = 0;
//     fd_arr[fd]->inode_num = 0;


//     container->data = fd;
//     Send((void *)&container, -FILE_SERVER);
//     TracePrintf(0, "Open: message sent.\n");
//     return 0;
// }

// int Read(int fd, void *buf, int size) {
//     TracePrintf(0, "Create function called from iolib!\n");
//     struct fd *target_fd = fd_arr[fd];
//     struct file *target_file = target_fd->file;

//     fread(target_file)

//     return 0;
// }

// int Write(int fd, void *, int) {
//     return 0;
// }
// int Seek(int, int, int) {
//     return 0;
// }
int Dummy(char *path) { //used to send a dummy message
    TracePrintf(0, "dummy: message sending.\n");
    struct msg *container = malloc(sizeof(struct msg));//TODO: malloc here?
    TracePrintf(0, "testset2\n");
    container->type = DUMMY;

    // TracePrintf(0, "Making directory %s\n", path);
    TracePrintf(0, "testset2\n");
    Send(container, -FILE_SERVER);
    TracePrintf(0, container->content);
    (void) path;
    return 0;
}

int MkDir(char *path) { //used to send a dummy message
    TracePrintf(0, "MkDir: message sending.\n");
    struct msg *container = malloc(sizeof(struct msg));//TODO: malloc here?
    // TracePrintf(0, "testset2\n");
    container->type = MKDIR;
    strcpy(container->content, path);
    // container->content = path;
    container->data = current_directory; //data is the directory

    // TracePrintf(0, "Making directory %s\n", path);
    // TracePrintf(0, "testset2\n");
    Send(container, -FILE_SERVER);
    TracePrintf(0, container->content);
    
    // (void) path;
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