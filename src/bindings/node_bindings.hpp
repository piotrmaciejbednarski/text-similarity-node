#pragma once

#include "../engine/similarity_engine.hpp"
#include <memory>
#include <napi.h>
#include <type_traits>

namespace TextSimilarity::Bindings {

// Main addon class for Node.js integration
class TextSimilarityAddon {
public:
  static Napi::Object Initialize(Napi::Env env, Napi::Object exports);

private:
  // Singleton engine instance
  static std::unique_ptr<Core::ISimilarityEngine> engine_;
  static std::once_flag init_flag_;

  // Initialize engine singleton
  static void ensure_engine_initialized();

  // Synchronous methods
  static Napi::Value CalculateSimilarity(const Napi::CallbackInfo &info);
  static Napi::Value CalculateDistance(const Napi::CallbackInfo &info);
  static Napi::Value CalculateSimilarityBatch(const Napi::CallbackInfo &info);

  // Asynchronous methods (Promise-based)
  static Napi::Value CalculateSimilarityAsync(const Napi::CallbackInfo &info);
  static Napi::Value CalculateDistanceAsync(const Napi::CallbackInfo &info);
  static Napi::Value
  CalculateSimilarityBatchAsync(const Napi::CallbackInfo &info);

  // Configuration methods
  static Napi::Value SetGlobalConfiguration(const Napi::CallbackInfo &info);
  static Napi::Value GetGlobalConfiguration(const Napi::CallbackInfo &info);
  static Napi::Value GetSupportedAlgorithms(const Napi::CallbackInfo &info);
  static Napi::Value GetMemoryUsage(const Napi::CallbackInfo &info);
  static Napi::Value ClearCaches(const Napi::CallbackInfo &info);

  // Utility methods
  static Napi::Value ParseAlgorithmType(const Napi::CallbackInfo &info);
  static Napi::Value GetAlgorithmName(const Napi::CallbackInfo &info);

  // Helper functions for type conversion
  static Core::AlgorithmType ExtractAlgorithmType(const Napi::Value &value);
  static Core::AlgorithmConfig ExtractConfig(const Napi::Object &config_obj);
  static Napi::Object ConfigToObject(Napi::Env env,
                                     const Core::AlgorithmConfig &config);
  static Napi::Object ResultToObject(Napi::Env env,
                                     const Core::SimilarityResult &result);
  static Napi::Object ResultToObject(Napi::Env env,
                                     const Core::DistanceResult &result);

  // Input validation
  static bool ValidateStringInput(const Napi::Value &value);
  static bool ValidateConfigInput(const Napi::Value &value);

public:
  // Error creation helpers
  static Napi::Error CreateError(Napi::Env env,
                                 const Core::SimilarityError &error);
  static Napi::Error CreateValidationError(Napi::Env env,
                                           const std::string &message);
};

// Async worker for Promise-based operations
template <typename ResultType> class AsyncWorker : public Napi::AsyncWorker {
public:
  AsyncWorker(Napi::Env env, std::string s1, std::string s2,
              Core::AlgorithmType algorithm, Core::AlgorithmConfig config,
              std::function<ResultType()> work_func)
      : Napi::AsyncWorker(env), deferred_(Napi::Promise::Deferred::New(env)),
        s1_(std::move(s1)), s2_(std::move(s2)), algorithm_(algorithm),
        config_(std::move(config)), work_func_(std::move(work_func)) {}

  ~AsyncWorker() override = default;

  Napi::Promise GetPromise() { return deferred_.Promise(); }

protected:
  void Execute() override {
    try {
      result_ = work_func_();
    } catch (const std::exception &e) {
      SetError(e.what());
    } catch (...) {
      SetError("Unknown error occurred");
    }
  }

  void OnOK() override {
    Napi::Env env = Env();

    if constexpr (std::is_same_v<ResultType, Core::SimilarityResult>) {
      if (result_.is_success()) {
        deferred_.Resolve(Napi::Number::New(env, result_.value()));
      } else {
        auto error = TextSimilarityAddon::CreateError(env, result_.error());
        deferred_.Reject(error.Value());
      }
    } else if constexpr (std::is_same_v<ResultType, Core::DistanceResult>) {
      if (result_.is_success()) {
        deferred_.Resolve(Napi::Number::New(env, result_.value()));
      } else {
        auto error = TextSimilarityAddon::CreateError(env, result_.error());
        deferred_.Reject(error.Value());
      }
    }
  }

  void OnError(const Napi::Error &error) override {
    deferred_.Reject(error.Value());
  }

private:
  Napi::Promise::Deferred deferred_;
  std::string s1_, s2_;
  Core::AlgorithmType algorithm_;
  Core::AlgorithmConfig config_;
  std::function<ResultType()> work_func_;
  ResultType result_;
};

// Batch async worker
class BatchAsyncWorker : public Napi::AsyncWorker {
public:
  BatchAsyncWorker(Napi::Env env,
                   std::vector<std::pair<std::string, std::string>> pairs,
                   Core::AlgorithmType algorithm, Core::AlgorithmConfig config,
                   Core::ISimilarityEngine *engine)
      : Napi::AsyncWorker(env), deferred_(Napi::Promise::Deferred::New(env)),
        pairs_(std::move(pairs)), algorithm_(algorithm),
        config_(std::move(config)), engine_(engine) {}

  Napi::Promise GetPromise() { return deferred_.Promise(); }

protected:
  void Execute() override;
  void OnOK() override;
  void OnError(const Napi::Error &error) override;

private:
  Napi::Promise::Deferred deferred_;
  std::vector<std::pair<std::string, std::string>> pairs_;
  Core::AlgorithmType algorithm_;
  Core::AlgorithmConfig config_;
  Core::ISimilarityEngine *engine_;
  std::vector<Core::SimilarityResult> results_;
};

} // namespace TextSimilarity::Bindings