# CXSOCK
A single header wrapper library for linux socket programming

## USAGE
Copy the header file `cxsock.h` into your project and you are good to go.

useful documented functions are given below:
1. `int make_socket(const char *host_str, const char *port_str, int socktype, int flags, struct addrinfo **res_out)`
Creates a socket using the given host, port, socket type and address flags to create a socket,
and sets the `res_out` pointer(if it's not `NULL`) to the utilized address info(this is allocated)

Returns negative number if failure occured

2. `ssize_t sendall(int sockfd, const void *buf, size_t len, int flags)`
blocks the main thread until it sends all the data

3. `ssize_t recvall(int sockfd, void *buf, size_t len, int flags)`
blocks the main thread until it reads everything from the net socket

4. `void pack32(const void *data, char *out)`
packs the 32 bit data 0 -> LSB to 4 -> MSB(little endian)

5. `void unpack32(const char *packed, void *out)`
unpacks the 32 bit data from 0 -> LSB to 4 -> MSB(little endian)

6. `void pack64(const void *data, char *out)`
packs the 64 bit data 0 -> LSB to 8 -> MSB(little endian)

7. `void unpack64(const char *packed, void *out)`
unpacks the 64 bit data from 0 -> LSB to 8 -> MSB(little endian)

8. `int is_big_endian()`
as the name suggests, returns whether or not this system is big endian, otherwise little endian.
(THIS FUNCTION IS NOT COMPILE TIME)

9. `pack32_inplace(...)`
a macro that calls `pack32` but packs the given data in the same memory location

10. `unpack32_inplace(...)`
a macro that calls `unpack32` but unpacks the given data in the same memory location

## LICENSE
Licensed under MIT
