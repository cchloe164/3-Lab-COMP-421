#include <comp421/iolib.h>

int fd_nums_in_use[MAX_OPEN_FILES];

struct file_descriptor {
    int fd_num; // file descriptor number
    int inode_num;  // inode number of file
    void *cur_pos;  // current position inside file
} fd

// what are all of these parameters used for?
struct msg {    // 32-byte all-purpose message
    int type;   // format type?
    int data2;
    char content[16];
    void *ptr;
}

void sendMsg(int type, int data2, char[16] content, void *ptr, int pid) {
    struct msg *container;
    container->type = type;
    container->data2 = data2;
    container->content = content;
    container->ptr = ptr;

    if (Send((void *)&container, pid) == 0) {
        printf("sendMsg: message sent.\n");
    } else {
        printf("sendMsg: ERROR.\n");
    }
} 

int Open(char *pathname) {
    return 0;
}