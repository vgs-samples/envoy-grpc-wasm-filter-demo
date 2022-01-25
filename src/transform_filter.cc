#include <string>
#include <unordered_map>
#include <sstream>

#include "proxy_wasm_intrinsics.h"
#include "transform.pb.h"

class TransformRootContext : public RootContext {
public:
    explicit TransformRootContext(uint32_t id, std::string_view root_id)
        : RootContext(id, root_id) {}

    bool onStart(size_t) override;
    bool onConfigure(size_t) override;
    void onTick() override;
};

class TransformContext : public Context {
public:
    explicit TransformContext(uint32_t id, RootContext *root)
        : Context(id, root) {}

    FilterHeadersStatus onRequestHeaders(uint32_t headers,
                                         bool end_of_stream) override;
    FilterDataStatus onRequestBody(size_t body_buffer_length,
                                   bool end_of_stream) override;
};

class TransformGrpcCallHandler
    : public GrpcCallHandler<google::protobuf::Value> {
public:
    TransformGrpcCallHandler(TransformContext *context, bool header);
    void onSuccess(size_t body_size) override;
    void onFailure(GrpcStatus status) override;

private:
    // The context handler is attached to
    TransformContext *_context;
    // Are we currently processing header of request or not?
    bool _header;
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
    const auto path = getRequestHeader(":path");
    const auto method = getRequestHeader(":method");
    std::stringstream out;
    out << "onRequestHeaders with path=" << path->view()
        << ", method=" << method->view();
    LOG_INFO(out.str());

    GrpcService grpc_service;
    grpc_service.mutable_envoy_grpc()->set_cluster_name("grpc");
    std::string grpc_service_string;
    grpc_service.SerializeToString(&grpc_service_string);

    HeaderRequest request;
    request.set_path(std::string(path->view()).c_str());

    auto result = getRequestHeaderPairs();
    auto pairs = result->pairs();

    LOG_TRACE(std::string("headers: ") + std::to_string(pairs.size()));
    for (auto &p : pairs) {
        LOG_TRACE(std::string(p.first) + std::string(" -> ") +
                  std::string(p.second));
        RequestHeaderItem *item = request.add_headers();
        item->set_key(std::string(p.first));
        item->set_value(std::string(p.second));
    }

    // TODO
    return FilterHeadersStatus::StopAllIterationAndBuffer;
}

FilterDataStatus TransformContext::onRequestBody(size_t body_buffer_length,
                                                 bool end_of_stream) {
    // TODO:
    return FilterDataStatus::Continue;
}

TransformGrpcCallHandler::TransformGrpcCallHandler(TransformContext *context,
                                                   bool header)
    : _context(context), _header(header) {}

void TransformGrpcCallHandler::onSuccess(size_t body_size) {
    // TODO:
}

void TransformGrpcCallHandler::onFailure(GrpcStatus status) {
    // TODO:
}
