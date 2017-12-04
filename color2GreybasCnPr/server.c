#include "csapp.c"
#define QUEUESIZE 2
#define LOOP 5
#define threadno 4
#define Pmode S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
void *ServHelp(void *args);
void *ServMain(void *args);
void echo(int connfd);
int hno;
pthread_mutex_t *hmut;
typedef struct {
    int buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    int des;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

int serving ;
void millisleep(int milliseconds)
{
    usleep(milliseconds * 1000);
}
void queueAdd (queue *q, int in)
{
    q->buf[q->tail] = in;
    //printf("\nadded :%d at tail :%d",in,q->tail);
    q->tail++;
    //printf("Qtail%d Qhead %d",q->tail,q->head);
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
    {
        q->full = 1;
        //printf("\n Que Full");
    }
    q->empty = 0;

    return;
}
queue *queueInit (int lfd)
{ 
    //printf("\ninitializing Buffer");
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);
    q->des=lfd;
    //printf("\ndescriptor %d",lfd);
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}
void queueDel (queue *q, int *out)
{
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

void echo(int connfd)
{    int count=0; 
int fsize,size;
struct stat st;
size_t n;
char buf[MAXLINE],bufGrey[MAXLINE];
rio_t rio,Grey;
int frclfdW,frclfdM,filsize,rem;
frclfdW=Open("fromclient.jpg",O_WRONLY|O_CREAT,Pmode);
Rio_readinitb(&rio, connfd);
if((n = Rio_readlineb(&rio, buf, 6)) != 0)
    filsize=atoi(buf);
rem=filsize;
printf("Recived Image File size%d\n",filsize);

do{
    n =read(connfd, buf, 256);
    // printf("Bytes read%d\n",n);
    count=count+n;
    write(frclfdW,buf,n);
    rem=rem-n;
    //printf("total %d bytes read",count);
    // printf("Bytes Remianing%d\n",rem);
}while(rem>0 && n>=0);
Close(frclfdW);
printf("Total %d bytes read\n",count);
printf("Processing image\n");
if(fork()==0)
{
    execl("~/eos/project/color2GreybasCnPr/im","./im","fromclient.jpg",(char *)NULL);
}
int status,ret;
ret=0;
waitpid(-1,&status,0);
if(WIFEXITED(status))
{
    ret=WEXITSTATUS(status);
}
if(ret==0){
    printf("Conversion Completed sucessfully\n");
}
else
{
    printf("Failed");
}
stat("Gray_Image.jpg", &st);
fsize = st.st_size;
sprintf(buf,"%d",fsize);
printf("\nSize of file to be transfered %s\n",buf);
Rio_writen(connfd,buf,MAXLINE);
frclfdM=Open("Gray_Image.jpg",O_RDONLY,0);
Rio_readinitb(&Grey,frclfdM);
count=0;
while (1) {

    size=read(frclfdM,bufGrey, MAXLINE);
    //printf("size%d",size)
    if(size<=0)
        break;
    write(connfd,bufGrey,size);
    count=count+size;
    printf("Count%d\n",count);
}
Close(frclfdM);
}
void *ServMain(void *q){
    printf("\nMainthread");
    struct hostent *hp;
    char *haddrp;
    int connfd[LOOP],clientlen;
    queue *fifo;
    struct sockaddr_in clientaddr;
    int lfd,i;
    fifo = (queue *)q;
    lfd=fifo->des;
    //printf("\n LFD %d",lfd);
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->full) {
            printf ("\nproducer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
            //printf ("\nwait Que emptied");
        }
        clientlen = sizeof(clientaddr);
        //printf("\nWaiting for connection...%d",i);
        connfd[i] = Accept(lfd, (SA *)&clientaddr, &clientlen); // Ronak add mallock calls and use this for making name of image files to be stored
        hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                           sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        haddrp = inet_ntoa(clientaddr.sin_addr);
        printf("\nServer connected to %s (%s)\n", hp->h_name, haddrp);
        //printf("\nConnceted Connection descriptior %d",connfd);
        //printf("\n adding into que no :%d",i);
        queueAdd (fifo, connfd[i]);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notEmpty);
        printf("\niteration %d complete",i);
        millisleep (100);
    }
}
void *ServHelp(void *q){
    //printf("\n helper thread");
    queue *fifo;
    int i, srvfd;
    fifo = (queue *)q;
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->empty) {
            printf ("\nconsumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        //printf ("\nwait released");
        queueDel (fifo, &srvfd);
        //printf("\nServing request for Conncector %d",srvfd);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
        //printf ("\n consumer: recieved %d.\n", srvfd);
        echo(srvfd);
        Close(srvfd);
        printf("\nconncetion closed for %d",srvfd);
        millisleep(100);
    }
    return (NULL);
}

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    listenfd = Open_listenfd(argv[1]);
    //printf("\n listenfd%d",listenfd);
    queue *fifo;
    int i;
    fifo = queueInit(listenfd);
    pthread_t maint,helpr[5];
    if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }
    //printf("\n Creating helper and main thread");

    pthread_create (&maint, NULL, ServMain, fifo);
    for(i=0;i<4;i++)
    {
        pthread_create (&helpr[i], NULL, ServHelp, fifo);
    }
    for(i=0;i<4;i++)
    {
        pthread_join (helpr[i], NULL);
    }
    pthread_join (maint, NULL);
    exit(0);
}
