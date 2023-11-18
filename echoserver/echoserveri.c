#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{   
    // listenfd : 연결 요청을 기다리는 데 사용되는 소켓
    // connfd : 연결된 클라이언트와 통신하는 데 사용되는 소켓
    int listenfd, connfd;
    // 클라이언트 주소의 크기를 저장
    socklen_t clientlen;
    // 클라이언트의 주소 정보를 저장하는 구조체
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    // 클라이언트의 호스트 이름과 포트 번호를 저장하는 문자열
    char client_hostname[MAXLINE], client_port[MAXLINE];

    // 인자 검사 및 에러 메시지
    // 2가 아니면 메시지 출력하고 종료
    // 올바른 수의 인자를 받았는 지 확인하는 것
    if (argc != 2) {
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}