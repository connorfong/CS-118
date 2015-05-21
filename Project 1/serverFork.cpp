/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <cstdio>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include <fstream>
#include <map>
#include <ctime>
using namespace std;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */	
}

//@param
//val is not negative
string mItoa(size_t val)
{
   string res = "";
   do
   {
       char ch = '0' + val%10;
       res = ch + res;
       val /= 10;
   }while(val > 0);
   return res;
}

string composeTimeStr()
{
   time_t now = time(0);

   tm *ltm = localtime(&now);
   string wDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
   string mons[] = {"Jan", "Feb", "Wed", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   char buf[256];
   sprintf(buf, "Date: %s, %d %s %d %d:%d:%d GMT\n", 
	wDays[ltm->tm_wday].c_str(), 
	ltm->tm_mday, 
	mons[ltm->tm_mon].c_str(), 
	1900 + ltm->tm_year,
	ltm->tm_hour, 
	ltm->tm_min, 
	ltm->tm_sec);
   return string(buf);
}

//@param
//contentType support: htm, html, jpeg, jpg, gif
//respState include: "200 OK", "404 Not Found"
string genHeader(string contentType, string respState, size_t fLen)
{
   string headerStr = "HTTP/1.1 " + respState + "\n";
   headerStr += composeTimeStr();
   map<string, string> contentMap;
   contentMap["htm"] = "text/HTML";
   contentMap["html"] = "text/HTML";
   contentMap["jpeg"] = "image/jpeg";
   contentMap["jpg"] = "image/jpeg";
   contentMap["gif"] = "image/gif";
   contentMap["default"] = "application/octet-stream";
   if(contentMap.find(contentType) == contentMap.end())
	headerStr += "Content-Type: application/octet-stream";
   else
	headerStr += "Content-Type: " + contentMap[contentType] + "\n";
   headerStr += "Content-Length: " + string(mItoa(fLen)) + "\n\n";
   puts(headerStr.c_str());
   return headerStr;
}

void handle404(int sock)
{
	string errorPage = "<html><body>Error 404, Page Not Found~~!</body></html>";
	string headerStr = genHeader("html", "404 Not Found", errorPage.length());
	if( 0 > write(sock, headerStr.c_str(), headerStr.length()) )
	    error("ERROR writing to socket");
	if( 0 > write(sock, errorPage.c_str(), errorPage.length()+1) )
            error("ERROR writing to socket");
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n;
   char buffer[256];
      
   bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) 
	error("ERROR reading from socket");
   printf("Here is the message: %s\n",buffer);

   char *resBuf = NULL;
   int addrStart = 0;
   while(buffer[addrStart] != ' ') addrStart++; 
   while(buffer[addrStart] == ' ') addrStart++; 
   int addrEnd = addrStart;

   while(buffer[addrEnd+1] != ' ') 
       addrEnd++; 

   if(addrEnd > addrStart)
   {
        string fName(buffer + addrStart + 1, addrEnd - addrStart);
	ifstream inf(fName.c_str(), ios::in|ios::binary);
	
	int i;
	for(i=fName.length()-1; i>=0 && fName[i]!='.'; i--);
	string fType = "Default";
	if(i>=0)
	   fType = fName.substr(i+1, fName.length()-1-i); 
	
	if(inf)
	{
	   string resBuf((std::istreambuf_iterator<char>(inf)), std::istreambuf_iterator<char>());
	   string headerStr = genHeader(fType, "200 OK", resBuf.length());
	   n = write(sock, headerStr.c_str(), headerStr.length());
	   n = write(sock, resBuf.c_str(), resBuf.length() + 1);
	}
	else
	{
	   handle404(sock);
	}
   }
   else
   {
	handle404(sock);
   }
   
   if (n < 0) 
	error("ERROR writing to socket");
}
