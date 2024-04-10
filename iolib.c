#include <comp421/iolib.h>
#include <dirent.h>

#define OPEN 0;

struct file *files[MAX_OPEN_FILES]; // index = fd num
int open_files = 0;

struct file {
    int used; // 0 if free, 1 if in use
    int fd_num; // file descriptor number
    void *fptr; // DIR or FILE ptr
    int is_file;
    void *cur_pos;  // current position inside file
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
        new_file->used = 0;
        new_file->fd_num = fd;
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
    files[fd]->fptr = (void *)fptr;
    files[fd]->is_file = 1;
    files[fd]->cur_pos = 0;
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
// int Close(int fd) {
//     return 0;
// }

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