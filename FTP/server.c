#include <stdio.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#define BUFFER_SIZE 1024
#define BACKLOG 20
#define SUCCESS "S"
#define FAILURE "F"
#define USERNAME "shenzy"
#define PASSWORD "123456"
int server_port;
struct sockaddr_in serverAddr, client_addr;
char root_dir[BUFFER_SIZE];
int listenfd, recvfd;
int port_pointer = 1024;

int get_port(){
    if(port_pointer == 5000){
        port_pointer = 1025;
        return port_pointer;
    }else {
        port_pointer++;
        return port_pointer;
    }
}
//show error infomation
void errorInfo(char* err_info){
    printf("Oups! %s\n", err_info);  
}

int serverStart(){
    int sockfd;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errorInfo("Create socket failed!");
        exit(-1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        errorInfo("bind failed!");
        close(sockfd);
        exit(-1);
    }

    if (listen(sockfd, BACKLOG) < 0) {
        errorInfo("listen failed");
        close(sockfd);
        exit(-1);
    }

    printf("Create server listen socket successfully: %s:%d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
    return sockfd;
}

int login(){
    char username[BUFFER_SIZE],password[BUFFER_SIZE];
    recv(recvfd, username, BUFFER_SIZE, 0);
    recv(recvfd, password, BUFFER_SIZE, 0);
    if(!strcmp(username, USERNAME) && !strcmp(password, PASSWORD)){
        printf("Client:%d login successfully!\n",ntohs(client_addr.sin_port));
        send(recvfd, SUCCESS, BUFFER_SIZE, 0);
    } 
    else{
        errorInfo("Client login failed!");
        send(recvfd, FAILURE, BUFFER_SIZE, 0);
        return -1;
    }
    return 0;
}

void execPwd(){
    char buf[BUFFER_SIZE];
    send(recvfd, SUCCESS, BUFFER_SIZE, 0);
    getcwd(buf, BUFFER_SIZE);
    send(recvfd, buf, BUFFER_SIZE, 0);
}

void execDir(){
    char buf[BUFFER_SIZE], data_port[BUFFER_SIZE];
    int port = get_port();
    send(recvfd, SUCCESS, BUFFER_SIZE, 0);
    sprintf(data_port, "%d", port);
    send(recvfd, data_port, BUFFER_SIZE, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, BACKLOG);
    int datarecvfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
    
    system("ls > .temp");
    int file = open("./.temp", O_RDONLY);
    int len;
    while((len = read(file, buf, BUFFER_SIZE))>0) {
        if (send(datarecvfd, buf, len, 0) != len) {
            errorInfo("Send error!");
            close(sockfd);
            close(datarecvfd);
            return;
        }
    }
    close(sockfd);
    close(datarecvfd);
    system("rm .temp");
    printf("COMMAND DIR SUCCEED!\n");
    return;
}

void execQuit(){
    send(recvfd, SUCCESS, sizeof(char), 0);
    return;
}

void execCd(char* path){
    int k = chdir(path);
    char newPath[BUFFER_SIZE];
    if (k == 0) {
        getcwd(newPath, BUFFER_SIZE);
        send(recvfd, SUCCESS, BUFFER_SIZE, 0);
        send(recvfd, newPath, BUFFER_SIZE, 0);        
    } 
    else {
        send(recvfd, FAILURE, BUFFER_SIZE, 0);
        errorInfo("chdir error!");
    }
    return;
}

void execGet(char* filename){        
    char data_port[BUFFER_SIZE], buf[BUFFER_SIZE];
    FILE* file = fopen(filename, "rb");
    if (!file) {
        send(recvfd, FAILURE, BUFFER_SIZE, 0);
        return;
    }
    send(recvfd, SUCCESS, BUFFER_SIZE, 0);
    int port = get_port();
    sprintf(data_port, "%d", port);
    send(recvfd, data_port, BUFFER_SIZE, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, BACKLOG);
    int getfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
    int len;
    while ((len = fread(buf, 1, BUFFER_SIZE, file))>0) {
        if (send(getfd, buf, len, 0) != len) {
            errorInfo("Send data error!");
            return;
        }
    }
    printf("file %s get successfully!\n", filename);
    close(getfd);
    close(sockfd);
    fclose(file);
}

void execPut(char* filename){
    char data_port[BUFFER_SIZE], buf[BUFFER_SIZE];
    int port = get_port();
    FILE *file = fopen(filename, "wb");
    if (!file){
        errorInfo("Can't open file!");
        return ;
    } 
    else{
        send(recvfd, SUCCESS, BUFFER_SIZE, 0);
    }
    sprintf(data_port, "%d", port);
    send(recvfd, data_port, BUFFER_SIZE, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, BACKLOG);
    int putfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
    int len;
    while ((len = recv(putfd, buf, BUFFER_SIZE, 0)) >0){
        if (fwrite(buf, 1, len, file) != len) {
            errorInfo("File write error!");
            return;
        }
    }
    printf("file %s put successfully!\n", filename);
    close(putfd);
    close(sockfd);
    fclose(file);
    return;
}

int startService(){
    if (login() == -1) return -1;
    printf("IN COMMNAND SESSION\n");
    char buf[BUFFER_SIZE];
    while (1) {
        memset(buf, 0, sizeof(buf));
        if (recv(recvfd, buf, BUFFER_SIZE, 0) <= 0){
            printf("Receive erro:%s(errno:%d)\n", strerror(errno), errno);
            return -1;
        }                                          
        printf("COMMAND[%s]\n",buf);
        if (strcmp(buf, "get") == 0){
            recv(recvfd, buf, BUFFER_SIZE, 0);
            execGet(buf);
            continue;
        }
        if (strcmp(buf, "put") == 0) {
            recv(recvfd, buf, BUFFER_SIZE, 0);
            execPut(buf);
            continue;
        }
        if (strcmp(buf, "quit") == 0) {
            execQuit();
            printf("COMMAND SESSION OVER!\n");
            return 0;
        }
        if (strcmp(buf, "dir") == 0) {
            execDir();
            continue;
        }
        if (strcmp(buf, "cd") == 0) {
            recv(recvfd, buf, BUFFER_SIZE, 0);
            execCd(buf);
            continue;
        }
        if (strcmp(buf, "pwd") == 0) {
            execPwd();
            continue;
        }
        printf("Command Error!\n");
    }
    return 0;
}

void waitSession(){
    int pid;
    while (1) {
        printf("Server ready, wait client's connection...\n");
        recvfd = accept(listenfd, NULL, NULL);
        if (recvfd < 0) {
            errorInfo("accept failed");
            exit(-1);
        }
        int len = sizeof(client_addr);
        getpeername(recvfd, (struct sockaddr*)&client_addr, &len);
        printf("accept a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        switch (pid = fork()) {
            case -1:
                errorInfo("The fork failed!");
                break;
            case 0:
                close(listenfd);
                if (startService() == -1) exit(-1);
                len = sizeof(client_addr);
                getpeername(recvfd, (struct sockaddr*)&client_addr, &len);
                printf("Client %s:%d quit!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                close(recvfd);
                exit(0);            
            default:
                close(recvfd);
                continue;
        }
    }
    return;
}

int main(int argc, char *argv[])
{
    if (argc == 2)
        server_port = atoi(argv[1]);
    else {
        printf("Usage: %s [port]\n", argv[0]);
        exit(-1);
    }
    printf("The server port is %d\n", server_port);
    getcwd(root_dir, sizeof(root_dir));
    listenfd = serverStart();
    waitSession();
    return 0;
}