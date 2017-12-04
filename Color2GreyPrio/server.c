#include "csapp.c"
#define Pmode S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define QUEUESIZE 6
#define LOOP 10
#define threadno 3
void *ServHelp(void *args);
void *ServMain(void *args);
void echo(int connfd);
int hno;
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
    q->tail++;
    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
    {
        q->full = 1;
        printf("\n Que Full");
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
{
    int count=0;
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
        execl("/home/ronak/eos/project/Color2GreyPrio/im","./im","fromclient.jpg",(char *)NULL);
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
    printf("\nMainthread waiting for clients");
    struct hostent *hp;
    char *haddrp;
//    int connfd[LOOP],clientlen;
    int *connfd,clientlen;
    queue *fifo;
    struct sockaddr_in clientaddr;
    int lfd,i;
    fifo = (queue *)q;
    lfd=fifo->des;
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->full) {
            printf ("\nproducer: queue FULL.\n");
            pthread_cond_wait (fifo->notFull, fifo->mut);
        }
        clientlen = sizeof(clientaddr);
        
        connfd = Malloc(sizeof(int));
        printf("Connfd %p", connfd);
        *connfd = Accept(lfd, (SA *)&clientaddr, &clientlen);
        hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                           sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        haddrp = inet_ntoa(clientaddr.sin_addr);
        printf("\nServer connected to %s (%s)\n", hp->h_name, haddrp);
        queueAdd (fifo, *connfd);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notEmpty);
//        millisleep (30);
    }
}
void *ServHelp(void *q){
    queue *fifo;
    int i, srvfd;
    fifo = (queue *)q;
    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock (fifo->mut);
        while (fifo->empty) {
            printf ("\nconsumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &srvfd);
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);
        echo(srvfd);
        Close(srvfd);
        printf("\nconncetion closed for %d",srvfd);
        millisleep(10);
    }
    return (NULL);
}

int main(int argc, char **argv)
{
    int listenfd,min_priority, policy;;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);
    queue *fifo;
    int i;

    fifo = queueInit(listenfd);
    pthread_t maint,helpr[3];
    struct sched_param my_param;
    pthread_attr_t maint_attr;
    pthread_attr_t helpr_attr;
    my_param.sched_priority = sched_get_priority_min(SCHED_FIFO);
    min_priority = my_param.sched_priority;
    pthread_setschedparam(pthread_self(), SCHED_RR, &my_param);
    pthread_getschedparam (pthread_self(), &policy, &my_param);
    pthread_attr_init(&maint_attr);
    pthread_attr_init(&helpr_attr);
    pthread_attr_setinheritsched(&maint_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&helpr_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&maint_attr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&helpr_attr, SCHED_FIFO);
    my_param.sched_priority = min_priority;
    pthread_attr_setschedparam(&helpr_attr, &my_param);
    my_param.sched_priority = min_priority + 6;
    pthread_attr_setschedparam(&maint_attr, &my_param);


    if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }
    pthread_create (&maint,&maint_attr, ServMain, fifo);
    for(i=0;i<3;i++)
    {
        pthread_create (&helpr[i],&helpr_attr, ServHelp,fifo);
    }
    for(i=0;i<3;i++)
    {
        pthread_join (helpr[i], NULL);
    }
    pthread_join (maint, NULL);
    pthread_attr_destroy(&maint_attr);
    pthread_attr_destroy(&helpr_attr);
    exit(0);
}
