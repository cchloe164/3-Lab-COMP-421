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
#define OPEN 0
#define CLOSE 1
#define CREATE 2
#define MKDIR 11
#define NONE -1
#define BLOCK_FREE 0
#define BLOCK_USED 1
#define DUMMY 50

int pid;
int getPid();
//Function signatures
int markUsed(int blocknum);
void init();
int readInodeBlock(int block_index, int num_inodes_to_read, void *buf, int start_index);
int readInode(int inode_num, void *buf);
void addFreeInode(struct inode *node);
int readBlock(int block_index, void *buf);
//building the list in memory of free disk blocks

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
struct msg { //Dummy message structure from the given PDF
    int type; 
    int data2;
    char content[16];
    void *ptr;
};

struct in_str { //an inode linkedlist class
    struct inode *node;
    struct in_str *next;
    struct in_str *prev;
};
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

void mkdirHandler() {
    //gotta go down the inodes from the root until you get to the parent directory, then add a new struct directory to the inode and increment size
    //also update the parent inode size
}

void createHandler() {
    //go down the nodes from the root until get to what you're creating, then add to the directory
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
                if (node->type == INODE_FREE) {
                    //the inode is free
                    addFreeInode(node);
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

//find free block

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
    struct inode *node = (struct inode *)((char *)buffer + ((inode_num - 1) % inodes_per_block) * sizeof(struct inode));
    memcpy(buf, node, sizeof(struct inode));
    free(buffer);
    return 0;
}

/**
Reads the ith direct or indirect block of the inode, returns the block number. returns -1 if error
*/

// int getBlockOfInode(int inode_num, int block_index, void *buf) {

// }

/**
parses the path, returns the parent inode number if the path is valid or else returns an error 
*/
int findParent(char *name) {
    TracePrintf(1, "in FindParent\n");
    TracePrintf(0, "finding parent inode in string %s", name);

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
        int read_s = readInode(ROOTINODE, curr_inode);
        if (read_s == ERROR) {
            TracePrintf(0, "error reading root inode in path validation\n");
            return -1;
        }
        curr_inode_num = ROOTINODE;
    } else {
        TracePrintf(0, "TODO in pathfinder: replace lines here with current directory\n");
        return -1;
        // int read_s = readInode(current_directory, curr_inode);
        // curr_inode_num = current_directory;
        // if (read_s == ERROR) {
        //     TracePrintf(0, "error reading relative inode in path validation\n");
        //     return -1;
        // }
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
        return curr_inode_num;
    }
    int traversed = 0; //number of traversed directories
    //traverse to the second to last one, num_tokens - 1, which should be the parent
    for (traversed = 0; traversed < num_tokens - 1; traversed++) {
        char *current_string = tokens[traversed]; //the current directory to go to
        TracePrintf(1, "Looking for directory %s\n", current_string);
        if (curr_inode->type != INODE_DIRECTORY) { //the parent is not a directory. should never hit this, in theory
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

void addFreeInode(struct inode *node) { //stolen from previous lab
    
    TracePrintf(1, "Pushing free inode to waiting queue!\n");
    // wrap process as new queue item
    struct in_str *new = malloc(sizeof(struct in_str));
    new->node = node;
    new->next = NULL;
    new->prev = NULL;

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
marks the block at blocknum as used
*/
int markUsed(int block_num) {

    freeBlocks[block_num] = BLOCK_USED;
    return 0;
}

int getPid() {
    return pid;
}