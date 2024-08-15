#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS_NUMBER 5

/**
 * 将socket设置成非阻塞
 * 文件读写方式默认使用的是阻塞的方式，使用O_NONBLOCK可以改成非阻塞的形式
 * 阻塞方式：进程在设备没有准备好的时候进入睡眠直至资源就绪
 * 非阻塞方式：进程在设备没有准备好的时候进行不挂起，或者轮询
 * 
 * 例如如果设置了 O_NONBLOCK 标志，read 和 write 的行为是不同的 ，如果进程没有数据就绪时调用了 read ，
 * 或者在缓冲区没有空间时调用了 write ，系统只是简单的返回 EAGAIN，而不会阻塞进程.
 */
int set_non_blocking(int fd){
    int old_state = fcntl(fd, F_GETFL);
    int new_state = old_state | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_state);

    return old_state;
}

void addfd(int epollfd, int fd){
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd; // epoll_data_t is union 使用fd表示事件所属的目标fd
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); //EPOLL_CTL_ADD往事件表中注册fd上的事件
    set_non_blocking(fd);
}

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

    //--------------EPOLL------------------
    epoll_event events[MAX_EVENTS_NUMBER];
    int epollfd = epoll_create(5); // 使用一个fd来唯一标识内核中的事件表
    assert(ret != -1);
    addfd(epollfd, listenfd);

    while(1){
        int number = epoll_wait(epollfd, events, MAX_EVENTS_NUMBER, -1); //返回事件就绪的数量
        if(number < 0){
            printf("epoll_wait failed\n");
            return -1;
        }

        for(int i = 0; i < number; ++i){
            const auto& event = events[i]; // const auto&由于event是struct，为了避免copy使用&
            const auto eventfd = event.data.fd; // int类型没必要使用&

            if(eventfd == listenfd){
                struct sockaddr_in client;
                socklen_t client_addrlength = sizeof(client);
                int sockfd = accept(listenfd, (struct sockaddr*)(&client), &client_addrlength);
                addfd(epollfd, sockfd);
            }else if(event.events & EPOLLIN){
                char buf[1024] = {0};
                while(1){
                    memset(buf, '\0', sizeof(buf));
                    int recv_size = recv(eventfd, buf, sizeof(buf), 0);
                    if(recv_size < 0){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            break;
                        }
                        printf("socketfd %d receive failed\n", eventfd);
                        close(eventfd);
                        break;
                    }else if(recv_size == 0){
                        close(eventfd);
                        break;
                    }else{
                        send(eventfd, buf, recv_size, 0);
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
