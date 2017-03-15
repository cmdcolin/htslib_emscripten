/* The MIT License

   Copyright (c) 2008 by Genome Research Ltd (GRL).
                 2010 by Attractive Chaos <attractor@live.co.uk>

   Emscripten updates added by Colin Diesh <colin.diesh@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <config.h>

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <emscripten.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "htslib/knetfile.h"
#include "socklib.h"


typedef enum {
  MSG_READ,
  MSG_WRITE
} msg_state_t;

typedef struct {
  int fd;
  msg_t msg;
  msg_state_t state;
} server_t;


server_t server;
msg_t echo_msg;
int echo_read;
int echo_wrote;



/* This function tests if the file handler is ready for reading (or
 * writing if is_read==0). */
static int socket_wait(int fd, int is_read)
{
    printf("socket_wait\n");
    fd_set fds, *fdr = 0, *fdw = 0;
    struct timeval tv;
    int ret;
    tv.tv_sec = 5; tv.tv_usec = 0; // 5 seconds time out
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    if (is_read) fdr = &fds;
    else fdw = &fds;
    ret = select(fd+1, fdr, fdw, 0, &tv);
    if (ret == -1) perror("select");
    return ret;
}
void finish(int result) {
    printf("fin!\n");
  if (server.fd) {
    close(server.fd);
    server.fd = 0;
  }
  emscripten_force_exit(result);
}
/* This function does not work with Windows due to the lack of
 * getaddrinfo() in winsock. It is addapted from an example in "Beej's
 * Guide to Network Programming" (http://beej.us/guide/bgnet/). */
static int socket_connect(const char *host, const char *port)
{
    int fd, res;
    struct sockaddr_in addr;
    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
          
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8010);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        perror("inet_pton failed");
        return -1;
    }
    res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (res == -1 && errno != EINPROGRESS) {
        perror("connect failed");
        return -1;
    }
    fcntl(fd, F_SETFL, O_NONBLOCK);

    return fd;
}

void main_loop() {
    printf("HImain!\n");
  static char out[1024*2];
  static int pos = 0;
  fd_set fdr;
  fd_set fdw;
  int res;

  // make sure that server.fd is ready to read / write
  FD_ZERO(&fdr);
  FD_ZERO(&fdw);
  FD_SET(server.fd, &fdr);
  FD_SET(server.fd, &fdw);
  res = select(64, &fdr, &fdw, NULL, NULL);
  if (res == -1) {
    perror("select failed");
    finish(EXIT_FAILURE);
  }

  if (server.state == MSG_READ) {
    if (!FD_ISSET(server.fd, &fdr)) {
      return;
    }

#if !TEST_DGRAM
    // as a test, confirm with ioctl that we have data available
    // after selecting
    int available;
    res = ioctl(server.fd, FIONREAD, &available);
    assert(res != -1);
    assert(available);
#endif

    res = do_msg_read(server.fd, &server.msg, echo_read, 0, NULL, NULL);
    if (res == -1) {
      return;
    } else if (res == 0) {
      perror("server closed");
      finish(EXIT_FAILURE);
    }

    echo_read += res;

    // once we've read the entire message, validate it
    if (echo_read >= server.msg.length) {
      assert(!strcmp(server.msg.buffer, "PING"));
      finish(EXIT_SUCCESS);
    }
  }

  if (server.state == MSG_WRITE) {
    if (!FD_ISSET(server.fd, &fdw)) {
      return;
    }
    printf("WRITE!\n");

    res = do_msg_write(server.fd, &echo_msg, echo_wrote, 0, NULL, 0);
    if (res == -1) {
      return;
    } else if (res == 0) {
      perror("server closed");
      finish(EXIT_FAILURE);
    }

    echo_wrote += res;

    // once we're done writing the message, read it back
    if (echo_wrote >= echo_msg.length) {
      server.state = MSG_READ;
    }
  }
}
void not_main_loop() {
    static char out[1024*2];
    static int pos = 0;
    fd_set fdr;
    fd_set fdw;
    int res;
    printf("mainloop\n");

    // make sure that fd is ready to read / write
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    FD_SET(server.fd, &fdr);
    FD_SET(server.fd, &fdw);
    res = select(64, &fdr, &fdw, NULL, NULL);
    if (res == -1) {
        perror("select failed");
        finish(EXIT_FAILURE);
    }

    if (server.state == MSG_READ) {
        if (!FD_ISSET(server.fd, &fdr)) {
            return;
        }

        res = do_msg_read(server.fd, &server.msg, echo_read, 0, NULL, NULL);
        if (res == -1) {
            return;
        } else if (res == 0) {
            perror("server closed");
            finish(EXIT_FAILURE);
        }

        echo_read += res;

        // once we've read the entire message, validate it
        if (echo_read >= server.msg.length) {
            assert(!strcmp(server.msg.buffer, "TEST"));
            finish(EXIT_SUCCESS);
        }
    }

    if (server.state == MSG_WRITE) {
        if (!FD_ISSET(server.fd, &fdw)) {
            return;
        }

        res = do_msg_write(server.fd, &echo_msg, echo_wrote, 0, NULL, 0);
        if (res == -1) {
            return;
        } else if (res == 0) {
            perror("server closed");
            finish(EXIT_FAILURE);
        }

        echo_wrote += res;

        // once we're done writing the message, read it back
        if (echo_wrote >= echo_msg.length) {
            server.state = MSG_READ;
        }
    }
}


static off_t my_netread(int fd, void *buf, off_t len)
{
    off_t rest = len, curr, l = 0;
    /* recv() and read() may not read the required length of data with
     * one call. They have to be called repeatedly. */
    while (rest) {
        if (socket_wait(fd, 1) <= 0) break; // socket is not ready for reading
        curr = netread(fd, (void*)((char*)buf + l), rest);
        /* According to the glibc manual, section 13.2, a zero returned
         * value indicates end-of-file (EOF), which should mean that
         * read() will not return zero if EOF has not been met but data
         * are not immediately available. */
        if (curr == 0) break;
        l += curr; rest -= curr;
    }
    return l;
}
int khttp_connect_file_followup(knetFile *fp) {
    int ret, l = 0;
    char *buf, *p;
    buf = (char*)calloc(0x10000, 1); // FIXME: I am lazy... But in principle, 64KB should be large enough.
    l += sprintf(buf + l, "GET %s HTTP/1.0\r\nHost: %s\r\n", fp->path, fp->http_host);
    l += sprintf(buf + l, "Range: bytes=%lld-\r\n", (long long)fp->offset);
    l += sprintf(buf + l, "\r\n");
    if ( netwrite(fp->fd, buf, l) != l ) { free(buf); return -1; }
    l = 0;
    while (netread(fp->fd, buf + l, 1)) { // read HTTP header; FIXME: bad efficiency
        if (buf[l] == '\n' && l >= 3)
            if (strncmp(buf + l - 3, "\r\n\r\n", 4) == 0) break;
        ++l;
    }
    buf[l] = 0;
    if (l < 14) { // prematured header
        free(buf);
        netclose(fp->fd);
        fp->fd = -1;
        return -1;
    }
    ret = strtol(buf + 8, &p, 0); // HTTP return code
    if (ret == 200 && fp->offset>0) { // 200 (complete result); then skip beginning of the file
        off_t rest = fp->offset;
        while (rest) {
            off_t l = rest < 0x10000? rest : 0x10000;
            rest -= my_netread(fp->fd, buf, l);
        }
    } else if (ret != 206 && ret != 200) {
        // failed to open file
        free(buf);
        netclose(fp->fd);
        switch (ret) {
        case 401: errno = EPERM; break;
        case 403: errno = EACCES; break;
        case 404: errno = ENOENT; break;
        case 407: errno = EPERM; break;
        case 408: errno = ETIMEDOUT; break;
        case 410: errno = ENOENT; break;
        case 503: errno = EAGAIN; break;
        case 504: errno = ETIMEDOUT; break;
        default:  errno = (ret >= 400 && ret < 500)? EINVAL : EIO; break;
        }
        fp->fd = -1;
        return -1;
    }
    free(buf);
    fp->is_ready = 1;
    return 0;
}
// The callbacks for the async network events have a different signature than from
// emscripten_set_main_loop (they get passed the fd of the socket triggering the event).
// In this test application we want to try and keep as much in common as the timed loop
// version but in a real application the fd can be used instead of needing to select().
void async_message_loop(int fd, void* userData) {
    printf("%s callback\n", userData);
    main_loop();
}

void async_open_loop(int fd, void* fp) {
    printf("open!!!!!!!!!!!");
    khttp_connect_file_followup((knetFile *)fp);
}

void error_callback(int fd, int err, const char* msg, void* userData) {
    int error;
    socklen_t len = sizeof(error);

    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
    printf("%s callback\n", userData);
    printf("error message: %s\n", msg);
}



/**************************
 * HTTP specific routines *
 **************************/

knetFile *khttp_parse_url(const char *fn, const char *mode)
{
    knetFile *fp;
    char *p, *proxy, *q;
    int l;
    if (strstr(fn, "http://") != fn) return 0;
    // set ->http_host
    for (p = (char*)fn + 7; *p && *p != '/'; ++p);
    l = p - fn - 7;
    fp = (knetFile*)calloc(1, sizeof(knetFile));
    fp->http_host = (char*)calloc(l + 1, 1);
    strncpy(fp->http_host, fn + 7, l);
    fp->http_host[l] = 0;
    for (q = fp->http_host; *q && *q != ':'; ++q);
    if (*q == ':') *q++ = 0;
    // get http_proxy
    proxy = getenv("http_proxy");
    // set ->host, ->port and ->path
    if (proxy == 0) {
        fp->host = strdup(fp->http_host); // when there is no proxy, server name is identical to http_host name.
        fp->port = strdup(*q? q : "80");
        fp->path = strdup(*p? p : "/");
    } else {
        fp->host = (strstr(proxy, "http://") == proxy)? strdup(proxy + 7) : strdup(proxy);
        for (q = fp->host; *q && *q != ':'; ++q);
        if (*q == ':') *q++ = 0; 
        fp->port = strdup(*q? q : "80");
        fp->path = strdup(fn);
    }
    fp->type = KNF_TYPE_HTTP;
    fp->ctrl_fd = fp->fd = -1;
    fp->seek_offset = 0;
    return fp;
}



// The callbacks for the async network events have a different signature than from
// emscripten_set_main_loop (they get passed the fd of the socket triggering the event).
// In this test application we want to try and keep as much in common as the timed loop
// version but in a real application the fd can be used instead of needing to select().
void async_main_loop(int fd, void* userData) {
    printf("HI!\n");
  printf("%s callback\n", userData);
  main_loop();
}

int khttp_connect_file(knetFile *fp)
{
    printf("kconnect\n");
    if (fp->fd != -1) netclose(fp->fd);
    fp->fd = socket_connect(fp->host, fp->port);
    printf("%d\n",fp->fd);
    server.fd = fp->fd;
    {
      int z;
      struct sockaddr_in adr_inet;
      socklen_t len_inet = sizeof adr_inet;
      z = getsockname(server.fd, (struct sockaddr *)&adr_inet, &len_inet);
      if (z != 0) {
        perror("getsockname");
        finish(EXIT_FAILURE);
      }
      char buffer[1000];
      sprintf(buffer, "%s:%u", inet_ntoa(adr_inet.sin_addr), (unsigned)ntohs(adr_inet.sin_port));
      // TODO: This is not the correct result: We should have a auto-bound address
      char *correct = "0.0.0.0:0";
      printf("got (expected) socket: %s (%s), size %d (%d)\n", buffer, correct, strlen(buffer), strlen(correct));
      assert(strlen(buffer) == strlen(correct));
      assert(strcmp(buffer, correct) == 0);
    }

    {
      int z;
      struct sockaddr_in adr_inet;
      socklen_t len_inet = sizeof adr_inet;
      z = getpeername(server.fd, (struct sockaddr *)&adr_inet, &len_inet);
      if (z != 0) {
        perror("getpeername");
        finish(EXIT_FAILURE);
      }
      char buffer[1000];
      sprintf(buffer, "%s:%u", inet_ntoa(adr_inet.sin_addr), (unsigned)ntohs(adr_inet.sin_port));
      char correct[1000];
      sprintf(correct, "127.0.0.1:%u", 8010);
      printf("got (expected) socket: %s (%s), size %d (%d)\n", buffer, correct, strlen(buffer), strlen(correct));
      assert(strlen(buffer) == strlen(correct));
      assert(strcmp(buffer, correct) == 0);
    }

    emscripten_set_socket_error_callback("error", error_callback);
    emscripten_set_socket_open_callback("open", async_main_loop);
    emscripten_set_socket_message_callback("message", async_main_loop);
    return 0;
}


/********************
 * Generic routines *
 ********************/

knetFile *knet_open(const char *fn, const char *mode)
{
    knetFile *fp = 0;
    if (mode[0] != 'r') {
        fprintf(stderr, "[kftp_open] only mode \"r\" is supported.\n");
        return 0;
    }
    if (strstr(fn, "http://") == fn) {
        fp = khttp_parse_url(fn, mode);
        if (fp == 0) return 0;
        khttp_connect_file(fp);
    }
    if (fp && fp->fd == -1) {
        knet_close(fp);
        return 0;
    }
    return fp;
}

knetFile *knet_dopen(int fd, const char *mode)
{
    knetFile *fp = (knetFile*)calloc(1, sizeof(knetFile));
    fp->type = KNF_TYPE_LOCAL;
    fp->fd = fd;
    return fp;
}

ssize_t knet_read(knetFile *fp, void *buf, size_t len)
{
    printf("knet_read\n");
    off_t l = 0;
    if (fp->fd == -1) return 0;
    if (fp->is_ready == 0)
        khttp_connect_file(fp);
    else l = my_netread(fp->fd, buf, len);
    fp->offset += l;
    return l;
}

off_t knet_seek(knetFile *fp, off_t off, int whence)
{
    if (whence == SEEK_SET && off == fp->offset) return 0;
    if (fp->type == KNF_TYPE_LOCAL) {
        /* Be aware that lseek() returns the offset after seeking, while fseek() returns zero on success. */
        off_t offset = lseek(fp->fd, off, whence);
        if (offset == -1) return -1;
        fp->offset = offset;
        return fp->offset;
    } else if (fp->type == KNF_TYPE_FTP) {
        if (whence == SEEK_CUR) fp->offset += off;
        else if (whence == SEEK_SET) fp->offset = off;
        else if (whence == SEEK_END) fp->offset = fp->file_size + off;
        else return -1;
        fp->is_ready = 0;
        return fp->offset;
    } else if (fp->type == KNF_TYPE_HTTP) {
        if (whence == SEEK_END) { // FIXME: can we allow SEEK_END in future?
            fprintf(stderr, "[knet_seek] SEEK_END is not supported for HTTP. Offset is unchanged.\n");
            errno = ESPIPE;
            return -1;
        }
        if (whence == SEEK_CUR) fp->offset += off;
        else if (whence == SEEK_SET) fp->offset = off;
        else return -1;
        fp->is_ready = 0;
        return fp->offset;
    }
    errno = EINVAL;
    fprintf(stderr,"[knet_seek] %s\n", strerror(errno));
    return -1;
}

int knet_close(knetFile *fp)
{
    if (fp == 0) return 0;
    if (fp->ctrl_fd != -1) netclose(fp->ctrl_fd); // FTP specific
    if (fp->fd != -1) {
        /* On Linux/Mac, netclose() is an alias of close(), but on
         * Windows, it is an alias of closesocket(). */
        if (fp->type == KNF_TYPE_LOCAL) close(fp->fd);
        else netclose(fp->fd);
    }
    free(fp->host); free(fp->port);
    free(fp->response); free(fp->retr); // FTP specific
    free(fp->path); free(fp->http_host); // HTTP specific
    free(fp);
    return 0;
}

