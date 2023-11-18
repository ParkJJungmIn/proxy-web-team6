/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
		char *longmsg);

int main(int argc, char **argv) {
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr,
				&clientlen);  // line:netp:tiny:accept
		Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
				0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit(connfd);   // line:netp:tiny:doit
		Close(connfd);  // line:netp:tiny:close
	}


}
void doit(int fd){
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);

	printf("Request geaders:\n");
	printf("%s", buf);
	// sscanf는 입력을 받음
	sscanf(buf, "%s %s %s", method, uri, version);
	// method 가 GET 인지 확인. 두개가 다르면 에러메세지
	if(strcasecmp(method,"GET")) {
		clienterror(fd, method,"501","Not implemented","Tiny does not implement this method");
		return;
	}
	// read_requesthdrs 는 요청 헤더를 읽고 무시한다.
	read_requesthdrs(&rio);

	/* Parse URI from GET request */
	// 파일을 업로드 하는데 이상이 생기면 404 에러
	is_static = parse_uri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) {
		clienterror(fd, filename, "404", "Not found",
				"Tiny couldn’t find this file");
		return;
	}
	// 파일의 형식이 잘못되었음
	if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden",
					"Tiny couldn’t read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);
	}
	
	else { /* Serve dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden",
					"Tiny couldn’t run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
	}
}

// 에러 메세지를 클라이언트로 보내는 역할
void clienterror(int fd, char *cause, char *errnum,	char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
// 타이니 서버는 헤더의 어떤 정보도 쓰지 않기 때문에 요청 헤더를 읽고 무시한다.
void read_requesthdrs(rio_t *rp)
{
	char buf[MAXLINE];
	Rio_readlineb(rp, buf, MAXLINE);

	while(strcmp(buf, "\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}

	return;
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
	// uri에서 문자열을 찾는데 사용함
	// cgi-bin은 실행 폴더인데 이 폴더 안에 없다면 static content 로 판단하여 조건문을 통과한다.
	if (!strstr(uri, "cgi-bin")) { /* Static content */

		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);

		if (uri[strlen(uri)-1] == ’/’)
			strcat(filename, "home.html");
		return 1;
	}
	else { /* Dynamic content */
		// uri에서 ?를 찾고 그 포인터를 반환
		ptr = index(uri, '?');

		if (ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}
		else strcpy(cgiargs, "");

		strcpy(filename, ".");
		// 두개의 문자열을 합침
		strcat(filename, uri);

		return 0;
	}
}
// 지역 파일의 내용을 포함하고 있는 본체를 갖는 http응답을 보낸다. 
void serve_static(int fd, char *filename, int filesize)
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	// 파일 이름의 접미어 부분을 검사해서 파일 타입을 결정
	get_filetype(filename, filetype);

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	
	// 요청한 파일의 내용을 연결 식별자 fd로 복사해서 응답 본체로 보낸다. 
	Rio_writen(fd, buf, strlen(buf));

	printf("Response headers:\n");
	printf("%s", buf);

	/* Send response body to client */
	// 읽기 위해서 filename을 오픈 하고 식별자를 얻어온다.
	srcfd = Open(filename, O_RDONLY, 0);
	
	// mmap 함수는 요청한 파일을 가상메모리 영역으로 매핑한다.
	// mmap을 호출하면 파일 srcfd의 첫번째 filesize바이트를 주소 srcp에서 시작하는 사적 읽기 허용 가상메모리 영역으로 매핑한다.
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	// 매핑한 후에 식별자는 필요 없기 때문에 닫아준다. 메모리 누수 예방	
	Close(srcfd);
	// 주소 srcp에서 시작하는 filesize 바이트를 클라이언트의 연결 식별자로 복사한다.
	Rio_writen(fd, srcp, filesize);
	// 매핑된 가상메모리 주소를 반환한다. 메모리 누수 예방
	Munmap(srcp, filesize);
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0) { /* Child */
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);
		Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
		Execve(filename, emptylist, environ); /* Run CGI program */
	}
	Wait(NULL); /* Parent waits for and reaps child */
}
