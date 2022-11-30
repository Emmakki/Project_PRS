#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>

#define RCVSIZE 1024
#define SEG_SIZE 6

int genPort (int port) {
    int newPort = 1000 + (rand()%8999);
    while (newPort == port){
        newPort = 1000 + (rand()%8999);
    }
    printf("Nouveau Port pour le client1: %d \n",newPort);
    return newPort;
}

char* genSeq (int sequence){
    char* seq;
    //free(seq);
    seq = malloc(sizeof(char));
    char val[6];
    sprintf(val,"%d",sequence);
    //printf("%d\n",sequence);
    int j = strlen(val)-1;
    for(int i=5; i> 5-strlen(val);i--){
        seq[i]= val[j];
        j--;
    }
    //printf("Done\n");
    for(int i=5-strlen(val);i>=0;i--){
        seq[i] = '0';
    }
    //printf("Done 2\n");

    return seq;
}

clock_t RTT(clock_t start,clock_t end){
    clock_t diff = end - start;
    return diff;
}

int ack (char* buf){
    char* num = NULL;
    num = malloc(sizeof(char));
    int j = 0;
    for (int i=3; i<strlen(buf);i++){
        num[j] = buf[i];
        j++;
    }

    //printf("Avant conversion: %s\n",num);
    return atoi(num);
}

int main (int argc, char *argv[]) {

    if(!(argc == 2)){
        perror("erreur parametre d'entree: ./serveur1 NumeroPort\n");
        return -1;
    }

    //initialisation variables UDP
    struct sockaddr_in add, add_cli;
    int port= atoi(argv[1]);
    int valid= 1;
    memset((char*)&add,0,sizeof(add));
    memset((char*)&add_cli,0,sizeof(add_cli));

    int size_cli = sizeof(add_cli);
    char buffer[RCVSIZE];

    //creation socket
    int serveur = socket(AF_INET,SOCK_DGRAM,0);
    if(serveur<0){return -1;} else {printf("descripteur serveur: %d\n",serveur);}

    setsockopt(serveur, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

    add.sin_family= AF_INET;
    add.sin_port= htons(port);
    add.sin_addr.s_addr= htonl(INADDR_ANY);

    //init serveur
    if (bind(serveur, (struct sockaddr*) &add, sizeof(add)) == -1) {
        perror("Bind failed\n");
        close(serveur);
        return -1;
    }

    //Reception
   // printf("Attente message du client\n");
    int size_mes = recvfrom (serveur, (char*)buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&add_cli, &size_cli);
    if(size_mes<0){
        printf("don't receive\n");
        return 0;
    }
    buffer[size_mes]= '\0';
    //printf("taille: %d\n", size_mes);
    printf("Client: %s\n",buffer);
    int portClient;
    //Ouverture connection avec le client et envoi du nouveau port
    if (strcmp(buffer,"SYN")==0){
        printf("Generation du nouveau port\n");
        portClient = genPort(port);

        //creation message
        char portACK[20] = "SYN-ACK";
        char newPort[5];
        sprintf(newPort,"%d",portClient);
        //printf("%s\n", newPort);
        strcat(portACK,newPort);
        printf("%s \n", portACK);

        //envoie du message
        if(sendto(serveur,portACK, strlen(portACK),MSG_CONFIRM,(const struct sockaddr *)&add_cli, sizeof(add_cli)) < 0){
            printf("fail\n");
        }else{
            printf("message send\n");
        }

    }

    //Attente de la réception ACK
    size_mes = recvfrom (serveur, (char*)buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&add_cli, &size_cli);
    if(size_mes<0){
        printf("don't receive\n");
        return 0;
    }
    buffer[size_mes]= '\0';
    printf("%s\n",buffer);

    //Création d'une nouvelle socket pour le client
    struct sockaddr_in cli;
    memset((char*)&cli,0,sizeof(cli));
    int serveur_cli1 = socket(AF_INET,SOCK_DGRAM,0);
    if(serveur_cli1<0){return -1;} else {printf("descripteur nouvelle socket: %d\n",serveur_cli1);}
    setsockopt(serveur_cli1, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
    cli.sin_family= AF_INET;
    cli.sin_port= htons(portClient);
    cli.sin_addr.s_addr= htonl(INADDR_ANY);
    if (bind(serveur_cli1, (struct sockaddr*) &cli, sizeof(cli)) == -1) {
        perror("Bind failed\n");
        close(serveur_cli1);
        return -1;
    }

    //Réception nom du fichier
    int size= recvfrom (serveur_cli1, (char*)buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&add_cli, &size_cli);
    if(size_mes<0){
        printf("don't receive\n");
        return 0;
    }
    buffer[size]= '\0';
    printf("Client: %s\n",buffer);

    //Traitement avec le client: envoi du fichier demandé

        //ouverture fichier souce
    char* nom_fichier = buffer;
    FILE* fichier_src;
    if(!(fichier_src = fopen(nom_fichier,"r"))){
        printf("Erreur ouverture du fichier <%s> \n",nom_fichier);
        return EXIT_FAILURE;
    }
    printf("ouverture fichier\n");

        //init num séquence
    int sequence = 0;
    char* seq = genSeq(sequence);

        //Fragmentation du document
    char* segment= NULL;
    char* fragment = NULL;
    int i = 0;
    fragment =(char*)malloc(SEG_SIZE);
    segment =(char*)malloc(SEG_SIZE);

        //Paramètre de gestion
    //init timer
    clock_t start, end;

    //init Slow Start
    int cwnd = 1;

    //while(!(feof(fichier_src))){
    for (int a=0; a<50; a++){

        //char slow [cwnd][SEG_SIZE] = {""};
        printf("\n");
        for(int j=0;j<cwnd;j++){
            if (!(feof(fichier_src))){

                sequence++;
                i++;
                seq = genSeq(sequence);
                printf("Sequence: %s\n", seq);

                fread(segment,1,SEG_SIZE,fichier_src);
                printf ("%d : %s \n",i,segment);

                //Création du message
                fragment = seq;
                strcat(fragment,segment);
                printf("%s \n", fragment);

                //Envoi du segment
                if(sendto(serveur_cli1,fragment, strlen(fragment),MSG_CONFIRM,(const struct sockaddr *)&add_cli, sizeof(add_cli)) < 0){
                    printf("fail\n");
                }else{
                    printf("message send\n");
                    start = clock();
                }
            }
        }

        //Attente réception ACK
        int size = recvfrom (serveur_cli1, (char*)buffer,RCVSIZE,MSG_WAITALL,(struct sockaddr *)&add_cli, &size_cli);
        if (strncmp(buffer,"ACK",3)==0){
            int num_ack = ack(buffer);
            printf("%s\n",buffer);
            //printf("%d\n",num_ack);
            end = clock();
            if(sequence == num_ack){
                cwnd +=1;
            }
        }

        clock_t rtt = RTT(start,end);

    }
    fclose(fichier_src);


        //Demande la fermeture du client
    if(sendto(serveur_cli1,"FIN", strlen("FIN"),MSG_CONFIRM,(const struct sockaddr *)&add_cli, sizeof(add_cli)) < 0){
        printf("fail\n");
    }

    
    close(serveur);
    return 0;
}

