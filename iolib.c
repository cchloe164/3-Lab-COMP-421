#include <comp421/iolib.h>
#include <dirent.h>

#define OPEN 0;
#define CLOSE 1;

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
                printf("findFreeFile: free spot %d found in file storage.\n", fd);
                return fd;
            }
        }
    } else {
        printf("findFreeFile: ERROR max # open files reached.\n");
        return -1;
    }
}

void resetFile(struct file *file) {
    files->used = 0;
    files->pathname = "";
    files->ftpr = 0;
    files->is_file = -1;
    files->cur_pos = 0;
    files->inode_num = -1;
}

void sendMsg(int type, int data2, char[16] content, void *ptr, int pid) {
    struct msg *container;
    container->type = type;
    container->data = data2;
    container->content = content;
    container->ptr = ptr;

    if (Send((void *)&container, pid) == 0) {
        printf("sendMsg: message sent.\n");
    } else {
        printf("sendMsg: ERROR.\n");
    }
}

/**
 * This request opens the file named by pathname . If the file exists, this request returns a file de-
 * scriptor number that can be used for future requests on this opened file; the file descriptor number
 * assigned for this newly open file must be the lowest available (unused) file descriptor number that
 * could be assigned for this newly opened file. If the file does not exist, or if any other error occurs, this
 * request returns ERROR. It is not an error to Open() a directory; the contents of the open directory
 * (the bytes in the format of a directory) can then be read using Read(), although it is an error to
 * attempt to Write() to the open directory. If the Open is successful, the current position for Read
 * or Write operations on this file descriptor begins at position 0.
 * Within a process, each successful call to Open or Create must return a new, unique file descriptor
 * number for the open file. Each such instance of a file descriptor open to this file thus has its own,
 * separate current position within the file.
*/
int Open(char *pathname) {

    // check if there is space in storage
    int fd = findFreeFile();
    if (fd == -1) {
        return ERROR;
    }

    // check if file is valid
    // TODO: implement functionality for directories
    FILE *fptr = fopen(pathname, "r+");
    if (fptr == NULL) {
        printf("Open: ERROR file does not exist.\n");
        return ERROR;
    }
    files[fd]->pathname = pathname;
    files[fd]->fptr = (void *)fptr;
    files[fd]->is_file = 1;
    open_files++;

    struct msg *container;
    container->type = OPEN;
    container->data = fd;
    // container->content = content;
    // container->ptr = ptr;
    
    if (Send((void *)&container, pid) == 0)
    {
        printf("Open: message sent.\n");
        return 0;
    }
    else
    {
        printf("Open: ERROR.\n");
        return ERROR;
    }
}

/**
 * This request closes the open file specified by the file descriptor number fd . If fd is not the descriptor
 * number of a file currently open in this process, this request returns ERROR; otherwise, it returns 0
*/
int Close(int fd) {
    if (files[fd]->used == 0) { // file is not open
        return ERROR;
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
    return 0;
}

// int Create(char *pathname) {
//     return 0;
// }

// int Read(int fd, void *buf, int size) {
//     return 0;
// }
// int Write(int fd, void *, int) {
//     return 0;
// }
// int Seek(int, int, int) {
//     return 0;
// }
// int Link(char *, char *) {
//     return 0;
// }
// int Unlink(char *) {
//     return 0;
// }
// int SymLink(char *, char *) {
//     return 0;
// }
// int ReadLink(char *, char *, int) {
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