#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT "33936" //main server TCP port number

//get sockaddr, IPv4 or IPv6 from Beej's Tutorial
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    //initialize TCP socket from Beej's Tutorial
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char countryName [50]; //country name input from the user
    int userid; //user id input from the user
    int found, ufound; //indicator of whether country/user id is found or not
    int new_friend; //the search result

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //booting up client
    printf("The client is up and running\n\n");
    while(1){
        //start socket connection
        if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("client: connect");
                close(sockfd);
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            return 2;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);

        freeaddrinfo(servinfo);

        //User inputs
        printf("Please enter the User ID: ");
        scanf("%d", &userid);
        printf("Please enter the Country Name: ");
        scanf("%s", countryName);

        //send the country name and user id to the main server using TCP
        if(send(sockfd, &userid, sizeof(int), 0) == -1){
            perror("send");
        }
        if(send(sockfd, countryName, strlen(countryName), 0) == -1){
            perror("send");
        }
        printf("\nThe client has sent User %d and %s to Main server using TCP\n\n", userid, countryName);

        //receive reply from main server to see if the country exist or not
        if (recv(sockfd, &found, sizeof(int), 0) == -1) {
                perror("recv");
            }

        if (found == 0){ //The country name is not found
            printf("%s not found\n\n", countryName);
            close(sockfd);
            printf("-----Start a new request-----\n");
            continue;
        }
        else{ //country found

            //receive reply form main server to see if the user id exist or not
            if (recv(sockfd, &ufound, sizeof(int), 0) == -1) {
                perror("recv");
            }

            //Determine if user id is found or not
            if (ufound == 0){
                printf("User %d not found\n\n", userid);
                close(sockfd);
                printf("-----Start a new request-----\n");
                continue;
            }
            else{
                //receive the result from main server
                if (recv(sockfd, &new_friend, sizeof(int), 0) == -1) {
                    perror("recv");
                }
                if(new_friend == -1){
                    printf("The client has received result from Main Server: None\n\n");
                }
                else{
                    printf("The client has received result from Main Server: User %d is a possible friend of User %d in %s\n\n", new_friend, userid, countryName);
                }
            }
        }

        close(sockfd);
        printf("-----Start a new request-----\n");
    }
    return 0;
}

