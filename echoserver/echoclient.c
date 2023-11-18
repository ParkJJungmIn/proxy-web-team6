#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    // 클라이언트 파일 디스크립터 파일 : 수신된 데이터를 버퍼(데이터의 크기)에 저장하고 동일한 데이터를 클라이언트에 다시 전송하는 작업을 수행함.
    char *host, *port, buf[MAXLINE];
    // host, port(number), buf (MAXLINE의 크기)
    rio_t rio;
    // rio는 네트워크 프로그래밍에서 버퍼링 및 입출력 관리를 용이하게 하기 위해 생성

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    // 입력과 출력에 대해 준비된 열린 소켓 식별자(유일하게 하나 있는 거)를 리턴한다.
    Rio_readinitb(&rio, clientfd);
    // Rio init은 리오 구조체를 초기화함.

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        // writend은 서버로 데이터를 보냄.
        Rio_readlineb(&rio, buf, MAXLINE);
        // realineb는 서버로부터 데이터를 읽음.
        Fputs(buf, stdout);
        // 표준 입력을 한 줄 읽어와서 출력함.
    }
    Close(clientfd);
    exit(0);
}