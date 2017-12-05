#include "csapp.c"
#define Pmode S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
int main(int argc, char **argv)
{
    int size,count,outSize;
    int clientfd,ifd,port,oimgfd;
    char *host, buf[MAXLINE],ibuf[MAXLINE],names[MAXLINE],dir[32],cwd[1024];
    rio_t rio,rio_i,rio_o;
    struct stat st;
    
    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port> <imageFileName>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    clientfd = Open_clientfd(host,argv[2]);
	sprintf(names,"%s","Gray");
	 sprintf(names,"%s_%d",names,clientfd);
	 sprintf(names,"%s%s",names,".jpg");
	 //printf("image:%s",names);
    oimgfd=Open(names,O_WRONLY|O_CREAT,Pmode);
    Rio_readinitb(&rio, clientfd); 
    ifd = Open(argv[3],O_RDWR, 0);
    Rio_readinitb(&rio_i,ifd);

    stat(argv[3], &st);
    int fsize = st.st_size;

    sprintf(buf,"%d",fsize);
    //printf("Chosen Image file size of file%s",buf);
    Rio_writen(clientfd,buf,MAXLINE);

    count=0;
    printf("Sending image to Server...\n");
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
    printf("Processed File size%d\n",Greyfilesize);
    n=0;

    do{
        n =read(clientfd, buf, 256);
        // printf("Bytes read%d\n",n);
        count=count+n;
        write(oimgfd,buf,n);
        remGreySize=remGreySize-n;
    }while(remGreySize>0 && n>=0);

    printf("Received image from server. Displaying image...\n");

    Close(clientfd);

    Close(oimgfd);
    getcwd(cwd, sizeof(cwd));
	sprintf(dir,"%s",cwd);
	sprintf(dir,"%s%s",dir,"/displayImg");
    if(fork()==0)
    {
       execl(dir,"./displayImg",argv[3],names,(char *)NULL);
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
