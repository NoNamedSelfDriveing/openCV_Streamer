# openCV_Streamer

openCV_Streamer - streamP2P

---

# How to Comfile

### client.cpp
    g++ -o client.exe client.cpp `pkg-config opencv --cflags --libs`

### server.cpp
    g++ -o server.exe server.cpp `pkg-config opencv --cflags --libs`

.exe인 이유는 .gitignore로 실행파일 upload를 block하기 위함.

---

# Source

## client.cpp
```
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

int main()
{
        int sock;
        int send_len;
        int c;

        struct sockaddr_in addr;
        double weight = 800;
        double height = 600;

        VideoCapture capture = 0;
        Mat frame;
        vector<uchar> ibuff;
        vector<int> param = vector<int>(2);

        char windowName[] = "Client Camera";
        int sendSize = 65535;
        char buff[sendSize];

        sock = socket(AF_INET, SOCK_DGRAM, 0);

        addr.sin_family = AF_INET;
        addr.sin_port = htons(9000);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if(!capture.isOpened()) {
                exit(1);
        }


        //I fixed It.
        param[0] = CV_IMWRITE_JPEG_QUALITY;
        param[1] = 55; //default(95) 0-100

        while (1) {
                capture >> frame;

                imencode(".jpg", frame, ibuff, param);

                for (int i = 0; i < ibuff.size(); i++)
                {
                        buff[i]=ibuff[i];
                }

                send_len = sendto(sock, buff, ibuff.size(), 0, (struct sockaddr *)&addr, sizeof(addr));

                if (send_len==-1)
                        perror("socket");

                if( waitKey(1) > 0)
                        break;
        }

        cvDestroyWindow(windowName);
        close(sock);
        return 0;
}
```

## server.cpp
```
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

int main(int argc, const char * argv[])
{
    int sock;
    struct sockaddr_in addr;

    double weight = 800;
    double height = 600;

    char windowName[] = "Server Camera";
    Mat image = Mat(weight, height, CV_8UC3);

    int receiveSize = 65*1024;
    char buff[receiveSize];
    int c;
    int received;

    vector<uchar> ibuff;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if( (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))<0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    cvNamedWindow( windowName, CV_WINDOW_AUTOSIZE );
    //cvNamedWindow( "test", CV_WINDOW_AUTOSIZE );

    while(1){
        printf("Recieved\n");
        memset(buff, 0, receiveSize);
        received = recvfrom(sock, buff, receiveSize, 0, NULL,NULL);

        if(received != -1)
        {
            for(int i = 0; i < received; i++)
            {
                ibuff.push_back((uchar)buff[i]);
            }

            image = imdecode( Mat(ibuff), CV_LOAD_IMAGE_COLOR);

            imshow("test", image);

            GaussianBlur(image, image, Size(7,7), 1.5, 1.5);
            cvtColor(image, image, COLOR_BGR2GRAY);
            Canny(image, image, 0, 30, 3);

            imshow(windowName, image);

            ibuff.clear();
        }

        else
        {
            perror("sock");
        }

        if( waitKey(1) > 0)
            break;
    }

    cvDestroyWindow(windowName);
    close(sock);
    return 0;
}
```

---


## 소스 설명
