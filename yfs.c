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

#define INODESPERBLOCK BLOCKSIZE / INODESIZE

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
#define ERMSG -2
#define REPLYMSG -3
#define FREEFILE 0

struct msg { //Dummy message structure from the given PDF
    int type; 
    int data;
    char content[16];
    void *ptr;
};

struct in_str { //an inode linkedlist class
    // struct inode *node;
    int inode_num;
    struct in_str *next;
    struct in_str *prev;
};
int pid;
int getPid();
//Function signatures
int sectorBytes(struct inode *node); // how many bytes are used in the last used block/sector of the node (use this to calculate how much free space is left in current block, or where free space exists)
struct inode *findInodePtr(int inode_num); //DEFUNCT; if you need this logic, find it in SETNEWINODE
int findDirectoryEntry(struct inode *curr_inode, int curr_inode_num, char *name); // Searches for file or directory named "name" in the inode (presumably a directory type)
int markUsed(int blocknum); //marks a block as used
void init(); //initializes the fs structure
int readInodeBlock(int block_index, int num_inodes_to_read, void *buf, int start_index);  //used only for init() to read blocks that contain inodes
int readInode(int inode_num, void *buf); //Super useful: takes in an inode number and a buffer, reads the contents of the inode into the buffer (please remember to write back to disk)
int writeInodeToDisk(int inode_num, struct inode *inode_to_cpy); //writes the inode info contained in inode_to_cpy to the inode in disk specified by inode_num
void addFreeInode(int inode_num); //used in init to add inodes to the freelist; if you free an inode, add it here
int findFreeBlock(); //finds a free block
int writeDirectoryToInode(struct inode *node, int curr_inode, int inode_num, char *name); //writes a dir_entry containing inode_num and name to the inode curr_inode.
int findParent(char *name, int curr_directory); //finds the parent of the path
int readBlock(int block_index, void *buf); //reads the block 
int numBlocksUsedBy(struct inode *node); //calculates the number of blocks used by the inode (based on size)
int setNewInode(int inode_num, short type, short nlink, int reuse, int size, int parent_inode_num); //creates a new inodes
//building the list in memory of free disk blocks
void mkDirHandler(struct msg *message, int senderPid); //handles mkdir requests
void openHandler(struct msg *message, int senderPid);
int getLastSector(struct inode *node); //gets the last sector used in the inode
int getFreeInode(); //gets a free inode
char *findLastDirName(char *name); //finds the last directory name in a path (after the last '/', or the whole string)
int replyError(struct msg *message, int pid);
int replyWithInodeNum(struct msg *message, int pid, int inode_num);
void chDirHandler(struct msg *message, int senderPid);
void createHandler(struct msg *message, int senderPid);
void statHandler(struct msg *message, int senderPid);
int checkPath(struct msg *message);
void rmDirHandler(struct msg *message, int senderPid);
void prepFree(struct inode *curr_inode);

int freeDirectoryEntry(struct inode *curr_inode, int curr_inode_num, int inode_num);


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
        return -1;
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
                    case OPEN: {
                        TracePrintf(0, "Received OPEN message type\n");
                        openHandler(message, receive);
                        break;
                    }
                    case MKDIR: {
                        TracePrintf(0, "Received MKDIR message type\n");
                        mkDirHandler(message, receive);
                        break;
                    }
                    case CREATE: {
                        TracePrintf(0, "Received CREATE message type\n");
                        createHandler(message, receive);
                        break;
                    }
                    case CHDIR: {
                        TracePrintf(0, "Received CHDIR message type\n");
                        chDirHandler(message, receive);
                        break;
                    }
                    case STAT: {
                        TracePrintf(0, "Received STAT message type\n");
                        statHandler(message, receive);
                        break;
                    }
                    case RMDIR: {
                        TracePrintf(0, "Received RMDIR message type\n");
                        rmDirHandler(message, receive);
                        break;
                    }
                    case DUMMY: {
                        TracePrintf(0, "Received DUMMY message type\n");
                        // mkdirhandler(message);
                        strcpy(message->content,"Hello\n"); //testing whether the message is received properly (see iolib handler)

                        Reply(message, receive);
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

void openHandler(struct msg *message, int sender_pid) {
    TracePrintf(0, "Received pathname %s\tcur dir %d\tpid %d\n", message->content, message->data, sender_pid);
    int inum = checkPath(message);
    if (inum == ERROR)
    {
        replyError(message, sender_pid);
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
    char *path = message->content;
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
    if (matching_inode != ERROR) {
        TracePrintf(0, "Create: directory %s already exists in inode %i. \n", new_directory_name, parent_inode_num);
        replyError(message, senderPid);
        return;
    }
    
    // TracePrintf(1, "We are here4\n");
    int new_inode_num = getFreeInode();
    TracePrintf(1, "Create: Setting new inode %i with type %i and parent %i\n", new_inode_num, INODE_DIRECTORY, parent_inode_num);
    setNewInode(new_inode_num, INODE_REGULAR, 1, 0, 0, parent_inode_num); //TODO: change this -1
    // TracePrintf(1, "We are here6\n");
    writeDirectoryToInode(parentInode, parent_inode_num, new_inode_num, new_directory_name);
    // entry->name = findLastDirName(path);
    //update parent
    writeInodeToDisk(parent_inode_num, parentInode);
    TracePrintf(1, "Create: parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);
    replyWithInodeNum(message, senderPid, new_inode_num);
}

/**

*/
// void openHandler() {
//     //go down the nodes from the root until get to what you're opening, then add to the directory
// }

/**
This request returns information about the file indicated by pathname in the information structure
at address statbuf . The information structure is defined within comp421/iolib.h as follows
The fields in the information structure are copied from the information in the file’s inode. 
On success, this request returns 0; on any error, the value ERROR is returned.
*/

void statHandler(struct msg *message, int senderPid) {
    int inode_num = checkPath(message);
    if (inode_num == ERROR) { //error reaching directory or directory does not exist
        replyError(message, senderPid);
    } else { //gotta read the info from the node into a stat. put into contents (struct stat is 16 bytes)
        void *buf = malloc(sizeof(struct inode));
        readInode(inode_num, buf);
        struct inode *node = (struct inode *)buf;
        struct Stat *statbuf = (struct Stat *)message->content;
        statbuf->inum = inode_num;
        statbuf->type = node->type;
        statbuf->size = node->size;
        statbuf->nlink = node->nlink;
        //reply with that contents
        message->type = REPLYMSG;
        message->data = inode_num;
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
    char *path = message->content;
    int curr_directory = message->data;
    int parent_inode_num = findParent(path, curr_directory);
    if (parent_inode_num == ERROR) {
        //handle error here
        TracePrintf(0, "Error finding parent_inode num in checkpath\n");
        // Reply(message, senderPid); 
        replyError(message, senderPid);
    }
    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in checkpath\n");
        // replyError(message, senderPid);
        replyError(message, senderPid);
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) { 
        TracePrintf(0, "parent inode %i is not a directory\n", parent_inode_num);
        // replyError(message, senderPid);
        replyError(message, senderPid);
    }
    char *my_name = findLastDirName(path);

    int matching_inode_num = findDirectoryEntry(parentInode, parent_inode_num, my_name);
    if (matching_inode_num == ERROR) {
        replyError(message, senderPid);
    }
    //we are a valid directory entry. now need to free the inode and update the parent's entry

    struct inode *curr_inode = malloc(sizeof(struct inode));
    if (readInode(matching_inode_num, curr_inode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in checkpath\n");
        // replyError(message, senderPid);
        replyError(message, senderPid);
    }
    
    prepFree(curr_inode);
    writeInodeToDisk(matching_inode_num, curr_inode);
    addFreeInode(matching_inode_num);


    //update the parent's entry
    freeDirectoryEntry(parentInode, parent_inode_num, matching_inode_num);
}

/**
Prepares the inode to be freed. Sets all fields to corresponding free values
*/

void prepFree(struct inode *curr_inode) {
    //set the fields of the free inode (in the buffer), then write it to disk with updates
    curr_inode->type = INODE_FREE;
    curr_inode->nlink = 0;
    curr_inode->reuse = curr_inode->reuse + 1;
    curr_inode->size = 0;
    //TODO: check direct and indirect fields here
    return;
}

/**
Basically checks the path and returns the directory inode at the end of the path if it exists/is valid. iolib should change the directory to this inode
*/

void chDirHandler(struct msg *message, int senderPid) {
    int inode_num = checkPath(message);
    if (inode_num == ERROR) { //error reaching directory or directory does not exist
        replyError(message, senderPid);
    } else { //gotta check if it is a directory type
        void *buf = malloc(sizeof(struct inode));
        readInode(inode_num, buf);
        struct inode *node = (struct inode *)buf;
        if (node->type == INODE_DIRECTORY) {
            replyWithInodeNum(message, senderPid, inode_num);
        } else {
            replyError(message, senderPid);
        }
    }
}

/**
will Reply with messages that contain useful contents for iolib maintenance
*/
void mkDirHandler(struct msg *message, int senderPid) {
    //gotta go down the inodes from the root until you get to the parent directory, then add a new struct directory to the inode and increment size
    //also update the parent inode size
    char *path = message->content;
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
                int inode_num = ((block_index - 1) * INODESPERBLOCK) + in_idx; //calculate the inode number
                if (node->type == INODE_FREE) {
                    //the inode is free
                    TracePrintf(1, "Found a free inode!\n");
                    addFreeInode(inode_num);
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
Checks the path contained in the message's contents. returns the inode number if the path is valid, else ERROR if anything else. 
copied from top of createhandler / mkdirhandler (they should be the same)
*/
int checkPath(struct msg *message) {
    //go down the nodes from the root until get to what you're creating, then check that entry
    //gotta go down the inodes from the root until you get to the parent directory
    char *path = message->content;
    int curr_directory = message->data;

    // Chloe's fanangling:
    if (strcmp(".", path) == 0) {   // stay in current directory
        TracePrintf(0, "checkPath: Stay in current directory\n");
        return curr_directory;
    }

    if (strcmp("..", path) == 0) {
        // TracePrintf(0, "checkPath: Go to parent file\n");
        // struct inode *curr_inode = malloc(sizeof(struct inode));
        // int read_s = readInode(curr_directory, curr_inode);
        // if (read_s == ERROR)
        // {
        //     TracePrintf(0, "checkPath: ERROR reading current node\n");
        //     return ERROR;
        // }
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
        return -1;
    }

    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in checkpath\n");
        // replyError(message, senderPid);
        return -1;
    }
    // TracePrintf(1, "We are here1\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i\n", parent_inode_num, parentInode->type, parentInode->size);

    // next logic: search for the last entry in the dir to see if it exists. if it does, then return error.
    if (parentInode->type != INODE_DIRECTORY) { 
        TracePrintf(0, "parent inode %i is not a directory\n", parent_inode_num);
        // replyError(message, senderPid);
        return -1;
    }
    char *new_directory_name = findLastDirName(path);

    int matching_inode = findDirectoryEntry(parentInode, parent_inode_num, new_directory_name);
    //will be error or an inode number. return whatever we get. 

    return matching_inode;

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
int readInode(int inode_num, void *buf) {
    TracePrintf(1, "Reading inode %i\n", inode_num);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    int block_num = inode_num / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        return -1;
    }
    // struct inode *node = ((struct inode *)buffer)[(inode_num - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num) % inodes_per_block) * sizeof(struct inode));
    memcpy(buf, node, sizeof(struct inode));
    free(buffer);
    TracePrintf(1, "Finished reading inode %i\n", inode_num);
    return 0;
}

/**
Reads the ith direct or indirect block of the inode, returns the block number. returns -1 if error
*/

// int getBlockOfInode(int inode_num, int block_index, void *buf) {

// }

/*
(NOW DEFUNCT Function: incorporated into the SETNEWINODE and SETINODE functions)
Reason: need the sector buffer pointer in order to WriteSector() to disk, 
otherwise the inode will only be changed in the copy here but not on the file system. 
calculates and returns the inode's (hypothetical) pointer
*/

struct inode *findInodePtr(int inode_num) {
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    int block_num = inode_num / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inode_num, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
        //TODO: what to return here?
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inode_num - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num) % inodes_per_block) * sizeof(struct inode));
    return node;
}
/**
Sets fields of new inode. size input does not include size of initial directories
*/

int setNewInode(int inode_num, short type, short nlink, int reuse, int size, int parent_inode_num) {
    TracePrintf(1, "Setting inode %i with type %i, parent %i\n", inode_num, type, parent_inode_num);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    void *start = buffer;
    int block_num = inode_num / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inode_num, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
        //TODO: what to return here?
        return -1;
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inode_num - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num) % inodes_per_block) * sizeof(struct inode));
    node->type = type;
    node->nlink = nlink;
    node->reuse = reuse;
    node->size = size;
    node->direct[0] = findFreeBlock();

    writeDirectoryToInode(node, inode_num, inode_num, "."); //TODO: write this method?
    writeDirectoryToInode(node, inode_num, parent_inode_num, "..");
    TracePrintf(1, "size of node is %i\n", node->size);
    WriteSector(block_num, start);
    //TODO: set the inode first . and .. to itself and parentinodenum, respectively. 
    return 0;

}


/**
Sets fields of the inode in disk. size input does  include size of initial directories
*/

int writeInodeToDisk(int inode_num, struct inode *inode_to_cpy) {
    TracePrintf(1, "Setting inode %i with type %i\n", inode_num, inode_to_cpy->type);
    void *buffer = malloc(SECTORSIZE); //the buffer to read into
    void *start = buffer;
    int block_num = inode_num / (inodes_per_block) + 1;
    int status = ReadSector(block_num, buffer);
    TracePrintf(1, "Finding inode pointer for inode %i at block number %i\n", inode_num, block_num);
    if (status == ERROR) {
        // handle ReadSector failure
        free(buffer);
        TracePrintf(0, "Error finding inode pointer\n");
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inode_num - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num) % inodes_per_block) * sizeof(struct inode));
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
Frees the inode dir_entry (set the inode_num field to 0). almost identical to findDirectoryEntry
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
                continue;
            }
            if (current_entry->inum == inode_to_remove) { //this is what is changed
                //found the next inode
                dir_found = true;
                //check if it is a directory type occurs at the top of the traverse tokens loop
                found_inode_num = current_entry->inum;
                TracePrintf(1, "Found node %s, with inode %i!\n", found_inode_num, found_inode_num);
                current_entry->inum = 0;
                // curr_inode_num = current_entry->inum;
                // readInode(curr_inode_num, curr_inode); //read the new inode into the current inode field
                break;
            }
            TracePrintf(1, "Entry inode %s does not match %s\n", current_entry->inum, inode_to_remove);
            current_entry = (struct dir_entry *)((char *)current_entry + sizeof (struct dir_entry));

        }
        blocks_iterated++;
        size_traversed += bytes_to_traverse_in_block;
    }
    (void)size_traversed;
    if (!dir_found) { //didn't find the current subdirectory child in the current directory spot
        TracePrintf(1, "Couldn't find node %s!\n", inode_to_remove);
        return ERROR;
    } //otherwise, continue iterating through the path
    return found_inode_num;
}

/**
writes a directory to the inode
*/
int writeDirectoryToInode(struct inode *node, int curr_inode, int inode_num, char *name) {

    int size = node->size;    
    //TODO: need to check if there is space in the current sector before writing to it, else move to next sector
    if (size > (NUM_DIRECT * BLOCKSIZE)) {

    }
    int bytes_into_sector = sectorBytes(node);
    TracePrintf(2, "%i bytes into the directory sector for write directory to inode\n", bytes_into_sector);
    
    // int dirs_into_sector = bytes_into_sector / sizeof(struct dir_entry);
    int sector = getLastSector(node);
    TracePrintf(2, "%i sectors into the inode for write directory to inode\n");
    void *buf = malloc(SECTORSIZE);
    if (ReadSector(sector, buf) == ERROR) {
        //handle error here
        return -1;
    }
    void *start = buf;//need to keep track of the start so I can writesector()
    //int inode num in block = ((i * INODESIZE) + Blocksize - 1) / blocksize;
    //curb = curblock + inodesize * (i % (INODESPERBLOCK))
    buf = buf + bytes_into_sector; 
    
    struct dir_entry *entry = (struct dir_entry *)buf;
    entry->inum = inode_num;
    
    // entry->name = name;
    strcpy(entry->name, name);
    node->size = node->size + sizeof(struct dir_entry);
    TracePrintf(1, "size of struct dir_entry: %i\n", sizeof(struct dir_entry));
    TracePrintf(1, "Writing new directory %s to inode %i's sector\n", name, curr_inode);

    if (WriteSector(sector, start) == ERROR) {
        return -1;
    }
    return 0;
}

/**
parses the path, returns the parent inode number if the path is valid or else returns an error 
*/
int findParent(char *name, int curr_directory) {
    TracePrintf(1, "in FindParent\n");
    TracePrintf(0, "finding parent inode in string %s\n", name);

    //first, parse the name to make sure it is a valid path structure (30 characters or less, with null)
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
    //then, parse into separations by slashes, track number of nodes
    int root = clean[0] == '/'; //is the path from the root
    struct inode *curr_inode = malloc(sizeof(struct inode)); //stores the current inode
    int curr_inode_num;
    if (root) { //if it's the root, set the current inode
        TracePrintf(1, "we are pathfinding from the root\n");
        int read_s = readInode(ROOTINODE, curr_inode);
        if (read_s == ERROR) {
            TracePrintf(0, "error reading root inode in path validation\n");
            return -1;
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
            return -1;
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
void addFreeInode(int inode_num) { //stolen from previous lab
    
    TracePrintf(1, "Pushing free inode to waiting queue!\n");
    // wrap process as new queue item
    struct in_str *new = malloc(sizeof(struct in_str));
    // new->node = node;
    new->next = NULL;
    new->prev = NULL;
    new->inode_num = inode_num;

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
    TracePrintf(1, "found a free inode! %i\n", nextChild->inode_num);
    free_nodes_size--;
    return nextChild->inode_num;
}

/**
finds a block from the free list and removes it. returns the block number
*/

int findFreeBlock() {
    int page;
    if (numFreeBlocks == 0) {
        return -1;
    }
    for (page = 0; page < numBlocks; page++)
    {
        if (freeBlocks[page] == BLOCK_FREE)
        {
            TracePrintf(0, "Found free block %d\n", page);
            freeBlocks[page] = BLOCK_USED;
            numFreeBlocks--;
            return page;
        }
    }
    TracePrintf(0, "ERROR: No free block found!\n");
    return -1;
}

/**
finds the blocknum of the last sector in the inode, else ERROR
*/
int getLastSector(struct inode *node) {
    int size = node->size;
    int sector_num = size / BLOCKSIZE;
    if (sector_num < NUM_DIRECT) {
        return node->direct[sector_num];
    } else {
        void *indirect_buf = malloc(SECTORSIZE);
        if (ReadSector(node->indirect, indirect_buf) == ERROR) {
            return ERROR;
        }
        int *sectors = (int *)indirect_buf;
        free(indirect_buf);
        return sectors[sector_num - NUM_DIRECT];
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

int replyWithInodeNum(struct msg *message, int pid, int inode_num) {
    message->type = REPLYMSG;
    message->data = inode_num;
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