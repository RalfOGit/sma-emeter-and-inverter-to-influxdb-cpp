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

#define _CRT_SECURE_NO_WARNINGS
#include <Url.hpp>

#ifdef LIB_NAMESPACE
using namespace LIB_NAMESPACE;
#else
using namespace libralfogit;
#endif


/**
 *  Default constructor.
 */
Url::Url(void) :
    url(),
    protocol(),
    host(), 
    port(0),
    path()
{}

/**
 * Constructor providing parsing results of the given url - eg "http://192.168.178.19/status".
 * @param url_ the url string
 */
Url::Url(const std::string& url_) :
    url(url_),
    protocol(),
    user(),
    password(),
    host(),
    port(0),
    path(),
    query(),
    fragment()
{
    parseUrl(url, protocol, host, user, password, port, path, query, fragment);
}

/**
 * Constructor providing parsing results of the given url - eg "http://192.168.178.19/status".
 * @param protocol_ the url protocol, e.g. "http"
 * @param user_ the user credential part, e.g. ://user:password@
 * @param password_ the password credential part, e.g. ://user:password@
 * @param host_ the host ip address, e.g. "192.168.1.2"
 * @param path_ the path, e.g. "/some_path"
 * @param query_ the optional query part of the url, e.g. "?some_query"
 * @param fragment_ the optional fragment part of the url, e.g. "#some_fragment"
 */
Url::Url(const std::string& protocol_, const std::string& user_, const std::string& password_, const std::string& host_, const std::string& path_, const std::string& query_, const std::string& fragment_) :
    protocol(protocol_),
    user(user_),
    password(password_),
    host(host_),
    path(path_),
    query(query_),
    fragment(fragment_)
{
    // percent encode critical characters in the path, query and fragment with their %hex url encoding; e.g. ' ' is replace by %20
    path     = percentEncode(path, '/');
    query    = percentEncode(query, '?');
    fragment = percentEncode(fragment, '#');

    url = protocol;
    url.append("://");
    if (user.length() > 0 || password.length() > 0) {
        url.append(user);
        url.append(":");
        url.append(password);
        url.append("@");
    }
    url.append(host);
    if (protocol == "http") {
        url.append(":80");
        port = 80;
    }
    else if (protocol == "https") {
        url.append(":443");
        port = 443;
    }
    url.append(path);
    url.append(query);
    url.append(fragment);
}

/**
 * Get url string.
 * @return the url
 */
const std::string& Url::getUrl(void) const {
    return url;
}

/**
 * Get protocol part of the url as string.
 * @return the protocol
 */
const std::string& Url::getProtocol(void) const {
    return protocol;
}

/**
 * Get user part of the url as string.
 * @return the user or ""
 */
const std::string& Url::getUser(void) const {
    return user;
}

/**
 * Get password part of the url as string.
 * @return the password or ""
 */
const std::string& Url::getPassword(void) const {
    return password;
}

/**
 * Get host part of the url as string.
 * @return the host name
 */
const std::string& Url::getHost(void) const {
    return host;
}

/**
 * Get port number part of the url as integer.
 * @return the port number
 */
int Url::getPort(void) const {
    return port;
}

/**
 * Get path part of the url as string.
 * @return the path
 */
const std::string& Url::getPath(void) const {
    return path;
}

/**
 * Get query part of the url as string.
 * @return the path
 */
const std::string& Url::getQuery(void) const {
    return query;
}

/**
 * Get fragment part of the url as string.
 * @return the path
 */
const std::string& Url::getFragment(void) const {
    return fragment;
}

/**
 * Parse a given url string and extract protocol, host, port and path.
 * @param url input  - the url input string
 * @param protocol output - the protocol part of the url
 * @param host output     - the host part of the url
 * @param port output     - the port number part of the url
 * @param path output     - the path part of the url
 * @param query output    - the query part of the url
 * @param fragment output - the fragment part of the url
 * @return 0: if successful, -1: if unsuccesful
 */
int Url::parseUrl(const std::string& url, std::string& protocol, std::string& user, std::string& password, std::string& host, int& port, std::string& path, std::string& query, std::string& fragment) {
    std::string::size_type offs = 0;

    // parse protocol
    std::string::size_type offs_protocol_end = url.find("://", offs);
    if (offs_protocol_end != std::string::npos) {
        protocol = url.substr(0, offs_protocol_end);
        if (protocol == "http" ) { port = 80; }
        if (protocol == "https") { port = 443; }
        offs = offs_protocol_end + 3;
    }

    // parse credentials
    std::string::size_type offs_credential_end = url.find_first_of("@/", offs);
    if (offs_credential_end != std::string::npos && url[offs_credential_end] == '@') {
        std::string credential = url.substr(offs, offs_credential_end - offs);
        std::string::size_type i = credential.find(":");
        if (i != std::string::npos) {
            user = credential.substr(0, i);
            password = credential.substr(i + 1);
        }
        else {
            user = credential;
            password = "";
        }
        offs = offs_credential_end + 1;
    }

    // parse hostname
    if (offs_protocol_end != std::string::npos) {
        std::string::size_type i = url.find_first_of(":/", offs);
        if (i != std::string::npos) {
            host = url.substr(offs, i - offs);
            offs = i;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }

    // parse port
    std::string::size_type offs_port = url.find(":", offs);
    if (offs_port == offs) {
        offs = offs_port + 1;
        int n = sscanf(url.c_str() + offs, "%d", &port);
        if (n != 1) {
            return -1;
        }
    }

    // parse path
    std::string::size_type offs_path = url.find("/", offs);
    if (offs_path != std::string::npos) {
        offs = offs_path;
        std::string::size_type i = url.find_first_of("?#", offs);
        if (i != std::string::npos) {
            path = url.substr(offs_path, i - offs);
            offs = i;
        }
        else {
            path = url.substr(offs_path);
        }
    }
    else {
        path = "";
    }

    // parse query
    std::string::size_type offs_query = url.find("?", offs);
    if (offs_query != std::string::npos) {
        offs = offs_query + 1;
        std::string::size_type i = url.find("#", offs);
        if (i != std::string::npos) {
            query = url.substr(offs_query, i - offs);
            offs = i;
        }
        else {
            query = url.substr(offs_query);
        }
    }
    else {
        query = "";
    }

    // parse fragment
    std::string::size_type offs_fragment = url.find("#", offs);
    if (offs_fragment != std::string::npos) {
        offs = offs_fragment + 1;
        fragment = url.substr(offs);
    }
    else {
        fragment = "";
    }

    // replace special characters in the path, query and fragment with their %hex url encoding; e.g. ' ' is replace by %20
    path     = percentEncode(path,     '/');
    query    = percentEncode(query,    '?');
    fragment = percentEncode(fragment, '#');
    user     = percentEncode(user,     '@');
    password = percentEncode(password, '@');

    return 0;
}

/**
 * Replace special characters in the query with their %hex url encoding; e.g. ' ' is replace by %20
 * @param url_component input - the input string
 * @param url_component_identifier input - the first character in the url component, i.e. '/', '?', '#' or '@'
 * @return the input string, where all special characters are replaced
 */
std::string Url::percentEncode(const std::string& url_component, const std::string::value_type url_component_identifier) {
    std::string result;
    size_t offs = 0;

    // keep the first '/', '?' or '#' character (don't do this for '@'!)
    if (url_component.length() > 0) {
        if (url_component[0] == url_component_identifier) {
            result.append(1, url_component_identifier);
            offs = 1;
        }
    }

    // loop over all characters and replace special chaacters with their %hex url encoding
    for (size_t i = offs; i < url_component.length(); ++i) {
        std::string::value_type c = url_component[i];

        // RFC 3986 defines the following characters to be allowed in url components:
        //   path component:      pchar    = unreserved / pct-encoded / sub-delims / ":" / "@"
        //   query component:     query    = *(pchar / "/" / "?")
        //   fragment component:  fragment = *(pchar / "/" / "?")
        //   userinfo component:  userinfo = unreserved / pct-encoded / sub-delims / ":")
        bool unreserved = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                           (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~');
        bool subdelims  = (c == '!' || c == '$' || c == '&' || c == '\''|| c == '(' || c == ')' ||
                           c == '*' || c == '+' || c == ',' || c == ';' || c == '=');
        bool extra      = (c == ':' || c == '@');
        bool extra_qf   = (c == '/' || c == '?');

        // copy any allowed character to the result string
        switch (url_component_identifier) {
            case '/' : 
                if (unreserved || subdelims || extra) {
                    result.append(1, c);
                    continue;
                }
                break;
            case '?':
            case '#':
                if (unreserved || subdelims || extra || extra_qf) {
                    result.append(1, c);
                    continue;
                }
                break;
            case '@':
                if (unreserved || subdelims || c == ':') {
                    result.append(1, c);
                    continue;
                }
                break;
        }
        // copy any pct-encoded character (i.e. "%" HEXDIG HEXDIG) to the result string
        if (c == '%' && (i+2) < url_component.length()) {
            std::string::value_type h1 = url_component[i + 1];
            std::string::value_type h2 = url_component[i + 2];
            if (((h1 >= 'a' && h1 <= 'f') || (h1 >= 'A' && h1 <= 'F') || (h1 >= '0' && h1 <= '9')) &&
                ((h2 >= 'a' && h2 <= 'f') || (h2 >= 'A' && h2 <= 'F') || (h2 >= '0' && h2 <= '9'))) {
                result.append(1, c);
                result.append(1, h1);
                result.append(1, h2);
                i = i + 2;
                continue;
            }
        }
        // pct-encode any other character and append it to the result string
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02X", c);
        result.append(1, '%');
        result.append(buffer);
    }
    return result;
}
