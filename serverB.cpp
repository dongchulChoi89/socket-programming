/*
 Socket Programming
 Dongchul Choi(USC ID: 7806678422) EE450 Session 2
 The basic overall structures and codes related socket programming are from 'Beej-Guide-Network-Programming'
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

#define MAXBUFLEN 1024
#define LOCALHOST "127.0.0.1"
#define PORT_SERVER_B "22422"
#define PORT_AWS_UDP "23422"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// roundTo2decimal function when used for calculationg delay
double roundTo2decimal(double original);

int main(void)
{
    // Print the monitor
    cout << "The Server B is up and running using UDP on port "<< PORT_SERVER_B << "." << endl;
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
    while (1) {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP
        if ((rv = getaddrinfo(NULL, PORT_SERVER_B, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("server B: socket");
                continue;
            }
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("server B: bind");
                continue;
            }
            break;
        }
        if (p == NULL) {
            fprintf(stderr, "server B: failed to bind socket\n");
            return 2;
        }
        freeaddrinfo(servinfo);
        
        addr_len = sizeof their_addr;
        
        // Receive from AWS
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        
        // Get single string and seprate it by '/'
        // Save each segment into the vector of 'info'
        string single(buf);
        vector<string> info;
        size_t pos = single.find("/");
        string tmp = single;
        
        while(pos != string::npos){
            single.erase(pos);
            tmp.erase(0, pos+1);
            pos = tmp.find("/");
            info.push_back(single);
            single = tmp;
        }
        // Index Information of 'info':
        // idx 0: prop, idx 1: trans, idx [2:info.size()-1]: dist
        // Save elements into each proper data type and data structures
        string propS = info[0];
        string transS = info[1];
        string fileSizeS = info[2];
        // Remove no more useful elements from the 'info' vector
        info.erase(info.begin());
        info.erase(info.begin());
        info.erase(info.begin());
        
        // Convert each element of the vector into a temporary array
        // Then, using the index of the temporary array, assing elements into the proper data type and structures
        // Index Information:
        // idx [0:distTmpSize/2-1]: distIdx, idx [distTmpSize/2:distTmpSize-1]: distVal
        int distTmp[info.size()];
        for (int i=0; i<info.size(); i++) {
            distTmp[i] = atoi(info[i].c_str());
        }
        int distTmpSize = sizeof(distTmp)/sizeof(distTmp[0]);
        int distIdx[distTmpSize/2];
        for (int i=0; i<distTmpSize/2; i++) {
            distIdx[i] = distTmp[i];
        }
        int distVal[distTmpSize/2];
        for (int i=distTmpSize/2; i<distTmpSize; i++) {
            int idx = i-distTmpSize/2;
            distVal[idx] = distTmp[i];
        }
        // Convert string into proper data type
        double prop = atof(propS.c_str()); // km/s
        double trans = atof(transS.c_str()); // bytes/s
        double fileSize = atof(fileSizeS.c_str()); // bit
        // Use the size of distIdx array as standards for others
        // Each array has same length of distIdx array because the index of each properties(i.e. Tt, Tp, delay ..) are corresponds to the distance array
        int distArrSize = sizeof(distIdx)/sizeof(distIdx[0]); // size of distIdx array
        
        // Print the monitor
        cout << "The Server B has received data for calculation: " << endl;
        cout << fixed << setprecision(2) << "* Propagation speed: "<< prop <<" km/s;" << endl;
        cout << fixed << setprecision(2) << "* Transmission speed "<< trans << " Bytes/s;" << endl;
        for (int i=0; i<distArrSize; i++) {
            cout << "* Path length for destination "<< distIdx[i] <<": " << distVal[i] << ";" << endl;
        }
        
        close(sockfd);
        
        // Phase 2. Calculation
        // the unit of result(i.e. dProp, dTrans, delay) is Seconds
        /*
         double prop // km/s
         double trans // bytes/s
         double fileSize // bit
         int distVal[] // km
         */
        double dProp[distArrSize]; // s
        double dTrans[distArrSize]; // s
        double delay[distArrSize]; // s
        
        for (int i=0; i<distArrSize; i++) {
            dProp[i] = distVal[i] / prop;
            dTrans[i] = fileSize / (trans * 8);
            delay[i] = dTrans[i] + dProp[i];
            // after calculations, round up
            dProp[i] = roundTo2decimal(dProp[i]);
            dTrans[i] = roundTo2decimal(dTrans[i]);
            delay[i] = roundTo2decimal(delay[i]);
        }
        // Print the monitor
        cout << "The Server B has finished the calculation of the delays:" << endl;
        cout << "----------------------------------" << endl;
        cout << "Destination\t\tDelay" << endl;
        cout << "----------------------------------" << endl;
        for (int i=0; i<distArrSize; i++) {
            cout << fixed << setprecision(2) << distIdx[i] << "\t\t\t" << delay[i] << endl;
        }
        cout << "----------------------------------" << endl;
        
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        if ((rv = getaddrinfo(LOCALHOST, PORT_AWS_UDP, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            
            return 1;
        }
        // loop through all the results and make a socket
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("server B: socket");
                continue;
            }
            break;
        }
        if (p == NULL) {
            fprintf(stderr, "server B: failed to create socket\n");
            return 2;
        }
        // Send to AWS
        // Make single string token to send
        string tokenToAws = "";
        ostringstream sstream;
        for (int i=0; i<distArrSize; i++)
        {
            sstream << fixed << setprecision(2) << dTrans[i];
            string sstreamTmp(sstream.str());
            sstream.str("");
            tokenToAws += sstreamTmp + "/";
        }
        for (int i=0; i<distArrSize; i++)
        {
            sstream << fixed << setprecision(2) << dProp[i];
            string sstreamTmp(sstream.str());
            sstream.str("");
            tokenToAws += sstreamTmp + "/";
        }
        for (int i=0; i<distArrSize; i++)
        {
            sstream << fixed << setprecision(2) << delay[i];
            string sstreamTmp(sstream.str());
            sstream.str("");
            tokenToAws += sstreamTmp + "/";
        }
        strcpy(buf, tokenToAws.c_str());
        
        if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
                               p->ai_addr, p->ai_addrlen)) == -1) {
            perror("server B: sendto");
            exit(1);
        }
        freeaddrinfo(servinfo);
        
        // Print the monitor
        cout << "The Server B has finished sending the output to AWS" << endl;
        close(sockfd);
        
    }
    return 0;
}

// roundTo2decimal function when used for calculationg delay
double roundTo2decimal(double original){
    return (int)(original * 100 + 0.5 ) / 100.00;
}
