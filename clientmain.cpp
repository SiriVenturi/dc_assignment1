#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <calcLib.h>

#define DEBUG

#define BUFFER_SIZE 1024

void handleError(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        handleError("Usage: ./client <host>:<port>");
    }

    // Parse input <ip>:<port> syntax
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);
    int port = atoi(Destport);
    
#ifdef DEBUG
    printf("Host %s, and port %d.\n", Desthost, port);
#endif

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        handleError("ERROR: CANT CONNECT TO HOST");
    }

    // Resolve host
    struct sockaddr_in serv_addr;
    struct hostent* server = gethostbyname(Desthost);
    if (server == NULL) {
        handleError("ERROR: RESOLVE ISSUE");
    }

    // Set up server address struct
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        handleError("ERROR: CANT CONNECT TO HOST");
    }

    // Read protocol from server
    char buffer[BUFFER_SIZE];
    char serverResponse[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    memset(serverResponse, 0, BUFFER_SIZE);
    while (1) {
        ssize_t n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0) {
            handleError("ERROR: Reading from socket");
        }
        strcat(serverResponse, buffer);
        if (strstr(serverResponse, "\n\n") != NULL) break;
    }

#ifdef DEBUG
    printf("Received protocol response: %s", serverResponse);
#endif

    // Check if TEXT TCP 1.0 is supported
    if (strstr(serverResponse, "TEXT TCP 1.0") == NULL) {
        printf("ERROR: MISSMATCH PROTOCOL\n");
        close(sockfd);
        return 1;
    }

    // Send OK to the server
    char* okResponse = "OK\n";
    if (write(sockfd, okResponse, strlen(okResponse)) < 0) {
        handleError("ERROR: Writing to socket");
    }

#ifdef DEBUG
    printf("Sent OK to the server.\n");
#endif

    // Read the operation from the server
    memset(buffer, 0, BUFFER_SIZE);
    if (read(sockfd, buffer, BUFFER_SIZE - 1) < 0) {
        handleError("ERROR: Reading from socket");
    }
    
    printf("ASSIGNMENT: %s", buffer);

    // Initialize calcLib
    initCalcLib();

    // Parse the operation
    char op[BUFFER_SIZE];
    double val1, val2;
    sscanf(buffer, "%s %lf %lf", op, &val1, &val2);

    double result = 0;
    if (strcmp(op, "add") == 0) {
        result = (int)(val1 + val2);
    } else if (strcmp(op, "sub") == 0) {
        result = (int)(val1 - val2);
    } else if (strcmp(op, "mul") == 0) {
        result = (int)(val1 * val2);
    } else if (strcmp(op, "div") == 0) {
        result = (int)(val1 / val2);  // Truncate the result for int division
    } else if (strcmp(op, "fadd") == 0) {
        result = val1 + val2;
    } else if (strcmp(op, "fsub") == 0) {
        result = val1 - val2;
    } else if (strcmp(op, "fmul") == 0) {
        result = val1 * val2;
    } else if (strcmp(op, "fdiv") == 0) {
        result = val1 / val2;
    } else {
        handleError("ERROR: Unknown operation");
    }

#ifdef DEBUG
    printf("Calculated result: %.8g\n", result);
#endif

    // Send the result to the server
    char resultStr[BUFFER_SIZE];
    if (op[0] == 'f') {
        snprintf(resultStr, BUFFER_SIZE, "%.8g\n", result);
    } else {
        snprintf(resultStr, BUFFER_SIZE, "%d\n", (int)result);
    }

    if (write(sockfd, resultStr, strlen(resultStr)) < 0) {
        handleError("ERROR: Writing result to socket");
    }

#ifdef DEBUG
    printf("Sent result: %s", resultStr);
#endif

    // Read server's response (OK or ERROR)
    memset(buffer, 0, BUFFER_SIZE);
    if (read(sockfd, buffer, BUFFER_SIZE - 1) < 0) {
        handleError("ERROR: Reading from socket");
    }

    printf("%s", buffer);

    // Close the socket
    close(sockfd);

    return 0;

