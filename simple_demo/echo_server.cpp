#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> // C 和 C++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件的名称
#include <netinet/in.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
    if(argc <= 2){
        printf("Need IP address and portname\n");
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0); // PF_INET IPv4协议族 SOCK_STREAM流服务TCP
    printf("Socket is %d", listenfd);
    assert(listenfd >= 1); // 从1开始才是listenfd  

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // host (small) -> network (big)
    inet_pton(AF_INET, ip, &address.sin_addr); // 将点分十进制ip转换成sin_addr的二进制

    int ret = 0;
    ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address)); // socket命名(绑定地址)
    assert(ret != -1); // -1 means failure

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int sockfd = accept(listenfd, (struct sockaddr*)(&client), &client_addrlength);

    char buf_size[1024] = {0};
    int recv_size = 0;
    recv_size = recv(sockfd, buf_size, sizeof(buf_size), 0);

    int send_size = 0;
    send_size = send(sockfd, buf_size, recv_size, 0);

    close(sockfd);
    close(listenfd);

    return 0;
}

