#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

class TransformRootContext : public RootContext {
public:
    explicit TransformRootContext(uint32_t id, std::string_view root_id)
        : RootContext(id, root_id) {}

    bool onStart(size_t) override;
    bool onConfigure(size_t) override;
    void onTick() override;
};

class TransformContext;
class TransformGrpcCallHandler
    : public GrpcCallHandler<google::protobuf::Value> {
public:
    TransformGrpcCallHandler(TransformContext *context, bool header);
    void onSuccess(size_t body_size) override;
    void onFailure(GrpcStatus status) override;

private:
    TransformContext *context_;
    bool header;
};

class TransformContext : public Context {
public:
    explicit TransformContext(uint32_t id, RootContext *root)
        : Context(id, root) {}

    FilterHeadersStatus onRequestHeaders(uint32_t headers,
                                         bool end_of_stream) override;
    FilterDataStatus onRequestBody(size_t body_buffer_length,
                                   bool end_of_stream) override;
    FilterTrailersStatus onRequestTrailers(
        uint32_t body_buffer_length) override;
    void onDone() override;
};
static RegisterContextFactory register_TransformContext(
    CONTEXT_FACTORY(TransformContext), ROOT_FACTORY(TransformRootContext),
    "my_root_id");

bool TransformRootContext::onStart(size_t) {
    LOG_TRACE("onStart");
    return true;
}

bool TransformRootContext::onConfigure(size_t) {
    LOG_TRACE("onConfigure");
    proxy_set_tick_period_milliseconds(1000);  // 1 sec
    return true;
}

void TransformRootContext::onTick() { LOG_TRACE("onTick"); }

FilterHeadersStatus TransformContext::onRequestHeaders(uint32_t headers,
                                                       bool end_of_stream) {
    // TODO
    return FilterHeadersStatus::StopAllIterationAndBuffer;
}

FilterDataStatus TransformContext::onRequestBody(size_t body_buffer_length,
                                                 bool end_of_stream) {
    // TODO:
    return FilterDataStatus::Continue;
}

FilterTrailersStatus TransformContext::onRequestTrailers(
    uint32_t body_buffer_length) {
    logInfo(std::string("onRequestTrailers "));
    return FilterTrailersStatus::Continue;
}

void TransformContext::onDone() { logInfo("onDone " + std::to_string(id())); }

TransformGrpcCallHandler::TransformGrpcCallHandler(TransformContext *context,
                                                   bool header) {
    context_ = context;
    this->header = header;
}

void TransformGrpcCallHandler::onSuccess(size_t body_size) {
    // TODO:
}

void TransformGrpcCallHandler::onFailure(GrpcStatus status) {
    // TODO:
}
