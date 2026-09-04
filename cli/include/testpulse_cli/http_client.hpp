#pragma once

#include <string>

namespace testpulse_cli {

struct HttpResponse {
    long statusCode = 0;
    std::string body;
    bool networkError = false;
    std::string networkErrorMessage;
};

// An injectable seam: unit tests (section 5) use a fake implementation
// with canned responses; the real end-to-end test (5.7) and the actual
// CLI binary use CurlHttpClient against a real server.
class HttpClient {
public:
    virtual ~HttpClient() = default;
    virtual HttpResponse Post(const std::string& url, const std::string& body,
                               const std::string& bearerToken) = 0;
    virtual HttpResponse Get(const std::string& url, const std::string& bearerToken) = 0;
};

class CurlHttpClient : public HttpClient {
public:
    HttpResponse Post(const std::string& url, const std::string& body,
                       const std::string& bearerToken) override;
    HttpResponse Get(const std::string& url, const std::string& bearerToken) override;
};

}  // namespace testpulse_cli
