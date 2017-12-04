#include "csapp.c"
#define Pmode S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct hostent *hp;
    char *haddrp;
    int count=0;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                       sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    haddrp = inet_ntoa(clientaddr.sin_addr);
    printf("server connected to %s (%s)\n", hp->h_name, haddrp);

    echo(connfd);
    Close(connfd);

    exit(0);

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
        execl("/home/adu/EOS/Project2/Clientserver/Color2Greythread/im","./im","fromclient.jpg",(char *)NULL);
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
