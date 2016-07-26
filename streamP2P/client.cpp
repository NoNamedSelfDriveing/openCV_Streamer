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

        VideoCapture capture(0);
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
