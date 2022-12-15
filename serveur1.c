#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#define RCVSIZE 1024
#define SEG_SIZE 6

int genPort (int port) {
    int newPort = 1000 + (rand()%8999);
    while (newPort == port){
        newPort = 1000 + (rand()%8999);
    }
    return newPort;
}

char* genSeq (int sequence){
    char* seg;
    seg = malloc(sizeof(char));
    char val[6];
    sprintf(val,"%d",sequence);
    int j = strlen(val)-1;
    for(int i=5; i> 5-strlen(val);i--){
        seg[i]= val[j];
        j--;
    }
    for(int i=5-strlen(val);i>=0;i--){
        seg[i] = '0';
    }
    return seg;
    free(seg);
}
/*
int RTT(clock_t start,clock_t end){
    double rtt = ((double)(end - start))/CLOCKS_PER_SEC;
    FILE* fichier_rtt;
    if(!(fichier_rtt = fopen("rtt.log","a"))){
        printf("Error opening file rtt.log \n");
        return EXIT_FAILURE;
    }
    fprintf(fichier_rtt, "%lf %lf\n",((double)clock())/CLOCKS_PER_SEC,rtt);
    fclose(fichier_rtt);
    return 0;
}*/

int execTime(clock_t start,clock_t end, int data){
    double diff = ((double)(end - start))/CLOCKS_PER_SEC;
    //Ecriture dans un fichier de suivi
    FILE* fichier_time;
    if(!(fichier_time = fopen("execution.log","a"))){
        printf("Error opening file execution.log \n");
        return EXIT_FAILURE;
    }

    fprintf(fichier_time,"\nNouvelle execution: fragment de %d octets\n", SEG_SIZE);
    fprintf(fichier_time, "%lf s -> débit: %lf o/s",diff,data/diff);
    fclose(fichier_time);
    return 0;
}

int ack (char* buf){
    char* num = NULL;
    num = malloc(sizeof(char));
    int j = 0;
    for (int i=3; i<strlen(buf);i++){
        num[j] = buf[i];
        j++;
    }

    return atoi(num);
    free(num);
}

struct Socket {
    int socket;
    int port;
    struct sockaddr_in clientAddress;
    socklen_t clientSize;
    struct sockaddr_in address;
    char buffer[RCVSIZE];
    int valid;
};

struct Socket generateSocket(char* argPort) {

    struct sockaddr_in address, clientAddress;
    int port = atoi(argPort);
    int valid = 1;
    memset((char*)&address,0,sizeof(address));
    memset((char*)&clientAddress,0,sizeof(clientAddress));

    int clientSize = sizeof(clientAddress);

    int socketLink = socket(AF_INET,SOCK_DGRAM,0);
    if (socketLink < 0) exit(-1);

    setsockopt(socketLink, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketLink, (struct sockaddr*) &address, sizeof(address)) == -1) {
        perror("Bind failed\n");
        close(socketLink);
        exit(-1);
    }

    struct Socket newSocket;

    newSocket.socket = socketLink;
    newSocket.port = port;
    newSocket.clientAddress = clientAddress;
    newSocket.clientSize = clientSize;
    newSocket.address = address;
    newSocket.valid = valid;

    return newSocket;
}
/*
int moniACK(struct Socket socket, struct Socket transfertSocket, int sequence, int rsize, char* fragment, clock_t start,clock_t end){
    int num_ack = 0;
    int somethingToRead = select(transfertSocket.socket+1, &set, NULL,NULL,&timeout);
    if(somethingToRead==-1){
        printf("No feedbacks received, resending fragment\n");
        sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
    }else if(FD_ISSET(transfertSocket.socket,&set)){
        int size = recvfrom (transfertSocket.socket, (char*)socket.buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&socket.clientAddress, &socket.clientSize);
        if (strncmp(socket.buffer,"ACK",3)==0){
            printf("\nReceived ACK -> %s\n",socket.buffer);
            num_ack = ack(socket.buffer); 
            printf("Number -> %i\n",num_ack);         
            if(sequence == num_ack){
                end = clock();
                //RTT(start, end);
            } else {
                printf("Seq do not match, resending\n");
                sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
            }
        }
    } else {
        printf("Ended timer\n");
        //Renvoi du segment
        sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
    }
    return 0;
}*/

int sendFragment (struct Socket socket, struct Socket transfertSocket, int sequence, int rsize, char* fragment, clock_t start,clock_t end){
    //se mettre en attente non bloquante: select
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(transfertSocket.socket, &set);

    timeout.tv_sec=1;
    timeout.tv_usec=0;

    if(sendto(transfertSocket.socket,fragment, rsize,0,(const struct sockaddr *)&socket.clientAddress, sizeof(socket.clientAddress)) < 0){
        printf("Error sending fragment, resending\n");
        sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
    }else{
        printf ("Send packet %i: %s \n",sequence, fragment);
        start = clock();
    }

    //Attente ACK
    int num_ack = 0;
    int somethingToRead = select(transfertSocket.socket+1, &set, NULL,NULL,&timeout);
    if(somethingToRead==-1){
        printf("No feedbacks received, resending fragment\n");
        sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
    }else if(FD_ISSET(transfertSocket.socket,&set)){
        int size = recvfrom (transfertSocket.socket, (char*)socket.buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&socket.clientAddress, &socket.clientSize);
        if (strncmp(socket.buffer,"ACK",3)==0){
            printf("\nReceived ACK -> %s\n",socket.buffer);
            num_ack = ack(socket.buffer); 
            printf("Number -> %i\n",num_ack);         
            if(sequence == num_ack){
                end = clock();
                //RTT(start, end);
            } else {
                printf("Seq do not match, resending\n");
                sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
            }
        }
    } else {
        printf("Ended timer\n");
        //Renvoi du segment
        sendFragment (socket, transfertSocket,sequence, rsize, fragment, start, end);
    }
    return 0;
}

int sendFile(struct Socket socket, struct Socket transfertSocket) {
    char* nom_fichier = socket.buffer;
    FILE* fichier_src;
    if(!(fichier_src = fopen(nom_fichier,"r"))){
        printf("Error opening file <%s> \n",nom_fichier);
        return EXIT_FAILURE;
    }
    printf("Starting fragmenting file %s\n", nom_fichier);

    int sequence = 1;
    char* segment= NULL;
    char* fragment = NULL;
    char* seq = NULL;
    fragment =(char*)malloc(SEG_SIZE+6);
    segment =(char*)malloc(SEG_SIZE);
    int data;

    clock_t start, end;

    while (!(feof(fichier_src))){
    //for (int a=0; a<300; a++){          //test    
        char* seq = genSeq(sequence);

        printf("\nSeq %s\n", seq);

        int rr= fread(segment,1,SEG_SIZE,fichier_src);
        printf("rr value %d\n", rr);
        data += rr;

        fragment = seq;
        memcpy(fragment+6, segment, rr);
        sendFragment(socket, transfertSocket, sequence, rr+6, fragment, start, end);
        free(seq);
        sequence++; 

    }

    fclose(fichier_src);

    if(sendto(transfertSocket.socket,"FIN", strlen("FIN"),0,(const struct sockaddr *)&socket.clientAddress, sizeof(socket.clientAddress)) < 0){
        printf("Couldnt send last message in file socket\n");
    }

    return data;
}


int main (int argc, char *argv[]) {
    //Temps d'execution et débit
    clock_t execStart, execEnd;

    if(!(argc == 2)){
        perror("Missing port as first argument\n");
        return -1;
    }

    struct Socket serverSocket = generateSocket(argv[1]);
    printf("Server started, listening on port %s\nAwaiting client connection\n",argv[1]);
    
    int messageSize = recvfrom(serverSocket.socket, (char *) &serverSocket.buffer, RCVSIZE, MSG_WAITALL, (struct sockaddr*)&serverSocket.clientAddress, &serverSocket.clientSize);
    if (messageSize == -1) {
        printf("Received empty message on server socket\n");
        return 0;
    } else {
        printf("Succesfully received msg of length %i\n", messageSize);
    }

    serverSocket.buffer[messageSize]= '\0';

    int generatedPort = genPort(serverSocket.port);
    char portChar[4];
    sprintf(portChar, "%d", generatedPort);

    if(strcmp(serverSocket.buffer,"SYN") == 0) {
        printf("File socket port will be %i\n", generatedPort);

        char ACKMessage[20] = "SYN-ACK";
        strcat(ACKMessage, portChar);
        printf("Sending ACK with port to client: %s \n", ACKMessage);

        if(sendto(serverSocket.socket,ACKMessage, strlen(ACKMessage),0,(const struct sockaddr *)&serverSocket.clientAddress, sizeof(serverSocket.clientAddress)) < 0){
            printf("Couldnt send ACK\n");
        }

    }

    messageSize = recvfrom (serverSocket.socket, (char*)serverSocket.buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&serverSocket.clientAddress, &serverSocket.clientSize);
    if (messageSize < 0) {
        printf("Received empty message on server socket\n");
        return 0;
    } else {
        printf("Succesfully received msg of length %i\n", messageSize);
    }

    serverSocket.buffer[messageSize]= '\0';

    if(strcmp(serverSocket.buffer,"ACK") == 0) {
        execStart = clock();
        struct Socket fileTransfertSocket = generateSocket(portChar);

        messageSize = recvfrom (fileTransfertSocket.socket, (char*)serverSocket.buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&serverSocket.clientAddress, &serverSocket.clientSize);
        if(messageSize < 0){
            printf("Received empty message on file socket\n");
            return 0;
        } else printf("Succesfully received msg of length %i\n", messageSize);

        serverSocket.buffer[messageSize]= '\0';

        int fileSize = sendFile(serverSocket,fileTransfertSocket);

        execEnd = clock();
        execTime(execStart,execEnd, fileSize);

    }
    
    close(serverSocket.socket);
    return 0;
}
