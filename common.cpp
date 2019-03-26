#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "common.h"
#include "handle.h"

extern time_t server_started;
extern int server_bytes_sent;
extern int server_requests;


/*
 * initialize the status variables and
 * set the thread attribute to detached 
 */
void setup(pthread_attr_t *attrp) {//设置独立线程，即线程结束后无需调用pthread_join阻塞等待线程结束
    pthread_attr_init(attrp);
    pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);  //将线程设置为分离模式

    time(&server_started);//请求时间， 将当前时间写入server_started
    server_requests = 0;
    server_bytes_sent = 0;


    //屏蔽SIGPIPE信号  如果不屏蔽SIGPIPE信号，则在对方断开链接后依然多次发送消息的情况下会导致本进程崩溃（https://blog.csdn.net/yusiguyuan/article/details/22885667）
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//忽略信号
    sa.sa_flags = SA_NODEFER;//
    if(sigemptyset(&sa.sa_mask) == -1 || //初始化信号集为空
        sigaction(SIGPIPE, &sa, 0) == -1) { //屏蔽SIGPIPE
        perror("failed to ignore SIGPIPE");
        exit(EXIT_FAILURE);
    }
}

/*-----------------------------------------------------------------
    skip_rest_of_header(FILE *)
    skip over all request info until a CCRNL is seen
------------------------------------------------------------------*/

void skip_rest_of_header(FILE *fp) {
    char  buf[BUFSIZ] = "";
    while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0);   //当http报文中出现两个\r\n时，意味着请求头部结束，下面是请求体
    //fgets函数按行读取，BUFSIZ参数设定了单次读取的最大值
}

/*-----------------------------------------------------------------
    read content length in head at the post method.
------------------------------------------------------------------*/
int read_content_length(FILE *fp) {
    int length = 0;
    char  buf[BUFSIZ] = "";
    while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
        char *arg = strchr(buf, ':');   //找到buf中第一次出现:的位置，返回指向这个位置的指针
        if (arg == NULL)
            continue;
        *arg = '\0';  //将当前行的：替换为\0，使buf中的内容阶段到\0为止，即为了比较：之前的内容，插入一个结束符
        if(strcasecmp(buf, "Content-Length") == 0)    //忽略大小写的比较字符串  如果：以前的内容是content-length，则：后的内容即为内容长度，将长度值转换
        //为int型赋值给length
            length = atoi(++arg);
    }
    return length;
}

/*
 * make sure all paths are below the current directory
 */
void sanitize(char *str) {
    char *src, *dest;
    src = dest = str;

    while(*src) {  
        if(strncmp(src, "/../", 4) == 0)   //strncmp比较前maxlen个字符
            src += 3;
        else if(strncmp(src, "//", 2) == 0)
            src++;
        else if(strncmp(src, "/./", 3) == 0)
            src += 2;
        else
            *dest++ = *src++;
    }
    *dest = '\0';
    // if(*str == '/')
    //     strcpy(str, str + 1);
    
    if(str[0] == '\0' || strcmp(str, "./") == 0
        || strcmp(str, "./..") == 0)
        strcpy(str, "/");

    //解码中文字符
    src = dest = str;
    for (; *dest != '\0'; ++dest) {
        if (*dest == '%') {
            int code;
            if (sscanf(dest+1, "%x", &code) != 1) 
                code = '?';
            *src++ = code;
            dest += 2;
        }
        else {
            *src++ = *dest;
        }
    }
}
    
/* handle built-in URLs here. Only one so far is “status" */
int built_in(char *arg, int fd) {
    FILE *fp;

    if(strcmp(arg, "status") != 0)
        return 0;
    http_reply(fd, &fp, 200, "OK", "text/plain", NULL);

    fprintf(fp, "Server started: %s", ctime(&server_started));
    fprintf(fp, "Total requests: %d\n", server_requests);
    fprintf(fp, "Bytes sent out: %d\n", server_bytes_sent);
    fclose(fp);
    return 1;
}

int http_reply(int fd, FILE **fpp, int code, const char *msg, const char *type, const char *content) {
    FILE *fp = fdopen(fd, "w");   //使用文件指针为了简化代码，避免read和write的系统调用，但是会增加系统调用，多了一个拷贝步骤。
    int bytes = 0;
    
    if(fp != NULL) {
        bytes = fprintf(fp, "HTTP/1.0 %d %s\r\n", code, msg);
        bytes += fprintf(fp, "Content-type: %s\r\n\r\n", type);  //输入两个\r\n,标志请求头部结束，下面是请求体
        if(content)
            bytes += fprintf(fp, "%s\r\n", content);
    }
    fflush(fp);  //刷新缓冲区，向文件描述符fd写入
    if(fpp)
        *fpp = fp;  
    else
        close(fd);
    return bytes;
}

/*-----------------------------------------------------------------
    simple functions first:
        not_implemented(fd)     unimplemented HTTP command
        and do_404(item, fd)  #include <dirent.h>  no such object
------------------------------------------------------------------*/
void not_implemented(int fd) {
    http_reply(fd, NULL, 501, "Not Implemented", "text/plain", 
        "That command is not implemented");
}

/*-----------------------------------------------------------------
    the directory listing section 
    isadir() uses stat, not_exist() uses stat
------------------------------------------------------------------*/

bool isadir(char *f) {
     struct stat info;     //定义stat结构体，用于存储文件的信息
     return (stat(f, &info) != -1 && S_ISDIR(info.st_mode));  //检查是否是一个目录  stat（char* filename, stat *）用于根据文件名获取文件的信息
}

bool not_exist(char *f) {
    struct stat info;
    return (stat(f, &info) == -1);  //无法读取文件信息，即文件不存在
}

/*-----------------------------------------------------------------
    functions to cat files here.
    file_type(filename) returns the 'extension': cat uses it
------------------------------------------------------------------*/

char *file_type(char *f) {
    char *cp;
    if((cp = strrchr(f, '.')) != NULL) return cp + 1;    //查找f中最后一次出现.的位置，并返回指向这个位置的指针
    return (char *)"/";
}

/*
 * input absolute path, and change work space; 
 */
void setdir(const char *abpath) {
    if(abpath == NULL) {

    }
}

void setNonBlock(int fd) {
    int opts = fcntl(fd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(sock,GETFL)");
        return;
    }
    opts = opts | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opts) < 0) {
        perror("fcntl(sock,GETFL)");
        return;
    }
}