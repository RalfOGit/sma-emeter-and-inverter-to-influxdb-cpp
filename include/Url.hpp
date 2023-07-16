#ifndef __RALFOGIT_URL_HPP__
#define __RALFOGIT_URL_HPP__

#include <string>

#ifdef LIB_NAMESPACE
namespace LIB_NAMESPACE {
#else
namespace libralfogit {
#endif

    /**
     *  Class implementing url parsing.
     */
    class Url {
    public:

        Url(void);
        Url(const std::string& url);
        Url(const std::string& protocol, const std::string& user_, const std::string& password_, const std::string& host, const std::string& path, const std::string& query, const std::string& fragment);

        const std::string& getUrl(void) const;
        const std::string& getProtocol(void) const;
        const std::string& getUser(void) const;
        const std::string& getPassword(void) const;
        const std::string& getHost(void) const;
              int          getPort(void) const;
        const std::string& getPath(void) const;
        const std::string& getQuery(void) const;
        const std::string& getFragment(void) const;

        static int parseUrl(const std::string& url, std::string& protocol, std::string& user, std::string& password, std::string& host, int& port, std::string& path, std::string& query, std::string& fragment);

        static std::string percentEncode(const std::string& url_component, const std::string::value_type url_component_identifier);

    private:
        std::string url;
        std::string protocol;
        std::string user;
        std::string password;
        std::string host;
        int         port;
        std::string path;
        std::string query;
        std::string fragment;
    };

}   // namespace ralfogit

#endif
