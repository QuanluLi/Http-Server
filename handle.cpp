#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unordered_map>
#include <pthread.h>

#include "handle.h"
#include "common.h"
#include "./threadpool/mutex.h"
#include "./myepoll/cepoll.h"


extern int server_bytes_sent;
extern Mutex mapmux;
extern std::unordered_map<int, user_data> um;
extern Epoll myepoll;

/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
---------------------------------------------------->  //包含了Linux C 中的函数--------------*/
void *handle_call(void *fdptr) {
    //在线程中阻塞SIGPIPE信号，让主线程处理该线程
    // sigset_t sgmask;
    // sigemptyset(&sgmask);
    // sigaddset(&sgmask, SIGPIPE);//添加要被阻塞的信号
    // int t = pthread_sigmask(SIG_BLOCK, &sgmask, NULL);
    // if(t != 0) {
    //     printf("file: %s, line: %d, block sigpipe error\n", __FILE__, __LINE__);
    // }
    pthread_t tid = pthread_self();
    printf("this is handle_call. tid is %lu\n", tid);
    fflush(stdout);
    FILE *fpin;
    char request[BUFSIZ];
    int fd = *(int *)fdptr;
    free(fdptr);
//检查um中有没有fd，如果没有，则在um中添加fd
    mapmux.lock();
    int count = um.count(fd);   
    if(count != 0) {
        mapmux.unlock();
        return NULL;
    }
    else {
        um[fd];
    }
    mapmux.unlock();

    while(recv(fd, request, 1, MSG_PEEK | MSG_DONTWAIT) <= 0) {  //判断fd中是否有数据到达，没有则清空um套接字列表,最大字节设为1，并且没有真正取走数据
        printf("nonononononononononono message  tid is %lu\n", tid);
        mapmux.lock();
        um.erase(fd);
        mapmux.unlock();
        close(fd);
        return NULL;
    }
    
    

    fpin = fdopen(fd, "r");
    printf("开始获取http请求行 fd is %d. tid is %lu\n", fd, tid);
    fflush(stdout);
    // while(recv(fd, request, 1, 0) > 0) {
    //     printf("%s", request);
    // }
    fgets(request, BUFSIZ, fpin);//读取整行，遇到回车符结束
    if(request[0] == '\0') {
        printf("request error.tid is %lu\n", tid);
        mapmux.lock();
        um.erase(fd);
        mapmux.unlock();
        close(fd);
        return NULL;
    }
    printf("got a call on %d: request = %s  tid is %lu\n", fd, request, (tid));
    fflush(stdout);
    // skip_rest_of_header(fpin);//忽略请求头部

    process_rq(request, fd, fpin);//处理请求
    printf("请求处理完成。tid is %lu\n", (tid));
    fflush(stdout);
    return NULL;
}
//处理请求的函数
void process_rq(char *rq, int fd, FILE *fpin) {
    char cmd[BUFSIZ], url[BUFSIZ], version[BUFSIZ], cpath[BUFSIZ], query_string[BUFSIZ] = "\0";
    char path[BUFSIZ], bufPost[BUFSIZ];
    struct user_data ud;
    
    if(sscanf(rq, "%s %s %s", cmd, url, version) != 3) return;   //sscanf从一个指定的字符串中读取与指定格式相符合的数据到后面的字符串中。
    //int sscanf(string str, sring fmt, mixed var1, mixed var2, ...);
    strcpy(path, url);

    char *arg = strchr(path, '?');   //url中的第一个？后面是query， 即查询字符串
    if (arg != NULL)
    {
        *arg = '\0';
        strcpy(query_string, ++arg);
    }
    sanitize(path);//获得请求路径(请求路径)，和解码中文路径
    getcwd(cpath,BUFSIZ);//获得当前路径
    strcat(cpath, path); //完整路径

    strcpy(ud.method, cmd);
    strcpy(ud.cpath, cpath);
    strcpy(ud.url, url);

    if(strcmp(cmd, "GET") == 0) {   //对get的操作，将查询字符串拷贝到ud中
        skip_rest_of_header(fpin);//忽略请求头部
        strcpy(ud.c_arg.query_string, query_string);
    }
    else if(strcmp(cmd, "POST") == 0) {        //对post的操作，将需要添加的内容拷贝到ud当中
        int content_length = read_content_length(fpin);
        fgets(bufPost, content_length + 1, fpin);
        ud.c_arg.p_arg.content_length = content_length;
        strcpy(ud.c_arg.p_arg.pstring, bufPost);
    }

    mapmux.lock();
    um[fd] = ud;
    mapmux.unlock();
    myepoll.ModEvent(fd, Epoll::ETOUT);
    return;
}   
//响应函数
void *process_rp(void *fdptr) {
    //在线程中阻塞SIGPIPE信号，让主线程处理该线程
    sigset_t sgmask;   //定义线程的掩码，线程库根据该掩码决定将信号发送到哪个线程
    sigemptyset(&sgmask);
    sigaddset(&sgmask, SIGPIPE);//添加要被阻塞的信号
    int t = pthread_sigmask(SIG_BLOCK, &sgmask, NULL);  
    if(t != 0) {
        printf("file: %s, line: %d, block sigpipe error\n", __FILE__, __LINE__);
    }
    pthread_t tid = pthread_self();
    printf("this is process_rp.tid is %lu\n", tid);

    int fd = *(int *)fdptr;
    free(fdptr);

    struct user_data ud;
    int count;
    mapmux.lock();
    count = um.count(fd);
    if(count != 0) {
        ud = um[fd];
    }
    mapmux.unlock();
    if(count == 0) {
        close(fd);
        return NULL;
    }

    if(strcmp(ud.method, "GET") == 0) {
        if(built_in(ud.cpath, fd))
        ;
        else if (not_exist(ud.cpath)) /* does the arg exist */
            do_404(ud.url, fd);       /* n: tell the user */
        else if (isadir(ud.cpath))    /* is it a directory? */
            do_ls(ud.url, fd);        /* y: list contents */
        else {                     /* otherwise */
            if(ud.c_arg.query_string[0] == '\0')
                do_cat(ud.cpath, fd);
            else
                execute_cgi(fd, &ud);
        }
    }
    else if(strcmp(ud.method, "POST") == 0) {
        if(built_in(ud.cpath, fd))
        ;
        else {
            execute_cgi(fd, &ud);
        }
    }
    else
        not_implemented(fd);

    mapmux.lock();
    um.erase(fd);
    mapmux.unlock();
    close(fd);
    return NULL;
}

    
void do_404(char *item, int fd){
    http_reply(fd, NULL, 404, "Not Foud", "text/html", 
        "<HTML><TITLE>Not Found</TITLE>\r\n<BODY><P>The item you seek is not here\r\n</BODY></HTML>\r\n");
}    // getcwd(cpath,BUFSIZ);//获得当前路径
    // strcat(cpath, arg);
    

void do_ls(char *dir, int fd) {
    char cpath[BUFSIZ];
    getcwd(cpath,BUFSIZ);//获得当前路径
    strcat(cpath, dir);  //当前路径＋请求查询的路径
    //printf("file: %s, line: %d, path is %s\n", __FILE__, __LINE__, cpath);
    DIR *dirptr;
    struct dirent *direntp;
    struct stat st;
    FILE *fp;
    int bytes = 0;

    bytes = http_reply(fd, &fp, 200, "OK", "text/html", NULL);
    if(fp == NULL) {
        close(fd);
        return ;
    }
    bytes += fprintf(fp, "<HTML><TITLE>%s</TITLE>\r\n<BODY>", dir);
    //bytes += fprintf(fp, "Listing of Directory %s\n", dir);
    if((dirptr = opendir(cpath)) != NULL) {
        while((direntp = readdir(dirptr)) != NULL) {
            char tp[BUFSIZ];
            strcpy(tp, cpath);
            strcat(tp, direntp->d_name);  //d_name是dirent结构体中存储的文件名成员变量
            lstat(tp,&st);//获取文件信息，跟stat区别：stat查看符号链接所指向的文件信息，而lstat则返回符号链接的信息
            if(S_ISDIR(st.st_mode)) {
                if(strcmp(".", direntp->d_name) == 0 || strcmp("..", direntp->d_name) == 0) 
                    continue;
                bytes += fprintf(fp, "&nbsp;<A href=\"%s/\">%s</A>&nbsp;&nbsp;[DIR]<br/>", direntp->d_name, direntp->d_name);
            }
            else {
                bytes += fprintf(fp, "&nbsp;<A href=\"%s\">%s</A>&nbsp;&nbsp;%ld&nbsp;bytes<br/>", 
                    direntp->d_name, direntp->d_name, st.st_size);
            }
            
        }
        closedir(dirptr);
    }
    bytes += fprintf(fp, "</body></html>");
    fclose(fp);
    server_bytes_sent += bytes;
}

/* di_car(filename, fd): sends header then the contents */

void do_cat(char *f, int fd) {
    char *extension = file_type(f);
    const char *type = "text/plain";
    FILE *fpsock, *fpfile;
    int c, bytes = 0;

    if( strcmp(extension, "html") == 0)
        type = "text/html";
    else if(strcmp(extension, "htm") == 0)
        type = "image/html";
    else if(strcmp(extension, "gif") == 0)
        type = "image/gif";
    else if(strcmp(extension, "jpg") == 0)
        type = "image/jpeg";
    else if(strcmp(extension, "jpeg") == 0)
        type = "image/jpeg";
    else if(strcmp(extension, "png") == 0)
        type = "image/png";
    else if(strcmp(extension, "bmp") == 0)
        type = "image/x-xbitmap";
    
    fpsock = fdopen(fd, "w");
    fpfile = fopen(f, "r");
    if(fpsock != NULL && fpfile != NULL) {
        bytes = http_reply(fd, &fpsock, 200, "OK", type, NULL);
        while((c = getc(fpfile)) != EOF) {
            putc(c, fpsock);
            bytes++;
        }
        fclose(fpfile);
        fclose(fpsock);
    }
    server_bytes_sent += bytes;
}

void execute_cgi(int fd, struct user_data *ud) {
    char buf[BUFSIZ];
    int cgi_output[2];
    int cgi_input[2];

    pid_t pid;
    int status;

    /* 建立管道*/
    if (pipe(cgi_output) < 0) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }
    /*建立管道*/
    if (pipe(cgi_input) < 0) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }


    if ((pid = fork()) < 0 ) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }

    if(pid == 0) {// child process
        close(fd);    //fork之后，文件描述符也是共享的，父进程和子进程共享同一个文件偏移量。各自关闭不需要使用的文件描述符能够避免两个进程对同一文件的
        //混乱写入，并且不需要由父进程等待子进程结束；如不关闭各自不需要的描述符，则父进程需要等待子进程结束后再执行，即追加写入数据到子进程改变的文件偏移量之后。
        char meth_env[BUFSIZ];
        char query_env[BUFSIZ];
        char length_env[BUFSIZ];
        /* 把 STDOUT 重定向到 cgi_output 的写入端 */
        dup2(cgi_output[1], STDOUT_FILENO); //STDOUT_FILENO = 1
        /* 把 STDIN 重定向到 cgi_input 的读取端 */
        dup2(cgi_input[0], STDIN_FILENO); //STDIN_FILENO = 0
        /* 关闭 cgi_input 的写入端 和 cgi_output 的读取端 */
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*设置 request_method 的环境变量*/
        sprintf(meth_env, "REQUEST_METHOD=%s", ud->method);    //格式化输出
        putenv(meth_env);
        if (strcasecmp(ud->method, "GET") == 0) {
            /*设置 query_string 的环境变量*/
            sprintf(query_env, "QUERY_STRING=%s", ud->c_arg.query_string);
            putenv(query_env);
        }
        else {   /* POST */
            /*设置 content_length 的环境变量*/
            sprintf(length_env, "CONTENT_LENGTH=%d", ud->c_arg.p_arg.content_length);
            putenv(length_env);
        }
        execl(ud->cpath, ud->cpath, (char*)NULL);
        exit(1);
    }
    else { // father process
        /* 关闭 cgi_input 的读取端 和 cgi_output 的写入端 */
        close(cgi_output[1]);
        close(cgi_input[0]);

        if (strcasecmp(ud->method, "POST") == 0) {
            /*把 POST 数据写入 cgi_input，现在重定向到 STDIN */
            write(cgi_input[1], ud->c_arg.p_arg.pstring, ud->c_arg.p_arg.content_length);
        }

        FILE *fpout = fdopen(fd, "w");
        fprintf(fpout, "HTTP/1.0 200 OK\r\n");
        fflush(fpout);

        while (read(cgi_output[0], buf, 1) > 0) {
            send(fd, buf, 1, 0);
        }

        /*关闭管道*/
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*等待子进程*/
        waitpid(pid, &status, 0);
    }
}    
