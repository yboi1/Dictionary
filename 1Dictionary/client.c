#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define N 32
#define R 1 // Register
#define L 2 // Login
#define Q 3 // query
#define H 4 // History

typedef struct
{
    int type;
    char name[N];
    char data[256];
} MSG;

int Do_register(int sockfd, MSG *msg)
{
    msg->type = R;
    
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();

    printf("Input passwd:");
    scanf("%s", msg->data);

    if(send(sockfd, msg, sizeof(MSG), 0) < 0)
    {
        printf("Fail to send.\n");
        return -1;
    }

    if(recv(sockfd, msg, sizeof(MSG), 0) < 0)
    {
        printf("Fail to resv.\n");
        return -1;
    }
    // Ok! or usr already exist
    printf("%s\n", msg->data);

    return 0;

}

int Do_login(int sockfd, MSG *msg)
{
    msg->type = L;
    
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();

    printf("Input passwd:");
    scanf("%s", msg->data);
    //向服务器发送消息，等待服务器返回登录结果
    if(send(sockfd, msg, sizeof(MSG), 0) < 0)
    {
        printf("Fail to send.\n");
        return -1;
    }
    //接受服务器返回的消息，验证查询/登录是否成功
    if(recv(sockfd, msg, sizeof(MSG), 0) < 0)
    {
        printf("Fail to resv.\n");
        return -1;
    }
    //验证返回的消息
    if(strncmp(msg->data, "OK", 3) == 0)
    {
        printf("Login ok.\n");
        return 1;
    }
    else
    {
        printf("%s\n",msg->data);
    }
    
    return 0;

}
int Do_query(int sockfd, MSG *msg)
{
    msg->type = Q;
    puts("--------------");

    while(1)
    {
        printf("Input word:");
        scanf("%s", msg->data);
        getchar();

        //输入#号退出
        if(strncmp(msg->data,"#", 1) == 0)
        break;
        //向服务器发送数据，等待回复
        if(send(sockfd, msg, sizeof(MSG), 0) < 0)
        {
            printf("Fail to send.\n");
            return -1;
        }
        //接受服务器数据
        if(recv(sockfd, msg, sizeof(MSG), 0) < 0)
        {
            printf("Fail to recv.\n");
            return -1;
        }
        //将接收到的单词注释打印出来
        printf("%s\n",msg->data);
        
    }
    return 0;
}
int Do_history(int sockfd, MSG *msg)
{
    msg->type = H;

    send(sockfd, msg, sizeof(MSG), 0);

    while(1)
    {
        recv(sockfd, msg, sizeof(MSG), 0);
        
        if(msg->data[0] == '\0')
            break;

        printf("%s\n",msg->data);
        
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serveraddr;
    int n;
    MSG msg;
    if (argc != 3)
    {
        printf("Not Enough../n");
        return -1;
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("fail to socket.\n");
        return -1;
    }

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(argv[2]));

    if ((connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        perror("fail to connect.\n");
        return -1;
    }

    while (1)
    {
        printf("*******************************************************\n");
        printf("*1.Register           2.Login           3.Quit        *\n");
        printf("*******************************************************\n");
        printf("Please choose:");

        scanf("%d", &n);
        getchar();

        switch (n)
        {
        case 1:
            Do_register(sockfd, &msg);
            break;
        case 2:
            if (Do_login(sockfd, &msg) == 1)
            {
                goto next;
            }
            break;
        case 3:
            close(sockfd);
            exit(0);
        default:
            printf("Invalied data cmd. \n");
        }
    }
next:
    while (1)
    {
        printf("*******************************************************\n");
        printf("*1.query_word      2.histort_record        3.Quit     *\n");
        printf("*******************************************************\n");
        printf("Please choose:");
        scanf("%d",&n);
        getchar();

        switch(n)
        {
        case 1:
            Do_query(sockfd, &msg);
            break;
        case 2:
            Do_history(sockfd, &msg);
            break;
        case 3:
            close(sockfd);
            exit(0);
            break;
        default:
            printf("Invalied data cmd.\n");
        }
    }
    return 0;
}