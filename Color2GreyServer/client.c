#include "csapp.c"
#define Pmode S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
int main(int argc, char **argv)
{
    int size,count,outSize;
    int clientfd,ifd,port,oimgfd;
    char *host, buf[MAXLINE],ibuf[MAXLINE];
    rio_t rio,rio_i,rio_o;
    struct stat st;
    ifd=Open("go.jpg",O_RDWR, 0);
    oimgfd=Open("CLR2Grey.jpg",O_WRONLY|O_CREAT,Pmode);
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    //port = atoi(argv[2]);

    clientfd = Open_clientfd(host,argv[2]);
    Rio_readinitb(&rio, clientfd);
    Rio_readinitb(&rio_i,ifd);

    stat("go.jpg", &st);
    int fsize = st.st_size;

    sprintf(buf,"%d",fsize);
    printf("\nChosen Image file size of file%s",buf);
    Rio_writen(clientfd,buf,MAXLINE);

    count=0;
    while (1) {
        size=read(ifd,ibuf, MAXLINE);
        if(size<=0)
            break;
        write(clientfd,ibuf,size);
        count=count+size;
        //printf("Count%d",count);
    }
    Close(ifd);
    int n=0;
    int Greyfilesize;
    int remGreySize;
    if((n = Rio_readlineb(&rio, buf, 6)) != 0)
        Greyfilesize=atoi(buf);
    remGreySize=Greyfilesize;
    printf("\nProcessed File size%d",Greyfilesize);
    n=0;

    do{
        n =read(clientfd, buf, 256);
        // printf("Bytes read%d\n",n);
        count=count+n;
        write(oimgfd,buf,n);
        remGreySize=remGreySize-n;
        //printf("total %d bytes read",count);
        // printf("Bytes Remianing%d\n",rem);
    }while(remGreySize>0 && n>=0);


    Close(clientfd);

    Close(oimgfd);
    if(fork()==0)
    {
        execl("/home/adu/EOS/Project2/Clientserver/Color2GreyServer/displayImg","./displayImg","CLR2Grey.jpg",(char *)NULL);
    }
    int status,ret;
    ret=0;
    waitpid(-1,&status,0);
    if(WIFEXITED(status))
    {
        ret=WEXITSTATUS(status);
    }
    if(ret==0){
        printf("\nCompletedsucessfully");
    }
    else
    {
        printf("\nDisplay failedFailed");
    }
    exit(0);
}
