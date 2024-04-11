#include <comp421/iolib.h>
#include <dirent.h>

#define OPEN 0;
#define CLOSE 1;
#define CREATE 2

struct file *files[MAX_OPEN_FILES]; // index = fd num
int open_files = 0;

struct file {
    int used; // 0 if free, 1 if in use
    int fd_num; // file descriptor number
    char *pathname;
    void *fptr; // DIR or FILE ptr
    int is_file;
    int cur_pos;  // current position inside file
    int inode_num;  // inode number of file
}

// what are all of these parameters used for?
struct msg {    // 32-byte all-purpose message
    int type;   // format type?
    int data;
    char content[16];
    void *ptr;
}

/* Set up file storage.*/
void initFileStorage() {
    TracePrintf(0, "Initializing file storage!\n");
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        struct file *new_file;
        new_file->fd_num = fd;
        resetFile(new_file);
        files[fd] = new_file;
    }
}

/* Return free file from storage. -1 if storage is full.*/
int findFreeFile() {
    if (open_files < MAX_OPEN_FILES) {
        int fd;
        for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
            if (files[fd]->used == 0) {
                TracePrintf("findFreeFile: free spot %d found in file storage.\n", fd);
                return fd;
            }
        }
    } else {
        TracePrintf("findFreeFile: ERROR max # open files reached.\n");
        return -1;
    }
}

/**
 * Reset given file descriptor to be free.
*/
void resetFile(struct file *file) {
    files->used = 0;
    files->pathname = "";
    files->ftpr = 0;
    files->is_file = -1;
    files->cur_pos = 0;
    files->inode_num = -1;
}

/**
 * This request opens the file named by pathname.
*/
int Open(char *pathname) {

    // build message
    struct msg *container;
    container->type = OPEN;

    // check if there is space in storage
    int fd = findFreeFile();
    if (fd == -1) {
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }

    // check if file is valid
    // TODO: implement functionality for directories
    FILE *fptr = fopen(pathname, "r+");
    if (fptr == NULL) {
        TracePrintf(1, "Open: ERROR file does not exist.\n");
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }
    files[fd]->pathname = pathname;
    files[fd]->fptr = (void *)fptr;
    files[fd]->is_file = 1;
    open_files++;

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

    if (files[fd]->used == 0) { // file is not open
        TracePrintf(1, "Close: file is not open.\n");
        container->content = "ERROR";
        Send((void *)&container, pid);
        return -1;
    }

    char *target_file = files[fd]->pathname;
    // ensure that file is closed for all of its fds
    int i;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (files[i]->pathname == target_file) {    
            fclose(files[i]->fptr); // close file stream
            resetFile(files[i]);
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
    files[fd]->pathname = pathname;
    files[fd]->fptr = (void *)fptr;
    files[fd]->is_file = 1;
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
// int MkDir(char *) {
//     return 0;
// }
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