#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <sstream>
#include <string>
#include <thread>

namespace testpulse_cli_test {

// A minimal, real HTTP/1.1 stub server over a raw POSIX socket -- used
// only by the real end-to-end test (submit_e2e_test.cc) to prove the CLI's
// actual CurlHttpClient talks to a real server correctly, not a mocked
// HttpClient. Single-threaded, handles one request at a time, enough for
// a test's sequential requests.
class StubServer {
public:
    // handler receives (method, path, requestBody) and returns
    // (statusCode, responseBody).
    using Handler = std::function<std::pair<int, std::string>(const std::string&, const std::string&,
                                                                const std::string&)>;

    explicit StubServer(Handler handler) : handler_(std::move(handler)) {
        listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;  // let the OS pick a free port
        bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        socklen_t len = sizeof(addr);
        getsockname(listenFd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);

        listen(listenFd_, 8);
        running_ = true;
        thread_ = std::thread([this] { AcceptLoop(); });
    }

    ~StubServer() {
        running_ = false;
        shutdown(listenFd_, SHUT_RDWR);
        close(listenFd_);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string Url() const { return "http://127.0.0.1:" + std::to_string(port_); }

    std::string LastPath() const { return lastPath_; }
    std::string LastMethod() const { return lastMethod_; }
    std::string LastBody() const { return lastBody_; }

private:
    void AcceptLoop() {
        while (running_) {
            int clientFd = accept(listenFd_, nullptr, nullptr);
            if (clientFd < 0) {
                if (!running_) {
                    return;
                }
                continue;
            }
            HandleClient(clientFd);
            close(clientFd);
        }
    }

    void HandleClient(int clientFd) {
        std::string raw;
        char buf[4096];
        ssize_t n;
        size_t headerEnd = std::string::npos;
        while ((n = recv(clientFd, buf, sizeof(buf), 0)) > 0) {
            raw.append(buf, static_cast<size_t>(n));
            headerEnd = raw.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                size_t contentLength = ParseContentLength(raw.substr(0, headerEnd));
                size_t bodyReceived = raw.size() - (headerEnd + 4);
                if (bodyReceived >= contentLength) {
                    break;
                }
            }
        }
        if (headerEnd == std::string::npos) {
            return;
        }

        std::istringstream requestLine(raw.substr(0, raw.find("\r\n")));
        std::string method, path, httpVersion;
        requestLine >> method >> path >> httpVersion;

        std::string body = raw.substr(headerEnd + 4);

        lastMethod_ = method;
        lastPath_ = path;
        lastBody_ = body;

        auto [status, responseBody] = handler_(method, path, body);

        std::ostringstream response;
        response << "HTTP/1.1 " << status << " X\r\n"
                  << "Content-Type: application/json\r\n"
                  << "Content-Length: " << responseBody.size() << "\r\n"
                  << "Connection: close\r\n\r\n"
                  << responseBody;
        std::string responseStr = response.str();
        send(clientFd, responseStr.data(), responseStr.size(), 0);
    }

    static size_t ParseContentLength(const std::string& headers) {
        size_t pos = headers.find("Content-Length:");
        if (pos == std::string::npos) {
            pos = headers.find("content-length:");
        }
        if (pos == std::string::npos) {
            return 0;
        }
        pos += std::string("Content-Length:").size();
        return static_cast<size_t>(std::stoul(headers.substr(pos)));
    }

    Handler handler_;
    int listenFd_ = -1;
    int port_ = 0;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::string lastMethod_;
    std::string lastPath_;
    std::string lastBody_;
};

}  // namespace testpulse_cli_test
