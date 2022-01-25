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
    TransformContext *_transformContext;
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

    auto res = removeRequestHeader("content-length");
    if (res != WasmResult::Ok) {
        LOG_ERROR("Remove header failed: " + toString(res));
    } else {
        LOG_ERROR("Remove header ok: " + toString(res));
    }

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

    HeaderStringPairs initial_metadata;
    std::string requestPayload = request.SerializeAsString();
    res = root()->grpcCallHandler(
        grpc_service_string, "Transform", "TransformHeader", initial_metadata,
        requestPayload, 1000,
        std::unique_ptr<GrpcCallHandlerBase>(
            new TransformGrpcCallHandler(this, true)));
    if (res != WasmResult::Ok) {
        LOG_ERROR("Sending gRPC failed: " + toString(res));
    }
    return FilterHeadersStatus::StopAllIterationAndBuffer;
}

FilterDataStatus TransformContext::onRequestBody(size_t body_buffer_length,
                                                 bool end_of_stream) {
    // TODO:
    return FilterDataStatus::Continue;
}

TransformGrpcCallHandler::TransformGrpcCallHandler(TransformContext *context,
                                                   bool header)
    : _transformContext(context), _header(header) {}

void TransformGrpcCallHandler::onSuccess(size_t body_size) {
    WasmDataPtr response_data =
        getBufferBytes(WasmBufferType::GrpcReceiveBuffer, 0, body_size);
    const HeaderResponse &response = response_data->proto<HeaderResponse>();

    _transformContext->setEffectiveContext();

    if (this->_header) {
        for (size_t i = 0; i < response.headers_size(); ++i) {
            RequestHeaderItem item = response.headers(i);
            if (item.mutable_value()) {
                auto res = removeRequestHeader(item.key());
                LOG_TRACE("Remove header " + item.key());
                if (res == WasmResult::Ok) {
                    LOG_TRACE("Remove header ok: " + toString(res));
                } else {
                    LOG_ERROR("Remove header failed: " + toString(res));
                }
            } else {
                LOG_TRACE("Add header " + item.key() + " => " + item.value());
                auto res = addRequestHeader(item.key(), item.value());
                if (res == WasmResult::Ok) {
                    LOG_TRACE("Add header ok: " + toString(res));
                } else {
                    LOG_ERROR("Add header failed: " + toString(res));
                }
            }
        }
    } else {
    }
    auto res = continueRequest();
    if (res == WasmResult::Ok) {
        LOG_INFO("Continue ok: " + toString(res));
    } else {
        LOG_ERROR("Continue Failed: " + toString(res));
    }
}

void TransformGrpcCallHandler::onFailure(GrpcStatus status) {
    std::stringstream out;
    auto p = getStatus();
    out << "gRPC failed with status " << static_cast<int>(status) << " "
        << std::string(p.second->view()) << " header = " << _header;
    LOG_ERROR(out.str());
    closeRequest();
}
