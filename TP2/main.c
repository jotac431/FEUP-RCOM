#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

#define IP_MAX_SIZE 16

#define SERVER_PORT 21

struct url{
    char* username;
    char* password;
    char* host;
    char* urlPath;
    
};
static char filename[500];

int parser(int argc, char** argv, struct url *url){
    //ftp://[<user>:<password>@]<host>/<url-path>
    if (argc < 2){
        printf("Not enought arguments\n");
        return -1;
    }

    char *url_string;
    /* get initial */
    url_string = strtok(argv[1], "//");

    printf("ftp : %s\n", url_string);
    
    if (strcmp(url_string, "ftp:")!=0) {
        printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }
    url_string = strtok(NULL, "\0");
    
    //get host with user:password
    char *up_host;
    up_host = strtok(url_string, "/");
    printf("up_host : %s\n", up_host);

    //get url
    url->urlPath = strtok(NULL, "\0");
    printf("url : %s\n", url->urlPath);

    char copy[500];
    
    strcpy(copy,url->urlPath);
    char* url_string2 = strtok(copy, "/");
    
    while( url_string2 != NULL ) {
        strcpy(filename,url_string2);
        url_string2 = strtok(NULL, "/");
    }

    //user:password@host
    if (strstr(up_host, "@") != NULL) {
        if (strstr(up_host, ":") == NULL) {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;    
        }
        
        //user 
        url->username = strtok(up_host, ":");
        if (strstr(url->username, "@") != NULL) {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;    
        }
        printf("user : %s\n", url->username);
        up_host = strtok(NULL, "\0");

        //password
        if (up_host[0] != '@') {
            url->password = strtok(up_host, "@");
            printf("password : %s\n", url->password);
            up_host = strtok(NULL, "\0");
        } else {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;   
        }

        //host
        url->host = strtok(up_host, "\0");
        printf("host : %s\n", url->host);

    } else {
        //host
        url->username = "anonymous";
        url->password = "";
        url->host = strtok(up_host, "\0");
        printf("host : %s\n", url->host);
    }
         
    return 0;
}

int writeSocket(int fd, char* cmd) {
    
    int bytesSent;

    if ((bytesSent = write(fd, cmd, strlen(cmd))) != strlen(cmd)) {		
        fprintf(stderr, "Error writing message to socket!\n");
        return -1;
    }

	printf("Command: %s", cmd);

    return bytesSent;
}

int readSocket(char* response, FILE* socket){
    int H, T, U, space, responseCode;
    
    H = fgetc(socket);
    T = fgetc(socket);
    U = fgetc(socket);
    space = fgetc(socket);

    responseCode = (H-'0')*100 + (T-'0')*10 + U-'0';
    
    if(space != ' '){
        fgets(response, 200, socket);
        while(response[3] != ' ') {
            fgets(response, 200, socket);
        }
    } else {
        fgets(response, 200, socket);
    }

    printf("\n%s\n", response);

    return responseCode;        
}

int createSocket(char* ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int manageSocket(char* cmd, char* arg1, char* arg2, int sockfd1, FILE* sockfile1, char* buf, int code){
    
    int responseCode;
    
    free(cmd);
    cmd = malloc(256);
    strcat(cmd, arg1);
    strcat(cmd, arg2);
    strcat(cmd, "\n");

    if(writeSocket(sockfd1, cmd) < 0) {
        printf("Error writing to socket");
        return -1;
    }

    if((responseCode = readSocket(buf, sockfile1)) < 0){
        printf("Error reading socket\n");
        return -1;
    }

    if(responseCode != code){
        printf("Bad response\n");
        return -1;
    }
}

int getPassivePort(int *pasv_port, char* buf){

    int a,b,c,d,e,f;
    sscanf(buf, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &a,&b,&c,&d,&e,&f);

    *pasv_port = e * 256 + f;
    
    return 0;
}

int writeToFile(FILE* socket_file2){
    
    FILE* myFile = fopen(filename, "w");

    int s;
    while(true){
        unsigned char content[300];
        bool end;
        s = fread(content, 1, 300, socket_file2);
        if(s < 300) end = true;
        s = fwrite(content, 1, s, myFile);
        if(end) break;
    }
}


int main(int argc, char** argv)
{
    struct url url;
    if(parser(argc, argv, &url) < 0) {
        return -1;
    }
    
    struct hostent *h;
    if ((h = gethostbyname(url.host)) == NULL) {
        herror("gethostbyname()");
        return -1;
    }

    char ip[IP_MAX_SIZE];
    strcpy(ip, inet_ntoa(*((struct in_addr *) h->h_addr))); 
    printf("IP Address : %s\n", ip);

    int sockfd1;
    if ((sockfd1 = createSocket(ip, SERVER_PORT)) < 0) {
        return -1;
    }

    FILE * sockfile1 = fdopen(sockfd1, "r+");

    char buf[1000];
    int responseCode;

    //welcome message
    if((responseCode = readSocket(buf, sockfile1)) < 0){
        printf("Error reading socket\n");
        return -1;
    }

    if(responseCode != 220){
        printf("Bad response\n");
        return -1;
    }

    char* cmd = malloc(256);

    //username
    manageSocket(cmd, "user ", url.username, sockfd1, sockfile1, buf, 331);
    
    //password
    manageSocket(cmd, "pass ", url.password, sockfd1, sockfile1, buf, 230);

    //passive mode
    manageSocket(cmd, "pasv ", "", sockfd1, sockfile1, buf, 227);

    int pasv_port;

    getPassivePort(&pasv_port, buf);

    int sockfd2;

    if ((sockfd2 = createSocket(ip, pasv_port)) < 0) {
        return -1;
    }

    FILE * socket_file2 = fdopen(sockfd2, "r+");

    //retrieve command
    manageSocket(cmd, "retr ", url.urlPath, sockfd1, sockfile1, buf, 150);

    //Write to file
    writeToFile(socket_file2);

    //Read transfer complete
    if((responseCode = readSocket(buf, sockfile1)) < 0){
        printf("Error reading socket\n");
        return -1;
    }

    if(responseCode != 226){
        printf("Bad response\n");
        return -1;
    }

    if (close(sockfd2)<0) {
        perror("close()");
        return -1;
    }

    if (close(sockfd1)<0) {
        perror("close()");
        return -1;
    }
    return 0;
}
