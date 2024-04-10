#include <comp421/filesystem.h>

//building the list in memory of free disk blocks

/**
Other processes using the file system send requests to the server and receive replies from the server,
 using the message-passing interprocess communication calls provided by the Yalnix kernel.

 your file server process implements most of the functionality of the Yalnix file system, 
 managing a cache of inodes and disk blocks and performing all input and output with the disk drive.
*/
struct msg { //Dummy message structure from the given PDF
    int data1; 
    int data2;
    char data3[16];
    void *ptr; 
    };
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

    if (Register(FILE_SERVER) != 0) {
        TracePrintf(0, "Error registering main process \n");
    }
    init(); //must format the file system on the disk before running the server
    if (argc > 1) {
        int pid = Fork();
        if (pid == 0) {
            Exec(argv[1], argv + 1);
        }
    }
    // "While 1 loop handling requests"; slide 9
    while (1) {
        TracePrintf(0, "Server listening for message \n");
        struct msg message;
        //Receive client request message, handle the request, reply back to client
        int receive = Receive(message);
        //handle cases: error, deadlock break
        TracePrintf(0, "Server received message signal! ID %i\n", receive);

        if (receive == ERROR) {
            TracePrintf(0, "Server error message signal!\n");
            continue; //TODO: add error handling
        }
        else if (receive == 0) {//blocking message
            TracePrintf(0, "Server received blocking message! All processes were listening.\n");
            //handle blocked message here
        }
        //now we have to parse the message and do something with it

        //TODO: create a message type, encode the library codes in one of the fields, and call handlers?


    }


}

void init() {
    //set up the file system? inodes and stuff
}