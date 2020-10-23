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
#include <signal.h>

#define UDP_PORT "32936"        //my UDP port number
#define TCP_PORT "33936"        //my TCP port number
#define SERVER_A "30936"        //server A port number
#define SERVER_B "31936"        //server B port number

#define MAXDATASIZE 100         //max data size for tcp
#define MAXBUFLEN 100           //max buffer length for udp
#define BACKLOG 10              //max backlog #

void sigchld_handler(int s){
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

//get sockaddr, IPv4 or IPv6 from Beej's Tutorial
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {

    int numbytes, numbytesA, numbytesB, numbytes_udp; //number of data sent and received

    //initialize TCP socket from Beej's Tutorial
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int userid; //user id requested by the client

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, TCP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
       if ((sockfd = socket(p->ai_family, p->ai_socktype,
               p->ai_protocol)) == -1) {
           perror("server: socket");
           continue;
       }

       if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
               sizeof(int)) == -1) {
           perror("setsockopt");
           exit(1);
       }

       if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
           close(sockfd);
           perror("server: bind");
           continue;
       }

       break;
     }

     freeaddrinfo(servinfo); // all done with this structure

     if (p == NULL)  {
         fprintf(stderr, "server: failed to bind\n");
         exit(1);
     }

     if (listen(sockfd, BACKLOG) == -1) {
         perror("listen");
         exit(1);
     }

     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }

    //initialize UDP socket from Beej's Tutorial
    int sockfd_udp;
    struct addrinfo hints_udp, *servinfo_udp, *p_udp;
    int rv_udp;
    struct sockaddr_storage their_addr_udp;
    char buf_udp[MAXBUFLEN];
    socklen_t addr_len_udp;
    char s_udp[INET6_ADDRSTRLEN];

    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_UNSPEC;
    hints_udp.ai_socktype = SOCK_DGRAM;
    hints_udp.ai_flags = AI_PASSIVE;

    if((rv_udp = getaddrinfo(NULL, UDP_PORT, &hints_udp, &servinfo_udp)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_udp));
        return 1;
    }

    //loop through all the results and bind to the first we can
    for(p_udp = servinfo_udp; p_udp != NULL; p_udp = p_udp->ai_next) {
        if ((sockfd_udp = socket(p_udp->ai_family, p_udp->ai_socktype,
                        p_udp->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
        }

        if (bind(sockfd_udp, p_udp->ai_addr, p_udp -> ai_addrlen) == -1) {
                close(sockfd_udp);
                perror("listener: bind");
                continue;
        }

        break;
    }

    if (p_udp == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo_udp);

    //initialize server A UDP address from Beej's Tutorial
    struct addrinfo hints_udp_A, *servinfo_udp_A, *p_udp_A;
    int rv_udp_A;

    memset(&hints_udp_A, 0, sizeof hints_udp_A);
    hints_udp_A.ai_family = AF_UNSPEC;
    hints_udp_A.ai_socktype = SOCK_DGRAM;

    if((rv_udp_A = getaddrinfo("127.0.0.1", SERVER_A, &hints_udp_A, &servinfo_udp_A)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_udp_A));
        return 1;
    }

    p_udp_A = servinfo_udp_A; //UDP datagram will be sent here

    if (p_udp_A == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    //initialize server B UDP address from Beej's Tutorial
    struct addrinfo hints_udp_B, *servinfo_udp_B, *p_udp_B;
    int rv_udp_B;

    memset(&hints_udp_B, 0, sizeof hints_udp_B);
    hints_udp_B.ai_family = AF_UNSPEC;
    hints_udp_B.ai_socktype = SOCK_DGRAM;

    if ((rv_udp_B = getaddrinfo("127.0.0.1", SERVER_B, &hints_udp_B, &servinfo_udp_B)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_udp_B));
      return 1;
    }

    p_udp_B = servinfo_udp_B; //UDP datagram will be sent here

    if (p_udp_B == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    //boot up the main server
    printf("The Main server is up and running\n");

    //receiving the country lists from the two servers
    char server_A_countries[10][21]; //create an array to store countries from server A
    char server_B_countries[10][21]; //create an array to store countries from server B
    int term = 0; //stop receiving country names from the two servers
    char *msg = "Send me the country list";
    int serverA_count=0, serverB_count=0; //count the number of countries per server
    int i=0;

    //requesting the country lists from both servers
    if ((numbytes_udp = sendto(sockfd_udp, msg, strlen(msg), 0,
			 p_udp_A->ai_addr, p_udp_A->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
    if ((numbytes_udp = sendto(sockfd_udp, msg, strlen(msg), 0,
			 p_udp_B->ai_addr, p_udp_B->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	//receiving the country names from each server using UDP, adapted from Beej's Tutorial
	while (term != 2){
        addr_len_udp = sizeof(their_addr_udp);
        if ((numbytes = recvfrom(sockfd_udp, buf_udp, MAXBUFLEN-1, 0,
                                 (struct sockaddr *)&their_addr_udp, &addr_len_udp)) == -1){
            perror("recvfrom");
            exit(1);
        }
        buf_udp[numbytes] = '\0';
        if (buf_udp[0] == '0'){
            strcpy(server_A_countries[serverA_count], buf_udp);
            memmove(server_A_countries[serverA_count], server_A_countries[serverA_count]+1,
                    strlen(server_A_countries[serverA_count]+1) + 1);
            //printf("country name received: %s\n", server_A_countries[serverA_count]);
            serverA_count++;
        }
        else if (buf_udp[0] == '1'){
            strcpy(server_B_countries[serverB_count], buf_udp);
            memmove(server_B_countries[serverB_count], server_B_countries[serverB_count]+1,
                    strlen(server_B_countries[serverB_count]+1) + 1);
            //printf("country name received: %s\n", server_B_countries[serverB_count]);
            serverB_count++;
        }
        else if (buf_udp[0] == '2'){
            term++;
        }
    }

    printf("\nThe Main server has received the country list from server A using UDP over port "SERVER_A"\n");
    printf("\nThe Main server has received the country list from server B using UDP over port "SERVER_B"\n");

    //display the countries each server has
    printf("\nServer A\n");
    for (i=0; i<serverA_count; i++){
        printf("%s\n", server_A_countries[i]);
    }

    printf("\nServer B\n");
    for (i=0; i<serverB_count; i++){
        printf("%s\n", server_B_countries[i]);
    }

    //main loop - adapted from Beej's Tutorial
    while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		//printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

            //receiving request from client using TCP
            if (recv(new_fd, &userid, sizeof(int), 0) == -1) {
                perror("recv");
            }
            int userRequest = userid; //here's the user request
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
            }
            buf[numbytes] = '\0';
            char *countryRequest = buf; //here's the country request

			printf("\nThe Main server has received the request on User %d in %s from the client using TCP over port "TCP_PORT"\n", userRequest, countryRequest);

            //check if the received country name is from either server
            int found = 0, ufound, new_friend;

            for(i=0; i<serverA_count;i++){
                if(strcmp(countryRequest, server_A_countries[i])==0){
                    found = 1;
                    printf("\n%s shows up in server A\n\n", countryRequest);
                }
            }
            for(i=0; i<serverB_count;i++){
                if(strcmp(countryRequest, server_B_countries[i])==0){
                    found = 2;
                    printf("\n%s shows up in server B\n\n", countryRequest);
                }
            }
            //if country name not found, reply and close the socket
            //if country name found, contact the corresponding server for further steps
            if(found == 0){
                printf("\n%s does not show up in server A&B\n\n", countryRequest);
                printf("The Main Server has sent 'Country Name: Not Found' to the client using TCP over port "TCP_PORT"\n\n");

                if (send(new_fd, &found, sizeof(int), 0) == -1){
                    perror("send");
                }
            }
            else if(found == 1){
                //tell the client that country name exists
                if (send(new_fd, &found, sizeof(int), 0) == -1){
                    perror("send");
                }
                //send user id and country name to server A
                if (sendto(sockfd_udp, &userRequest, sizeof(int), 0,
                        p_udp_A->ai_addr, p_udp_A->ai_addrlen) == -1) {
                    perror("talker: sendto");
                }
                if (sendto(sockfd_udp, countryRequest, strlen(countryRequest), 0,
                        p_udp_A->ai_addr, p_udp_A->ai_addrlen) == -1) {
                    perror("talker: sendto");
                }
                printf("The Main Server has sent request from User %d to server A using UDP over port "UDP_PORT"\n\n", userRequest);

                //receive user id information from server A
                if ((numbytes = recvfrom(sockfd_udp, &ufound, sizeof(int), 0,
                    (struct sockaddr *)&their_addr_udp, &addr_len_udp)) == -1){
                    perror("recvfrom");
                    exit(1);
                }

                if(ufound == 0){
                    printf("The Main server has received 'User ID: Not Found' from server A\n\n");

                    //send user not found reply to client
                    if (send(new_fd, &ufound, sizeof(int), 0) == -1){
                        perror("send");
                    }
                    printf("The Main Server has sent error to client using TCP over port "TCP_PORT"\n\n");
                }
                else{
                    //send user found reply to client
                    if (send(new_fd, &ufound, sizeof(int), 0) == -1){
                        perror("send");
                    }

                    //receive search result from server A
                    if ((numbytes = recvfrom(sockfd_udp, &new_friend, sizeof(int), 0,
                        (struct sockaddr *)&their_addr_udp, &addr_len_udp)) == -1){
                        perror("recvfrom");
                        exit(1);
                    }
                    if(new_friend == -1){ //if result is none
                        printf("The Main Server has received searching result of None from server A\n\n");
                    }
                    else{ //if there is a result
                        printf("The Main Server has received searching result of User %d from server A\n\n", new_friend);
                    }

                    //send the search result to client
                    if (send(new_fd, &new_friend, sizeof(int), 0) == -1){
                        perror("send");
                    }
                    printf("The Main Server has sent searching result to client using TCP over port "TCP_PORT"\n\n");
                }

            }
            else if(found == 2){
                //tell the client that country name exists
                if (send(new_fd, &found, sizeof(int), 0) == -1){
                    perror("send");
                }
                //send user id and country name to server B
                if (sendto(sockfd_udp, &userRequest, sizeof(int), 0,
                        p_udp_B->ai_addr, p_udp_B->ai_addrlen) == -1) {
                    perror("talker: sendto");
                }
                if (sendto(sockfd_udp, countryRequest, strlen(countryRequest), 0,
                        p_udp_B->ai_addr, p_udp_B->ai_addrlen) == -1) {
                    perror("talker: sendto");
                }
                printf("The Main Server has sent request from User %d to server B using UDP over port "UDP_PORT"\n\n", userRequest);

                //receive user id information from server B
                if ((numbytes = recvfrom(sockfd_udp, &ufound, sizeof(int), 0,
                    (struct sockaddr *)&their_addr_udp, &addr_len_udp)) == -1){
                    perror("recvfrom");
                    exit(1);
                }

                if(ufound == 0){
                    printf("The Main server has received 'User ID: Not Found' from server B\n\n");

                    //send user not found reply to client
                    if (send(new_fd, &ufound, sizeof(int), 0) == -1){
                        perror("send");
                    }
                    printf("The Main Server has sent error to client using TCP over port "TCP_PORT"\n\n");
                }
                else{
                    //send user found reply to client
                    if (send(new_fd, &ufound, sizeof(int), 0) == -1){
                        perror("send");
                    }

                    //receive search result from server B
                    if ((numbytes = recvfrom(sockfd_udp, &new_friend, sizeof(int), 0,
                        (struct sockaddr *)&their_addr_udp, &addr_len_udp)) == -1){
                        perror("recvfrom");
                        exit(1);
                    }
                    if(new_friend == -1){ //if result is none
                        printf("The Main Server has received searching result of None from server B\n\n");
                    }
                    else{ //if there is a result
                        printf("The Main Server has received searching result of User %d from server B\n\n", new_friend);
                    }

                    //send the search result to client
                    if (send(new_fd, &new_friend, sizeof(int), 0) == -1){
                        perror("send");
                    }
                    printf("The Main Server has sent searching result to client using TCP over port "TCP_PORT"\n\n");
                }

            }
			//close the socket
			close(new_fd);
			exit(0);
		}
	}

    return 0;
}
