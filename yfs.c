#include <comp421/filesystem.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
// #include "iolib.c"
#include <comp421/yalnix.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#define INODESPERBLOCK BLOCKSIZE / INODESIZE

#define OPEN 0
#define CLOSE 1
#define CREATE 2
#define MKDIR 11
#define NONE -1
#define BLOCK_FREE 0
#define BLOCK_USED 1
#define DUMMY 50

struct msg { //Dummy message structure from the given PDF
    int type; 
    int data;
    char content[16];
    void *ptr;
};

struct in_str { //an inode linkedlist class
    struct inode *node;
    int inode_num;
    struct in_str *next;
    struct in_str *prev;
};
int pid;
int getPid();
//Function signatures
int sectorBytes(struct inode *node);
struct inode *findInodePtr(int inode_num);
int markUsed(int blocknum);
void init();
int readInodeBlock(int block_index, int num_inodes_to_read, void *buf, int start_index);
int readInode(int inode_num, void *buf);
void addFreeInode(struct inode *node, int inode_num);
int findFreeBlock();
int writeDirectoryToInode(struct inode *node, int inode_num, char *name);
int findParent(char *name, int curr_directory);
int readBlock(int block_index, void *buf);
int setNewInode(int inode_num, short type, short nlink, int reuse, int size, int parent_inode_num);
//building the list in memory of free disk blocks
void mkDirHandler(struct msg *message, int senderPid);
int getLastSector(struct inode *node);
int getFreeInode();
char *findLastDirName(char *name);

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
                TracePrintf(0, "Received message type %d\n", type);
                TracePrintf(0, "Received message content %s\n", message->content);
                switch(type) {
                    case NONE: {
                        TracePrintf(0, "Received NONE message type\n");
                        break;
                    }
                    case MKDIR: {
                        TracePrintf(0, "Received MKDIR message type\n");
                        mkDirHandler(message, receive);
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
        Reply(message, senderPid);
    }
    struct inode *parentInode = malloc(sizeof(struct inode));
    if (readInode(parent_inode_num, parentInode) == ERROR) {
        //handler error here
        TracePrintf(0, "Error reading parent_inode in mkdirhandler\n");
        Reply(message, senderPid);
    }
    TracePrintf(1, "We are here1\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i", parent_inode_num, parentInode->type, parentInode->size);

    //next logic: search for the last entry in the dir to see if it exists. if it does, then return error.

    //if the entry doesn't exist, then create the dir_entry
    //gotta figure out where the sector with space is
    //read that sector to a buffer
    //store the buffer's start
    //move the buffer pointer to the empty spot
    //set the fields appropriately
    // int parent_size = parentInode->size;
    int sector = getLastSector(parentInode);
    void *buf = malloc(SECTORSIZE);
    TracePrintf(1, "We are here2\n");
    if (ReadSector(sector, buf) == ERROR) {
        //handle error here
        TracePrintf(0, "Error reading sector in mkdirhandler\n");
        Reply(message, senderPid);
    }
    TracePrintf(1, "We are here3\n");
    void *start = buf;
    //int inode num in block = ((i * INODESIZE) + Blocksize - 1) / blocksize;
    //curb = curblock + inodesize * (i % (INODESPERBLOCK))
    int bytes_into_sector = sectorBytes(parentInode);
    // int dirs_into_sector = bytes_into_sector / sizeof(struct dir_entry);
    buf = buf + bytes_into_sector; 
    
    //TODO: need to check if there is space in the current sector before writing to it
    TracePrintf(1, "We are here4\n");
    int new_inode_num = getFreeInode();
    TracePrintf(1, "Setting new inode %i with type %i and parent %i\n", new_inode_num, INODE_DIRECTORY, parent_inode_num);
    setNewInode(new_inode_num, INODE_DIRECTORY, -1, 0, 0, parent_inode_num);
    TracePrintf(1, "We are here6\n");
    struct dir_entry *entry = (struct dir_entry *)buf;
    entry->inum = new_inode_num;
    strcpy(entry->name, findLastDirName(path));
    // entry->name = findLastDirName(path);
    parentInode->size = parentInode->size + sizeof(struct dir_entry);
    TracePrintf(1, "Writing new directory to sector\n");
    WriteSector(sector, start);
    TracePrintf(1, "Done writing new directory to sector\n");
    TracePrintf(1, "parent is inode number %i with inode type %i and size %i", parent_inode_num, parentInode->type, parentInode->size);
    Reply(message, senderPid);

}

void createHandler() {
    //go down the nodes from the root until get to what you're creating, then add to the directory
}


/**
returns the number of bytes into the last sector
*/
int sectorBytes(struct inode *node) {
    return (node->size % sizeof(SECTORSIZE));
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
                    addFreeInode(node, inode_num);
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
reads the block at index block_index, which contains a bunch of inodes
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
finds the last directory name
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
    }
    //TODO Check this logic compared to readInode
    // struct inode *node = ((struct inode *)buffer)[(inode_num - 1) % inodes_per_block];//this should be the node
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num) % inodes_per_block) * sizeof(struct inode));
    node->type = type;
    node->nlink = nlink;
    node->reuse = reuse;
    node->size = size;
    node->direct[0] = findFreeBlock();
    
    writeDirectoryToInode(node, inode_num, "."); //TODO: write this method?
    writeDirectoryToInode(node, parent_inode_num, "..");
    TracePrintf(1, "size of node is %i\n", node->size);
    WriteSector(block_num, start);
    //TODO: set the inode first . and .. to itself and parentinodenum, respectively. 
    return 0;


}

/**

*/
int writeDirectoryToInode(struct inode *node, int inode_num, char *name) {

    int size = node->size;    
    //TODO: need to check if there is space in the current sector before writing to it, else move to next sector
    if (size > (NUM_DIRECT * BLOCKSIZE)) {

    }
    int bytes_into_sector = sectorBytes(node);
    
    // int dirs_into_sector = bytes_into_sector / sizeof(struct dir_entry);
    int sector = getLastSector(node);
    void *buf = malloc(SECTORSIZE);
    if (ReadSector(sector, buf) == ERROR) {
        //handle error here
        return -1;
    }
    void *start = buf;
    //int inode num in block = ((i * INODESIZE) + Blocksize - 1) / blocksize;
    //curb = curblock + inodesize * (i % (INODESPERBLOCK))
    buf = buf + bytes_into_sector; 
    
    struct dir_entry *entry = (struct dir_entry *)buf;
    entry->inum = inode_num;
    
    // entry->name = name;
    strcpy(entry->name, name);
    node->size = node->size + sizeof(struct dir_entry);

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
        TracePrintf(0, "TODO in pathfinder: replace lines here with current directory\n");
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
    //TODO: FREE EVERYTHING
    // free(block);
    // free(indirect_block);
    return curr_inode_num;
}
/**
Adds a free inode
*/
void addFreeInode(struct inode *node, int inode_num) { //stolen from previous lab
    
    TracePrintf(1, "Pushing free inode to waiting queue!\n");
    // wrap process as new queue item
    struct in_str *new = malloc(sizeof(struct in_str));
    new->node = node;
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