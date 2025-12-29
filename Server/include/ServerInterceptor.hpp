#pragma once

#include <atomic>
#include <chrono>

#include <grpcpp/support/server_interceptor.h>
#include <grpcpp/support/interceptor.h>
#include <grpcpp/grpcpp.h>

#include <spdlog/spdlog.h>

class ServerInterceptor final : public grpc::experimental::Interceptor {
public:
	explicit ServerInterceptor(grpc::experimental::ServerRpcInfo *info,
							   std::shared_ptr<spdlog::logger> logger)
		: info_(info)
		, logger_(std::move(logger))
		, start_(std::chrono::steady_clock::now()) { }

public:
	void Intercept(grpc::experimental::InterceptorBatchMethods *methods) override {
		using grpc::experimental::InterceptionHookPoints;

		if (methods->QueryInterceptionHookPoint(InterceptionHookPoints::POST_RECV_INITIAL_METADATA))
			method_ = info_ ? info_->method() : "";

		if (methods->QueryInterceptionHookPoint(InterceptionHookPoints::POST_RECV_MESSAGE))
			recv_messages_.fetch_add(1, std::memory_order_relaxed);

		if (methods->QueryInterceptionHookPoint(InterceptionHookPoints::PRE_SEND_CANCEL))
			cancelled_.store(true, std::memory_order_relaxed);

		if (methods->QueryInterceptionHookPoint(InterceptionHookPoints::PRE_SEND_STATUS)) {
			const auto end = std::chrono::steady_clock::now();
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();

			const grpc::Status st = methods->GetSendStatus();
			int code = st.error_code();

			if (logger_) {
				logger_->info(
					"[grpc] method={} code={} cancelled={} recv_msgs={} latency_ms={}",
					method_, code,
					cancelled_.load(std::memory_order_relaxed),
					recv_messages_.load(std::memory_order_relaxed),
					ms
				);
			}
		}

		methods->Proceed();
	}

private:
	grpc::experimental::ServerRpcInfo *info_;
	std::shared_ptr<spdlog::logger> logger_;

	std::chrono::steady_clock::time_point start_;
	std::string method_;
	std::atomic<int64_t> recv_messages_{0};
	std::atomic<bool> cancelled_{false};
};

class ServerInterceptorFactory final
	: public grpc::experimental::ServerInterceptorFactoryInterface {
public:
	explicit ServerInterceptorFactory(std::shared_ptr<spdlog::logger> logger)
		: logger_(std::move(logger)) { }

	grpc::experimental::Interceptor* CreateServerInterceptor(
		grpc::experimental::ServerRpcInfo *info
	) override
	{
		return new ServerInterceptor(info, logger_);
	}

private:
	std::shared_ptr<spdlog::logger> logger_;
};
