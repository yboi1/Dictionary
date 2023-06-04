#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//多进程
#include <unistd.h>
#include <signal.h>

#include<sqlite3.h>

#include<time.h>

#define N 32
#define R 1 // Register
#define L 2 // Login
#define Q 3 // query
#define H 4 // History

#define DATABASE "my.db"
typedef struct
{
    int type;
    char name[N];
    char data[256];
} MSG;


//函数的声明
int do_client(int acceptfd, sqlite3 *db);
int Do_register(int acceptfd, MSG *msg, sqlite3 *db);
int Do_login(int acceptfd, MSG *msg, sqlite3 *db);
int Do_query(int acceptfd, MSG *msg, sqlite3 *db);
int Do_history(int acceptfd, MSG *msg, sqlite3 *db);

int main(int argc, const char *argv[])
{
    int sockfd;
    struct sockaddr_in serveraddr;
    int n;
    MSG msg;
    int acceptfd;
    pid_t pid;
    sqlite3 *db;
    if (argc != 3)
    {
        printf("Not Enough../n");
        return -1;
    }

    if (sqlite3_open(DATABASE, &db) != SQLITE_OK)
    {
        printf("%s\n", sqlite3_errmsg(db));
    }
    else
    {
        printf("open DATABASE success. \n");
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

    //绑定
    if ((bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0))
    {
        perror("fail to bind.\n");
        return -1;
    }
    // 设置为监听模式
    if ((listen(sockfd, 5)) < 0)
    {
        printf("fail to listen..\n");
        return -1;
    }

    //处理僵尸进程
    signal(SIGCHLD, SIG_IGN);

    while(1)
    {
        if((acceptfd = accept(sockfd, NULL, NULL))<0)
        {
            printf("fail to listen.\n");
            return -1;
        }
        if((pid = fork()) < 0)
        {
            perror("fail to fork.\n");
            return -1;
        }
        else if(pid == 0) //儿子进程
        {
            //处理客户端的具体消息
            close(sockfd);
            do_client(acceptfd, db);
        }
        else
        {
            close(acceptfd);
        }

    }
    return 0;
}

int do_client(int acceptfd, sqlite3 *db)
{
    MSG msg;
    while(recv(acceptfd, &msg, sizeof(msg), 0) > 0)
    {
        printf("%d\n", msg.type);
        switch(msg.type)
        {
            case R:
            {
                Do_register(acceptfd, &msg, db);
                break;
            }
            case L:
            {
                Do_login(acceptfd, &msg, db);
                break;
            }
            case Q:
            {
                Do_query(acceptfd, &msg, db);
                break;
            }
            case H:
            {
                Do_history(acceptfd, &msg, db);
                break;
            }
            default:
                printf("Invalied data mag.\n");
                break;
        }
    }
    close(acceptfd);
    exit(0);

    return 0;
}

int Do_register(int acceptfd, MSG *msg, sqlite3 *db)
{
    char *errmsg;
    char sql[512];

    //将字符串拷贝到sql数组中
    //sprintf(sql, "insert into uer values('%s', %s);", msg->name, msg->data);
    sprintf(sql, "insert into usr values('%s', %s);", msg->name, msg->data);
    printf("%s\n", sql);

    if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        printf("%s\n",errmsg);
        strcpy(msg->data, "usr name already exist.");
    }
    else
    {
        printf("client register ok!\n");
        strcpy(msg->data, "OK!");
    }
    //向客户端返回查询结果
    if(send(acceptfd, msg, sizeof(MSG), 0) < 0)
    {
        perror("Fail to send.");
        return -1;
    }
    return -1;
}

int Do_login(int acceptfd, MSG *msg, sqlite3 *db)
{
    char sql[512];
    //一下均为sqlite3_get_table 的参数及返回值
    char *errmsg; //携带错误消息
    char **pazResult;
    int pnRow;
    int pnColumn;
    sprintf(sql, "select * from usr where name = '%s' and pass = '%s'", msg->name, msg->data);
    printf("%s\n", sql);
    
    if(sqlite3_get_table(db, sql, &pazResult, &pnRow, &pnColumn, &errmsg) != SQLITE_OK)
    {
        printf("%s\n", errmsg);
        return -1;
    }
    else
    {
        printf("get_table ok!\n");
    }
    //验证查询到的是否为有效信息
    //name设置的为主键，所以返回的Row只有0/1两种结果
    if(pnRow == 1)
    {
        strcpy(msg->data, "OK");
        send(acceptfd, msg, sizeof(MSG), 0);
        return 1;
    }
    else
    {
        strcpy(msg->data, "Uer/Pass error.");      
        send(acceptfd, msg, sizeof(MSG), 0);
    }
    return 0;

}

int Do_searchword(int acceptfd, MSG *msg, char word[])
{
    FILE *fp;
    int len = 0;
    char temp[512] = {};
    int result;
    char *p;
    //打开字典进行查询
    if((fp = fopen("dict.txt", "r")) == NULL)
    {
        perror("Fail to fopen.\n");
        strcpy(msg->data, "Fail to open dict.txt");
        send(acceptfd, msg, sizeof(MSG), 0);
        return -1;
    }
    len = strlen(word);
    //打印出需要查询的单词
    printf("%s, len = %d\n", word, len);

    //由于字典中一行代表一个单词及解释，所以通过fgets逐行判断
    //fgets(数组地址,大小,文件操作符)
    while(fgets(temp, 512, fp) != NULL)
    {
        //printf("temp = %s\n",temp);
        
        result = strncmp(temp, word, len);
        //srtncmp中是根据字典序排序的，所以可以根据下面进行分类
        if(result < 0)
        {
            continue;
        }
        //如果要查询的单词更大则查询失败
        //如果要查询到匹配长度的单词，需要检验是否为word的子串
        if(result > 0 || ((result == 0) && temp[len] != ' '))
        {
            break;
        }
        //将指针移动到注释开始处
        p = temp + len;
        while(*p == ' ')
        {
            p++;
        }

        strcpy(msg->data, p);
        printf("found world:%s\n", msg->data);

        fclose(fp);
        return 1; //查询成功
    }

    fclose(fp);
    return 0;  //查询失败
}

int get_date(char *date)
{
    time_t t;
    struct tm *tp;

    time(&t);

    //时间格式的转换
    tp = localtime(&t);

    sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year+1900,tp->tm_mon+1, tp->tm_mday,
            tp->tm_hour, tp->tm_min, tp->tm_sec);
    return 0;
}
int Do_query(int acceptfd, MSG *msg, sqlite3 *db)
{
    char word[64];
    strcpy(word, msg->data);
    int found;
    char date[128];
    char sql[512];
    char *errmsg;
    found = Do_searchword(acceptfd, msg, word);
    
    //查询成功，将用户名，单词，日期存到历史记录中去
    if(found == 1)
    {
       get_date(date);

       sprintf(sql, "insert into record values('%s','%s','%s')", msg->name, date, word);
       //printf("sql:%s",sql);

       if((sqlite3_exec(db, sql, NULL, NULL, &errmsg)) != SQLITE_OK)
       {
            printf("%s\n",errmsg);
            return -1;
       }
    }
    else
    {
        strcpy(msg->data, "Not found!");
    }

    //将查询到的结果返回到客户端
    send(acceptfd, msg, sizeof(MSG), 0);

    return 0;
}
/*
int history_callback(void* arg, int f_num, char** f_value, char** f_name) {
    for (int i = 0; i < f_num; i++) {
        printf("Column name: %s\n", f_name[i]);
        printf("Column value: %s\n", f_value[i]);
    }
    return 0;
}
f_num：它是结果集中当前行的列数。即每一行的字段数量。
f_value：它是一个 char** 类型的指针数组，包含当前行每个字段的值。f_value[i] 表示当前行第 i 列的值。
f_name：它是一个 char** 类型的指针数组，包含每个字段的名称。f_name[i] 表示结果集中第 i 列的列名。
*/
int history_callback(void *arg, int f_num, char **f_value, char **f_name)
{
    int acceptfd;
    MSG msg;

    acceptfd = *((int *)arg);
    sprintf(msg.data,"%s , %s", f_value[1], f_value[2]);

    send(acceptfd, &msg, sizeof(MSG), 0);

    return 0;
}

int Do_history(int acceptfd, MSG *msg, sqlite3 *db)
{
    char sql[512] = {};
    char *errmsg;

    sprintf(sql, "select * from record where name = '%s'", msg->name);
    if((sqlite3_exec(db, sql, history_callback, (void *)(long)&acceptfd, &errmsg)) != SQLITE_OK)
    {
        printf("%s\n",errmsg);
    }
    else
    {
        printf("Query record done.\n");
    }

    msg->data[0] = '\0';
    send(acceptfd, msg, sizeof(MSG), 0);
    return 0;
}