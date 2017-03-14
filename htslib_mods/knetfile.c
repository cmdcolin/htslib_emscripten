/* The MIT License

   Copyright (c) 2008 by Genome Research Ltd (GRL).
                 2010 by Attractive Chaos <attractor@live.co.uk>

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

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <emscripten.h>
#include <assert.h>


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

/* This function does not work with Windows due to the lack of
 * getaddrinfo() in winsock. It is addapted from an example in "Beej's
 * Guide to Network Programming" (http://beej.us/guide/bgnet/). */
static int socket_connect(const char *host, const char *port)
{
#define __err_connect(func) do { perror(func); freeaddrinfo(res); return -1; } while (0)

    printf("%s!\n", host);
    int ai_err, on = 1, fd;
    struct linger lng = { 0, 0 };
    struct addrinfo hints, *res = 0;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    /* In Unix/Mac, getaddrinfo() is the most convenient way to get
     * server information. */
    if ((ai_err = getaddrinfo(host, port, &hints, &res)) != 0) { fprintf(stderr, "can't resolve %s:%s: %s\n", host, port, gai_strerror(ai_err)); return -1; }
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) __err_connect("socket");
    printf("here2!\n");
    fcntl(fd, F_SETFL, O_NONBLOCK);
    printf("here3!\n");
  
    int rs = connect(fd, res->ai_addr, res->ai_addrlen);
    if (rs == -1 && errno != EINPROGRESS) {
        perror("connect failed");
        return (EXIT_FAILURE);
    }
    printf("here4!\n");

    freeaddrinfo(res);


    return fd;
}

void finish(int result) {
  if (server.fd) {
    close(server.fd);
    server.fd = 0;
  }
  emscripten_force_exit(result);
}
void main_loop() {
    static char out[1024*2];
    static int pos = 0;
    fd_set fdr;
    fd_set fdw;
    int res;

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
    khttp_connect_file_followup((knetFile *)fp);
}

void error_callback(int fd, int err, const char* msg, void* userData) {
    int error;
    socklen_t len = sizeof(error);

    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
    printf("%s callback\n", userData);
    printf("error message: %s\n", msg);
}




/*************************
 * FTP specific routines *
 *************************/

static int kftp_get_response(knetFile *ftp)
{
    unsigned char c;
    int n = 0;
    char *p;
    if (socket_wait(ftp->ctrl_fd, 1) <= 0) return 0;
    while (netread(ftp->ctrl_fd, &c, 1)) { // FIXME: this is *VERY BAD* for unbuffered I/O
        //fputc(c, stderr);
        if (n >= ftp->max_response) {
            ftp->max_response = ftp->max_response? ftp->max_response<<1 : 256;
            ftp->response = (char*)realloc(ftp->response, ftp->max_response);
        }
        ftp->response[n++] = c;
        if (c == '\n') {
            if (n >= 4 && isdigit(ftp->response[0]) && isdigit(ftp->response[1]) && isdigit(ftp->response[2])
                && ftp->response[3] != '-') break;
            n = 0;
            continue;
        }
    }
    if (n < 2) return -1;
    ftp->response[n-2] = 0;
    return strtol(ftp->response, &p, 0);
}

static int kftp_send_cmd(knetFile *ftp, const char *cmd, int is_get)
{
    if (socket_wait(ftp->ctrl_fd, 0) <= 0) return -1; // socket is not ready for writing
    int len = strlen(cmd);
    if ( netwrite(ftp->ctrl_fd, cmd, len) != len ) return -1;
    return is_get? kftp_get_response(ftp) : 0;
}

static int kftp_pasv_prep(knetFile *ftp)
{
    char *p;
    int v[6];
    kftp_send_cmd(ftp, "PASV\r\n", 1);
    for (p = ftp->response; *p && *p != '('; ++p);
    if (*p != '(') return -1;
    ++p;
    sscanf(p, "%d,%d,%d,%d,%d,%d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    memcpy(ftp->pasv_ip, v, 4 * sizeof(int));
    ftp->pasv_port = (v[4]<<8&0xff00) + v[5];
    return 0;
}


static int kftp_pasv_connect(knetFile *ftp)
{
    char host[80], port[10];
    if (ftp->pasv_port == 0) {
        fprintf(stderr, "[kftp_pasv_connect] kftp_pasv_prep() is not called before hand.\n");
        return -1;
    }
    sprintf(host, "%d.%d.%d.%d", ftp->pasv_ip[0], ftp->pasv_ip[1], ftp->pasv_ip[2], ftp->pasv_ip[3]);
    sprintf(port, "%d", ftp->pasv_port);
    ftp->fd = socket_connect(host, port);
    if (ftp->fd == -1) return -1;
    return 0;
}

int kftp_connect(knetFile *ftp)
{
    ftp->ctrl_fd = socket_connect(ftp->host, ftp->port);
    if (ftp->ctrl_fd == -1) return -1;
    kftp_get_response(ftp);
    kftp_send_cmd(ftp, "USER anonymous\r\n", 1);
    kftp_send_cmd(ftp, "PASS kftp@\r\n", 1);
    kftp_send_cmd(ftp, "TYPE I\r\n", 1);
    return 0;
}

int kftp_reconnect(knetFile *ftp)
{
    if (ftp->ctrl_fd != -1) {
        netclose(ftp->ctrl_fd);
        ftp->ctrl_fd = -1;
    }
    netclose(ftp->fd);
    ftp->fd = -1;
    return kftp_connect(ftp);
}

// initialize ->type, ->host, ->retr and ->size
knetFile *kftp_parse_url(const char *fn, const char *mode)
{
    knetFile *fp;
    char *p;
    int l;
    if (strstr(fn, "ftp://") != fn) return 0;
    for (p = (char*)fn + 6; *p && *p != '/'; ++p);
    if (*p != '/') return 0;
    l = p - fn - 6;
    fp = (knetFile*)calloc(1, sizeof(knetFile));
    fp->type = KNF_TYPE_FTP;
    fp->fd = -1;
    /* the Linux/Mac version of socket_connect() also recognizes a port
     * like "ftp", but the Windows version does not. */


    fp->port = strdup("21");
    fp->host = (char*)calloc(l + 1, 1);
    if (strchr(mode, 'c')) fp->no_reconnect = 1;
    strncpy(fp->host, fn + 6, l);
    fp->retr = (char*)calloc(strlen(p) + 8, 1);
    sprintf(fp->retr, "RETR %s\r\n", p);
    fp->size_cmd = (char*)calloc(strlen(p) + 8, 1);
    sprintf(fp->size_cmd, "SIZE %s\r\n", p);
    fp->seek_offset = 0;
    return fp;
}
// place ->fd at offset off
int kftp_connect_file(knetFile *fp)
{
    int ret;
    long long file_size;
    if (fp->fd != -1) {
        netclose(fp->fd);
        if (fp->no_reconnect) kftp_get_response(fp);
    }
    kftp_pasv_prep(fp);
    kftp_send_cmd(fp, fp->size_cmd, 1);
#ifndef _WIN32
    // If the file does not exist, the response will be "550 Could not get file
    // size". Be silent on failure, hts_idx_load can be trying the existence of .csi or .tbi.
    if ( sscanf(fp->response,"%*d %lld", &file_size) != 1 ) return -1;
#else
    const char *p = fp->response;
    while (*p != ' ') ++p;
    while (*p < '0' || *p > '9') ++p;
    file_size = strtoint64(p);
#endif
    fp->file_size = file_size;
    if (fp->offset>=0) {
        char tmp[32];
#ifndef _WIN32
        sprintf(tmp, "REST %lld\r\n", (long long)fp->offset);
#else
        strcpy(tmp, "REST ");
        int64tostr(tmp + 5, fp->offset);
        strcat(tmp, "\r\n");
#endif
        kftp_send_cmd(fp, tmp, 1);
    }
    kftp_send_cmd(fp, fp->retr, 0);
    kftp_pasv_connect(fp);
    ret = kftp_get_response(fp);
    if (ret != 150) {
        fprintf(stderr, "[kftp_connect_file] %s\n", fp->response);
        netclose(fp->fd);
        fp->fd = -1;
        return -1;
    }
    fp->is_ready = 1;
    return 0;
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

int khttp_connect_file(knetFile *fp)
{
    if (fp->fd != -1) netclose(fp->fd);
    fp->fd = socket_connect(fp->host, fp->port);

    emscripten_set_socket_error_callback("error", error_callback);
    emscripten_set_socket_open_callback(fp, async_open_loop);
    emscripten_set_socket_message_callback(fp, async_message_loop);

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
    if (strstr(fn, "ftp://") == fn) {
        fp = kftp_parse_url(fn, mode);
        if (fp == 0) return 0;
        if (kftp_connect(fp) == -1) {
            knet_close(fp);
            return 0;
        }
        kftp_connect_file(fp);
    } else if (strstr(fn, "http://") == fn) {
        fp = khttp_parse_url(fn, mode);
        if (fp == 0) return 0;
        khttp_connect_file(fp);
    } else { // local file
        int fd = open(fn, O_RDONLY);
        if (fd == -1) {
            perror("open");
            return 0;
        }
        fp = (knetFile*)calloc(1, sizeof(knetFile));
        fp->type = KNF_TYPE_LOCAL;
        fp->fd = fd;
        fp->ctrl_fd = -1;
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
    off_t l = 0;
    if (fp->fd == -1) return 0;
    if (fp->type == KNF_TYPE_FTP) {
        if (fp->is_ready == 0) {
            if (!fp->no_reconnect) kftp_reconnect(fp);
            kftp_connect_file(fp);
        }
    } else if (fp->type == KNF_TYPE_HTTP) {
        if (fp->is_ready == 0)
            khttp_connect_file(fp);
    }
    if (fp->type == KNF_TYPE_LOCAL) { // on Windows, the following block is necessary; not on UNIX
        size_t rest = len;
        ssize_t curr;
        while (rest) {
            do {
                curr = read(fp->fd, (void*)((char*)buf + l), rest);
            } while (curr < 0 && EINTR == errno);
            if (curr < 0) return -1;
            if (curr == 0) break;
            l += curr; rest -= curr;
        }
    } else l = my_netread(fp->fd, buf, len);
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
#ifdef KNETFILE_MAIN
int main(void)
{
    char *buf;
    knetFile *fp;
    int type = 4, l;
    buf = calloc(0x100000, 1);
    if (type == 0) {
        fp = knet_open("knetfile.c", "r");
        knet_seek(fp, 1000, SEEK_SET);
    } else if (type == 1) { // NCBI FTP, large file
        fp = knet_open("ftp://ftp.ncbi.nih.gov/1000genomes/ftp/data/NA12878/alignment/NA12878.chrom6.SLX.SRP000032.2009_06.bam", "r");
        knet_seek(fp, 2500000000ll, SEEK_SET);
        l = knet_read(fp, buf, 255);
    } else if (type == 2) {
        fp = knet_open("ftp://ftp.sanger.ac.uk/pub4/treefam/tmp/index.shtml", "r");
        knet_seek(fp, 1000, SEEK_SET);
    } else if (type == 3) {
        fp = knet_open("http://www.sanger.ac.uk/Users/lh3/index.shtml", "r");
        knet_seek(fp, 1000, SEEK_SET);
    } else if (type == 4) {
        fp = knet_open("http://www.sanger.ac.uk/Users/lh3/ex1.bam", "r");
        knet_read(fp, buf, 10000);
        knet_seek(fp, 20000, SEEK_SET);
        knet_seek(fp, 10000, SEEK_SET);
        l = knet_read(fp, buf+10000, 10000000) + 10000;
    }
    if (type != 4 && type != 1) {
        knet_read(fp, buf, 255);
        buf[255] = 0;
        printf("%s\n", buf);
    } else write(fileno(stdout), buf, l);
    knet_close(fp);
    free(buf);
    return 0;
}
#endif