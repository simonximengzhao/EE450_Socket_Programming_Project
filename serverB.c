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

#define UDP_PORT "31936"

#define MAXBUFLEN 100 //maximum buffer length for the packets

//get sockaddr, IPv4 or IPv6 from Beej's Tutorial
void *get_in_addr(struct sockaddr *sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//remove the duplicate entries in an array
int duplicate_remov(int arr[], int size){
    int i,j,k; //loop variables

    for(i=0; i<size; i++){
        for(j=i+1; j<size; j++){
            if(arr[i] == arr[j] && (arr[i] != -1 || arr[j] != -1)){ //if there's a duplicate
                for(k=j; k<size; k++){
                    arr[k] = arr[k + 1]; //delete the duplicate
                }
            size--; //decrement size
            j--; //shift the element and not increment j
            }
        }
    }
}

//count the number of integers in an array >=0
int countP(int* arr, int n)
{
    int pos_count = 0;
    int i;
    for (i = 0; i < n; i++) {
        if (arr[i] >= 0)
            pos_count++;
    }
    return pos_count;
}

//find the minimum of two numbers
int min(int a, int b)
{
    return b ^ ((a ^ b) & -(a < b));
}

//count the number of countries on the data list
int country_count(FILE *fp){
    char countryCount = 0; //total number of countries
    char datafile[255]; //storing the line of reading

    //finding out the number of countries
    while(!feof(fp)){
      fscanf(fp, "%s", datafile);
      int number = atoi(datafile);
      if (number == 0 && datafile[0] != '0'){
        countryCount++;
      }
    }

    return countryCount; //returns the total number of countries
}

int main(void) {

    //initialize UDP server socket from Beej's Tutorial
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p -> ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

    //processing data2.txt
    FILE *data;
    data = fopen("data2.txt","r"); //reading the txt file
    char datafile[255];
    char countryCount = country_count(data); //the total number of countries
    int i = 0, j = 0, k = 0; //counters
    char ws; //white space character checker
    rewind(data);

    //create the list of country names
    char countryName[countryCount+1][21]; //the names of the countries
    while(!feof(data)){
      fscanf(data, "%s", datafile);
      int number = atoi(datafile);
      if (number == 0 && datafile[0] != '0'){
        strcpy(countryName[i], datafile); //copy the country name to the list
        memmove(countryName[i]+1, countryName[i], sizeof(countryName[i]));
        countryName[i][0] = '1'; //insert a 0 in front of country name to indicate it's from server B
        i++;
      }
    }

    countryName[countryCount][0] = '2'; //add termination signal at the end of the array

    rewind(data);
    int user_num_per_country[countryCount]; //array of number of users per country
    int user_count = 0; //maximum number of users in data

    //find the total number of users per country and store in an array
    fscanf(data, "%[^\n]%*c", datafile); //scan the first line of data2.txt
    while(!feof(data)){
      fscanf(data, "%[^\n]%*c", datafile);
      int num = atoi(datafile);
      if(num != 0 || (datafile[0] == '0' && num == 0)){
        user_count++; // increment country count
        user_num_per_country[j] = user_count; //update user count per country
      }
      else if (num == 0 && datafile[0] != '0'){

        user_count = 0;
        j++;
      }
    }

    //create a new user data 3D array with initializing all -1
    int userData[10][100][150]; //userData[A][B][C], A= number of countries, B=C=number of users
    int ii=0, jj=0, kk=0; //counters
    for (ii=0;ii<10;ii++){
      for (jj=0;jj<100;jj++){
        for (kk=0;kk<150;kk++){
          userData[ii][jj][kk] = -1;
        }
      }
    }

    rewind(data);
    //loop through data and compose the 3D array
    fscanf(data, "%[^\n]%*c", datafile); //scan the first line of data2.txt
    int c=0, r=0, w=0; //c=country, r=row, w=word
    while(!feof(data)){ //scan until the end of file
      fscanf(data, "%s%c", datafile, &ws);
      if (ws == '\n') {
              if (atoi(datafile) == 0 && datafile[0] != '0'){
                c++; // increment c if there's new country
                r = 0; //reset r
                w = 0; //reset w
              }
              else {
                userData[c][r][w] = atoi(datafile); //store the user id
                r++; // increment r for a new line of data
                w = 0; // reset w for new line
              }
      }
      else{
        userData[c][r][w] = atoi(datafile); //store the user id
        w++; //increment w
      }
    }

    //get rid of the duplicate entries
    for(i=0; i<10; i++){
        for(j=0; j<100; j++){
            duplicate_remov(userData[i][j], 150);
        }
    }

    fclose(data);

    //booting up the server completed
    printf("The server B is up and running using UDP on port "UDP_PORT"\n\n");

    //forwarding country list to the main server
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf[numbytes] = '\0';
	//printf("listener: packet contains \"%s\"\n", buf);

    for (i=0; i<(countryCount+1);i++){
        char *msg = countryName[i];
        if ((numbytes = sendto(sockfd, msg, strlen(msg), 0,
			 (struct sockaddr *)&their_addr, addr_len)) == -1) {
		perror("talker: sendto");
		exit(1);
        }
    }

    printf("The server B has sent a country list to Main Server\n\n");

    //main loop:
    while(1){
        int userRequest; //the requested user id
        int countryIndex = 0; //the index of the requested country
        int userIndex = 0; //the index of the requested user
        int ufound = 0; //to indicate if the user id is found or not
        addr_len = sizeof their_addr;

        //receive the country name and user id
        if ((numbytes = recvfrom(sockfd, &userRequest, sizeof(int) , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        char *countryRequest = buf; //here's the country request
        char updatedRequest[] = "1";

        printf("The server B has received request for finding possible friends of User %d in %s\n\n", userRequest, countryRequest);

        strcat(updatedRequest,countryRequest); //insert a 1 in front of country name to indicate it's from server B

        //Now searching for the requested user id from the requested country
        //First, find the index on the country list
        for(countryIndex=0; countryIndex<countryCount;countryIndex++){
            if(strcmp(updatedRequest, countryName[countryIndex])==0){
                break;
            }
        }

        //find the user index
        for(userIndex=0; userIndex<user_num_per_country[countryIndex]; userIndex++){
            if(userRequest == userData[countryIndex][userIndex][0]){
                break;
            }
        }

        //Now go to the 3D matrix and find the user from the corresponding user
        for(i=0; i<100; i++){
            if (userData[countryIndex][i][0] == userRequest){
                ufound = 1; //we found the user id from the country
                break;
            }
        }

        //send reply back to the main server
        if (ufound == 0){
            printf("User %d does not show up in %s\n\n", userRequest, countryRequest);

            //send the reply to main server indicate user id not found
            if ((numbytes = sendto(sockfd, &ufound, sizeof(int), 0,
                 (struct sockaddr *)&their_addr, addr_len)) == -1) {
            perror("talker: sendto");
            exit(1);
            }

            printf("The server B has sent 'User %d not found' to Main Server\n\n", userRequest);
        }
        else{

            //send the reply back to main server to indicate user id found
            if ((numbytes = sendto(sockfd, &ufound, sizeof(int), 0,
                 (struct sockaddr *)&their_addr, addr_len)) == -1) {
            perror("talker: sendto");
            exit(1);
            }
            printf("The server B is searching for possible friends for User %d...\n\n", userRequest);

            //Recommendation process
            int connected = 0; //number of existing connected friend
            int common_neighbor = 0; //number of common friends with other people
            int max_neighbor = -1; //max number of common neighbor
            int friends = 0; //friend indicator
            int stranger[100]; //save the index of each stranger
            int num_connection[user_num_per_country[countryIndex]];
            int inc = 0, finc=0, sinc=0; //loop counters
            int new_friend; //suggested new friend
            int max_connection = 0; //maximum number of connection

            //find the number of connected friends
            for(i=0; i<user_num_per_country[countryIndex]; i++){
                while(userData[countryIndex][userIndex][inc] != -1){
                    if(userData[countryIndex][userIndex][inc] == userData[countryIndex][i][0] && userIndex != i){
                        connected = connected+1;
                    }
                    inc++;
                }
                inc = 0;
            }

            //find the index of the strangers
            for(i=0; i<user_num_per_country[countryIndex]; i++){
                if(userIndex != i){
                    for(j=0; j<user_num_per_country[countryIndex]; j++){
                        if(userData[countryIndex][userIndex][j] == userData[countryIndex][i][0] && userData[countryIndex][userIndex][j]){
                            friends = 1;//friends indicator
                        }
                        if(userData[countryIndex][userIndex][j] == -1){
                            break;
                        }
                    }
                    if(friends == 0){
                        stranger[sinc] = i; //update the stranger list
                        sinc++;
                    }
                    else{
                        friends = 0; //reset friends indicator
                    }
                }
            }

            if(connected == user_num_per_country[countryIndex] - 1){ //if the requested user is already connected to everyone, send signal -1
                printf("Here is the result: None\n\n");
                new_friend = -1;

                //send the result to main server
                if ((numbytes = sendto(sockfd, &new_friend, sizeof(int), 0,
                     (struct sockaddr *)&their_addr, addr_len)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                printf("The server B has sent the result to Main Server\n\n");
            }
            else if(connected == 0){ //if the requested user has no friend, suggest a friend with the most connections and at higher degree (lower user ID)
                //loop thru data to find a friend
                for(i=0; i<user_num_per_country[countryIndex]; i++){
                    num_connection[i] = countP(userData[countryIndex][i], user_num_per_country[countryIndex]); //find the number of connections
                    if(max_connection < num_connection[i]){
                        max_connection = num_connection[i];
                        new_friend = userData[countryIndex][i][0];
                    }
                    else if(max_connection == num_connection[i]){
                        new_friend = min(new_friend, userData[countryIndex][i][0]);
                    }
                }
                printf("Here is the result: User %d\n\n", new_friend);

                //send the result to main server
                if ((numbytes = sendto(sockfd, &new_friend, sizeof(int), 0,
                     (struct sockaddr *)&their_addr, addr_len)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                printf("The server B has sent the result to Main Server\n\n");
            }
            else{
                //find the common neighbor list between the requested id and each stranger
                for(i=0; i<(user_num_per_country[countryIndex] - connected - 1); i++){
                    for(j=0; j<user_num_per_country[countryIndex]; j++){
                        for(k=0; k<user_num_per_country[countryIndex]; k++){
                            if (userData[countryIndex][userIndex][j] == userData[countryIndex][stranger[i]][k]
                                && userData[countryIndex][userIndex][j] != -1 && userData[countryIndex][stranger[i]][k] != -1){
                                    common_neighbor++;
                                }
                        }
                    }
                    if(max_neighbor < common_neighbor){
                        max_neighbor = common_neighbor;
                        new_friend = userData[countryIndex][stranger[i]][0];
                    }
                    else if(max_neighbor == common_neighbor){
                        new_friend = min(new_friend, userData[countryIndex][stranger[i]][0]);
                    }
                    common_neighbor = 0;
                }

                printf("Here is the result: User %d\n", new_friend);

                //send the result to main server
                if ((numbytes = sendto(sockfd, &new_friend, sizeof(int), 0,
                     (struct sockaddr *)&their_addr, addr_len)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                printf("The server B has sent the result to Main Server\n\n");

            }
        }
    }
    return 0;
}
