#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 80000

char process = '0'; //Encrypting = 0, Decrypting = 1

void errorFunc(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    //Network Setup
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    char failedConnection[100];

    char plaintextBuffer[BUFFER_SIZE];
    char keyBuffer[BUFFER_SIZE];
//    char ciphertextBuffer[BUFFER_SIZE];
    char transmitBuffer[BUFFER_SIZE];
    char receivedBuffer[BUFFER_SIZE];

    //Plaintext -> Buffer
    int plaintextFD = open(argv[1], O_RDONLY);
    memset(plaintextBuffer, '\0', sizeof(plaintextBuffer));
    lseek(plaintextFD, 0, SEEK_SET);
    ssize_t readPlaintext = read(plaintextFD, plaintextBuffer, sizeof(plaintextBuffer));

    //Key -> Buffer
    int keyFD = open(argv[2], O_RDONLY);
    memset(keyBuffer, '\0', sizeof(keyBuffer));
    lseek(keyFD, 0, SEEK_SET);
    ssize_t readKey = read(keyFD, keyBuffer, sizeof(keyBuffer));

    if (readPlaintext > readKey) {
        char keyLengthError [100];
        sprintf(keyLengthError, "ERORR: The key '%s' is too short.", argv[2]);
        fprintf(stderr, "%s\n", keyLengthError);
        exit(1);
    }

    int i, testChar;
    for (i = 0; i < strlen(plaintextBuffer); i++) {
        testChar = (int)plaintextBuffer[i]; //typecasted to int
        if (((testChar < 65) && ((testChar != 32) && (testChar!=10))) || (testChar > 90)) { //range of chars for good input
            fprintf(stderr, "ERROR (otp_enc): Input contains bad characters.\n");
            exit(1);
        }
    }

    // Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

    // Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) { error("CLIENT: ERROR opening socket"); }

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { error("CLIENT: ERROR connecting"); }

    transmitBuffer[0] = process;
    transmitBuffer[1] = '.';

    for (i = 2; i < (readPlaintext + 2); i++) {
        transmitBuffer[i] = plaintextBuffer[i-2];
    }
    for (i = (2 + readPlaintext); i < (2 + readPlaintext + readKey); i++) {
        transmitBuffer[i] = keyBuffer[i - (readPlaintext + 2)];
    }
    //send & receive
    charsWritten = 0;
    charsRead = 0;
    charsWritten = send(socketFD, transmitBuffer, sizeof(transmitBuffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	//if (charsWritten < strlen(transmitBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

    memset(receivedBuffer, '\0', BUFFER_SIZE); // Clear out the buffer for reuse
	charsRead = recv(socketFD, receivedBuffer, readPlaintext, MSG_WAITALL); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
    if (receivedBuffer[0] == '?') {
        fprintf(stderr, "ERORR: Unable to contact otp_enc_d on PORT %d\n", portNumber);
        exit(2);
    }
    printf("%s", receivedBuffer);
    close(socketFD);
    exit(0);
}
