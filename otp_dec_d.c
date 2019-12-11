#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>

#define BUFFER_SIZE 80000

char process = '0'; //Encrypting = 1, Decrypting = 0

void errorFunc(const char *msg) {
    perror(msg);
    exit(1);
}

void childrenStatus();
int nextAvailable(int *, int, int);

int numConnected = 0;
int checkConnections = 1;
int connectionPID [5] = {-2, -2, -2, -2, -2};

char plaintextBuffer[BUFFER_SIZE];
char ciphertextBuffer[BUFFER_SIZE];
char keyBuffer[BUFFER_SIZE];
char transmitBuffer[BUFFER_SIZE];
char receivedBuffer[BUFFER_SIZE];

int main(int argc, char *argv[]) {
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead, charsWritten, index;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

    // Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) { error("ERROR opening socket"); }

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { error("ERROR on binding"); }
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    while (1) {

        childrenStatus();

        if (numConnected < 5) { //Checks if server is full
            // Accept a connection, blocking if one is not available until one connects
            sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
            establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
            if (establishedConnectionFD < 0) { error("ERROR on accept"); }
            else { numConnected++; }

            pid_t pid = fork();

            if (pid != 0) {
                index = nextAvailable(connectionPID, 5, (-2));
                connectionPID[index] = pid;
            }
            if (pid == 0) {
                charsRead = 0;
                while (charsRead == 0) {
                    charsRead = recv(establishedConnectionFD, receivedBuffer, sizeof(receivedBuffer), MSG_WAITALL);
                    if (charsRead < 0) { error("ERROR reading from socket."); }
                }

                if (receivedBuffer[0] == process) {
                    memset(plaintextBuffer, '\0', sizeof(ciphertextBuffer));
                    plaintextBuffer[0] = '?';
                    charsRead = send(establishedConnectionFD, plaintextBuffer, sizeof(plaintextBuffer), 0);
                    if (charsRead < 0) { error("ERROR writing to socket."); }
                    exit(0);
                }

                int i, divider, fileEnd;
                int ciphertextIndex = 0;
                int keyIndex = 0;

                for (i = 0; i < sizeof(receivedBuffer); i++) {
                    if (receivedBuffer[i] == '\n') { break; }
                }

                divider = i;

                for (i = 2; i < divider; i++) {
                    ciphertextBuffer[ciphertextIndex] = receivedBuffer[i];
                    ciphertextIndex++;
                }

                for (i = (divider + 1); i < sizeof(receivedBuffer); i++) {
                    if (receivedBuffer[i] == '\n') { break; }
                }

                fileEnd = i;

                for (i = (divider + 1); i < fileEnd; i++) {
                    keyBuffer[keyIndex] = receivedBuffer[i];
                    keyIndex++;
                }

                char temp;
                int keyNum, cipherNum, plainNum;
                memset(plaintextBuffer, '\0', sizeof(ciphertextBuffer));

                for(i = 0; i < (divider - 1); i++) {
                    if (ciphertextBuffer[i] == ' ') { ciphertextBuffer[i] = '@'; }
                    if (keyBuffer[i] == ' ') { keyBuffer[i] = '@'; }

                    cipherNum = (int) ciphertextBuffer[i];
                    keyNum = (int) keyBuffer[i];

                    cipherNum -= 64;
                    keyNum -=64;

                    plainNum = ((cipherNum - keyNum) % 27);
                    if (plainNum < 0) { plainNum += 27; }

                    plainNum += 64;

                    temp = (char) plainNum;
                    if (temp == '@') { temp = ' '; }

                    plaintextBuffer[i] = temp;
                }

                plaintextBuffer[i-1] = '\n';

                charsWritten = send(establishedConnectionFD, plaintextBuffer, sizeof(plaintextBuffer), 0);
                if (charsRead < 0) { error("ERROR writing to socket."); }
                close(establishedConnectionFD);
                exit(0);
            }
            else {}
        }

        else { printf("SERVER MESSAGE: No more sockets available...\n"); }
    }

    return 0;
}

void childrenStatus() {
    int i, pid, check;
    for (i = 0; i < 5; i++) {
        pid = connectionPID[i];
        pid_t checking = waitpid(pid, &check, WNOHANG);
        if ((checking == connectionPID[i]) || (checking == 1) || (checking == 2)) {
            connectionPID[i] = (-2);
            numConnected--;
        }
    }
}

int nextAvailable(int *array, int n, int def) {
    int i, done = 0;
    for (i = 0; i < n; i++) { //find the first default through iteration
        if (array[i] == def) { done = 1; }
        if (done) { return (i); }
    }
    if (!done) { return (-1); }
}
