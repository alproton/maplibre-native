#include <mbgl/storage/http_file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/logging.hpp>

#include <mbgl/util/async_request.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/http_header.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/util.hpp>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

#include <jni/jni.hpp>
#include "attach_env.hpp"

namespace mbgl {

class HTTPFileSource::Impl {
public:
    Impl(const ResourceOptions resourceOptions_, const ClientOptions clientOptions_)
        : resourceOptions(resourceOptions_.clone()),
          clientOptions(clientOptions_.clone()) {};

    android::UniqueEnv env{android::AttachEnv()};

    void setResourceOptions(ResourceOptions options) { resourceOptions = options; };
    ResourceOptions getResourceOptions() { return resourceOptions.clone(); };

    void setClientOptions(ClientOptions options) { clientOptions = options; };
    ClientOptions getClientOptions() { return clientOptions.clone(); };

private:
    ResourceOptions resourceOptions;
    ClientOptions clientOptions;
};

// Log level enum — ordinal values must match Java HttpRequestLogLevel
enum class HTTPRequestLogLevel : int {
    Verbose = 0,
    Stats = 1
};

struct HTTPRequestLogOptions {
    HTTPRequestLogLevel level = HTTPRequestLogLevel::Stats;
    Seconds statsDuration{0};
};

struct HTTPRequestStats {
    TimePoint windowStart = Clock::now();
    uint32_t totalRequests = 0;
    uint32_t successfulRequests = 0;
    uint32_t failedRequests = 0;
    std::vector<int64_t> successElapsedMs;

    void reset() {
        windowStart = Clock::now();
        totalRequests = 0;
        successfulRequests = 0;
        failedRequests = 0;
        successElapsedMs.clear();
    }

    void logAndReset(Seconds windowDuration) {
        if (totalRequests == 0) {
            reset();
            return;
        }

        int64_t minMs = 0, maxMs = 0, medianMs = 0, avgMs = 0;
        if (!successElapsedMs.empty()) {
            std::sort(successElapsedMs.begin(), successElapsedMs.end());
            minMs = successElapsedMs.front();
            maxMs = successElapsedMs.back();
            size_t n = successElapsedMs.size();
            medianMs = (n % 2 == 0)
                ? (successElapsedMs[n / 2 - 1] + successElapsedMs[n / 2]) / 2
                : successElapsedMs[n / 2];
            int64_t sum = 0;
            for (int64_t ms : successElapsedMs) { sum += ms; }
            avgMs = sum / static_cast<int64_t>(n);
        }

        Log::Info(Event::HttpRequest,
                  "Tile download stats (" + util::toString(windowDuration.count()) + "s window):"
                  " Total=" + util::toString(totalRequests) +
                  " Success=" + util::toString(successfulRequests) +
                  " Failed=" + util::toString(failedRequests) +
                  " Min=" + util::toString(minMs) + "ms" +
                  " Max=" + util::toString(maxMs) + "ms" +
                  " Avg=" + util::toString(avgMs) + "ms" +
                  " Median=" + util::toString(medianMs) + "ms");

        reset();
    }
};

class HTTPRequest : public AsyncRequest {
public:
    static constexpr auto Name() { return "org/maplibre/android/http/NativeHttpRequest"; };

    HTTPRequest(jni::JNIEnv&, const Resource&, FileSource::Callback);
    ~HTTPRequest();

    void onFailure(jni::JNIEnv&, int type, const jni::String& message);
    void onResponse(jni::JNIEnv&,
                    int code,
                    const jni::String& etag,
                    const jni::String& modified,
                    const jni::String& cacheControl,
                    const jni::String& expires,
                    const jni::String& retryAfter,
                    const jni::String& xRateLimitReset,
                    const jni::Array<jni::jbyte>& body);

    static std::atomic<bool> timingLogsEnabled;
    static std::mutex statsMutex;
    static HTTPRequestLogOptions logOptions;
    static HTTPRequestStats stats;

    jni::Global<jni::Object<HTTPRequest>> javaRequest;

private:
    void recordTileStats(int64_t elapsedMs, bool success);

    Resource resource;
    FileSource::Callback callback;
    Response response;
    TimePoint requestStartTime;

    util::AsyncTask async{[this] {
        // Calling `callback` may result in deleting `this`. Copy data to temporaries first.
        auto callback_ = callback;
        auto response_ = response;
        callback_(response_);
    }};

    static const int connectionError = 0;
    static const int temporaryError = 1;
    static const int permanentError = 2;
};

namespace android {

// JNI bridge for HttpRequestUtil static native methods
class HttpRequestUtil {
public:
    static constexpr auto Name() { return "org/maplibre/android/module/http/HttpRequestUtil"; }

    static void nativeSetTimingLogsEnabled(jni::JNIEnv&, const jni::Class<HttpRequestUtil>&,
                                           jni::jboolean enabled) {
        HTTPRequest::timingLogsEnabled.store(enabled, std::memory_order_relaxed);
    }

    static void nativeSetHttpRequestLogOptions(jni::JNIEnv&, const jni::Class<HttpRequestUtil>&,
                                               jni::jint level, jni::jlong durationSeconds) {
        std::lock_guard<std::mutex> lock(HTTPRequest::statsMutex);

        HTTPRequestLogOptions opts;
        opts.level = static_cast<HTTPRequestLogLevel>(level);
        opts.statsDuration = Seconds(durationSeconds);
        HTTPRequest::logOptions = opts;
        HTTPRequest::stats.reset();
    }
};

void RegisterNativeHTTPRequest(jni::JNIEnv& env) {
    static auto& javaClass = jni::Class<HTTPRequest>::Singleton(env);

#define METHOD(MethodPtr, name) jni::MakeNativePeerMethod<decltype(MethodPtr), (MethodPtr)>(name)

    jni::RegisterNativePeer<HTTPRequest>(env,
                                         javaClass,
                                         "nativePtr",
                                         METHOD(&HTTPRequest::onFailure, "nativeOnFailure"),
                                         METHOD(&HTTPRequest::onResponse, "nativeOnResponse"));

    // Register static native methods on HttpRequestUtil
    static auto& httpRequestUtilClass = jni::Class<HttpRequestUtil>::Singleton(env);
    jni::RegisterNatives(
        env,
        *httpRequestUtilClass,
        jni::MakeNativeMethod<decltype(&HttpRequestUtil::nativeSetTimingLogsEnabled),
                              &HttpRequestUtil::nativeSetTimingLogsEnabled>("nativeSetTimingLogsEnabled"),
        jni::MakeNativeMethod<decltype(&HttpRequestUtil::nativeSetHttpRequestLogOptions),
                              &HttpRequestUtil::nativeSetHttpRequestLogOptions>("nativeSetHttpRequestLogOptions"));
}

} // namespace android

HTTPRequest::HTTPRequest(jni::JNIEnv& env, const Resource& resource_, FileSource::Callback callback_)
    : resource(resource_),
      callback(callback_),
      requestStartTime(Clock::now()) {
    std::string dataRangeStr;
    std::string etagStr;
    std::string modifiedStr;

    if (resource.dataRange) {
        dataRangeStr = std::string("bytes=") + std::to_string(resource.dataRange->first) + std::string("-") +
                       std::to_string(resource.dataRange->second);
    }

    if (resource.priorEtag) {
        etagStr = *resource.priorEtag;
    } else if (resource.priorModified) {
        modifiedStr = util::rfc1123(*resource.priorModified);
    }

    jni::UniqueLocalFrame frame = jni::PushLocalFrame(env, 10);

    static auto& javaClass = jni::Class<HTTPRequest>::Singleton(env);
    static auto constructor =
        javaClass.GetConstructor<jni::jlong, jni::String, jni::String, jni::String, jni::String, jni::jboolean>(env);

    javaRequest = jni::NewGlobal(env,
                                 javaClass.New(env,
                                               constructor,
                                               reinterpret_cast<jlong>(this),
                                               jni::Make<jni::String>(env, resource.url),
                                               jni::Make<jni::String>(env, dataRangeStr),
                                               jni::Make<jni::String>(env, etagStr),
                                               jni::Make<jni::String>(env, modifiedStr),
                                               (jboolean)(resource_.usage == Resource::Usage::Offline)));
}

HTTPRequest::~HTTPRequest() {
    android::UniqueEnv env = android::AttachEnv();

    static auto& javaClass = jni::Class<HTTPRequest>::Singleton(*env);
    static auto cancel = javaClass.GetMethod<void()>(*env, "cancel");

    javaRequest.Call(*env, cancel);
}

void HTTPRequest::onResponse(jni::JNIEnv& env,
                             int code,
                             const jni::String& etag,
                             const jni::String& modified,
                             const jni::String& cacheControl,
                             const jni::String& expires,
                             const jni::String& jRetryAfter,
                             const jni::String& jXRateLimitReset,
                             const jni::Array<jni::jbyte>& body) {
    if (resource.kind == Resource::Kind::Tile && timingLogsEnabled) {
        auto elapsed = std::chrono::duration_cast<Milliseconds>(Clock::now() - requestStartTime);
        // A tile download is considered successful when the server returned
        // either a 2xx (OK, No Content, Partial Content, etc.) or 304
        // (Not Modified — cached data is still valid). Server errors (4xx, 5xx)
        // are not counted as successful since no usable tile data was obtained.
        // See https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
        bool success = ((code >= 200 && code < 300) || code == 304);

        if (logOptions.level == HTTPRequestLogLevel::Verbose) {
            size_t dataSize = body ? body.Length(env) : 0;
            Log::Info(Event::HttpRequest,
                      "Tile download completed: URL=" + resource.url +
                      " Status=" + util::toString(code) +
                      " Size=" + util::toString(dataSize) + "B" +
                      " Elapsed=" + util::toString(elapsed.count()) + "ms");
        } else if (logOptions.level == HTTPRequestLogLevel::Stats) {
            recordTileStats(elapsed.count(), success);
        }
    }

    using Error = Response::Error;

    if (etag) {
        response.etag = jni::Make<std::string>(env, etag);
    }

    if (modified) {
        response.modified = util::parseTimestamp(jni::Make<std::string>(env, modified).c_str());
    }

    if (cacheControl) {
        const auto cc = http::CacheControl::parse(jni::Make<std::string>(env, cacheControl).c_str());
        response.expires = cc.toTimePoint();
        response.mustRevalidate = cc.mustRevalidate;
    }

    if (expires) {
        response.expires = util::parseTimestamp(jni::Make<std::string>(env, expires).c_str());
    }

    if (code == 200 || code == 206) {
        if (body) {
            auto data = std::make_shared<std::string>(body.Length(env), char());
            jni::GetArrayRegion(env, *body, 0, data->size(), reinterpret_cast<jbyte*>(&(*data)[0]));
            response.data = data;
        } else {
            response.data = std::make_shared<std::string>();
        }
    } else if (code == 204 || (code == 404 && resource.kind == Resource::Kind::Tile)) {
        response.noContent = true;
    } else if (code == 304) {
        response.notModified = true;
    } else if (code == 404) {
        response.error = std::make_unique<Error>(Error::Reason::NotFound, "HTTP status code 404");
    } else if (code == 429) {
        std::optional<std::string> retryAfter;
        std::optional<std::string> xRateLimitReset;
        if (jRetryAfter) {
            retryAfter = jni::Make<std::string>(env, jRetryAfter);
        }
        if (jXRateLimitReset) {
            xRateLimitReset = jni::Make<std::string>(env, jXRateLimitReset);
        }
        response.error = std::make_unique<Error>(
            Error::Reason::RateLimit, "HTTP status code 429", http::parseRetryHeaders(retryAfter, xRateLimitReset));
    } else if (code >= 500 && code < 600) {
        response.error = std::make_unique<Error>(Error::Reason::Server,
                                                 std::string{"HTTP status code "} + util::toString(code));
    } else {
        response.error = std::make_unique<Error>(Error::Reason::Other,
                                                 std::string{"HTTP status code "} + util::toString(code));
    }

    async.send();
}

void HTTPRequest::onFailure(jni::JNIEnv& env, int type, const jni::String& message) {
    std::string messageStr = jni::Make<std::string>(env, message);

    if (resource.kind == Resource::Kind::Tile && timingLogsEnabled) {
        auto elapsed = std::chrono::duration_cast<Milliseconds>(Clock::now() - requestStartTime);

        if (logOptions.level == HTTPRequestLogLevel::Verbose) {
            Log::Warning(Event::HttpRequest,
                         "Tile download failed: URL=" + resource.url +
                         " Error=" + messageStr +
                         " Elapsed=" + util::toString(elapsed.count()) + "ms");
        } else if (logOptions.level == HTTPRequestLogLevel::Stats) {
            recordTileStats(elapsed.count(), false);
        }
    }

    using Error = Response::Error;

    switch (type) {
        case connectionError:
            response.error = std::make_unique<Error>(Error::Reason::Connection, messageStr);
            break;
        case temporaryError:
            response.error = std::make_unique<Error>(Error::Reason::Server, messageStr);
            break;
        default:
            response.error = std::make_unique<Error>(Error::Reason::Other, messageStr);
    }

    async.send();
}

std::atomic<bool> HTTPRequest::timingLogsEnabled{false};
std::mutex HTTPRequest::statsMutex;
HTTPRequestLogOptions HTTPRequest::logOptions;
HTTPRequestStats HTTPRequest::stats;

void HTTPRequest::recordTileStats(int64_t elapsedMs, bool success) {
    std::lock_guard<std::mutex> lock(statsMutex);

    stats.totalRequests++;
    if (success) {
        stats.successfulRequests++;
        stats.successElapsedMs.push_back(elapsedMs);
    } else {
        stats.failedRequests++;
    }

    auto elapsed = std::chrono::duration_cast<Seconds>(Clock::now() - stats.windowStart);
    if (elapsed >= logOptions.statsDuration) {
        stats.logAndReset(logOptions.statsDuration);
    }
}

HTTPFileSource::HTTPFileSource(const ResourceOptions& resourceOptions, const ClientOptions& clientOptions)
    : impl(std::make_unique<Impl>(resourceOptions.clone(), clientOptions.clone())) {}

HTTPFileSource::~HTTPFileSource() = default;

std::unique_ptr<AsyncRequest> HTTPFileSource::request(const Resource& resource, Callback callback) {
    return std::make_unique<HTTPRequest>(*impl->env, resource, callback);
}

void HTTPFileSource::setResourceOptions(ResourceOptions options) {
    impl->setResourceOptions(options.clone());
}

ResourceOptions HTTPFileSource::getResourceOptions() {
    return impl->getResourceOptions();
}

void HTTPFileSource::setClientOptions(ClientOptions options) {
    impl->setClientOptions(options.clone());
}

ClientOptions HTTPFileSource::getClientOptions() {
    return impl->getClientOptions();
}

} // namespace mbgl
