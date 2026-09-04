#include <testpulse_cli/http_client.hpp>

#include <curl/curl.h>

namespace testpulse_cli {

namespace {

size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

HttpResponse Perform(const std::string& url, const std::string& bearerToken, const char* method,
                      const std::string* body) {
    HttpResponse response;

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        response.networkError = true;
        response.networkErrorMessage = "curl_easy_init failed";
        return response;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + bearerToken).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (std::string(method) == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body->size()));
    }

    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        response.networkError = true;
        response.networkErrorMessage = curl_easy_strerror(code);
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

}  // namespace

HttpResponse CurlHttpClient::Post(const std::string& url, const std::string& body,
                                   const std::string& bearerToken) {
    return Perform(url, bearerToken, "POST", &body);
}

HttpResponse CurlHttpClient::Get(const std::string& url, const std::string& bearerToken) {
    return Perform(url, bearerToken, "GET", nullptr);
}

}  // namespace testpulse_cli
