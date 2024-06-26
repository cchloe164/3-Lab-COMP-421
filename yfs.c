#include <comp421/filesystem.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <comp421/iolib.h>
// #include "iolib.c"
#include <comp421/yalnix.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define block_from_size(a) (a / BLOCKSIZE)

#define INODESPERBLOCK BLOCKSIZE / INODESIZE
#define MAXINODESIZE BLOCKSIZE * (NUM_DIRECT + (BLOCKSIZE / sizeof(int))) //the max size for storage of an inode

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
#define FREEFILE 0

struct path_str {
    char _path[DIRNAMELEN];
    int length;
};

struct msg { //Dummy message structure from the given PDF
    int type; 
    int data;
    char content[16];
    void *ptr;
};

struct in_str { //an inode linkedlist class
    // struct inode *node;
    int inum;
    struct in_str *next;
    struct in_str *prev;
};

struct seek_info {
    int offset;
    int whence;
    int cur_pos;

};
struct read_info {
    int size; //the size to read
    int inum; // the inode num
    int cursor;
    void *buf; //the buffer to write to
};
struct link_strs { //structure for linking
    //31 if the name is 30 chars and a null
    char old[DIRNAMELEN];
    char new[DIRNAMELEN];
    int old_len;
    int new_len;
};

int pid;
int getPid();
//Function signatures
int sectorBytes(struct inode *node); // how many bytes are used in the last used block/sector of the node (use this to calculate how much free space is left in current block, or where free space exists)
struct inode *findInodePtr(int inum); //DEFUNCT; if you need this logic, find it in SETNEWINODE
int findDirectoryEntry(struct inode *curr_inode, int curr_inode_num, char *name); // Searches for file or directory named "name" in the inode (presumably a directory type)
int markUsed(int blocknum); //marks a block as used
void init(); //initializes the fs structure
int readInodeBlock(int block_index, int num_inodes_to_read, void *buf, int start_index);  //used only for init() to read blocks that contain inodes
int readInode(int inum, void *buf); //Super useful: takes in an inode number and a buffer, reads the contents of the inode into the buffer (please remember to write back to disk)
int writeInodeToDisk(int inum, struct inode *inode_to_cpy); //writes the inode info contained in inode_to_cpy to the inode in disk specified by inum
void addFreeInode(int inum); //used in init to add inodes to the freelist; if you free an inode, add it here
int findFreeBlock(); //finds a free block
int writeDirectoryToInode(struct inode *node, int curr_inode, int inum, char *name); //writes a dir_entry containing inum and name to the inode curr_inode.
int findParent(char *name, int curr_directory); //finds the parent of the path
int readBlock(int block_index, void *buf); //reads the block 
int numBlocksUsedBy(struct inode *node); //calculates the number of blocks used by the inode (based on size)
int setNewInode(int inum, short type, short nlink, int reuse, int size, int parent_inode_num); //creates a new inodes
//building the list in memory of free disk blocks
void mkDirHandler(struct msg *message, int senderPid); //handles mkdir requests
void writeHandler(struct msg *message, int senderPid);
void openHandler(struct msg *message, int senderPid);
int getSector(struct inode *node, int sector_num); // gets the sector used in the inode
int getLastSector(struct inode *node); // gets the last sector used in the inode
int getFreeInode(); //gets a free inode
char *findLastDirName(char *name); //finds the last directory name in a path (after the last '/', or the whole string)
int replyError(struct msg *message, int pid);
int replyWithInodeNum(struct msg *message, int pid, int inum);
void chDirHandler(struct msg *message, int senderPid);
void createHandler(struct msg *message, int senderPid);
void statHandler(struct msg *message, int senderPid);
int checkPath(struct msg *message, int senderPid);
void rmDirHandler(struct msg *message, int senderPid);
void prepFree(struct inode *curr_inode);
int fillLastBlock(struct inode *node, int node_num);

int freeDirectoryEntry(struct inode *curr_inode, int curr_inode_num, int inum);
int checkEmpty(int inum);
int getInodeType(int inum);
int getInodeSize(int inum);
int freeInode(int matching_inode_num);
int writeBlock(int block_index, void *buf);

void unlinkHandler(struct msg *message, int senderPid);
char *getPath(struct msg *mesage, int senderPid);
void linkHandler(struct msg *message, int senderPid);
void dummyHandler(struct msg *message, int senderPid);
int checkPathHelper(char *path, int curr_directory);
int resetInodeSize(int inode_num);
void seekHandler(struct msg *message, int senderPid);
int addNewEmptyBlock(struct inode *node, int block_index);
void readHandler(struct msg *message, int senderPid);
int addFreeBlock(int block_num);

int numBlocks;
int numFreeBlocks; //the number of free blocks
int *freeBlocks;
int inodes_per_block; 
struct in_str *free_nodes_head;
struct in_str *free_nodes_tail;
int free_nodes_size;
int pid;
/**
Other processes using the file system send requests to the server and receive replies from the server,
 using the message-passing interprocess communication calls provided by the Yalnix kernel.

 your file server process implements most of the functionality of the Yalnix file system, 
 managing a cache of inodes and disk blocks and performing all input and output with the disk drive.
*/

//create more message types
//assumes the library functions are implemented

/**
As part of its initialization, the YFS server should use the Register kernel call to register itself as the FILE_SERVER server. 
It should then Fork and Exec a first client user process.
 In particular, like any C main() program, your file server process is passed an argc and argv . 
 If this argc is greater than 1 (the first argument, argv[0], 
 is the name of your file server program itself), then your server should Fork and the child process
  should then Exec the first client user program, as:
Exec(argv[1], argv + 1);

*/
int main(int argc, char** argv) {
    TracePrintf(0, "Running main in server!\n");
    inodes_per_block = BLOCKSIZE / INODESIZE;
    free_nodes_size = 0;
    int server_id = Register(FILE_SERVER);
    if (server_id != 0) {
        TracePrintf(0, "Error registering main process \n");
        return ERROR;
    }
    init(); //must format the file system on the disk before running the server
    if (argc > 1) {
        pid = Fork();
        if (pid == 0) {
            Exec(argv[1], argv + 1);
        } else {
            // "While 1 loop handling requests"; slide 9
            while (1) {
                TracePrintf(0, "Server listening for message \n");
                void *buffer = malloc(sizeof(struct msg));
                // TracePrintf(0, "Size of msg struct %d\n", sizeof(struct msg));
                //Receive client request message, handle the request, reply back to client
                int receive = Receive(buffer);
                struct msg *message = (struct msg *)buffer;
                // struct msg *reply_message = malloc(sizeof(struct msg));
                //handle cases: error, deadlock break

                if (receive == ERROR) {
                    TracePrintf(0, "Server error message signal!\n");
                    continue; //TODO: add error handling
                }
                else if (receive == 0) {//blocking message
                    TracePrintf(0, "Server received blocking message! All processes were listening.\n");
                    //handle blocked message here
                    continue;
                }
                TracePrintf(0, "Server received message signal! ID %i\n", receive);
                //check the type field, then cast 
                //now we have to parse the message and do something with it

                //TODO: create a message type, encode the library codes in one of the fields, and call handlers?
                int type = message->type;
                TracePrintf(0, "Received message contents: type: %d\tdata: %d\tcontent: %s\n", type, message->data, message->content);
                // TracePrintf(0, "pathname: %s\n", message->ptr);
                switch(type) {
                    case NONE: {
                        TracePrintf(0, "Received NONE message type\n");
                        break;
                    }
                    case CREATE: {
                        TracePrintf(0, "Received CREATE message type\n");
                        createHandler(message, receive);
                        break;
                    }
                    case OPEN: {
                        TracePrintf(0, "Received OPEN message type\n");
                        openHandler(message, receive);
                        break;
                    }
                    case WRITE: {
                        TracePrintf(0, "Received WRITE message type\n");
                        writeHandler(message, receive);
                        break;
                    }
                    case MKDIR: {
                        TracePrintf(0, "Received MKDIR message type\n");
                        mkDirHandler(message, receive);
                        break;
                    }
                    case CHDIR: {
                        TracePrintf(0, "Received CHDIR message type\n");
                        chDirHandler(message, receive);
                        break;
                    }
                    case RMDIR: {
                        TracePrintf(0, "Received RMDIR message type\n");
                        rmDirHandler(message, receive);
                        break;
                    }
                    case UNLINK: {
                        TracePrintf(0, "Received UNLINK message type\n");
                        unlinkHandler(message, receive);
                        break;
                    }
                    case LINK: {
                        TracePrintf(0, "Received LINK message type\n");
                        linkHandler(message, receive);
                        break;
                    }
                    case STAT: {
                        TracePrintf(0, "Received STAT message type\n");
                        statHandler(message, receive);
                        break;
                    }
                    case SEEK: {
                        TracePrintf(0, "Received SEEK message type\n");
                        seekHandler(message, receive);
                        break;
                    }
                    case READ: {
                        TracePrintf(0, "Received READ message type\n");
                        readHandler(message, receive);
                        break;
                    }
                    case SHUTDOWN: {
                        TracePrintf(0, "Received SHUTDOWN message type. Shutting down.\n");
                        replyWithInodeNum(message, receive, 0);
                        Exit(0);
                        break;
                    }
                    case DUMMY: {
                        TracePrintf(0, "Received DUMMY message type\n");
                        // mkdirhandler(message);
                        strcpy(message->content,"Hello\n"); //testing whether the message is received properly (see iolib handler)

                        Reply(message, receive);
                        break;

                    }
                    case DUMMY2: {
                        TracePrintf(0, "Received DUMMY2 message type\n");
                        // mkdirhandler(message);
                        dummyHandler(message, receive);
                        break;
                    }
                    default: {
                        TracePrintf(0, "Received invalid message type\n");
                        break;
                    }
                }

            }
        }
    }
    
    return 0;

}

void readHandler(struct msg *message, int senderPid) {
    int curr_inode_num = message->data;
    struct read_info *info = malloc(sizeof(struct read_info));
    
    if (CopyFrom(senderPid, info, message->ptr, sizeof(struct read_info)) == ERROR) {
        TracePrintf(0, "ERROR copying pointer from in readHandler\n");
        replyError(message, senderPid);
        return;
    }
    int bytes_to_read = info->size; //size to read
    TracePrintf(0, "Read: Reading in %d bytes...\n", bytes_to_read);
    int inum = info->inum; //inode num
    int file_pos = info->cursor; //where we read from
    void *buf = info->buf; // the buffer to write to
    if (buf == NULL || file_pos < 0 || bytes_to_read < 0 || inum <= 0 || curr_inode_num < 1) {
        //error 
        TracePrintf(0, "invalid buffer, readfrom, size, or inode num field\n");
        replyError(message, senderPid);
        return;
    }
    struct inode *node = malloc(sizeof(struct inode));

    if (readInode(curr_inode_num, node) == ERROR) {
        TracePrintf(0, "ERROR reading inode %i in readhandler\n", curr_inode_num);
        replyError(message, senderPid);
        return;
    }
    int node_size = node->size;
    if (file_pos > node_size) {
        TracePrintf(0, "cannot read past EOF\n");
        replyError(message, senderPid);
        return;
    }
    if (bytes_to_read + file_pos > node_size) {
        TracePrintf(0, "Read: Resetting bytes_to_read\n");
        bytes_to_read = node_size - file_pos;
    }
    //iterate through and read from the blocks, update the pointer
    
    void *current_block = malloc(BLOCKSIZE); //the current block we are reading from
    int block_pos = file_pos % BLOCKSIZE;
    int total_bytes_remaining = bytes_to_read; //we decrement from this
    int last_block_idx = ((file_pos - bytes_to_read) / BLOCKSIZE) + 1;

    void *buf_ptr = malloc(bytes_to_read); //buffer to collect the contents
    void *buffer_content = buf_ptr;
    int offset;
    int block_idx;
    for (block_idx = (file_pos / BLOCKSIZE) + 1; block_idx >= last_block_idx; block_idx--)
    {
        //iterate through each block
        int block_num = getSector(node, block_idx); // the actual block
        TracePrintf(0, "Read: block index=%d\tblock num=%d\n", block_idx, block_num);
        if (readBlock(block_num, current_block) == ERROR)
        { // read the sector
            TracePrintf(0, "error reading the block in read\n");
            replyError(message, senderPid);
            return;
        }
        TracePrintf(0, "WHAT'S IN THE BLOCK: %s\n", current_block);
        //copy bytes over
        //handle case where we are on the last block and there is only part of the rest of the block

        int bytes_to_copy;
        if (total_bytes_remaining < BLOCKSIZE) {
            bytes_to_copy = total_bytes_remaining; //set it to the bytes remaining
        } else {
            bytes_to_copy = BLOCKSIZE;
        }
        //now copy everything over to the buffer
        offset = block_pos - bytes_to_copy;
        current_block += offset;
        TracePrintf(0, "Read: copying %d bytes from '%s'\n", bytes_to_copy, current_block);
        memcpy(buffer_content, current_block, bytes_to_copy);

        total_bytes_remaining -= bytes_to_copy;
        block_pos = BLOCKSIZE;
        buffer_content += bytes_to_copy;
        bytes_to_copy = BLOCKSIZE;
    }
    TracePrintf(0, "buffer_of_contents: %s\n", buf_ptr);
    if (CopyTo(senderPid, buf, buf_ptr, bytes_to_read) == ERROR) {
        TracePrintf(0, "error copying the buffer to the other buffer in read\n");
        replyError(message, senderPid);
        return;
    } //copy to the buffer from our buffer

    replyWithInodeNum(message, senderPid, file_pos + bytes_to_read); //reply with new location

}

void dummyHandler(struct msg *message, int senderPid) {
    struct link_strs* paths = malloc(sizeof(struct link_strs));
    if (CopyFrom(senderPid, paths, message->ptr, sizeof(struct link_strs)) == ERROR) {
        TracePrintf(0, "ERROR copying from in dummyhandler\n");
        replyError(message, senderPid);
    }
    TracePrintf(0, "Received dummy strings %s and %s, lengths %i, %i\n", paths->old, paths->new, paths->old_len, paths->new_len);
}

/**
Handles Seek:
This request changes the current file position of the open file specified by file descriptor number fd .
 The argument offset specifies a byte offset in the file relative to the position indicated by whence . 
 The value of offset may be positive, negative, or zero.
*/
void seekHandler(struct msg *message, int senderPid) {
    TracePrintf(0, "Made it to the seekHandler\n");
    int curr_inode_num = message->data;
    struct seek_info *info = malloc(sizeof(struct seek_info));
    if (CopyFrom(senderPid, info, message->ptr, sizeof(struct seek_info)) == ERROR) {
        TracePrintf(0, "ERROR copying pointer from in seekHandler\n");
        replyError(message, senderPid);
        return;
    }
    int offset = info->offset;
    int whence = info->whence;
    int new_pos = info->cur_pos;
    int node_size = getInodeSize(curr_inode_num);
    TracePrintf(1, "Received seek request for inode %i, which is currently at %i. offset %i and whence %i\n", curr_inode_num, new_pos, offset, whence);
    if (curr_inode_num < 1 || (whence != SEEK_SET && whence != SEEK_END && whence != SEEK_CUR)) {
        //error 
        TracePrintf(0, "invalid inode num\n");
        replyError(message, senderPid);
        return;
    }
    //check whence to edit the cur_pos based on 0, size, or leaving it alone
    //do if checking, then set the field or int to either the beginning, end, or stays the same
    if (whence == SEEK_SET) {
        new_pos = 0;
    } else if (whence == SEEK_END) {
        new_pos = node_size;
    }
    new_pos = new_pos + offset;
    //can't seek before the file (if negative offset)
    if (new_pos < 0) {
        //error: cur_pos is less than 0
        TracePrintf(0, "Tried to seek to a negative index \n");
        replyError(message, senderPid);
        return;
    }
    int highest_byte_in_block = node_size + (BLOCKSIZE - (node_size % BLOCKSIZE));
    if (new_pos <= highest_byte_in_block) {
        //just return the current position
        TracePrintf(0, "Returning from seek with new position %i, which is <= the current highest byte in block %i.\n", new_pos, highest_byte_in_block);
        replyWithInodeNum(message, senderPid, new_pos); //thi should put the cur_pos in the data field
        return;
    }
    //Hard case: where the pointer is after the size. Fill the gap/find free blocks as necessary, then return the pointer
    //gotta fill gap with /0s (but don't update the size)
    struct inode *node = malloc(sizeof(struct inode));
    if (readInode(curr_inode_num, node) == ERROR) {
        TracePrintf(0, "Error reading inode %i in seekhandler\n", curr_inode_num);
        replyError(message, senderPid);
        return;
    }

    if (new_pos >= (int) (BLOCKSIZE * (NUM_DIRECT + (BLOCKSIZE / sizeof(int))))) {
        //we've hit the max inode size
        TracePrintf(0, "Error setting new position %i in seekhandler. position is too large\n", new_pos);
        replyError(message, senderPid);
        return;

    }
    
    int new_block = new_pos / BLOCKSIZE;
    TracePrintf(0, "new_block is %i, new_pos is %i\n", new_block, new_pos);
    //now write a bunch of nulls, but don't change the size. actually only need to allocate a new free block
    if (addNewEmptyBlock(node, new_block) == ERROR) {
        TracePrintf(0, "Error adding empty block to inode %i in seek\n", curr_inode_num);
        replyError(message, senderPid);
        return;
    }
    if (writeInodeToDisk(curr_inode_num, node) == ERROR) {
        TracePrintf(0, "Error writing inode %i to disk in seek\n", curr_inode_num);
        replyError(message, senderPid);
        return;
    }
    //add this to the appropriate place in the 
    // int bytes_to_write = new_pos - node_size;
    // int last_block_idx = block_from_size(new_pos); //the last block we need to write
    // int cur_block_idx = block_from_size(node_size); //the first block we write from
    // int cur_block = getLastSector(node); //current block to start writing from in the inode
    // int cur_pos = node_size; //current writing position in the inode
    // void *null_block = malloc(bytes_to_write); //null block to copy from
    // memset(null_block, '\0', bytes_to_write); //set the bytes in the null block to null    
    // int pos_in_block = cur_pos % BLOCKSIZE; //the position in the block that we are writing
    //gotta send a message back with the new position in the open file in the data field
    TracePrintf(0, "Returning from seek with new position %i, which is > the current size %i.\n", new_pos, node_size);
    replyWithInodeNum(message, senderPid, new_pos);
}

/**
Handles Link: 
This request creates a (hard) link from the new file name newname to the existing file oldname . 
The files oldname and newname need not be in the same directory. The file oldname must not be a directory.
 It is an error if the file newname already exists. On success, this request returns 0; on any error, the
  value ERROR is returned.
*/

void linkHandler(struct msg *message, int senderPid) {
    struct link_strs *paths = malloc(sizeof(struct link_strs));
    if (CopyFrom(senderPid, paths, message->ptr, sizeof(struct link_strs)) == ERROR) {
        TracePrintf(0, "ERROR copying from in linkhandler\n");
        replyError(message, senderPid);
        return;
    }
    TracePrintf(0, "Received strings %s and %s, lengths %i, %i\n", paths->old, paths->new, paths->old_len, paths->new_len);
    //parsed it. now check the path of old, get the inode number
    char *oldpath = paths->old;
    char *newpath = paths->new;
    int curr_directory = message->data;
    int old_node_num = checkPathHelper(oldpath, curr_directory);
    if (old_node_num == ERROR) {
        TracePrintf(0, "Old Path %s is not valid\n", oldpath);
        replyError(message, senderPid);
        return;
    }
    if (getInodeType(old_node_num) == INODE_DIRECTORY) {
        //can't link a directory 
        TracePrintf(0, "Old Path %s is a directory. not link\n", oldpath);
        replyError(message, senderPid);
        return;
    }
    if (checkPathHelper(newpath, curr_directory) != ERROR) {
        //newpath already exists
        TracePrintf(0, "new Path %s already exists\n", newpath);
        replyError(message, senderPid);
        return;
    }
    //then get the parent of the new path
    int parent_inode_num = findParent(newpath, curr_directory);
    if (parent_inode_num == ERROR) {
        TracePrintf(0, "couldn't find parent of path %s\n", newpath);
        replyError(message, senderPid);
        return;
    }
    //then add a directory entry in the new path with the different name and old inode number
    char *new_name = findLastDirName(newpath);
    struct inode *parent_inode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parent_inode) == ERROR) {
        TracePrintf(0, "ERROR: Can't read a parent, number %i\n", parent_inode_num);
        replyError(message, senderPid);
        return;
    }
    
    if (writeDirectoryToInode(parent_inode, parent_inode_num, old_node_num, new_name) == ERROR) {
        TracePrintf(0, "error writing directory to inode %s\n", newpath);
        replyError(message, senderPid);
        return;
    }

    //update the nlinks for the old inode
    struct inode *node = malloc(sizeof(struct inode));
    if (readInode(old_node_num, node) == ERROR) {
        TracePrintf(0, "ERROR: Can't read linked node, number %i\n", old_node_num);
        replyError(message, senderPid);
        return;
    }
    node->nlink = node->nlink + 1;
    writeInodeToDisk(old_node_num, node);

    free(parent_inode);
    free(node);
    TracePrintf(0, "Exiting linkhandler successfully linking %s, %s\n", oldpath, newpath);
    replyWithInodeNum(message, senderPid, old_node_num);
    
}


/**
Handles unlink: 
This request removes the directory entry forpathname, and if this is the last link to a file, 
the file itself should be deleted by freeing its inode. 
The file pathname must not be a directory. On success, this request returns 0; on any error, the value ERROR is returned.
*/

void unlinkHandler(struct msg *message, int sender_pid) {
    //check path    
    TracePrintf(1, "in unlink Handler\n");

    int node_num = checkPath(message, sender_pid);
    int curr_directory = message->data;
    if (node_num == ERROR) {
        TracePrintf(0, "ERROR: can't find node %i in unlinkHandler\n", node_num);
        replyError(message, sender_pid);
        return;
    }
    //check the inode. if not directory, remove one link 
    if (getInodeType(node_num) == INODE_DIRECTORY) {
        TracePrintf(0, "ERROR: Can't unlink a directory, number %i\n", node_num);
        replyError(message, sender_pid);
        return;
    }
    //not a directory, so remove one link
    struct inode *node = malloc(sizeof(struct inode));
    if (readInode(node_num, node) == ERROR) {
        TracePrintf(0, "ERROR: Can't read a inode, number %i\n", node_num);
        replyError(message, sender_pid);
        return;
    }
    node->nlink = node->nlink - 1;
    

    //remove entry from parent (like removedirectory. think we call freeDirectory() here)
    struct inode *parent_node = malloc(sizeof(struct inode));
    char *path = getPath(message, sender_pid);
    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        TracePrintf(0, "ERROR: Can't find a parent, path %s\n",path);
        replyError(message, sender_pid);
        return;
    }
    if (readInode(parent_inode_num, parent_node) == ERROR) {
        TracePrintf(0, "ERROR: Can't read a parent, number %i\n", parent_inode_num);
        replyError(message, sender_pid);
        return;
    }

    if (freeDirectoryEntry(parent_node, parent_inode_num, node_num) == ERROR) {
        TracePrintf(0, "ERROR: Can't read free directory inode %i in directory number%i\n", node_num, parent_inode_num);
        replyError(message, sender_pid);
        return;
    }
    //if nlink = 0, freeInode()
    if (node->nlink == 0) {
        //free the inode
        TracePrintf(1, "node has 0  links now. Freeing node %i\n", node_num);
        freeInode(node_num);
    } else { //update the disk with the node
        //otherwise write to disk the new inode with one link removed
        //
        writeInodeToDisk(node_num, node);
    }
    free(node);
    free(parent_node);
    replyWithInodeNum(message, sender_pid, parent_inode_num);
    
}

/**
handles open
*/
void openHandler(struct msg *message, int sender_pid) {

    TracePrintf(0, "Received pathname %s\tcur dir %d\tpid %d\n", getPath(message, sender_pid), message->data, sender_pid);
    int inum = checkPath(message, sender_pid);
    if (inum == ERROR)
    {
        TracePrintf(0, "ERROR Opening\n");
        replyError(message, sender_pid);
        return;
    }
    else
    {
        replyWithInodeNum(message, sender_pid, inum);
    }
};

/**
    Creates a new directory entry with an empty inode at the end, name from the message. Copied from mkDirHandler, changed the node type created (it's now a INODE_REGULAR)
*/
void createHandler(struct msg *message, int senderPid) {
    //go down the nodes from the root until get to what you're creating, then add to the directory
        //gotta go down the inodes from the root until you get to the parent directory, then add a new struct directory to the inode and increment size
    //also update the parent inode size
    char *path = getPath(message, senderPid);
    TracePrintf(0, "Create: pathname %s\n", path);
    int curr_directory = message->data;
    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        //handle error here
        TracePrintf(0, "Create: Error finding parent_inode num in createHandler\n");
        replyError(message, senderPid);
        // Reply(message, senderPid); 
        return;
    }
    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Create: Error reading parent_inode in createHandler\n");
        replyError(message, senderPid);
        return;
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(1, "Create: parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) {
        TracePrintf(0, "Create: parent inode %i is not a directory\n", parent_inode_num);
        replyError(message, senderPid);
        return;
    }
    char *new_directory_name = findLastDirName(path);

    int matching_inode = findDirectoryEntry(parentInode, parent_inode_num, new_directory_name);
    int new_inode_num;
    if (matching_inode != ERROR) {
        TracePrintf(0, "Create: directory %s already exists in inode %i. truncating the file (this is not done yet).\n", new_directory_name, parent_inode_num);
        //maybe in a handler
        //check if the inode is a file type
        //if it is a file type, set the size to 0
        // replyError(message, senderPid);
        // return;
        if (getInodeType(matching_inode) == INODE_REGULAR) {
            new_inode_num = matching_inode;
            resetInodeSize(new_inode_num);
        } else {
            TracePrintf(0, "the inode %i is not a file type. Cannot truncate\n", matching_inode);
            replyError(message, senderPid);
            return;
        }

    } else {
        // TracePrintf(1, "We are here4\n");
        new_inode_num = getFreeInode();
        TracePrintf(1, "Create: Setting new inode %i with type %i and parent %i\n", new_inode_num, INODE_DIRECTORY, parent_inode_num);
        setNewInode(new_inode_num, INODE_REGULAR, 1, 0, 0, parent_inode_num); 
    }
    
    
    // TracePrintf(1, "We are here6\n");
    writeDirectoryToInode(parentInode, parent_inode_num, new_inode_num, new_directory_name);
    // entry->name = findLastDirName(path);
    //update parent
    writeInodeToDisk(parent_inode_num, parentInode);
    TracePrintf(1, "Create: parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);
    replyWithInodeNum(message, senderPid, new_inode_num);
    return;
}

/**
Resets the inode size (used for Create trunctation)
*/
int resetInodeSize(int inode_num) {
    TracePrintf(1, "Resetting inode %i's size field to 0\n", inode_num);
    struct inode *node = malloc(sizeof(struct inode));
    if (readInode(inode_num, node) == ERROR) {
        TracePrintf(0, "Error reading inode %i in reseatInodeSize\n", inode_num);
        return ERROR;
    }
    node->size = 0;
    return writeInodeToDisk(inode_num, node);
}

struct write_info
{
    char *content;
    int size;
    int inum;
};

/**
 * Return next block number.
*/
int nextSector(struct inode *node, unsigned int sector_i, int *content_left)
{
    if (sector_i > NUM_DIRECT + (BLOCKSIZE / sizeof(int)))
    { // check if memory limit exceeded
        TracePrintf(0, "Write: no more free blocks. adding new memory to file...\n");
        int new_block_num = getLastSector(node) + 1;
        int written = fillLastBlock(node, new_block_num);
        content_left -= written;
        return new_block_num;
    }
    else
    {
        return getSector(node, sector_i); 
    }
}

void writeHandler(struct msg *message, int senderPid)
{
    TracePrintf(0, "Write: extracting info from message\n");
    struct write_info temp;
    CopyFrom(senderPid, &temp, message->ptr, sizeof(struct write_info));
    int inum = temp.inum;
    int write_size = temp.size;
    char *content = malloc(write_size);
    CopyFrom(senderPid, content, temp.content, write_size);
    int cur_pos = message->data;
    TracePrintf(0, "inum: %d\tcontent: %s\tsize: %d\tcur pos: %d\n", inum, content, write_size, cur_pos);

    TracePrintf(0, "Write: reading in inode\n");
    void *buf = malloc(sizeof(struct inode));
    readInode(inum, buf);
    struct inode *node = (struct inode *)buf;

    if (node->type == INODE_DIRECTORY) {
        TracePrintf(0, "Write: ERROR attempting to write to a directory\n");
        replyError(message, senderPid);
        return;
    }
    
    unsigned int sector_i = (cur_pos / BLOCKSIZE) + 1;    // get index of current block
    void *buf1 = malloc(SECTORSIZE);
    void *start = buf1;
    TracePrintf(0, "Write: found sector index %d\n", sector_i);

    int sector_pos = cur_pos % BLOCKSIZE;    // get current position within block
    int space_left = BLOCKSIZE - sector_pos;
    TracePrintf(0, "Write: found position in sector %d\n", sector_pos);

    TracePrintf(0, "Write: start writing...\n");
    int content_left = write_size;
    // get next sector
    int sector = nextSector(node, sector_i, &content_left);
    // if (sector == 0) {
    //     TracePrintf(0, "Write: next sector is not allocated.\n");
    //     replyError(message, senderPid);
    //     return;
    // }
    
    while (content_left > space_left) {
        TracePrintf(0, "Write: Sector %d current content: %s\n", sector, start);
        TracePrintf(0, "Write: copying %d bytes of '%s' to new sector %d\n", space_left, content, sector);
        memcpy(buf1, content, space_left);

        if (WriteSector(sector, start) == ERROR)
        {
            replyError(message, senderPid);
            return;
        }
        content_left -= space_left;
        content += space_left;
        buf1 += space_left;

        // set up if new block/next round needed
        space_left = BLOCKSIZE;
        sector_i++;

        // get next sector
        sector = nextSector(node, sector_i, &content_left);
        // if (sector == 0) {
        //     TracePrintf(0, "Write: next sector is not allocated.\n");
        //     replyError(message, senderPid);
        //     return;
        // }
    }

    memcpy(buf1, content, content_left);
    TracePrintf(0, "Write: copying %d bytes of '%s' to final sector %d\n", content_left, content, sector);
    TracePrintf(0, "Write: Sector %d final content: %s\n", sector, start);
    if (WriteSector(sector, start) == ERROR)
    {
        replyError(message, senderPid);
        return;
    }

    message->data = write_size;
    Reply(message, senderPid);
    return;
}

/**
This request returns information about the file indicated by pathname in the information structure
at address statbuf . The information structure is defined within comp421/iolib.h as follows
The fields in the information structure are copied from the information in the file’s inode. 
On success, this request returns 0; on any error, the value ERROR is returned.
*/

void statHandler(struct msg *message, int senderPid) {
    int inum = checkPath(message, senderPid);
    if (inum == ERROR) { //error reaching directory or directory does not exist
        replyError(message, senderPid);
        return;
    } else { //gotta read the info from the node into a stat. put into contents (struct stat is 16 bytes)
        void *buf = malloc(sizeof(struct inode));
        readInode(inum, buf);
        struct inode *node = (struct inode *)buf;
        struct Stat *statbuf = (struct Stat *)message->content;
        statbuf->inum = inum;
        statbuf->type = node->type;
        statbuf->size = node->size;
        statbuf->nlink = node->nlink;
        //reply with that contents
        message->type = REPLYMSG;
        message->data = inum;
        Reply(message, senderPid);

    }
}


/**
Rmdirhandler: goes to the directory. checks if it is empty. if it is not empty (just ., .., and inode num 0 entries), error. if it is empty, update parent, free inode
Top stolen from the guts of checkpath(), as we need to edit the parent as well. 
*/
void rmDirHandler(struct msg *message, int senderPid) {

        //go down the nodes from the root until get to what you're creating, then check that entry
    //gotta go down the inodes from the root until you get to the parent directory
    char *path = getPath(message, senderPid);
    int curr_directory = message->data;
    if (strcmp(".", path) == 0 || strcmp("..", path) == 0) {
        TracePrintf(0, "ERROR: trying to remove . or ..\n");
        replyError(message, senderPid);
        return;
    } 
    //TODO: check if the path contains a /. or /..
    if (checkPath(message, senderPid) == curr_directory) {
        //can't remove yourself
        TracePrintf(0, "ERROR: can't remove yourself\n");
        replyError(message, senderPid);
        return;
    }
    //TODO check if contains /.. or /.
    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        //handle error here
        TracePrintf(0, "Error finding parent_inode num in checkpath\n");
        // Reply(message, senderPid); 
        replyError(message, senderPid);
        return;
    }
    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in checkpath\n");
        // replyError(message, senderPid);
        replyError(message, senderPid);
        return;
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) { 
        TracePrintf(0, "parent inode %i is not a directory\n", parent_inode_num);
        // replyError(message, senderPid);
        replyError(message, senderPid);
        return;
    }
    char *my_name = findLastDirName(path);

    int matching_inode_num = findDirectoryEntry(parentInode, parent_inode_num, my_name);
    if (matching_inode_num == ERROR) {
        replyError(message, senderPid);
        return;
    }
    //we are a valid directory entry. now need to free the inode and update the parent's entry
    //check if the directory entry is empty
    if (getInodeType(matching_inode_num) == INODE_DIRECTORY) { //check if it is a directory type
        if (checkEmpty(matching_inode_num)) { //check if it is empty
            if (freeInode(matching_inode_num) == ERROR) {
                replyError(message, senderPid);
            }
            //update the parent's entry
            else if (freeDirectoryEntry(parentInode, parent_inode_num, matching_inode_num) == ERROR) {
                replyError(message, senderPid);
            } else {
                replyWithInodeNum(message, senderPid, 0);

            }
        } else {
            TracePrintf(0, "Inode %i is not empty, so cannot delete it\n", matching_inode_num);
            // replyError(message, senderPid);
            replyError(message, senderPid);
            return;
        }
    } else {
        //not a directory entry
        TracePrintf(0, "Inode inode %i is not a directory, instead it is type %i so cannot delete the directory (or error reading inode)\n", matching_inode_num, getInodeType(matching_inode_num));
        // replyError(message, senderPid);
        replyError(message, senderPid);
        return;
    }
}
/**
Checks if the given directory is empty (either size 2*dir entry (then it's .. and .) or if not, all entries have inode 0)
*/
int checkEmpty(int inum) {
    if (getInodeType(inum) != INODE_DIRECTORY) { //not a directory
        TracePrintf(1, "checkempty directory is not actually a directory\n");
        return ERROR;
    }

    if (getInodeSize(inum) == 2 * sizeof(struct dir_entry)) {
        TracePrintf(1, "checkempty directory is empty\n");
        return true;
    }

    struct inode *curr_inode = malloc(sizeof(struct inode));
    if (readInode(inum, curr_inode) == ERROR) {
        TracePrintf(1, "Error reading inode in checkEmpty\n");
        return ERROR;
    }
    //otherwise, need to check if all the dir_entry inodes are 0
    //get the directory entry in this directory
    //iterate through the blocks of the inode and check if they are a directory. (from 0 to size)
    //If they are a directory, check if their string is the same as the current string. 
    int blocks_iterated = 0;
    void *block = malloc(BLOCKSIZE); //the current direct block 
    void *indirect_block = malloc(BLOCKSIZE); //the indirect block, if applicable
    struct dir_entry *current_entry; //the current entry we are checking 
    int dir_found = false; //have we found an entry that is nonzero?
    int block_num; //the current block number
    int size_traversed = 0; //number of bytes traversed
    int num_blocks_to_traverse = (curr_inode->size / (BLOCKSIZE * 1.0)) + 1; //number of blocks to traverse: size / blocksize + 1 (bc it rounds down)
    
    // int found_inode_num;
    while (!dir_found && blocks_iterated < num_blocks_to_traverse) {
        TracePrintf(1, "Searching block %i\n", blocks_iterated);
        // go through the block, iterate through the necessary number of directory entries
        int bytes_to_traverse_in_block = min(BLOCKSIZE, (curr_inode->size) - size_traversed); //this is the number of bytes to read from the current block
        int entries_to_traverse_in_block = bytes_to_traverse_in_block / sizeof(struct dir_entry); //this is the number of directory entries to check in the current block
        if (blocks_iterated < NUM_DIRECT) {
            //we are in the direct set of blocks
            block_num = curr_inode->direct[blocks_iterated]; // this is the block index to read from
        } else {
            //otherwise we are in indirect block. find the block index from the indirect block
            int read_status = readBlock(curr_inode->indirect, indirect_block);
            if (read_status == ERROR) {
                TracePrintf(0, "Error reading indirect block in path validation\n");
                return ERROR;
            }
            block_num = ((int *)indirect_block)[blocks_iterated - NUM_DIRECT];
        }
        int readStatus = readBlock(block_num, block); //read the block into memory
        if (readStatus == ERROR) {
                TracePrintf(0, "Error reading a subBlock in path validation\n");
                return ERROR;
            }
        current_entry = (struct dir_entry *)block; // start at the first entry
        int entries;
        for (entries = 0; entries < entries_to_traverse_in_block; entries++) {
            TracePrintf(1, "Current entry name is %s, corresponding to inode number %i\n", current_entry->name, current_entry->inum);
            //check all the entries in the block
            if (current_entry->inum == FREEFILE || (strcmp(".", current_entry->name) == 0) || (strcmp("..", current_entry->name) == 0)) {
                current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
                continue;
            } else {
                // dir_entry = true;
                TracePrintf(1, "there is a nonfree directory entry with name %s\n", current_entry->name);
                current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
                return false;
            }

        }
        blocks_iterated++;
        size_traversed += bytes_to_traverse_in_block;
    }
    return true;
    

}


/**
Gets the type of the node
*/
int getInodeType(int inum) {
    struct inode *curr_inode = malloc(sizeof(struct inode));
    if (readInode(inum, curr_inode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error freeing the inode in getInodeType\n");
        // replyError(message, senderPid);
        return ERROR;
    }
    int type = curr_inode->type;
    free(curr_inode);
    return type;
}

/**
Gets the type of the node
*/
int getInodeSize(int inum) {
    struct inode *curr_inode = malloc(sizeof(struct inode));
    if (readInode(inum, curr_inode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error freeing the inode in getInodeType\n");
        // replyError(message, senderPid);
        return ERROR;
    }
    int size = curr_inode->size;
    free(curr_inode);
    return size;
}

/**
frees the inode specified by inodenum. does not take care of parent directory handling
*/

int freeInode(int matching_inode_num) {

    struct inode *curr_inode = malloc(sizeof(struct inode));
    if (readInode(matching_inode_num, curr_inode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error freeing the inode in freeInode. we don't have a proper handler here\n");
        // replyError(message, senderPid);
        return ERROR;
    }
    
    prepFree(curr_inode);
    writeInodeToDisk(matching_inode_num, curr_inode);
    addFreeInode(matching_inode_num);
    free(curr_inode);
    return 0;
}
/**
Prepares the inode to be freed. Sets all fields to corresponding free values
*/

void prepFree(struct inode *curr_inode) {
    //set the fields of the free inode (in the buffer), then write it to disk with updates

    int num_blocks_to_remove = (curr_inode->size / BLOCKSIZE) + 1;
    curr_inode->type = INODE_FREE;
    curr_inode->nlink = 0;
    curr_inode->reuse = curr_inode->reuse + 1;
    curr_inode->size = 0;
    //TODO: check direct and indirect fields here and free their blocks
    int i;
    int *indirect_buf = malloc(SECTORSIZE);
    if (curr_inode->indirect != 0) {
        ReadSector(curr_inode->indirect, indirect_buf);
    }
    for (i = 0; i < num_blocks_to_remove; i++) {
        if (i < NUM_DIRECT) {
            if (curr_inode->direct[i] != 0) {
            //free the block
                addFreeBlock(i);
            }
        } else {
            //we are indirect block
            if (curr_inode->indirect != 0) {
            //indirect is not allocated; thus, we are in null territory
                if (indirect_buf[i] != 0) {
                    addFreeBlock(i);
                }
            }
            
            
        }
        
    }
    free(indirect_buf);
    return;
}

int addFreeBlock(int block_num) {
    freeBlocks[block_num] = BLOCK_FREE;
    numFreeBlocks += 1;
    return 0;

}
/**
Basically checks the path and returns the directory inode at the end of the path if it exists/is valid. iolib should change the directory to this inode
*/

void chDirHandler(struct msg *message, int senderPid) {
    TracePrintf(1, "Made it to the chdirhandler\n");
    int inum = checkPath(message, senderPid);
    if (inum == ERROR) { //error reaching directory or directory does not exist
        replyError(message, senderPid);
        return;
    } else { //gotta check if it is a directory type
        void *buf = malloc(sizeof(struct inode));
        readInode(inum, buf);
        struct inode *node = (struct inode *)buf;
        if (node->type == INODE_DIRECTORY) {
            replyWithInodeNum(message, senderPid, inum);
        } else {
            replyError(message, senderPid);
            return;
        }
    }
}

/**
will Reply with messages that contain useful contents for iolib maintenance
*/
void mkDirHandler(struct msg *message, int senderPid) {
    //gotta go down the inodes from the root until you get to the parent directory, then add a new struct directory to the inode and increment size
    //also update the parent inode size
    char *path = getPath(message, senderPid);
    int curr_directory = message->data;
    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        //handle error here
        TracePrintf(0, "Error finding parent_inode num in mkdirhandler\n");
        replyError(message, senderPid);
        return;
    }
    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in mkdirhandler\n");
        replyError(message, senderPid);
        return;
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(0, "parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) {
        TracePrintf(0, "parent inode %i is not a directory\n", parent_inode_num);
        replyError(message, senderPid);
        return;
    }
    char *new_directory_name = findLastDirName(path);

    int matching_inode = findDirectoryEntry(parentInode, parent_inode_num, new_directory_name);
    if (matching_inode != ERROR) {
        TracePrintf(0, "directory %s already exists in inode %i. \n", new_directory_name, parent_inode_num);
        replyError(message, senderPid);
        return;
    }
    

    //search the parent's directory for the current directory name; if it exists, then error


    //if the entry doesn't exist, then create the dir_entry
    //gotta figure out where the sector with space is
    //read that sector to a buffer
    //store the buffer's start
    //move the buffer pointer to the empty spot
    //set the fields appropriately
    // int parent_size = parentInode->size;
   
    // TracePrintf(1, "We are here4\n");
    int new_inode_num = getFreeInode();
    TracePrintf(1, "Setting new inode %i with type %i and parent %i\n", new_inode_num, INODE_DIRECTORY, parent_inode_num);
    setNewInode(new_inode_num, INODE_DIRECTORY, 1, 0, 0, parent_inode_num); //TODO: change this -1
    // TracePrintf(1, "We are here6\n");

    //THIS WAS COMMENTED OUT because it's handled by WriteDirtoInode function
    // int sector = getLastSector(parentInode);
    // void *buf = malloc(SECTORSIZE);
    // TracePrintf(1, "We are here2\n");
    // if (ReadSector(sector, buf) == ERROR) {
    //     //handle error here
    //     TracePrintf(0, "Error reading sector in mkdirhandler\n");
    //     Reply(message, senderPid);
    // }
    // TracePrintf(1, "We are here3\n");
    // void *start = buf;
    // //int inode num in block = ((i * INODESIZE) + Blocksize - 1) / blocksize;
    // //curb = curblock + inodesize * (i % (INODESPERBLOCK))
    // int bytes_into_sector = sectorBytes(parentInode);
    // // int dirs_into_sector = bytes_into_sector / sizeof(struct dir_entry);
    // buf = buf + bytes_into_sector; 
    // struct dir_entry *entry = (struct dir_entry *)buf;
    // entry->inum = new_inode_num;
    // strcpy(entry->name, findLastDirName(path));
    // parentInode->size = parentInode->size + sizeof(struct dir_entry);
    // TracePrintf(1, "Writing new directory to sector\n");
    // WriteSector(sector, start);
    // TracePrintf(1, "Done writing new directory to sector\n");
    writeDirectoryToInode(parentInode, parent_inode_num, new_inode_num, new_directory_name);
    // entry->name = findLastDirName(path);
    //update parent
    writeInodeToDisk(parent_inode_num, parentInode);
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i", parent_inode_num, parentInode->type, parentInode->size);
    replyWithInodeNum(message, senderPid, new_inode_num);

}




void init() {
    TracePrintf(0, "Initializing the free blocks and inodes\n");
    //TODO: make cache. for hashtable for the cache, steal from 321
    TracePrintf(1, "Reading the fs Header\n");
    void *buf = malloc(SECTORSIZE); //the buffer to read into
    int status = ReadSector(1, buf);//read the fs header first

    if (status == ERROR) {
        TracePrintf(0, "Error reading the sector 1 for fs. \n");
    }
    struct fs_header *header = (struct fs_header *)buf;
    //set up the file system? inodes and stuff
    //set up a hash, queue, dirty inodes linked list
    //queue of least recently used item for the cache
    numBlocks = header->num_blocks;
    numFreeBlocks = numBlocks; //number of free blocks
    freeBlocks = malloc(sizeof(int) * (numFreeBlocks + 1)); // allocate for the free blocks index list. +1 for the 0th block
    

    int num_inodes = header->num_inodes;
    TracePrintf(0, "there are %i inodes total in the fs\n", num_inodes);
    int num_used_blocks = ((num_inodes * INODESIZE) + BLOCKSIZE) / BLOCKSIZE; //number of blocks used by inodes (rounded up)
    int i;
    int num_inodes_to_read;
    int num_inodes_remaining = num_inodes;
    int k;
    TracePrintf(0, "Setting up the freeBlocks as default free\n");
    for (k = 0; k < numBlocks; k++) {//set all the blocks to free
        freeBlocks[k] = BLOCK_FREE;

    }
    //set freeBlocks[0] because the first block is taken by the boot
    freeBlocks[0] = BLOCK_USED;
    
    for (i = 0; i < num_used_blocks; i++) { // my understanding is that the used blocks should be contiguous starting from 0
        
        //iterate through each block and parse the inodes
        int block_index = i + 1; //skip the boot block
        freeBlocks[block_index] = BLOCK_USED; //mark the block as used
        numFreeBlocks = numFreeBlocks - 1;

        // int status = ReadSector(block_index, buf);//read the fs header first
        // if (status == ERROR) {
        //     TracePrintf(0, "Error reading the sector 1 for fs. \n");
        // }
        //we may be at the last block (inodes only cover part of the block. if so, set the num_blocks to read to the remaining)
        num_inodes_to_read = (num_inodes_remaining < inodes_per_block) ? num_inodes_remaining : inodes_per_block; 
        TracePrintf(1, "There are %i nodes to read\n", num_inodes_to_read);
        // int j;
        // if (block_index == 1) { // handle the fs_header 
        //     // int block_status = readBlock(block_index, num_inodes_to_read, buf, 1); //start reading at the 1st inode index bc it's the first block 
        //     // if (block_status == BLOCK_FREE) {
        //     //     //the block is free
        //     //     freeBlocks[block_index] = BLOCK_FREE;
        //     // } else {
        //     //     // update the freeblock list, number of free blocks
                        // freeBlocks[block_index] = BLOCK_USED;
                        // numFreeBlocks = numFreeBlocks - 1;
        //     // }
        //     continue;
        // } else 
        if (block_index >= 1) { //read the inodes and appropriately mark their blocks as used
            int start_index;
            int block_status;
            if (block_index == 1) {
                block_status = readInodeBlock(block_index, num_inodes_to_read, buf, 1);
                
                start_index = 1;
            } else {
                block_status = readInodeBlock(block_index, num_inodes_to_read, buf, 0);
                start_index = 0;
            }
            if (block_status == ERROR) {
                TracePrintf(0, "Error reading inode block. no logic here, so continuing\n");
            }
            struct inode *inodes = (struct inode *)buf; // cast buf as an array of inodes
            int in_idx;
            for (in_idx = start_index; in_idx < num_inodes_to_read + start_index; in_idx++) {
                struct inode *node = &inodes[in_idx];
                int inum = ((block_index - 1) * INODESPERBLOCK) + in_idx; //calculate the inode number
                if (node->type == INODE_FREE) {
                    //the inode is free
                    TracePrintf(1, "Found a free inode!\n");
                    addFreeInode(inum);
                } else {
                    //node is not free, so iterate through its directs and mark their blocks as used
                    int num_blocks = (int) (node->size / BLOCKSIZE) + 1;
                    int *direct = node->direct;
                    void *indirect_buf = malloc(SECTORSIZE); //the buffer to read into
                    if (num_blocks > NUM_DIRECT) { //overflows into the indirect buf
                        int new_status = ReadSector((int)node->indirect, indirect_buf);//read the indirect buffer
                        if (new_status == ERROR) {
                            TracePrintf(0, "Error reading the indirect buffer for the %ith inode in block %i, size %i.\n", in_idx, block_index, num_blocks);
                        }
                    }
                    int j;
                    for (j = 0; j < num_blocks; j++) {
                        //iterate through the block nums and mark them as used
                        if (j < NUM_DIRECT) {
                            // it is in the direct array
                            markUsed(direct[j]);
                        } else {
                            //we are above the direct num, so go into the indirectory
                            //TODO: test this
                            markUsed(((int *)indirect_buf)[j - NUM_DIRECT]);
                        }
                    } 
                    free(indirect_buf);
                }
            }
        }
        num_inodes_remaining = num_inodes_remaining - num_inodes_to_read;

    }
    free(buf);

}

/**
Gets the path
*/

char *getPath(struct msg *message, int senderPid) {
    struct path_str *paths = malloc(sizeof(struct path_str));
    if (CopyFrom(senderPid, paths, message->ptr, sizeof(struct link_strs)) == ERROR) {
        TracePrintf(0, "ERROR copying from in getPath\n");
    }
    char *new_path = paths->_path;
    TracePrintf(1, "received path %s\n", new_path);
    return paths->_path;
}

/*8
char clean[DIRNAMELEN];
    memset(clean, 0, DIRNAMELEN); //set the clean to 0 for later comparison
    int i;
    int null_exists = false;
    for (i=0; i < DIRNAMELEN; i++) { //iterate through each of the characters in the name, up till 30
        // clean[i] = name[i]; //copy the char over to the clean string
        if (name[i] == '\0') {   
            null_exists = true;
            break;
        }
    }
    if (null_exists == false) {
        // There is no null char in the first 30 of the char name. Check 31th at idx 30. If nott null, invalid path.
        if (name[DIRNAMELEN] != '\0') {
            return ERROR;
        }
    }
    strcpy(clean, name);
*/
/**
The innards of checkPath, because we needed to customize for linking (different message contents)
*/

int checkPathHelper(char *path, int curr_directory) {
    // Chloe's fanangling:
    if (strcmp(".", path) == 0) {   // stay in current directory
        TracePrintf(0, "checkPath: Stay in current directory\n");
        return curr_directory;
    }

    if (strcmp("..", path) == 0) {
        
        TracePrintf(0, "checkPath: Go to parent file\n");
        struct inode *curr_inode = malloc(sizeof(struct inode));
        int read_s = readInode(curr_directory, curr_inode);
        if (read_s == ERROR)
        {
            TracePrintf(0, "checkPath: ERROR reading current node\n");
            free(curr_inode);
            return ERROR;
        }

        int parent = findDirectoryEntry(curr_inode, curr_directory, "..");
        if (parent == ERROR) {
            TracePrintf(0, "error finding .. of the node\n");
            free(curr_inode);
            return ERROR;
        } else {
            free(curr_inode);
            return parent;
        }
        // int block_num = curr_inode->direct[0];  // parent stored in 2nd dir entry
        // void *block = malloc(BLOCKSIZE);
        // int readStatus = readBlock(block_num, block); // read the block into memory
        // if (readStatus == ERROR)
        // {
        //     TracePrintf(0, "Error reading a subBlock in path validation\n");
        //     return ERROR;
        // }
        // struct dir_entry *entry = (struct dir_entry *)block;

    }

    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        //handle error here
        TracePrintf(0, "Error finding parent_inode num in checkpath\n");
        // Reply(message, senderPid); 
        return ERROR;
    }

    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in checkpath\n");
        // replyError(message, senderPid);
        return ERROR;
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) { 
        TracePrintf(0, "parent inode %i is not a directory\n", parent_inode_num);
        // replyError(message, senderPid);
        return ERROR;
    }
    char *new_directory_name = findLastDirName(path);

    int matching_inode = findDirectoryEntry(parentInode, parent_inode_num, new_directory_name);
    //will be error or an inode number. return whatever we get. 

    return matching_inode;
}

/**
Checks the path contained in the message's contents. returns the inode number if the path is valid, else ERROR if anything else. 
copied from top of createhandler / mkdirhandler (they should be the same)
*/
int checkPath(struct msg *message, int senderPid) { //currently a wrapper around checkPath
    //go down the nodes from the root until get to what you're creating, then check that entry
    //gotta go down the inodes from the root until you get to the parent directory
    char *path = getPath(message, senderPid);
    int curr_directory = message->data;
    return checkPathHelper(path, curr_directory);
}
/**
returns the number of bytes into the last sector
*/
int sectorBytes(struct inode *node) {
    return (node->size % SECTORSIZE);
}
/**
reads the block at index block_index, which contains a bunch of inodes. This is for INIT only
Returns BLOCK_FREE if the block is free, else BLOCK_USED
*/

int readInodeBlock(int block_index, int num_inodes_to_read, void *buf, int start_index) {
    TracePrintf(1, "Reading inode block %i\n", block_index);
    // int in_idx;
    
    //TODO check the cache here;

    if (num_inodes_to_read + start_index == 0) {
        return BLOCK_FREE;
    }
    int status = ReadSector(block_index, buf);
    if (status == ERROR) {
        TracePrintf(0, "Error reading sector %i\n", block_index);
    }
    
    return BLOCK_USED;
}

/**
reads the block at index block_index,
Returns BLOCK_FREE if the block is free, else BLOCK_USED
*/

int readBlock(int block_index, void *buf) {
    TracePrintf(1, "Reading block %i\n", block_index);
    // int in_idx;
    
    //TODO check the cache here;

    int status = ReadSector(block_index, buf);
    if (status == ERROR) {
        TracePrintf(0, "Error reading sector %i\n", block_index);
    }
    return 0;

}   

/**
writes the block at index block_index
*/

int writeBlock(int block_index, void *buf) {
    TracePrintf(1, "writing block %i\n", block_index);
    // int in_idx;
    
    int status = WriteSector(block_index, buf);
    if (status == ERROR) {
        TracePrintf(0, "Error writing sector %i\n", block_index);
    }
    return 0;

} 


/**
finds the last directory name in the string (so if it's "/a/a/b/c", will return "c". or if "asdfasdf", will return "asdfasdf")
*/
char *findLastDirName(char *str) {
    int len = strlen(str);
    
    // If the string is empty or has no '/'
    if (len == 0 || strchr(str, '/') == NULL) {
        return str;
    }
    
    // Iterate from the end of the string
    int i;
    for (i = len - 1; i >= 0; i--) {
        if (str[i] == '/') {
            // Return the pointer to the character immediately after the '/'
            return &str[i + 1];
        }
    }
    // Should never reach here
    return str;
}
/**
Reads the inode. writes the inode to the buffer
*/
int readInode(int inum, void *buf) {
    TracePrintf(1, "Reading inode %i\n", inum);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    int block_num = inum / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        return ERROR;
    }
    // struct inode *node = ((struct inode *)buffer)[(inum - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inum) % inodes_per_block) * sizeof(struct inode));
    memcpy(buf, node, sizeof(struct inode));
    free(buffer);
    TracePrintf(1, "Finished reading inode %i\n", inum);
    return 0;
}

/**
Reads the ith direct or indirect block of the inode, returns the block number. returns -1 if error
*/

// int getBlockOfInode(int inum, int block_index, void *buf) {

// }

/*
(NOW DEFUNCT Function: incorporated into the SETNEWINODE and SETINODE functions)
Reason: need the sector buffer pointer in order to WriteSector() to disk, 
otherwise the inode will only be changed in the copy here but not on the file system. 
calculates and returns the inode's (hypothetical) pointer
*/

struct inode *findInodePtr(int inum) {
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    int block_num = inum / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inum, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
        //TODO: what to return here?
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inum - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inum) % inodes_per_block) * sizeof(struct inode));
    return node;
}
/**
Sets fields of new inode. size input does not include size of initial directories
*/

int setNewInode(int inum, short type, short nlink, int reuse, int size, int parent_inode_num) {
    TracePrintf(1, "Setting inode %i with type %i, parent %i\n", inum, type, parent_inode_num);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    void *start = buffer;
    int block_num = inum / (inodes_per_block) + 1;
    int status = readBlock(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inum, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
        //TODO: what to return here?
        return ERROR;
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inum - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inum) % inodes_per_block) * sizeof(struct inode));
    node->type = type;
    node->nlink = nlink;
    node->reuse = reuse;
    node->size = size;
    node->direct[0] = findFreeBlock();
    if (node->direct[0] ==  ERROR) {
        //error finding free block
        return ERROR;
    }

    writeDirectoryToInode(node, inum, inum, "."); //TODO: write this method?
    writeDirectoryToInode(node, inum, parent_inode_num, "..");
    TracePrintf(1, "size of node is %i\n", node->size);
    WriteSector(block_num, start);
    //TODO: set the inode first . and .. to itself and parentinodenum, respectively. 
    return 0;

}


/**
Sets fields of the inode in disk. size input does  include size of initial directories
*/

int writeInodeToDisk(int inum, struct inode *inode_to_cpy) {
    TracePrintf(1, "Setting inode %i with type %i\n", inum, inode_to_cpy->type);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    void *start = buffer;
    int block_num = inum / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inum, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inum - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inum) % inodes_per_block) * sizeof(struct inode));
    node->type = inode_to_cpy->type;
    node->nlink = inode_to_cpy->nlink;
    node->reuse = inode_to_cpy->reuse;
    node->size = inode_to_cpy->size;
    // node->direct = inode_to_cpy->direct;
    int i;
    for (i = 0; i < numBlocksUsedBy(node); ++i) {
        node->direct[i] = inode_to_cpy->direct[i];
    }
    node->indirect = inode_to_cpy->indirect;

    WriteSector(block_num, start);
    //TODO: set the inode first . and .. to itself and parentinodenum, respectively. 
    return 0;

}

/*
returns the number of blocks used by node
*/

int numBlocksUsedBy(struct inode *node) {
    return (node->size / BLOCKSIZE) + 1;
}

/**
Searches for the given name in a set of directories in the given inode. 
Returns ERROR if not found, otherwise returns the inodenum corresponding to the directory entry. 
Copied from the findParent() function as of 4/16/24 at 15:06pm (so if it's wrong, change both the findParent() and this, or call this in FindParent())
as of 4/16/24, also copied this for the freeDirectoryEntry (added a freeing line)
as of 4/17/24, copied this to checkEmpty
*/

int findDirectoryEntry(struct inode *curr_inode, int curr_inode_num, char *name) {
    char *current_string = name; //the current directory to go to
    TracePrintf(1, "Looking for directory %s in findDirectoryEntry in inode %i\n", current_string, curr_inode_num);
    if (curr_inode->type != INODE_DIRECTORY) { //the parent is not a directory. should never hit this, in theory
        TracePrintf(0, "Current parent (node %i) is of type %i\n", curr_inode_num, curr_inode->type);
        TracePrintf(0, "Current parent (node %i) is not a directory, should never hit this though\n", curr_inode_num);
        return ERROR;
    }
    //get the directory entry in this directory
    //iterate through the blocks of the inode and check if they are a directory. (from 0 to size)
    //If they are a directory, check if their string is the same as the current string. 
    int blocks_iterated = 0;
    void *block = malloc(BLOCKSIZE); //the current direct block 
    void *indirect_block = malloc(BLOCKSIZE); //the indirect block, if applicable
    struct dir_entry *current_entry; //the current entry we are checking 
    int dir_found = false; //have we found the directory?
    int block_num; //the current block number
    int size_traversed = 0; //number of bytes traversed
    int num_blocks_to_traverse = (curr_inode->size / (BLOCKSIZE * 1.0)) + 1; //number of blocks to traverse: size / blocksize + 1 (bc it rounds down)
    
    int found_inode_num;
    while (!dir_found && blocks_iterated < num_blocks_to_traverse) {
        TracePrintf(1, "Searching block %i\n", blocks_iterated);
        // go through the block, iterate through the necessary number of directory entries
        int bytes_to_traverse_in_block = min(BLOCKSIZE, (curr_inode->size) - size_traversed); //this is the number of bytes to read from the current block
        int entries_to_traverse_in_block = bytes_to_traverse_in_block / sizeof(struct dir_entry); //this is the number of directory entries to check in the current block
        if (blocks_iterated < NUM_DIRECT) {
            //we are in the direct set of blocks
            block_num = curr_inode->direct[blocks_iterated]; // this is the block index to read from
        } else {
            //otherwise we are in indirect block. find the block index from the indirect block
            int read_status = readBlock(curr_inode->indirect, indirect_block);
            if (read_status == ERROR) {
                TracePrintf(0, "Error reading indirect block in path validation\n");
                return ERROR;
            }
            block_num = ((int *)indirect_block)[blocks_iterated - NUM_DIRECT];
        }
        int readStatus = readBlock(block_num, block); //read the block into memory
        if (readStatus == ERROR) {
                TracePrintf(0, "Error reading a subBlock in path validation\n");
                return ERROR;
            }
        current_entry = (struct dir_entry *)block; // start at the first entry
        int entries;
        for (entries = 0; entries < entries_to_traverse_in_block; entries++) {
            TracePrintf(1, "Current entry name is %s, corresponding to inode number %i\n", current_entry->name, current_entry->inum);
            //check all the entries in the block
            if (current_entry->inum == FREEFILE) {
                current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
                continue;
            }
            if (strcmp(current_string, current_entry->name) == 0) {
                //found the next inode
                dir_found = true;
                //check if it is a directory type occurs at the top of the traverse tokens loop
                found_inode_num = current_entry->inum;
                TracePrintf(1, "Found directory %s, with inode %i!\n", current_string, found_inode_num);
                // curr_inode_num = current_entry->inum;
                // readInode(curr_inode_num, curr_inode); //read the new inode into the current inode field
                break;
            }
            TracePrintf(1, "Entry name %s does not match %s\n", current_entry->name, current_string);
            current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));

        }
        blocks_iterated++;
        size_traversed += bytes_to_traverse_in_block;
    }
    (void)size_traversed;
    if (!dir_found) { //didn't find the current subdirectory child in the current directory spot
        TracePrintf(1, "Couldn't directory %s!\n", current_string);
        return ERROR;
    } //otherwise, continue iterating through the path
    return found_inode_num;
}

/**
Frees the inode dir_entry (set the inum field to 0). almost identical to findDirectoryEntry
*/

int freeDirectoryEntry(struct inode *curr_inode, int curr_inode_num, int inode_to_remove) {
    // char *current_string = name; //the current directory to go to
    TracePrintf(1, "Looking for node %i in freeDirectoryEntry in inode %i\n", inode_to_remove, curr_inode_num);
    if (curr_inode->type != INODE_DIRECTORY) { //the parent is not a directory. should never hit this, in theory
        TracePrintf(0, "Current parent (node %i) is of type %i\n", curr_inode_num, curr_inode->type);
        TracePrintf(0, "Current parent (node %i) is not a directory, should never hit this though\n", curr_inode_num);
        return ERROR;
    }
    //get the directory entry in this directory
    //iterate through the blocks of the inode and check if they are a directory. (from 0 to size)
    //If they are a directory, check if their string is the same as the current string. 
    int blocks_iterated = 0;
    void *block = malloc(BLOCKSIZE); //the current direct block 
    void *indirect_block = malloc(BLOCKSIZE); //the indirect block, if applicable
    struct dir_entry *current_entry; //the current entry we are checking 
    int dir_found = false; //have we found the directory?
    int block_num; //the current block number
    int size_traversed = 0; //number of bytes traversed
    int num_blocks_to_traverse = (curr_inode->size / (BLOCKSIZE * 1.0)) + 1; //number of blocks to traverse: size / blocksize + 1 (bc it rounds down)
    
    int found_inode_num;
    while (!dir_found && blocks_iterated < num_blocks_to_traverse) {
        TracePrintf(1, "Searching block %i\n", blocks_iterated);
        // go through the block, iterate through the necessary number of directory entries
        int bytes_to_traverse_in_block = min(BLOCKSIZE, (curr_inode->size) - size_traversed); //this is the number of bytes to read from the current block
        int entries_to_traverse_in_block = bytes_to_traverse_in_block / sizeof(struct dir_entry); //this is the number of directory entries to check in the current block
        if (blocks_iterated < NUM_DIRECT) {
            //we are in the direct set of blocks
            block_num = curr_inode->direct[blocks_iterated]; // this is the block index to read from
        } else {
            //otherwise we are in indirect block. find the block index from the indirect block
            int read_status = readBlock(curr_inode->indirect, indirect_block);
            if (read_status == ERROR) {
                TracePrintf(0, "Error reading indirect block in free directory\n");
                return ERROR;
            }
            block_num = ((int *)indirect_block)[blocks_iterated - NUM_DIRECT];
        }
        int readStatus = readBlock(block_num, block); //read the block into memory
        if (readStatus == ERROR) {
                TracePrintf(0, "Error reading a subBlock in free directory\n");
                return ERROR;
            }
        current_entry = (struct dir_entry *)block; // start at the first entry
        int entries;
        for (entries = 0; entries < entries_to_traverse_in_block; entries++) {
            TracePrintf(1, "Current entry name is %s, corresponding to inode number %i\n", current_entry->name, current_entry->inum);
            //check all the entries in the block
            if (current_entry->inum == FREEFILE) {
                current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
                continue;
            }
            if (current_entry->inum == inode_to_remove) { //this is what is changed
                //found the next inode
                dir_found = true;
                //check if it is a directory type occurs at the top of the traverse tokens loop
                found_inode_num = current_entry->inum;
                TracePrintf(1, "Found node %i, with inode %i! settiing the dir_entry inode num to 0.\n", found_inode_num, found_inode_num);
                current_entry->inum = 0;
                // curr_inode_num = current_entry->inum;
                // readInode(curr_inode_num, curr_inode); //read the new inode into the current inode field
                break;
            }
            TracePrintf(1, "Entry inode %i does not match %i\n", current_entry->inum, inode_to_remove);
            // TracePrintf(1, "WE are still printing\n");
            current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
        
        }
        if (dir_found) { //directory found. write the block
            if (writeBlock(block_num, block) == ERROR) {
                TracePrintf(0, "Error writing a subBlock in free directory\n");
                return ERROR;
            }
        }
        blocks_iterated++;
        size_traversed += bytes_to_traverse_in_block;
    }
    (void)size_traversed;
    if (!dir_found) { //didn't find the current subdirectory child in the current directory spot
        TracePrintf(1, "Couldn't find node %s!\n", inode_to_remove);
        return ERROR;
    } //otherwise, continue iterating through the path
    TracePrintf(1, "returning from freeDirectoryEntry\n");
    return found_inode_num;
}

/**
writes a directory to the inode
*/
int writeDirectoryToInode(struct inode *node, int curr_inode, int inum, char *name) {

    int size = node->size;
    int bytes_into_sector = sectorBytes(node);
    int dirs_into_sector = bytes_into_sector / sizeof(struct dir_entry);
    int sector = getLastSector(node);
    
    TracePrintf(2, "%i bytes into the directory sector for write directory to inode\n", bytes_into_sector);
    //: need to check if there is space in the current sector before writing to it, else move to next sector
    // if (bytes_into_sector + sizeof(struct dir_entry) > BLOCKSIZE) { 
    //     //block is too small for adding a dir_entry; find the next one
    //     int bytes_filled = fillLastBlock(node, curr_inode); //fills the last block with nulls, adds a new block, returns the number of bytes written
    //     if (bytes_filled == ERROR) {
    //         TracePrintf(0, "Error filling last block for writeDirectoryToInode\n");
    //         return ERROR;
    //     }
    //     //update the size
    //     size = size + bytes_filled;
    //     //update the sector number
    //     sector = getSector(node, sector + 1);
    //     //update the bytes into sector
    //     bytes_into_sector = 0;
    // }
    
    TracePrintf(2, "%i sectors into the inode for write directory to inode\n");
    void *buf = malloc(SECTORSIZE);
    if (ReadSector(sector, buf) == ERROR) {
        //handle error here
        return ERROR;
    }
    void *start = buf;//need to keep track of the start so I can writesector()
    //int inode num in block = ((i * INODESIZE) + Blocksize - 1) / blocksize;
    //curb = curblock + inodesize * (i % (INODESPERBLOCK))
    buf = buf + bytes_into_sector; 
    
    struct dir_entry *entry = (struct dir_entry *)buf;
    entry->inum = inum;
    
    // entry->name = name;
    strcpy(entry->name, name);
    node->size = size + sizeof(struct dir_entry); //mindful of the fill potentially (this is why it's not node->size)
    TracePrintf(1, "size of struct dir_entry: %i\n", sizeof(struct dir_entry));
    TracePrintf(1, "Writing new directory %s with inode num %i to inode %i's sector\n", name, inum, curr_inode);
    //check if the new size is at the end of the block. if so, add a new block
    if (node->size % BLOCKSIZE == 0) {
        addNewEmptyBlock(node, dirs_into_sector + 1);
    }
    if (WriteSector(sector, start) == ERROR) {
        return ERROR;
    }
    if (writeInodeToDisk(curr_inode, node) == ERROR) {
        return ERROR;
    }
    return 0;
}


/**
THIS IS DEFUNCT. NOT TESTED fills the last block with nulls, adds a new block, returns the number of bytes written
*/

int fillLastBlock(struct inode *node, int node_num) {
    int size = node->size;
    int cur_block = getLastSector(node); //current block to start writing from in the inode
    int cur_block_pos = sectorBytes(node); //current writing position in the inode
    int bytes_to_write = BLOCKSIZE - cur_block_pos;
    // void *null_block = malloc(bytes_to_write); //null block to copy from
    void *block = malloc(BLOCKSIZE);
    if (readBlock(cur_block, block) == ERROR) {//read the block to the buffer
        TracePrintf(0, "ERROR reading the block %i in filllastblock\n", cur_block);
        return ERROR;
    }

    void *start = block; //store the start
    block = block + cur_block_pos;
    memset(block, '\0', bytes_to_write); //set the bytes in the null block to null
    if (writeBlock(cur_block, start) == ERROR) {
        TracePrintf(0, "ERROR writing the block %i in filllastblock\n", cur_block);
        //write the block to memory
        return ERROR;
    }
    int curr_inode_block = size / BLOCKSIZE; //current block # in inode
    //finally, add a new block and return the number of bytes written
    if (addNewEmptyBlock(node, curr_inode_block + 1) == ERROR) { //add a new empty block at the end
        TracePrintf(0, "ERROR adding new block to in filllastblock\n", node_num);
        return ERROR;
    }
    return bytes_to_write;
}

/**
Adds an empty block in the direct/indirect list, 
*/
int addNewEmptyBlock(struct inode *node, int block_index) {
    TracePrintf(0, "adding new empty block at index %i\n", block_index);
    if (block_index < NUM_DIRECT) {
        int new_block = findFreeBlock();
        if (new_block == ERROR) {
            return ERROR;
        }
        node->direct[block_index] = new_block;
        return 0;
    } else {
        if (node->indirect == 0) {
            //no indirect block yet. allocate a new one
            node->indirect = findFreeBlock();
            if (node->indirect == ERROR) {
                return ERROR; 
            }
        }
        int *indirect_buf = malloc(SECTORSIZE);
        if (readBlock(node->indirect, indirect_buf) == ERROR) {
            return ERROR;
        }
        int new_block = findFreeBlock();
        if (new_block == ERROR) {
            return ERROR;
        }
        indirect_buf[block_index] = new_block;
        writeBlock(node->indirect, indirect_buf);
        free(indirect_buf);
        return 0;
    }

}

/**
parses the path, returns the parent inode number if the path is valid or else returns an error 
*/
int findParent(char *name, int curr_directory) {
    TracePrintf(1, "in FindParent\n");
    TracePrintf(0, "finding parent inode in string %s\n", name);

    //first, parse the name to make sure it is a valid path structure (30 characters or less, with null)
    char clean[DIRNAMELEN];
    TracePrintf(1, "in FindParent\n");
    memset(clean, 0, DIRNAMELEN); //set the clean to 0 for later comparison
    TracePrintf(1, "in FindParent\n");
    int i;
    int null_exists = false;
    for (i=0; i < DIRNAMELEN; i++) { //iterate through each of the characters in the name, up till 30
        // clean[i] = name[i]; //copy the char over to the clean string
        if (name[i] == '\0') {   
            null_exists = true;
            break;
        }
    }
    TracePrintf(1, "in FindParent1\n");
    if (null_exists == false) {
        // There is no null char in the first 30 of the char name. Check 31th at idx 30. If nott null, invalid path.
        if (name[DIRNAMELEN] != '\0') {
            return ERROR;
        }
    }
    TracePrintf(1, "in FindParent2\n");
    strcpy(clean, name);
    TracePrintf(1, "in FindParent3\n");
    //then, parse into separations by slashes, track number of nodes
    int root = clean[0] == '/'; //is the path from the root
    TracePrintf(1, "in FindParent4\n");
    struct inode *curr_inode = malloc(sizeof(struct inode)); //stores the current inode
    TracePrintf(1, "in FindParent5\n");
    int curr_inode_num;
    TracePrintf(1, "in FindParent6\n");
    if (root) { //if it's the root, set the current inode
        TracePrintf(1, "we are pathfinding from the root\n");
        int read_s = readInode(ROOTINODE, curr_inode);
        if (read_s == ERROR) {
            TracePrintf(0, "error reading root inode in path validation\n");
            return ERROR;
        }
        curr_inode_num = ROOTINODE;
    } else {
        // TracePrintf(0, "TODO in pathfinder: replace lines here with current directory\n");
        TracePrintf(0, "we are pathfinding from the current directory\n");
        // return -1;
        int read_s = readInode(curr_directory, curr_inode);
        curr_inode_num = curr_directory;
        if (read_s == ERROR) {
            TracePrintf(0, "error reading relative inode in path validation\n");
            return ERROR;
        }
    }
    
    char *tokens[DIRNAMELEN / 2]; //can't have more than DIRNAMELEN / 2 tokens a/a/a/a/a/a/a/
    int num_tokens = 0;
    
    // Split clean by "/" characters
    char *token = strtok(clean, "/");

    while (token != NULL) {
        TracePrintf(1, "Token: %s\n", token);
        tokens[num_tokens++] = token;
        token = strtok(NULL, "/");
    }

    //iterate through the strings, traverse down the inodes until you are at the last one
    if (num_tokens == 1) {
        //we are already in the parent directory. return the root node or relative root
        TracePrintf(1, "Token: parent is root or relative root!\n");
        return curr_inode_num;
    }
    int traversed = 0; //number of traversed directories
    //traverse to the second to last one, num_tokens - 1, which should be the parent
    for (traversed = 0; traversed < num_tokens - 1; traversed++) {
        char *current_string = tokens[traversed]; //the current directory to go to
        TracePrintf(1, "Looking for directory %s\n", current_string);
        if (curr_inode->type != INODE_DIRECTORY) { //the parent is not a directory. should never hit this, in theory
            TracePrintf(0, "Current parent (node %i) is of type %i\n", curr_inode_num, curr_inode->type);
            TracePrintf(0, "Current parent (node %i) is not a directory, should never hit this though\n", curr_inode_num);
            return ERROR;
        }
        //get the directory entry in this directory
        //iterate through the blocks of the inode and check if they are a directory. (from 0 to size)
        //If they are a directory, check if their string is the same as the current string. 
        int blocks_iterated = 0;
        void *block = malloc(BLOCKSIZE);
        void *indirect_block = malloc(BLOCKSIZE);
        struct dir_entry *current_entry;
        int dir_found = false;
        int block_num;
        int size_traversed = 0; //number of bytes traversed
        int num_blocks_to_traverse = (curr_inode->size / (BLOCKSIZE * 1.0)) + 1; //number of blocks to traverse
        while (!dir_found && blocks_iterated < num_blocks_to_traverse) {
            // go through the block, iterate through the necessary number of directory entries
            int bytes_to_traverse_in_block = min(BLOCKSIZE, (curr_inode->size) - size_traversed); //this is the number of bytes to read from the current block
            int entries_to_traverse_in_block = bytes_to_traverse_in_block / sizeof(struct dir_entry); //this is the number of directory entries to check in the current block
            if (blocks_iterated < NUM_DIRECT) {
                //we are in the direct set of blocks
                block_num = curr_inode->direct[blocks_iterated]; // this is the block index to read from
                
            } else {
                //otherwise we are in indirect block. find the block index from the indirect block
                int read_status = readBlock(curr_inode->indirect, indirect_block);
                if (read_status == ERROR) {
                    TracePrintf(0, "Error reading indirect block in path validation\n");
                    return ERROR;
                }
                block_num = ((int *)indirect_block)[blocks_iterated - NUM_DIRECT];
            }
            int readStatus = readBlock(block_num, block); //read the block into memory
            if (readStatus == ERROR) {
                TracePrintf(0, "Error reading a subBlock in path validation\n");
                return ERROR;
            }
            current_entry = (struct dir_entry *)block; // start at the first entry
            int entries;
            for (entries = 0; entries < entries_to_traverse_in_block; entries++) {
                //check all the entries in the block
                if (current_entry->inum == FREEFILE) {
                    current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));
                    continue;
                }
                if (strcmp(current_string, current_entry->name) == 0) {
                    //found the next inode
                    dir_found = true;
                    TracePrintf(1, "Found directory %s!\n", current_string);
                    //check if it is a directory type occurs at the top of the traverse tokens loop
                    curr_inode_num = current_entry->inum;
                    readInode(curr_inode_num, curr_inode); //read the new inode into the current inode field
                    break;
                }
                current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));

            }
            blocks_iterated++;
            size_traversed += bytes_to_traverse_in_block;
        }
        (void)size_traversed;
        if (!dir_found) { //didn't find the current subdirectory child in the current directory spot
            TracePrintf(1, "Couldn't directory %s!\n", current_string);
            return ERROR;
        } //otherwise, continue iterating through the path

    }
    TracePrintf(1, "Complete path found!\n");
    return curr_inode_num;
}
/**
Adds a free inode
*/
void addFreeInode(int inum) { //stolen from previous lab
    
    TracePrintf(1, "Pushing free inode to waiting queue!\n");
    // wrap process as new queue item
    struct in_str *new = malloc(sizeof(struct in_str));
    // new->node = node;
    new->next = NULL;
    new->prev = NULL;
    new->inum = inum;

    // push onto queue
    if (free_nodes_size == 0) {
        free_nodes_head = new;
        free_nodes_tail = free_nodes_head;
    } else {
        free_nodes_tail->next = new;
        new->prev = free_nodes_tail;
        free_nodes_tail = new;
    }
    free_nodes_size++;
}

/**
finds an inode from the free list and removes it. returns the inode number
*/
int getFreeInode() {

    TracePrintf(1, "getting a free inode\n");
    struct in_str *nextChild = free_nodes_head;
    if (free_nodes_head != NULL) {
        if (free_nodes_head == free_nodes_tail) {
            free_nodes_head = NULL;
            free_nodes_tail = NULL;
        } //else just pop the head
        free_nodes_head = nextChild->next;
        free_nodes_head->prev = NULL;
    }
    TracePrintf(1, "found a free inode! %i\n", nextChild->inum);
    free_nodes_size--;

    //Clean the inode if it is not clean already

    return nextChild->inum;
}

/**
finds a block from the free list and removes it. returns the block number
*/

int findFreeBlock() {
    int page;
    if (numFreeBlocks == 0) {
        return ERROR;
    }
    for (page = 0; page < numBlocks; page++)
    {
        if (freeBlocks[page] == BLOCK_FREE)
        {
            TracePrintf(0, "Found free block %d\n", page);
            freeBlocks[page] = BLOCK_USED;
            numFreeBlocks--;
            //nullify the free block
            void *buf = malloc(BLOCKSIZE);
            memset(buf, '\0', BLOCKSIZE);
            writeBlock(page, buf);
            return page;
        }
    }
    TracePrintf(0, "ERROR: No free block found!\n");
    return ERROR;
}

/**
finds the blocknum of the last sector in the inode, else ERROR
*/
int getLastSector(struct inode *node) {
    int size = node->size;
    int sector_num = size / BLOCKSIZE;
    return getSector(node, sector_num); //the last sector shouldn't be 0, because there should be contents
}

/**
finds the blocknum of the sector in the inode, else 0 (not allocated)
*/
int getSector(struct inode *node, int sector_num) {
    if (sector_num < NUM_DIRECT) {
        return node->direct[sector_num];
    } else {
        if (node->indirect == 0) {
            //indirect is not allocated; thus, we are in null territory
            return 0;
        }
        void *indirect_buf = malloc(SECTORSIZE);
        if (ReadSector(node->indirect, indirect_buf) == ERROR) {
            return ERROR;
        }
        int *sectors = (int *)indirect_buf;
        int last_sector = sectors[sector_num - NUM_DIRECT];
        free(indirect_buf);
        return last_sector;
    }
}

/**
cleans the string
*/
// char *stringCleaner(char *str, int field_size) {

// }
/**
marks the block at blocknum as used
*/
int markUsed(int block_num) {

    freeBlocks[block_num] = BLOCK_USED;
    return 0;
}

int getPid() {
    return pid;
}

int replyWithInodeNum(struct msg *message, int pid, int inum) {
    message->type = REPLYMSG;
    message->data = inum;
    Reply(message, pid);
    return 0;
}

int replyError(struct msg *message, int pid) {
    message->type = ERMSG;
    message->data = ERMSG;
    // message->content = "ERROR\n"
    Reply(message, pid);
    return 0;
}