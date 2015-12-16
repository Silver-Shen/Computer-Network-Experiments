#include <stdio.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <stdlib.h>
#define BUFFER_SIZE 1024

int sockfd;
char user_name[BUFFER_SIZE], password[BUFFER_SIZE];
struct sockaddr_in serverAddr; //server address struct

//show error infomation
void errorInfo(char* err_info){
    printf("Oups! %s\n", err_info);  
}

//list the usage infomation
void help(){
    printf("Usage:\n");
    printf("get [filename]  | Get a file from server\n");
    printf("put [filename]  | Send a file to server\n");
    printf("pwd             | Get current path\n");
    printf("dir             | Show all files in the current path\n");
    printf("cd              | Change current path\n");
    printf("?               | show help information\n");
    printf("quit            | Quit the client\n");
}

//get the path from server
void execPwd(){
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    send(sockfd, "pwd", BUFFER_SIZE, 0);
    recv(sockfd, buf, BUFFER_SIZE, 0);
    if (buf[0] == 'S') {
        recv(sockfd, buf, BUFFER_SIZE, 0);        
        printf("%s\n", buf);
    } 
    else{
        errorInfo("Fail to get current path from server!");
    }
}

//list the file
void execDir(){
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    send(sockfd, "dir", BUFFER_SIZE, 0);
    recv(sockfd, buf, BUFFER_SIZE, 0);
    if (buf[0] == 'S'){
        recv(sockfd, buf, BUFFER_SIZE, 0);
        int data_port = atoi(buf);
        struct sockaddr_in dataAddr;
        dataAddr.sin_family = AF_INET;
        dataAddr.sin_port = htons(data_port);
        dataAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
        int dir_sockfd; 
        if ((dir_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            errorInfo("Fail to initial dir socket!");
            return;
        }
        if (connect(dir_sockfd, (struct sockaddr*)&dataAddr, sizeof(dataAddr)) == -1){
            errorInfo("Fail to connect to server!");
            return;
        }                 
        while (recv(dir_sockfd, buf, BUFFER_SIZE, 0) > 0 && strlen(buf)>0) printf("%s\n", buf);
        close(dir_sockfd);
    } 
    else {
        errorInfo("Dir Failed!");
    }
}

//try to quit from server, -1 for fail
int execQuit(){
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    send(sockfd, "quit", BUFFER_SIZE, 0); 
    recv(sockfd, buf, BUFFER_SIZE, 0);
    if (buf[0] == 'S'){
        close(sockfd);
        return 0;
    }
    else{
        errorInfo("Fail to quit!");
        return -1;
    }
}

//get a file from server
void execGet(char* remoteAddr, char* localAddr){
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    send(sockfd, "get", BUFFER_SIZE, 0);
    send(sockfd, remoteAddr, BUFFER_SIZE, 0); 
    recv(sockfd, buf, BUFFER_SIZE, 0);   
    if (buf[0] == 'S'){
        recv(sockfd, buf, BUFFER_SIZE, 0);   
        int data_port = atoi(buf);    
        struct sockaddr_in get_addr;        
        get_addr.sin_family = AF_INET;
        get_addr.sin_port = htons(data_port);
        get_addr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
        int get_sockfd;        
        if ((get_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            errorInfo("Fail to initial get socket!");
            return;
        }
        if (connect(get_sockfd, (struct sockaddr*)&get_addr, sizeof(get_addr)) == -1){
            errorInfo("Fail to connect to server!");
            return;
        }     
        FILE * fin = fopen(localAddr, "wb");
        int len;
        while ((len = recv(get_sockfd, buf, BUFFER_SIZE, 0)) > 0) {
            if (fwrite(buf, 1, len, fin) != len){
                errorInfo("File writing error!\n");
                close(get_sockfd);
                fclose(fin);
                return;
            }
        }    
        printf("%s has been downloaded to %s!\n", remoteAddr, localAddr);
        close(get_sockfd);
        fclose(fin); 
    }
    else{
        errorInfo("Fail to download the file!");
        return;
    }
}

//send a file to server
void execPut(char* remoteAddr, char* localAddr){
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    send(sockfd, "put", BUFFER_SIZE, 0);
    memcpy(buf, remoteAddr, BUFFER_SIZE);
    send(sockfd, buf, BUFFER_SIZE, 0);
    FILE *fin;                                         
    if ((fin = fopen(localAddr, "rb")) == NULL) {
        errorInfo("file not found!");         
        return;
    }
    recv(sockfd, buf, BUFFER_SIZE, 0);   
    if (buf[0] == 'S'){  
        recv(sockfd, buf, BUFFER_SIZE, 0);
        int data_port = atoi(buf);    
        struct sockaddr_in put_addr;        
        put_addr.sin_family = AF_INET;
        put_addr.sin_port = htons(data_port);
        put_addr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
        int put_sockfd;        
        if ((put_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            errorInfo("Fail to initial put socket!");
            return;
        }
        if (connect(put_sockfd, (struct sockaddr*)&put_addr, sizeof(put_addr)) == -1){
            errorInfo("Fail to connect to server!");
            return;
        }      
        int len;
        while ((len = fread(buf, 1, BUFFER_SIZE, fin)) > 0) {
            if (send(put_sockfd, buf, len, 0) != len) {
                errorInfo("File sending error!");
                close(put_sockfd);
                fclose(fin); 
                return;
            }
        }     
        printf("%s has been uploaded to %s!\n", localAddr, remoteAddr);
        close(put_sockfd);
        fclose(fin); 
    }
    else{
        errorInfo("Fail to upload");
        return;
    } 
}

//change directory
void execCd(char* newPath) {
    char buf[BUFFER_SIZE];
    send(sockfd, "cd", BUFFER_SIZE, 0);
    send(sockfd, newPath, BUFFER_SIZE, 0);
    recv(sockfd, buf, BUFFER_SIZE, 0);
    if (buf[0] == 'S') {
        printf("Dir command successful!\n");
        recv(sockfd, buf, BUFFER_SIZE, 0);
        printf("Current path: %s\n", buf);
    } 
    else {
        errorInfo("Dir command failed!");
    }
}

void startService(){
    int isQuit = 0;
    char command[BUFFER_SIZE];
    while (!isQuit){
        printf(">");
        memset(command, 0, sizeof(command));
        fgets(command, sizeof(command), stdin);        
        command[strlen(command)-1] = '\0'; //delete the final newline            
        if (command[0] == '\0') continue;        
        if (command[0] == '?') {help();continue;}        
        if (strcmp(command, "pwd") == 0) {execPwd();continue;}        
        if (strcmp(command, "dir") == 0) {execDir();continue;}        
        if (strcmp(command, "quit") == 0){
            if (execQuit() == -1) continue;
            isQuit = 1;break;
        }
        if (strncmp(command, "get ", 4) == 0){
            int offset = 4;            
            char remoteAddr[BUFFER_SIZE], localAddr[BUFFER_SIZE];
            memset(remoteAddr, 0, sizeof(remoteAddr));
            memset(localAddr, 0, sizeof(localAddr));
            while (*(command + offset) == ' ') offset++;
            int pt = 0;
            while (*(command + offset + pt) != ' '){
                remoteAddr[pt] = *(command + offset + pt);
                pt++;
            }                        
            offset += pt;
            while (*(command + offset) == ' ') offset++;
            pt = 0;            
            while (*(command + offset + pt) != '\0'){                
                localAddr[pt] = *(command + offset + pt);
                pt++;
            }                        
            execGet(remoteAddr, localAddr);
            continue;   
        }
        if (strncmp(command, "put ", 4) == 0){
            int offset = 4;
            char remoteAddr[BUFFER_SIZE], localAddr[BUFFER_SIZE];
            memset(remoteAddr, 0, sizeof(remoteAddr));
            memset(localAddr, 0, sizeof(localAddr));
            while (*(command + offset) == ' ') offset++;
            int pt = 0;
            while (*(command + offset + pt) != ' '){
                remoteAddr[pt] = *(command + offset + pt);
                pt++;
            }
            offset += pt;
            while (*(command + offset) == ' ') offset++;
            pt = 0;
            while (*(command + offset + pt) != '\0'){
                localAddr[pt] = *(command + offset + pt);
                pt++;
            }
            execPut(remoteAddr, localAddr);
            continue;
        }   
        if (strncmp(command, "cd ", 3) == 0){     
            int offset = 3;
            while(*(command + offset) == ' ') offset++;
            execCd(command + offset);
            continue;
        }
        help();
    }
}

void login(){
    char buf[BUFFER_SIZE];
    printf("username:");
    scanf("%s", user_name);
    send(sockfd, user_name, BUFFER_SIZE, 0);
    printf("password:");   
    scanf("%s", password);
    send(sockfd, password, BUFFER_SIZE, 0);
    recv(sockfd, buf, BUFFER_SIZE, 0);
    if(buf[0] == 'S'){
        printf("login successfully! Proceed to command session!\n");
        startService();
    }
    else{
        errorInfo("invalid user or wrong password, please connect and try again");
        exit(-1);
    }
}

void connectToServer(char* ipAddress, int port){
    //initialize the socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        errorInfo("Fail to initial socket!");
        exit(-1);
    }

    //prepare the address struct    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr);

    //try to connect
    int ret;
    if ((ret = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) == -1){
        errorInfo("Fail to connect to server!");
        exit(-1);
    }

    //proceed to login session
    login();
}

int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 3){//show usage of ftp-client
        printf("Usage: %s <host> [<port>=21]\n", argv[0]);
        exit(-1);
    }
    else if (argc == 2) connectToServer(argv[1], 21);
    else connectToServer(argv[1], atoi(argv[2]));
    return 0;
}