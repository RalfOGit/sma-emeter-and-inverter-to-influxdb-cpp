/*
 * Copyright(C) 2022 RalfO. All rights reserved.
 * https://github.com/RalfOGit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define poll(a, b, c)  WSAPoll((a), (b), (c))
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#endif

#include <HttpClient.hpp>
#include <Url.hpp>

#ifdef LIB_NAMESPACE
using namespace LIB_NAMESPACE;
#else
using namespace libralfogit;
#endif

/**
 *  Close the given socket in a platform portable way.
 */
static void close_socket(const int socket_fd) {
#ifdef _WIN32
    closesocket(socket_fd);
#else
    close(socket_fd);
#endif
}


/**
 *  Constructor.
 */
HttpClient::HttpClient(void) {
#ifdef _WIN32
    // initialize Windows Socket API with given VERSION.
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        perror("WSAStartup failure");
    }
#endif
    recv_buffer_size = 4096;
    recv_buffer = (char*)malloc(recv_buffer_size);
    if (recv_buffer == NULL) {
        recv_buffer_size = 0;
        perror("cannot allocate recv_buffer for HttpClient");
    }
}


/**
 *  Destructor.
 */
HttpClient::~HttpClient(void) {
    if (recv_buffer != NULL) {
        free(recv_buffer);
        recv_buffer = NULL;
        recv_buffer_size = 0;
    }
}


/**
 * Send http get request and receive http response and content payload
 * @param url http get request url
 * @param response http response string returned by server
 * @param content http content string retured by server
 * @return http return code
 */
int HttpClient::sendHttpGetRequest(const std::string& url, std::string& response, std::string& content) {
    return sendHttpRequest(url, "GET", "", response, content);
}


/**
 * Send http put request, receive http response and content payload
 * @param url http put request url
 * @param request_data request data string
 * @param response http response string returned by server
 * @param content http content string retured by server
 * @return http return code, or -1 if socket connect failed
 */
int HttpClient::sendHttpPutRequest(const std::string& url, const std::string& request_data, std::string& response, std::string& content) {
    return sendHttpRequest(url, "PUT", request_data, response, content);
}


/**
 * Send http post request, receive http response and content payload
 * @param url http put request url
 * @param request_data request data string
 * @param response http response string returned by server
 * @param content http content string retured by server
 * @return http return code, or -1 if socket connect failed
 */
int HttpClient::sendHttpPostRequest(const std::string& url, const std::string& request_data, std::string& response, std::string& content) {
    return sendHttpRequest(url, "POST", request_data, response, content);
}


int HttpClient::sendHttpRequest(const std::string& url, const std::string& method, const std::string& request_data, std::string& response, std::string& content) {

    // parse the given url
    std::string protocol;
    std::string user;
    std::string password;
    std::string host;
    int         port;
    std::string path;
    std::string query;
    std::string fragment;
    if (Url::parseUrl(url, protocol, user, password, host, port, path, query, fragment) < 0) {
        perror("url failure");
        return -1;
    }
    if (protocol != "http") {
        perror("only http is supported");
        return -1;
    }

    // establish tcp connection to server
    int socket_fd = connect_to_server(host, port);
    if (socket_fd < 0) {
        return socket_fd;
    }

    // assemble http request
    std::string request;
    request.reserve(256 + request_data.length());
    request.append(method).append(" ").append(path).append(query).append(fragment).append(" HTTP/1.1\r\n");
    request.append("Host: ").append(host).append("\r\n");
    request.append("User-Agent: ralfogit/1.0\r\n");
    request.append("Accept: */*\r\n");
    if (request_data.length() > 0) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Content-Length: %llu\r\n", (unsigned long long)request_data.length());
        request.append(buffer);
    }
    if (user.length() > 0 || password.length() > 0) {
        std::string base64 = base64_encode(user.append(":").append(password));
        request.append("Authorization: Basic ").append(base64).append("\r\n");
    }
    request.append("\r\n");
    request.append(request_data);

    // send http get request string, receive response and content
    int http_return_code = communicate_with_server(socket_fd, request, response, content);
    return http_return_code;
}


/**
 * Connect to the given server url.
 * @param host host part extracted from the given http put request url
 * @param port port number to connect to
 * @return socket file descriptor, or -1 if the connect attempt failed.
 */
int HttpClient::connect_to_server(const std::string& host, const int port) {

    // open socket
    int socket_fd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        perror("socket open failure");
        return -1;
    }

    // open connection to http server
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", port);
    struct addrinfo* addr = NULL;
    if (getaddrinfo(host.c_str(), buffer, NULL, &addr) == 0) {
        while (addr != NULL) {
            if (connect(socket_fd, addr->ai_addr, (int)addr->ai_addrlen) >= 0) {
                freeaddrinfo(addr);
                return socket_fd;
            }
            addr = addr->ai_next;
        }
        freeaddrinfo(addr);
    }
    perror("connecting stream socket failure");
    close_socket(socket_fd);
    return -1;
}


/**
 * Communicate with the given server - send http request, receive response and content.
 * @param socket_fd socket file descriptor
 * @param request http request string to be sent to server
 * @param response http response string returned by server
 * @param content http content string retured by server
 * @return http return code, or -1 if either the socket send or the socket recv request failed
 */
int HttpClient::communicate_with_server(const int socket_fd, const std::string& request, std::string& response, std::string& content) {

    // send http request string
    if (::send(socket_fd, request.c_str(), (int)request.length(), 0) != (int)request.length()) {
        perror("send stream socket failure");
        close_socket(socket_fd);
        return -1;
    }

    // receive http get response data
    size_t nbytes_total = recv_http_response(socket_fd);
    if (nbytes_total != (size_t)-1) {

        // parse http response data
        int http_return_code = parse_http_response(recv_buffer, nbytes_total, response, content);
        close_socket(socket_fd);
        return http_return_code;
    }
    close_socket(socket_fd);
    return -1;
}


/**
 * Receive http response and content
 * @param socket_fd socket file descriptor
 * @param recv_buffer a pointer to a buffer big enough to receive the http response including content data
 * @param recv_buffer_size the size of the recv_buffer
 * @return number of bytes received
 */
size_t HttpClient::recv_http_response(int socket_fd) {
    struct pollfd fds;
    size_t nbytes_total = 0;
    recv_buffer[nbytes_total] = '\0';
    bool http_header_complete = false;
    bool chunked_encode = false;
    size_t content_length = 0;
    size_t content_offset = 0;

    while (1) {
        fds.fd = socket_fd;
        fds.events = POLLIN;
        fds.revents = 0;

        // wait for a packet on the configured socket
        int pollresult = poll(&fds, 1, 5000);
        if (pollresult == 0) {
            perror("poll timeout");
            return (nbytes_total > 0 ? nbytes_total : -1);
        }
        if (pollresult < 0) {
            perror("poll failure");
            return (nbytes_total > 0 ? nbytes_total : -1);
        }

        bool pollnval = (fds.revents & POLLNVAL) != 0;
        bool pollerr = (fds.revents & POLLERR) != 0;
        bool pollhup = (fds.revents & POLLHUP) != 0;
        bool pollin = (fds.revents & POLLIN) != 0;

        // check poll result
        if (pollin == true) {
            // ensure receive buffer size
            if (recv_buffer_size - nbytes_total - 1 < 1024) {
                char *realloc_buffer = (char*)realloc(recv_buffer, 2 * recv_buffer_size);
                if (realloc_buffer != NULL) {
                    recv_buffer = realloc_buffer;
                    recv_buffer_size *= 2;
                }
            }

            // receive data
            int nbytes = recv(socket_fd, recv_buffer + nbytes_total, (int)(recv_buffer_size - nbytes_total - 1), 0);
            if (nbytes < 0) {
                perror("recv stream socket failure");
                return (nbytes_total > 0 ? nbytes_total : -1);
            }
            nbytes_total += nbytes;
            recv_buffer[nbytes_total] = '\0';

            // check if the entire http response header has been received and obtain content length information
            if (http_header_complete == false) {
                size_t content_offs = get_content_offset(recv_buffer, nbytes_total);
                if (content_offs != (size_t)-1) {
                    http_header_complete = true;
                    content_offset = content_offs;
                    chunked_encode = is_chunked_encoding(recv_buffer, content_offset);
                    content_length = get_content_length(recv_buffer, content_offset);
                    // if there is no content length information and the return code is 204 "no content", finish receive loop
                    if (chunked_encode == false && content_length == (size_t)-1 &&
                        get_http_return_code(recv_buffer, content_offs) == 204) {
                        break;
                    }
                }
            }
            // if the entire http response header has been received
            if (http_header_complete == true) {
                // check if the content length is explicitly given and if the entire content has been received
                if (content_length != (size_t)-1 &&
                    nbytes_total >= content_offset + content_length) {
                    //printf("recv: nbytes %d  nbytes_total %d  content_offset %d  content_length %d => done\n", nbytes, (int)nbytes_total, (int)content_offset, (int)content_length);
                    break;
                }
                // check if chunked transfer encoding is used and if all chunks have been received
                else if (chunked_encode == true) {
                    char* ptr = recv_buffer + content_offset;
                    size_t next_chunk_offset = -2;
                    while (next_chunk_offset != (size_t)-1 && next_chunk_offset != 0) {
                        next_chunk_offset = get_next_chunk_offset(ptr, nbytes_total - (ptr - recv_buffer));
                        //printf("recv: nbytes %d  nbytes_total %d  content_offset %d  next_chunk_offset %d\n", nbytes, (int)nbytes_total, (int)content_offset, (int)next_chunk_offset);
                        ptr += next_chunk_offset;
                    }
                    if (next_chunk_offset == 0) {
                        //printf("recv: nbytes %d  nbytes_total %d  content_offset %d  next_chunk_offset %d => done\n", nbytes, (int)nbytes_total, (int)content_offset, (int)next_chunk_offset);
                        break;
                    }
                }
            }
            //printf("recv: nbytes %d  nbytes_total %d  content_length %d  content_offset %d\n", nbytes, (int)nbytes_total, (int)content_length, (int)content_offset);
        }
        else {
            // test for error and hangup conditions only if there is no input data waiting
            if (pollnval == true) {
                perror("pollnval");
                return (nbytes_total > 0 ? nbytes_total : -1);
            }
            if (pollerr == true) {
                perror("pollerr");
                return (nbytes_total > 0 ? nbytes_total : -1);
            }
            if (pollhup == true) {  // this is a perfectly valid way to determine a transfer
                perror("pollhup");
                return (nbytes_total > 0 ? nbytes_total : -1);
            }
        }
    }
    return nbytes_total;
}


/**
 * Parse http answer and split into response and content.
 * @param answer input - a string holding both the http response header and response content
 * @param http_response output - a string holding the http response header
 * @param http_content output - a string holding the http response data
 * @return the http return code value or -1 in case of error
 */
int HttpClient::parse_http_response(const char* buffer, size_t buffer_size, std::string& http_response, std::string& http_content) {

    // extract http return code
    int http_return_code = get_http_return_code(buffer, buffer_size);
    if (http_return_code < 0) {
        http_response = std::string(buffer, buffer_size);
        return -1;
    }

    // check if chunked encoding is used
    bool chunked_encoding = is_chunked_encoding(buffer, buffer_size);
    if (chunked_encoding == true) {
        std::string temp_content;
        temp_content.reserve(buffer_size);

        // determine content offset
        size_t content_offset = get_content_offset(buffer, buffer_size);
        if (content_offset == (size_t)-1) {
            http_response = std::string(buffer, buffer_size);
            return -1;
        }

        // assemble chunked content
        size_t ptr = content_offset;
        size_t next_chunk_offset = -2;
        while (next_chunk_offset != (size_t)-1 && next_chunk_offset != 0) {
            next_chunk_offset = get_next_chunk_offset(buffer + ptr, buffer_size - ptr);
            // check if this chunk is complete
            if (next_chunk_offset != (size_t)-1) {
                size_t chunk_length = get_chunk_length(buffer + ptr, buffer_size - ptr);
                size_t chunk_offset = get_chunk_offset(buffer + ptr, buffer_size - ptr);
                temp_content.append(buffer + ptr + chunk_offset, chunk_length);
                ptr += next_chunk_offset;
            }
        }

        // prepare response and content strings
        http_response = std::string(buffer, content_offset);
        http_content = temp_content;
        if (next_chunk_offset == (size_t)-1) {
            return -1;
        }
    }
    else {
        // extract content length
        size_t content_length = get_content_length(buffer, buffer_size);
        if (content_length == (size_t)-1) {
            http_response = std::string(buffer, buffer_size);
            return http_return_code;
        }

        // determine content offset
        size_t content_offset = get_content_offset(buffer, buffer_size);
        if (content_offset == (size_t)-1) {
            http_response = std::string(buffer, buffer_size);
            return http_return_code;
        }

        // prepare response and content strings
        http_response = std::string(buffer, content_offset);
        http_content = std::string(buffer + content_offset, buffer_size - content_offset);
        if (http_content.length() != content_length) {
            return -1;
        }
    }

    return http_return_code;
}


/**
 * Parse http header and determine http return code.
 * @param buffer pointer to a buffer holding an http header
 * @param buffer_size size of the buffer; buffer_size must be one byte less than the underlying buffer size
 * @return the http return code if it is described in the http header, -1 otherwise
 */
int HttpClient::get_http_return_code(const char* buffer, size_t buffer_size) {
    const char* substr = find(buffer, buffer_size, "HTTP/1.1 ");    // a single SP character after HTTP/1.1 is mandatory
    if (substr == NULL) {
        substr = find(buffer, buffer_size, "HTTP/1.0 ");            // a single SP character after HTTP/1.0 is mandatory
    }
    if (substr != NULL) {
        substr += strlen("HTTP/1.1 ");
        substr = skipSpaceCharacters(substr, buffer_size - (substr - buffer));
        if (substr != NULL) {
            size_t num_chars = 0;
            int return_code = (int)scanUint(substr, buffer_size - (substr - buffer), &num_chars);
            if (num_chars > 0) {
                return return_code;
            }
        }
    }
    return -1;
}


/**
 * Parse http header and determine content length.
 * @param buffer pointer to a buffer holding an http header
 * @param buffer_size size of the buffer; buffer_size must be one byte less than the underlying buffer size
 * @return the content length if it is described in the http header, -1 otherwise
 */
size_t HttpClient::get_content_length(const char* buffer, size_t buffer_size) {
    const char* substr = find(buffer, buffer_size, "\r\nContent-Length:");
    if (substr != NULL) {
        substr += strlen("\r\nContent-Length:");
        substr = skipSpaceCharacters(substr, buffer_size - (substr - buffer));
        if (substr != NULL) {
            size_t num_chars = 0;
            size_t content_length = (size_t)scanUint(substr, buffer_size - (substr - buffer), &num_chars);
            if (num_chars > 0) {
                return content_length;
            }
        }
    }
    return -1;
}


/**
 * Parse http header and determine content offset.
 * @param buffer pointer to a buffer holding an http header
 * @param buffer_size size of the buffer
 * @return the offset of the first content data byte after the header
 */
size_t HttpClient::get_content_offset(const char* buffer, size_t buffer_size) {
    const char* substr = find(buffer, buffer_size, "\r\n\r\n");
    if (substr != NULL) {
        substr += strlen("\r\n\r\n");
        return (substr - buffer);
    }
    return -1;
}


/**
 * Parse http header and check if chunked transfer encoding is used.
 * @param buffer pointer to a buffer holding an http header
 * @param buffer_size size of the buffer; buffer_size must be one byte less than the underlying buffer size
 * @return true, if chunked encoding is used; false otherwise
 */
bool HttpClient::is_chunked_encoding(const char* buffer, size_t buffer_size) {
    const char* substr = find(buffer, buffer_size, "\r\nTransfer-Encoding: chunked");
    if (substr != NULL) {
        return true;
    }
    return false;
}


/**
 * Parse http chunk header and get chunk size.
 * @param buffer pointer to a buffer holding a chunk header
 * @param buffer_size size of the buffer
 * @return the chunk length, -1 otherwise
 */
size_t HttpClient::get_chunk_length(const char* buffer, size_t buffer_size) {
    if (get_chunk_offset(buffer, buffer_size) != (size_t)-1) {  // ensure that the chunk header is completely available
        const char* substr = skipSpaceCharacters(buffer, buffer_size);
        if (substr != NULL) {
            size_t num_chars = 0;
            size_t chunk_length = (size_t)scanHex(substr, buffer_size - (substr - buffer), &num_chars);
            if (num_chars > 0) {
                return chunk_length;
            }
        }
    }
    return -1;
}


/**
 * Parse http chunk header and determine chunk content offset.
 * @param buffer pointer to a buffer holding a chunk header
 * @param buffer_size size of the buffer
 * @return the offset of the first content data byte after the chunk header
 */
size_t HttpClient::get_chunk_offset(const char* buffer, size_t buffer_size) {
    const char* substr = find(buffer, buffer_size, "\r\n");
    if (substr != NULL) {
        substr += strlen("\r\n");
        if (substr <= buffer + buffer_size) {
            return (substr - buffer);
        }
    }
    return -1;
}


/**
 * Parse http chunk header and determine offset to next chunk.
 * @param buffer pointer to a buffer holding a chunk header
 * @param buffer_size size of the buffer
 * @return the offset to the next chunk header; 0 if this is the last chunk; -1 if there is not enough data
 */
size_t HttpClient::get_next_chunk_offset(const char* buffer, size_t buffer_size) {
    size_t chunk_length = get_chunk_length(buffer, buffer_size);
    if (chunk_length == 0) {
        return 0;
    }
    if (chunk_length == (size_t)-1) {
        return -1;
    }
    size_t chunk_offset = get_chunk_offset(buffer, buffer_size);
    if (chunk_offset == (size_t)-1) {
        return -1;
    }
    const char* ptr = buffer + chunk_offset + chunk_length;
    if (ptr + 2 > buffer + buffer_size) {
        return -1;
    }
    if (ptr[0] != '\r' || ptr[1] != '\n') {
        return -1;
    }
    return ptr + 2 - buffer;
}


/**
 * Base64 encoding, loosely modelled after Simon Josefssons' reference implementation for rfc3548.
 * @param input string
 * @return the base64 encoded input string
 */
std::string HttpClient::base64_encode(const std::string& input) {
    static const unsigned char b64_data[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string output((input.length() + 2) * 2 + 1, '\0');  // initialize twice the necessary size with '\0' characters
    const unsigned char* in = (const unsigned char*)input.data();
    unsigned char* out = (unsigned char*)output.data();
    std::string::size_type length = input.length();

    while (length > 0) {
        *out++ = b64_data[(in[0] >> 2) & 0x3f];
        *out++ = b64_data[((in[0] << 4) + (--length > 0 ? in[1] >> 4 : 0)) & 0x3f];
        *out++ = (length > 0 ? b64_data[((in[1] << 2) + (--length > 0 ? in[2] >> 6 : 0)) & 0x3f] : '=');
        *out++ = (length > 0 ? b64_data[in[2] & 0x3f] : '=');
        if (length > 0) {
            length--;
        }
        if (length > 0) {
            in += 3;
        }
    }

    *out = '\0';
    output.resize(out - (unsigned char*)output.data(), '\0');
    return output;
}


/**
 * Find a substring in a given target character buffer.
 * @param hay search target buffer; not necessarily null terminated
 * @param hay_size size of the search target buffer
 * @param needle null terminated search string
 * @return a pointer to the first occurence of needle in hay
 */
const char* HttpClient::find(const char* hay, size_t hay_size, const char* needle) {
    size_t i, len;
    char c = *needle;

    if (c == '\0') {
        return hay;
    }

    // simplistic string search algorithm with O(n2) worst case
    for (len = strlen(needle); len <= hay_size; hay_size--, hay++) {
        if (*hay == c) {
            for (i = 1;; i++) {
                if (i == len) {
                    return hay;
                }
                if (hay[i] != needle[i]) {
                    break;
                }
            }
        }
    }
    return NULL;
}


/**
 * Skip space characters starting at the given pointer and at most up to the given buffer size.
 * @param buffer pointer to a buffer holding text data
 * @param buffer_size size of the buffer
 * @return the ptr to the first non-space character, or null if the end of the buffer has been reached
 */
const char* HttpClient::skipSpaceCharacters(const char* buffer, size_t buffer_size) {
    const char* ptr = buffer;
    while (buffer_size > 0 && *ptr == ' ') {
        ++ptr, --buffer_size;
    }
    return (buffer_size == 0 ? NULL : ptr);
}


/**
 * Scan digits starting at the given pointer and at most up to the given buffer size and convert them to an unsigned integer
 * @param buffer pointer to a buffer holding text data
 * @param buffer_size size of the buffer
 * @param num_chars pointer to an integer receiving the number of hex digits that have been scanned
 * @return the unsigned value of the scanned digits
 */
size_t HttpClient::scanUint(const char* buffer, size_t buffer_size, size_t* num_chars) {
    const char* ptr = buffer;
    size_t int_value = 0;
    while (buffer_size > 0 && *ptr >= '0' && *ptr <= '9') {
        int digit = *ptr - '0';
        int_value = int_value * 10 + digit;
        ++ptr, --buffer_size;
    }
    if (num_chars != NULL) {
        *num_chars = ptr - buffer;
    }
    return int_value;
}


/**
 * Scan hex digits starting at the given pointer and at most up to the given buffer size and convert them to an unsigned integer
 * @param buffer pointer to a buffer holding text data
 * @param buffer_size size of the buffer
 * @param num_chars pointer to an integer receiving the number of hex digits that have been scanned
 * @return the unsigned value of the scanned hex digits
 */
size_t HttpClient::scanHex(const char* buffer, size_t buffer_size, size_t* num_chars) {
    const char* ptr = buffer;
    size_t int_value = 0;
    while (buffer_size > 0 && ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'A' && *ptr <= 'F') || (*ptr >= 'a' && *ptr <= 'f'))) {
        int digit = 0;
        if (*ptr >= '0' && *ptr <= '9') {
            digit = *ptr - '0';
        }
        else if (*ptr >= 'a' && *ptr <= 'f') {
            digit = *ptr - 'a' + 10;
        }
        else if (*ptr >= 'A' && *ptr <= 'F') {
            digit = *ptr - 'A' + 10;
        }
        int_value = int_value * 16 + digit;
        ++ptr, --buffer_size;
    }
    if (num_chars != NULL) {
        *num_chars = ptr - buffer;
    }
    return int_value;
}
