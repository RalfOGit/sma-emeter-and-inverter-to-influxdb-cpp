#ifndef __RALFOGIT_HTTPCLIENT_HPP__
#define __RALFOGIT_HTTPCLIENT_HPP__

#include <string>

#ifdef LIB_NAMESPACE
namespace LIB_NAMESPACE {
#else
namespace libralfogit {
#endif

    /**
     *  Class implementing a very basic http client.
     */
    class HttpClient {
    public:

        HttpClient(void);
        ~HttpClient(void);

        int sendHttpGetRequest(const std::string& url, std::string& response, std::string& content);
        int sendHttpPutRequest(const std::string& url, const std::string& request_data, std::string& response, std::string& content);
        int sendHttpPostRequest(const std::string& url, const std::string& request_data, std::string& response, std::string& content);

    protected:

        char* recv_buffer;
        size_t recv_buffer_size;

        int sendHttpRequest(const std::string& url, const std::string& method, const std::string& request_data, std::string& response, std::string& content);
        int connect_to_server(const std::string& host, const int port);
        int communicate_with_server(const int socket_fd, const std::string& request, std::string& response, std::string& content);
        size_t recv_http_response(int socket_fd);
        static int    parse_http_response(const char* buffer, size_t buffer_size, std::string& http_response, std::string& http_content);
        static int    get_http_return_code(const char* buffer, size_t buffer_size);
        static size_t get_content_length(const char* buffer, size_t buffer_size);
        static size_t get_content_offset(const char* buffer, size_t buffer_size);
        static bool   is_chunked_encoding(const char* buffer, size_t buffer_size);
        static size_t get_chunk_length(const char* buffer, size_t buffer_size);
        static size_t get_chunk_offset(const char* buffer, size_t buffer_size);
        static size_t get_next_chunk_offset(const char* buffer, size_t buffer_size);
        static std::string base64_encode(const std::string& text);
        static const char* find(const char* hay, size_t hay_size, const char* needle);
        static const char* skipSpaceCharacters(const char* buffer, size_t buffer_size);
        static size_t scanUint(const char* buffer, size_t buffer_size, size_t* num_hars);
        static size_t scanHex(const char* buffer, size_t buffer_size, size_t* num_hars);
    };

}   // namespace ralfogit

#endif
