#pragma once
#ifndef _CXSOCKS_H
#define _CXSOCKS_H

#include <sys/types.h>
#include <netdb.h>

#define SOCK_CONN_CLOSED 0
#define PROTOCOL_AUTO 0

// represents a stream connection(equivalent to SOCK_STREAM)
typedef struct streamconn_t {
    int fd;
    struct addrinfo *info;
} streamconn;

int is_big_endian();



int get_addrinfo_any(const char *host_str, const char *port_str, int socktype,
                            int flags, struct addrinfo **res_out);

// a simpler getaddrinfo that assumes you are using a SOCK_STREAM
int get_stream_addrinfo_any(const char *host_str, const char *port_str,
                            int flags, struct addrinfo **res_out);

// a simpler getaddrinfo that assumes you are using a SOCK_DGRAM
int get_dgram_addrinfo_any(const char *host_str, const char *port_str,
                            int flags, struct addrinfo **res_out);

#define gethostaddrinfo(PORT, HINTS, RES) getaddrinfo(NULL, (PORT), (HINTS), (RES))



// closes the connection(fd) and frees all the resources that has been allocated
void close_streamconn(streamconn connection);

int stream_connect(const char *host_str, const char *port_str);



int make_socket(const char *host_str, const char *port_str, int socktype, int flags, struct addrinfo **res_out);

// creates a listener socket with the current address, and the given port
int make_listener_sock(const char *port_str, int backlog);

// creates a binded dgram socket with the current address, and the given port,
// sets the output res_out with the matching address info used
int make_dgram_sock(const char *port_str, struct addrinfo **res_out);



// blocks the caller thread until it sends all the data
ssize_t sendall(int sockfd, const void *buf, size_t len, int flags);

// blocks the caller thread until it reads everything from the net socket
ssize_t recvall(int sockfd, void *buf, size_t len, int flags);

// blocks the caller thread until it fills the buffer at least with the length of "part_len"
ssize_t recvpart(int sockfd, void *buf, size_t buf_len, size_t part_len, int flags);



// packs the 32 bit data 0 -> LSB to 4 -> MSB
void pack32(const void *data, char *out);

// unpacks the 32 bit data from 0 -> LSB to 4 -> MSB
void unpack32(const char *packed, void *out);

// packs the 64 bit data 0 -> LSB to 8 -> MSB
void pack64(const void *data, char *out);

// unpacks the 64 bit data from 0 -> LSB to 8 -> MSB
void unpack64(const char *packed, void *out);


#define pack32_inplace(...) do { \
    void *v[] = { __VA_ARGS__ }; \
    for(size_t i = 0; i < (sizeof(v) / sizeof(v[0])); i++) \
        pack32(v, (char*)v); \
} while(0)

#define unpack32_inplace(...) do { \
    void *v[] = { __VA_ARGS__ }; \
    for(size_t i = 0; i < (sizeof(v) / sizeof(v[0])); i++) \
        unpack32((char*)v, v); \
} while(0)


#endif // _CXSOCKS_H

#ifdef CXSOCK_IMPL_ONCE
#undef CXSOCK_IMPL_ONCE

#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

static int _is_big_endian = -1;

int is_big_endian() {
    if(_is_big_endian < 0)
        _is_big_endian = htonl(1) == 1;

    return _is_big_endian;
}

void close_streamconn(streamconn conn) {
    close(conn.fd);
    freeaddrinfo(conn.info);
}


int get_addrinfo_any(const char *host_str, const char *port_str, int socktype,
                            int flags, struct addrinfo **res_out) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;

    return getaddrinfo(host_str, port_str, &hints, res_out);
}

int get_stream_addrinfo_any(const char *host_str, const char *port_str,
                            int flags, struct addrinfo **res_out) {
    return get_addrinfo_any(host_str, port_str, SOCK_STREAM, flags, res_out);
}

int get_dgram_addrinfo_any(const char *host_str, const char *port_str,
                            int flags, struct addrinfo **res_out) {
    return get_addrinfo_any(host_str, port_str, SOCK_DGRAM, flags, res_out);
}


int make_socket(const char *host_str, const char *port_str,
                int socktype, int flags, struct addrinfo **res_out) {
    struct addrinfo *res;
    if(get_addrinfo_any(host_str, port_str, socktype, flags, &res) < 0) 
        return -1;
    // else
    
    for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(fd < 0)
            continue;

        if(res_out != NULL)
            *res_out = p;
        else
            freeaddrinfo(res);

        return fd;
    }

    freeaddrinfo(res);
    return -1;
}

int make_listener_sock(const char *port_str, int backlog) {
    struct addrinfo *res = NULL;

    int status;

    if((status = get_stream_addrinfo_any(NULL, port_str, AI_PASSIVE, &res)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(sockfd < 0)
            continue;

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        // else

        if(listen(sockfd, backlog) < 0) {
            close(sockfd);
            continue;
        }
        // else

        freeaddrinfo(res);
        return sockfd;
    }

    // fail to bind / listen
    freeaddrinfo(res);
    return -1;
}

int make_dgram_sock(const char *port_str, struct addrinfo **res_out) {
    struct addrinfo *res = NULL;

    int status;

    if( (status = get_dgram_addrinfo_any(NULL, port_str, AI_PASSIVE, &res)) < 0 ) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(sockfd < 0)
            continue;
        // else

        if(res_out != NULL)
            *res_out = p;
        else
            freeaddrinfo(res);

        return sockfd;
    }

    freeaddrinfo(res);
    return -1;
}


ssize_t sendall(int sockfd, const void *buf, size_t len, int flags) {
    size_t send_len = 0;
    ssize_t status;

    do {
        status = send(sockfd, (char*)buf + send_len, len - send_len, flags);

        if(status < 0)
            return status;

        send_len += status;
    } while(send_len < len);

    return send_len;
}

ssize_t recvall(int sockfd, void *buf, size_t len, int flags) {
    size_t recv_len = 0;
    ssize_t status;

    do {
        status = recv(sockfd, (char*)buf + recv_len, len - recv_len, flags);

        if(status <= 0)
            return status;

        recv_len += status;
    } while(recv_len < len);

    return recv_len;
}

ssize_t recvpart(int sockfd, void *buf, size_t buf_len, size_t part_len, int flags) {
    if(buf_len < part_len)
        return -1;
    // else

    size_t recv_len = 0;
    ssize_t status;

    do {
        status = recv(sockfd, (char*)buf + recv_len, buf_len - recv_len, flags);

        if(status <= 0)
            return status;

        recv_len += status;
    } while(recv_len < part_len);

    return recv_len;
}


int stream_connect(const char *host_str, const char *port_str) {
    struct addrinfo *res = NULL;

    int status;

    if((status = get_stream_addrinfo_any(host_str, port_str, 0, &res)) < 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        // else

        freeaddrinfo(res);
        return sockfd;
    }

    // fail to connect
    freeaddrinfo(res);
    return -1;
}

// packs the 32 bit data 0 -> LSB to 4 -> MSB(to little endian)
void pack32(const void *data, char *out) {
    uint32_t cast = *(uint32_t*)data;

    for(size_t i = 0; i < sizeof(uint32_t); ++i)
        out[i] = (uint8_t)(cast >> (i * 8));
}

// unpacks the 32 bit data from 0 -> LSB to 4 -> MSB(from little endian)
void unpack32(const char *packed, void *out) {
    uint32_t result = 0;

    for(size_t i = 0; i < sizeof(uint32_t); ++i)
        result |= ((uint32_t)packed[i]) << (i * 8);

    uint32_t *cast_ptr = (uint32_t*)out;
    *cast_ptr = result;
}

// packs the 64 bit data 0 -> LSB to 8 -> MSB
void pack64(const void *data, char *out) {
    uint64_t cast = *(uint64_t*)data;

    for(size_t i = 0; i < sizeof(uint64_t); ++i)
        out[i] = (uint8_t)(cast >> (i * 8));
}

// unpacks the 64 bit data from 0 -> LSB to 8 -> MSB
void unpack64(const char *packed, void *out) {
    uint64_t result = 0;

    for(size_t i = 0; i < sizeof(uint64_t); ++i)
        result |= ((uint64_t)packed[i]) << (i * 8);

    uint64_t *cast_ptr = (uint64_t*)out;
    *cast_ptr = result;
}

#endif // CXSOCK_IMPL_ONCE
