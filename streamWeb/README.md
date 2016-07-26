# openCV_Streamer

openCV_Streamer - streamWeb

---

# How to Comfile

```
g++ -o combineServer.exe combineServer.cpp -std=c++0x  -lpthread `pkg-config opencv --cflags --libs`
```


---

# Source

```
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<poll.h>

#include<iostream>
#include<thread>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/core/core.hpp>

#define CONNMAX 10
#define BYTES 1024

using namespace std;
using namespace cv;

char *ROOT;
int listenfd, clients[CONNMAX];
void error(char *);

struct pollfd fds[999];
int poll_num;

//start server
void startServer(char *port)
{
    struct addrinfo hints, *res, *p;

    // getaddrinfo for host
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo( NULL, port, &hints, &res) != 0)
    {
        perror ("getaddrinfo() error");
        exit(1);
    }
    // socket and bind
    for (p = res; p != NULL; p = p->ai_next)
    {
        listenfd = socket (p->ai_family, p->ai_socktype, 0);
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }
    if (p == NULL)
    {
        perror ("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    // listen for incoming connections
    if ( listen (listenfd, 1000000) != 0 )
    {
        perror("listen() error");
        exit(1);
    }
}

void respond(int n)
{
    char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, fd, bytes_read, i;
    int keep_cnt = 99;

    rcvd = recv(fds[n].fd, mesg, 99999, 0);

    if (rcvd <= 0) {    // receive error or close
        close(fds[n].fd);
        poll_num--;
        for(i = n; i < poll_num; i++)
            fds[i].fd = fds[i+1].fd;
        fprintf(stderr,"%d Client disconnected upexpectedly.\n", n);
        return;
    }
    else    // message received
    {
        mesg[rcvd] = 0;
        printf("%d %s", n, mesg);

        reqline[0] = strtok (mesg, " \t\n");
        if ( strncmp(reqline[0], "GET\0", 4) == 0 )
        {
            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            if ( strncmp( reqline[2], "HTTP/1.0", 8) != 0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
            {
                write(fds[n].fd, "HTTP/1.0 400 Bad Request\n", 25);
            }
            else
            {
                if ( strncmp(reqline[1], "/\0", 2) == 0 )
                    reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

                strcpy(path, ROOT);
                strcpy(&path[strlen(ROOT)], reqline[1]);

                printf("%s\n", path);
                if ( (fd = open(path, O_RDONLY)) != -1 )    //FILE FOUND
                {
                    char *ext;
                    struct stat sb;
                    fstat(fd, &sb);

                    strcpy(mesg, "HTTP/1.0 200 OK\n");
                    send(fds[n].fd, mesg, strlen(mesg), 0);

                    sprintf(mesg, "Content-Length: %d\n", sb.st_size);
                    send(fds[n].fd, mesg, strlen(mesg), 0);
                    strcpy(mesg, "Connection: keep-alive\n");
                    send(fds[n].fd, mesg, strlen(mesg), 0);
                    //						sprintf(mesg,"Keep-Alive: timeout=20, max=%d\n", keep_cnt--);
                    //						send(clients[n], mesg, strlen(mesg), 0);

                    strcpy(mesg, "\n");
                    send(fds[n].fd, mesg, strlen(mesg), 0);
                    while ( (bytes_read = read(fd, data_to_send, BYTES)) > 0 )
                        write (fds[n].fd, data_to_send, bytes_read);
                }
                else    write(fds[n].fd, "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
            }
        }
    }
}

void captureFunc(){
    VideoCapture capture(0);
    Mat frame;

    if(!capture.isOpened()){
        perror("videocapture");
    }

    while(1){
        capture >> frame;
        imwrite("1.jpg", frame);
        //cout << "Complete\n" << endl;
    }
}

int main(int argc, char* argv[])
{
    struct sockaddr_in clientaddr;
    struct in_addr k;
    socklen_t addrlen;
    signed char c;
    thread capFuncThread(&captureFunc);

    //capFuncThread.join();

    //Default Values PATH = ~/ and PORT=10000
    char PORT[6];
    ROOT = getenv("PWD");
    strcpy(PORT,"10000");

    int slot=0;

    c = -1;
    //Parsing the command line arguments
    while ((c = getopt (argc, argv, "p:r:")) != -1) {
        switch (c)
        {
            case 'r':
                ROOT = (char*)malloc(strlen(optarg));
                strcpy(ROOT, optarg);
                break;
            case 'p':
                strcpy(PORT, optarg);
                break;
            case '?':
                fprintf(stderr,"Wrong arguments given!!!\n");
                exit(1);
            default:
                exit(1);
        }
    }

    printf("Server started at port no. %s%s%s with root directory as %s%s%s\n", "\033[92m", PORT, "\033[0m", "\033[92m", ROOT, "\033[0m");
    // Setting all elements to -1: signifies there is no client connected
    int i;
    for (i = 0; i < CONNMAX; i++) {
        fds[i].events = POLLIN;
    }

    startServer(PORT);

    addrlen = sizeof(clientaddr);

    fds[0].fd = listenfd;
    poll_num = 1;

    while (1)
    {
        int ret, fd_no;

        ret = poll (fds, poll_num, -1);
        if (ret < 0)
            perror("poll");

        if (fds[0].revents & POLLIN) {
            int new_fd;
            new_fd = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);
            if (poll_num >= CONNMAX) {
                fprintf(stderr, "************** No more space for new connection\n");
                close(new_fd);
                continue;
            }
            fds[poll_num].fd = new_fd;
            printf("new user: %d %s\n", poll_num, inet_ntoa(clientaddr.sin_addr));
            printf("=========================================\n");
            poll_num++;
            ret--;
        }
        fd_no = 1;
        while(ret > 0 && fd_no < poll_num) {
            if (fds[fd_no].revents & POLLIN) {
                respond(fd_no);
                ret--;
            }
            fd_no++;
        }
    }

    return 0;
}

```

---
