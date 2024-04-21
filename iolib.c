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
#define READ 3
#define WRITE 4
#define SEEK 5
#define LINK 6
#define UNLINK 7
#define SYMLINK 8
#define READLINK 9
#define MKDIR 10
#define RMDIR 11
#define CHDIR 12
#define STAT 13
#define SYNC 14
#define SHUTDOWN 15
#define NONE -1
#define BLOCK_FREE 0
#define BLOCK_USED 1
#define DUMMY 50
#define DUMMY2 51
#define ERMSG -2
#define REPLYMSG -3

// struct file *file_arr[MAX_OPEN_FILES];
struct fd *fd_arr[MAX_OPEN_FILES]; // index = fd num
int open_files = 0;
int init_storage = 0;

int current_directory = ROOTINODE;

struct fd {
    int used; // 0 if free, 1 if in use
    int type;   // dir or file
    int cur_pos;  // current position inside file
    int inum;  // inode number of file
};

struct path_str {
    char _path[DIRNAMELEN]; //idk if this should be bounded
    int length;
};
struct seek_info {
    int offset;
    int whence;
    int cur_pos;
};

struct read_info {
    int size;
    int inum;
    int cursor;
    void *buf;
};
// what are all of these parameters used for?
struct msg {    // 32-byte all-purpose message
    int type;   // format type?
    int data;
    char content[16];
    void *ptr;
};
struct link_strs { //structure for linking
    //31 if the name is 30 chars and a null
    char old[DIRNAMELEN]; //the old string
    char new[DIRNAMELEN]; //the new string
    int old_len; //length of the old string
    int new_len; //length of the new string
};


struct ext_msg {
    char content[100];
    int size;
    int inum;
};

/* Set up file storage.*/
void initFileStorage() {
    TracePrintf(0, "Initializing file storage!\n");
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        struct fd *new_fd = malloc(sizeof(struct fd));
        new_fd->used = 0;
        new_fd->cur_pos = 0;
        new_fd->inum = -1;
        fd_arr[fd] = new_fd;
    }
}

struct path_str *package_path(char *path) {
    // TracePrintf(0, "Packaging the path %s\n", path);
    struct path_str *path_str = malloc(sizeof(path_str));
    strcpy(path_str->_path, path);
    path_str->length = strlen(path);
    TracePrintf(0, "packaged the path %s\n", path_str->_path);
    return path_str;

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


/**
 * This request opens the file named by pathname.
*/
int Open(char *pathname) {
    TracePrintf(0, "OPENing file %s...\n", pathname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    // build message
    struct msg *container = malloc(sizeof(struct msg));
    container->type = OPEN;

    // check if there is space in storage
    int fd = findFreeFD();
    if (fd == -1) {
        return ERROR;
    }

    fd_arr[fd]->used = 1; 
    fd_arr[fd]->cur_pos = 0;

    // strcpy(container->content, pathname);
    container->ptr = package_path(pathname);
    container->data = current_directory;
    Send(container, -FILE_SERVER); // blocked here waiting for reply
    if (container->data == ERMSG) {
        return ERROR;
    }

    fd_arr[fd]->inum = container->data;
    TracePrintf(0, "Open: inode num %d set.\n", fd_arr[fd]->inum);

    free(container);
    TracePrintf(0, "Open: success.\n");
    return fd;
}

/**
 * This request closes the open file specified by the file descriptor number fd . If fd is not the descriptor
 * number of a file currently open in this process, this request returns ERROR; otherwise, it returns 0
*/
int Close(int fd) {
    TracePrintf(0, "CLOSEing fd %d...\n", fd);
    if (fd_arr[fd]->used == 0)
    { // file is not open
        TracePrintf(0, "Close: file is not open.\n");
        return ERROR;
    }
    else
    { // close file
        fd_arr[fd]->used = 0;
        open_files--;
        return 0;
    }
}

/**
 * This request creates and opens the new file named pathname .
 */
int Create(char *pathname) {
    TracePrintf(0, "CREATEing pathname %s...\n", pathname);

    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }

    // build message
    struct msg *container = malloc(sizeof(struct msg));
    container->type = CREATE;
    // check if there is space in storage
    int fd = findFreeFD();
    if (fd == -1)
    {
        return ERROR;
    }

    fd_arr[fd]->used = 1;
    fd_arr[fd]->cur_pos = 0;

    // strcpy(container->content, pathname);
    container->data = current_directory;
    container->ptr = package_path(pathname);
    Send(container, -FILE_SERVER);
    if (container->data == ERMSG) {
        return ERROR;
    }
    fd_arr[fd]->inum = container->data;
    TracePrintf(0, "Create: inode num %d set.\n", fd_arr[fd]->inum);

    free(container);
    TracePrintf(0, "Create: success.\n");
    return fd;
}



int Read(int fd, void *buf, int size) {
    TracePrintf(0, "READing fd %d...\n", fd);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    if (fd_arr[fd]->used == 0) {
        TracePrintf(0, "Read: file not open.\n");
        return ERROR;
    }

    // build message
    struct msg *container = malloc(sizeof(struct msg));
    struct read_info *read_info = malloc(sizeof(struct read_info));
    // strcpy(extra_info->content, buf);
    read_info->size = size;
    read_info->inum = fd_arr[fd]->inum;
    read_info->buf = buf;
    read_info->cursor = fd_arr[fd]->cur_pos;

    container->type = READ;
    container->data = fd_arr[fd]->inum;
    container->ptr = read_info;

    Send(container, -FILE_SERVER); //should return the updated cursor position in the data field
    if (container->data == ERMSG)
    {
        TracePrintf(0, "Read: error.\n");
        return ERROR;
    }
    fd_arr[fd]->cur_pos = container->data; //update the cursor position with the value returned by the read request

    free(container);
    free(read_info);
    TracePrintf(0, "Read: success.\n");
    return 0;
}

int Write(int fd, void *buf, int size) {
    TracePrintf(0, "WRITEing '%s' to fd %d...\n", buf, fd);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    if (fd_arr[fd]->used == 0)
    {
        TracePrintf(0, "Write: file not open.\n");
        return ERROR;
    }

    // build message
    struct msg *container = malloc(sizeof(struct msg));
    struct ext_msg *extra_info = malloc(sizeof(struct ext_msg));
    strcpy(extra_info->content, buf);
    extra_info->size = size;
    extra_info->inum = fd_arr[fd]->inum;
    container->type = WRITE;
    container->data = fd_arr[fd]->cur_pos;
    container->ptr = (void *)extra_info;

    Send(container, -FILE_SERVER);
    if (container->data == ERMSG)
    {
        return ERROR;
    }

    free(container);
    free(extra_info);
    TracePrintf(0, "Write: success.\n");
    return container->data;
}

int Seek(int fd, int offset, int whence) { //TODO: resume there
    TracePrintf(0, "SEEKing in fd %d to offset %i, with whence %i...\n", fd, offset, whence);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    if (fd_arr[fd]->used == 0)
    {
        TracePrintf(0, "Write: file not open.\n");
        return ERROR;
    }

    // build message
    struct msg *container = malloc(sizeof(struct msg));
    struct seek_info *seek_info = malloc(sizeof(struct seek_info));
    seek_info->offset = offset;
    seek_info->whence = whence;
    seek_info->cur_pos = fd_arr[fd]->cur_pos;
    container->type = SEEK;
    container->data = fd_arr[fd]->inum;
    container->ptr = (void *)seek_info;

    Send(container, -FILE_SERVER);
    if (container->data == ERMSG)
    {
        return ERROR;
    }
    fd_arr[fd]->cur_pos = container->data;
    free(container);
    // free(extra_info);
    TracePrintf(0, "Seek: success.\n");
    return container->data;
}


int Unlink(char *pathname) {
    TracePrintf(0, "Unlink: unlinking path %s\n", pathname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    struct msg *container = malloc(sizeof(struct msg));
    container->type = UNLINK;
    container->data = current_directory;
    // strcpy(container->content, pathname);
    container->ptr = package_path(pathname);
    Send(container, -FILE_SERVER);
    if (container->type == REPLYMSG) {
        TracePrintf(0, "Unlink: made directory %s\n", ((struct path_str *)container->ptr)->_path);
        TracePrintf(0, "Unlink: success.\n");
        free(container);
        return 0;
    } else if (container->type == ERMSG) {
        TracePrintf(0, "Error unlinking directory \n");
        free(container);
        return ERROR;
    } else {
        TracePrintf(0, "Unlink: should not reach this point.\n");
        free(container);
        return ERROR;
    }

}
int Link(char *oldname, char *newname) {
    TracePrintf(0, "Link: linking old path %s to new path %s\n", oldname, newname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    struct msg *container = malloc(sizeof(struct msg));
    container->type = LINK;
    container->data = current_directory;

    struct link_strs *strs = malloc(sizeof(struct link_strs));
    strcpy(strs->old, oldname);
    strcpy(strs->new, newname);
    strs->old_len = strlen(oldname);
    strs->new_len = strlen(newname);
    // link_strs->new = newname;
    container->ptr = strs;
    Send(container, -FILE_SERVER);
    if (container->type == REPLYMSG) {
        TracePrintf(0, "Link: made directory %s\n", ((struct path_str *)container->ptr)->_path);
        TracePrintf(0, "Link: success.\n");
        free(container);
        return 0;
    } else if (container->type == ERMSG) {
        TracePrintf(0, "Error linking directory \n");
        free(container);
        return ERROR;
    } else {
        TracePrintf(0, "Link: should not reach this point.\n");
        return ERROR;
    }
}

int Dummy2(char *oldname, char *newname) {
    TracePrintf(0, "Dummy: sending old path %s and new path %s\n", oldname, newname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    struct msg *container = malloc(sizeof(struct msg));
    container->type = DUMMY2;
    container->data = current_directory;

    struct link_strs *strs = malloc(sizeof(struct link_strs));
    strcpy(strs->old, oldname);
    strcpy(strs->new, newname);
    strs->old_len = strlen(oldname);
    strs->new_len = strlen(newname);
    // link_strs->new = newname;
    container->ptr = strs;
    Send(container, -FILE_SERVER);
    if (container->type == REPLYMSG) {
        TracePrintf(0, "Dummy: made directory %s\n", container->content);
        TracePrintf(0, "Dummy: success.\n");
        free(container);
        return 0;
    } else if (container->type == ERMSG) {
        TracePrintf(0, "Error linking directory \n");
        free(container);
        return ERROR;
    } else {
        TracePrintf(0, "Link: should not reach this point.\n");
        return ERROR;
    }
}

int Dummy(char *path) { //used to send a dummy message
    TracePrintf(0, "dummy: message sending.\n");
    struct msg *container = malloc(sizeof(struct msg));//TODO: malloc here?
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    TracePrintf(0, "testset2\n");
    container->type = DUMMY;

    // TracePrintf(0, "Making directory %s\n", path);
    TracePrintf(0, "testset2\n");
    Send(container, -FILE_SERVER);
    TracePrintf(0, container->content);
    (void) path;
    return 0;
}

int MkDir(char *pathname) { //used to send a dummy message
    TracePrintf(0, "MKDIRing pathname %s...\n", pathname);
    if (init_storage == 0)
    {
        initFileStorage();
        init_storage = 1;
    } 

    struct msg *container = malloc(sizeof(struct msg));//TODO: malloc here?
    container->type = MKDIR;
    // strcpy(container->content, pathname);
    container->data = current_directory; //data is the directory
    container->ptr = package_path(pathname);
    Send(container, -FILE_SERVER);
    if (container->type == REPLYMSG) {
        // fd_arr[fd]->inum = container->data;
        TracePrintf(0, "MkDir: made directory %s\n", ((struct path_str *)container->ptr)->_path);
        free(container);
        return 0;
    } else if (container->type == ERMSG) {
        TracePrintf(0, "Error making directory \n");
        free(container);
        return ERROR;
    } else {
        TracePrintf(0, "MkDir: should not reach this point.\n");
        return ERROR;
    }
    
    // (void) path;
}

int RmDir(char *pathname) {
    TracePrintf(0, "RMDIRing pathname %s...\n", pathname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    // build message
    struct msg *container = malloc(sizeof(struct msg));
    container->type = RMDIR;
    container->data = current_directory;
    container->ptr = package_path(pathname);
    // strcpy(container->content, pathname);
    Send(container, -FILE_SERVER);
    if (container->data == ERMSG)
    {
        TracePrintf(0, "RmDir: failed.\n");
        return ERROR;
    }

    free(container);
    TracePrintf(0, "RmDir: success.\n");
    return 0;
}

int ChDir(char *pathname) {
    TracePrintf(0, "CHDIRing pathname '%s'...\n", pathname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    // TracePrintf(0, "here1");
    // build message
    struct msg *container = malloc(sizeof(struct msg));
    // TracePrintf(0, "here4");
    container->type = CHDIR;
    // TracePrintf(0, "here3");
    container->data = current_directory;
    container->ptr = package_path(pathname);
    // TracePrintf(0, "here2");
    // strcpy(container->content, pathname);
    // TracePrintf(0, "here6");
    Send(container, -FILE_SERVER);
    // TracePrintf(0, "here9");
    if (container->data == ERMSG)
    {
        TracePrintf(0, "ChDir: Error changing to directory %d\n", current_directory);
        return ERROR;
    }
    current_directory = container->data;    // update cur_dir to new directory
    TracePrintf(0, "ChDir: changing to directory %d\n", current_directory);

    free(container);
    TracePrintf(0, "ChDir: success.\n");
    return 0;
}

int Stat(char *pathname, struct Stat *statbuf) {
    TracePrintf(0, "STATing pathname %s...\n", pathname);
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }

    // build message
    struct msg *container = malloc(sizeof(struct msg));
    container->type = STAT;
    container->data = current_directory;
    // strcpy(container->content, pathname);
    container->ptr = package_path(pathname);

    Send(container, -FILE_SERVER);
    if (container->data == ERMSG)
    {
        return ERROR;
    }
    struct Stat *res = (struct Stat *)container->content;
    statbuf->inum = res->inum;
    statbuf->type = res->type;
    statbuf->size = res->size;
    statbuf->nlink = res->nlink;

    free(container);
    TracePrintf(0, "Stat: success.\n");
    return 0;
}

int Sync(void) {
    if (init_storage == 0) {
        initFileStorage();
        init_storage = 1;
    }
    return 0;
}
int Shutdown(void) {
    Sync();
    TracePrintf(0, "Shutting down server.\n");
    struct msg *container = malloc(sizeof(struct msg));
    container->type = SHUTDOWN;
    Send(container, -FILE_SERVER);
    if (container->type == ERMSG) {
        TracePrintf(0, "Server shutdown failure.\n");
    } else {
        TracePrintf(0, "Server shutdown success.\n");
    }
    free(container);
    Exit(0);
    return 0;
}