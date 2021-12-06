#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define MAXSOCKETS   	10   //Limit of sockets.
#define TIMETOSLEEP  	10   //Time in seconds that pthread "SendMessages" will sleep.
#define PORT 			8080 //Port used for the connection
void *SendMessages (void *ptr);
void *Sensor_data (void *ptr);
void sendMsg(int sock, void* msg, uint32_t msgsize)
{
    if (write(sock, msg, msgsize) < 0)
    {
        printf("Can't send message.\n");
        close(sock);
        exit(1);
    }
    //printf("Message sent (%d bytes).\n", msgsize);
    return;
}

//Global variables used by ptheads
int32_t listSocket[MAXSOCKETS];
int32_t socketIndex;          //Socket Index
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;
char buffer[1024] = {0};
int32_t bSend_return;
float sensor_value = 0;
//Struct
typedef struct
{
    uint8_t sof;
    uint8_t sensor1;
    uint8_t sensor2;
    uint8_t sensor3;
    uint8_t sensor4;
    uint8_t sensor5;
    float data;
    uint32_t cs;
}share_data;
//Sensor Struct
#pragma pack(1)

typedef struct payload_t {
    float hr;
    float spo2;
} sensor;

#pragma pack()


int main(int argc, char  *argv[])
{
    int32_t server_fd;
    int32_t sinSize; 
    int32_t newScoketConnectionFD;
    struct sockaddr_in address;
    int32_t opt = 1;
    pthread_t pthSendMessages;
    pthread_t pthData_sensor;
    int iPthRc;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
       
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        return 1;
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        return -1;
    }
    iPthRc = pthread_create(&pthSendMessages, NULL, &SendMessages, NULL);
    if (iPthRc < 0)
        {
            printf("There was an error trying to create the pthread, iPthRc:%d\n", iPthRc);
            exit(1);
        }
    iPthRc = pthread_create(&pthData_sensor, NULL, &Sensor_data, NULL);
    if (iPthRc < 0)
        {
            printf("There was an error trying to create the pthread, iPthRc:%d\n", iPthRc);
            exit(1);
        }		
	    sinSize = sizeof(struct sockaddr_in);
    while (true)
	{
		printf("Waiting for the client\n");
		newScoketConnectionFD = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&sinSize);
        if (newScoketConnectionFD < 0)
		{
			printf("There was an error traying to accept in the socket, errno:%d\n", errno);
			continue;
		}
        printf("New client connection\n");
        pthread_mutex_lock(&socketMutex);

        listSocket[socketIndex] = newScoketConnectionFD;
        printf("Socket connection saved in the array\n");

		read(listSocket[socketIndex] , buffer, 1024);
        printf("Saving client data\n");
        sleep(1);
		pthread_mutex_unlock(&socketMutex);
	}

    return 0;
}

void *SendMessages (void *ptr)
{
    share_data server;
    //Data Sensor
    server.sof = 0xAA;
    server.sensor1 = 'M';
    server.sensor2 = 'A';
    server.sensor3 = 'X';
    server.sensor4 = '3';
    server.sensor5 = '0';
    server.data = sensor_value;
    server.cs = ~(sizeof(share_data));
	//Variable declaration.
    while (1)
    {
		pthread_mutex_lock(&socketMutex);
        if (buffer[0] == server.sof){
            printf("Sending Data\n");
            bSend_return = send(listSocket[socketIndex],&server,sizeof(server),MSG_NOSIGNAL);
            if (bSend_return <= 0){
			    printf("Shuting down a socket of index: %d\n", socketIndex);
			    shutdown(listSocket[socketIndex], SHUT_RDWR);
			    listSocket[socketIndex] = 0;
		    }
            else 
            {
                printf("Successful data sent\n");
                if(buffer[0] != 0 && socketIndex < 5){
                socketIndex++;
                }else if(socketIndex == 5 ){
                socketIndex = 0;
                }
            }
        }
        //Message implementation depending on your application
		pthread_mutex_unlock(&socketMutex);
		//This pthread must sleep for 10 seconds.
		sleep(TIMETOSLEEP);
	}
	return 0;
}

void *Sensor_data (void *ptr){
    const int PORT_S = 2300;
    const char* SERVERNAME = "localhost";
    int BUFFSIZE = sizeof(sensor);
    char buff[BUFFSIZE];
    int sock;
    int nread;
    
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERNAME, &server_address.sin_addr);
    server_address.sin_port = htons(PORT_S);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("ERROR: Socket creation failed\n");
        return 0;
    }

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("ERROR: Unable to connect to server\n");
        return 0;
    }

    //printf("Connected to %s\n", SERVERNAME);

    sensor data;
    data.hr = 0;
    data.spo2 = 0;
    while(true) {
        //printf("\nSending id=%d, counter=%d, temp=%f\n", data.id, data.counter, data.temp);
        sendMsg(sock, &data, sizeof(sensor));
        bzero(buff, BUFFSIZE);
        nread = read(sock, buff, BUFFSIZE);
        printf("Received %d bytes\n", nread);
        sensor *p = (sensor*) buff;
        printf("Received Hr=%f, SPO2=%f\n",
                p->hr, p->spo2);
        sensor_value = p->spo2;
    }
    
    // close the socket
    close(sock);
    return 0;
}
