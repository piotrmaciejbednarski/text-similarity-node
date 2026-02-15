#include "node_bindings.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace TextSimilarity::Bindings {

// Static member initialization
std::unique_ptr<Core::ISimilarityEngine> TextSimilarityAddon::engine_;
std::once_flag TextSimilarityAddon::init_flag_;

Napi::Object TextSimilarityAddon::Initialize(Napi::Env env,
                                             Napi::Object exports) {
  // Initialize engine
  ensure_engine_initialized();

  // Export synchronous methods
  exports.Set("calculateSimilarity",
              Napi::Function::New(env, CalculateSimilarity));
  exports.Set("calculateDistance", Napi::Function::New(env, CalculateDistance));
  exports.Set("calculateSimilarityBatch",
              Napi::Function::New(env, CalculateSimilarityBatch));

  // Export asynchronous methods
  exports.Set("calculateSimilarityAsync",
              Napi::Function::New(env, CalculateSimilarityAsync));
  exports.Set("calculateDistanceAsync",
              Napi::Function::New(env, CalculateDistanceAsync));
  exports.Set("calculateSimilarityBatchAsync",
              Napi::Function::New(env, CalculateSimilarityBatchAsync));

  // Export configuration methods
  exports.Set("setGlobalConfiguration",
              Napi::Function::New(env, SetGlobalConfiguration));
  exports.Set("getGlobalConfiguration",
              Napi::Function::New(env, GetGlobalConfiguration));
  exports.Set("getSupportedAlgorithms",
              Napi::Function::New(env, GetSupportedAlgorithms));
  exports.Set("getMemoryUsage", Napi::Function::New(env, GetMemoryUsage));
  exports.Set("clearCaches", Napi::Function::New(env, ClearCaches));

  // Export utility methods
  exports.Set("parseAlgorithmType",
              Napi::Function::New(env, ParseAlgorithmType));
  exports.Set("getAlgorithmName", Napi::Function::New(env, GetAlgorithmName));

  // Export algorithm type constants
  Napi::Object algorithms = Napi::Object::New(env);
  algorithms.Set("LEVENSHTEIN",
                 Napi::Number::New(
                     env, static_cast<int>(Core::AlgorithmType::Levenshtein)));
  algorithms.Set(
      "DAMERAU_LEVENSHTEIN",
      Napi::Number::New(
          env, static_cast<int>(Core::AlgorithmType::DamerauLevenshtein)));
  algorithms.Set(
      "HAMMING",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Hamming)));
  algorithms.Set("JARO", Napi::Number::New(
                             env, static_cast<int>(Core::AlgorithmType::Jaro)));
  algorithms.Set("JARO_WINKLER",
                 Napi::Number::New(
                     env, static_cast<int>(Core::AlgorithmType::JaroWinkler)));
  algorithms.Set(
      "JACCARD",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Jaccard)));
  algorithms.Set("SORENSEN_DICE",
                 Napi::Number::New(
                     env, static_cast<int>(Core::AlgorithmType::SorensenDice)));
  algorithms.Set(
      "OVERLAP",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Overlap)));
  algorithms.Set(
      "TVERSKY",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Tversky)));
  algorithms.Set(
      "COSINE",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Cosine)));
  algorithms.Set(
      "EUCLIDEAN",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Euclidean)));
  algorithms.Set(
      "MANHATTAN",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Manhattan)));
  algorithms.Set(
      "CHEBYSHEV",
      Napi::Number::New(env, static_cast<int>(Core::AlgorithmType::Chebyshev)));
  exports.Set("AlgorithmType", algorithms);

  // Export preprocessing mode constants
  Napi::Object preprocessing = Napi::Object::New(env);
  preprocessing.Set(
      "NONE",
      Napi::Number::New(env, static_cast<int>(Core::PreprocessingMode::None)));
  preprocessing.Set(
      "CHARACTER",
      Napi::Number::New(env,
                        static_cast<int>(Core::PreprocessingMode::Character)));
  preprocessing.Set(
      "WORD",
      Napi::Number::New(env, static_cast<int>(Core::PreprocessingMode::Word)));
  preprocessing.Set(
      "NGRAM",
      Napi::Number::New(env, static_cast<int>(Core::PreprocessingMode::NGram)));
  exports.Set("PreprocessingMode", preprocessing);

  // Export case sensitivity constants
  Napi::Object caseSensitivity = Napi::Object::New(env);
  caseSensitivity.Set(
      "SENSITIVE",
      Napi::Number::New(env,
                        static_cast<int>(Core::CaseSensitivity::Sensitive)));
  caseSensitivity.Set(
      "INSENSITIVE",
      Napi::Number::New(env,
                        static_cast<int>(Core::CaseSensitivity::Insensitive)));
  exports.Set("CaseSensitivity", caseSensitivity);

  return exports;
}

void TextSimilarityAddon::ensure_engine_initialized() {
  std::call_once(init_flag_,
                 []() { engine_ = Engine::create_similarity_engine(); });
}

// Synchronous methods

Napi::Value
TextSimilarityAddon::CalculateSimilarity(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    // Validate arguments
    if (info.Length() < 2) {
      throw Napi::TypeError::New(
          env, "Expected at least 2 arguments: string1, string2");
    }

    if (!ValidateStringInput(info[0]) || !ValidateStringInput(info[1])) {
      throw Napi::TypeError::New(env, "Arguments must be strings");
    }

    std::string s1 = info[0].As<Napi::String>().Utf8Value();
    std::string s2 = info[1].As<Napi::String>().Utf8Value();

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 2 && info[2].IsNumber()) {
      algorithm = ExtractAlgorithmType(info[2]);
    } else if (info.Length() > 2 && info[2].IsString()) {
      std::string algo_name = info[2].As<Napi::String>().Utf8Value();
      auto parsed = Core::parse_algorithm_type(algo_name);
      if (parsed.has_value()) {
        algorithm = *parsed;
      }
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 3 && info[3].IsObject()) {
      config = ExtractConfig(info[3].As<Napi::Object>());
    }

    auto result = engine_->calculate_similarity(s1, s2, algorithm, config);
    return ResultToObject(env, result);

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Null();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value
TextSimilarityAddon::CalculateDistance(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 2) {
      throw Napi::TypeError::New(
          env, "Expected at least 2 arguments: string1, string2");
    }

    if (!ValidateStringInput(info[0]) || !ValidateStringInput(info[1])) {
      throw Napi::TypeError::New(env, "Arguments must be strings");
    }

    std::string s1 = info[0].As<Napi::String>().Utf8Value();
    std::string s2 = info[1].As<Napi::String>().Utf8Value();

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 2) {
      algorithm = ExtractAlgorithmType(info[2]);
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 3 && info[3].IsObject()) {
      config = ExtractConfig(info[3].As<Napi::Object>());
    }

    auto result = engine_->calculate_distance(s1, s2, algorithm, config);
    return ResultToObject(env, result);

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Null();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value
TextSimilarityAddon::CalculateSimilarityBatch(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 1 || !info[0].IsArray()) {
      throw Napi::TypeError::New(
          env, "Expected array of string pairs as first argument");
    }

    Napi::Array pairs_array = info[0].As<Napi::Array>();
    std::vector<std::pair<std::string, std::string>> pairs;

    for (uint32_t i = 0; i < pairs_array.Length(); ++i) {
      Napi::Value pair_value = pairs_array.Get(i);
      if (!pair_value.IsArray()) {
        throw Napi::TypeError::New(
            env, "Each element must be an array of two strings");
      }

      Napi::Array pair = pair_value.As<Napi::Array>();
      if (pair.Length() != 2) {
        throw Napi::TypeError::New(
            env, "Each pair must contain exactly two strings");
      }

      std::string s1 = pair.Get(0u).As<Napi::String>().Utf8Value();
      std::string s2 = pair.Get(1u).As<Napi::String>().Utf8Value();
      pairs.emplace_back(std::move(s1), std::move(s2));
    }

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 1) {
      algorithm = ExtractAlgorithmType(info[1]);
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 2 && info[2].IsObject()) {
      config = ExtractConfig(info[2].As<Napi::Object>());
    }

    auto results =
        engine_->calculate_similarity_batch(pairs, algorithm, config);

    Napi::Array result_array = Napi::Array::New(env, results.size());
    for (size_t i = 0; i < results.size(); ++i) {
      result_array.Set(i, ResultToObject(env, results[i]));
    }

    return result_array;

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Null();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

// Asynchronous methods

Napi::Value
TextSimilarityAddon::CalculateSimilarityAsync(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 2) {
      throw Napi::TypeError::New(
          env, "Expected at least 2 arguments: string1, string2");
    }

    if (!ValidateStringInput(info[0]) || !ValidateStringInput(info[1])) {
      throw Napi::TypeError::New(env, "Arguments must be strings");
    }

    std::string s1 = info[0].As<Napi::String>().Utf8Value();
    std::string s2 = info[1].As<Napi::String>().Utf8Value();

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 2) {
      algorithm = ExtractAlgorithmType(info[2]);
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 3 && info[3].IsObject()) {
      config = ExtractConfig(info[3].As<Napi::Object>());
    }

    auto worker = new AsyncWorker<Core::SimilarityResult>(
        env, s1, s2, algorithm, config, [s1, s2, algorithm, config]() {
          return engine_->calculate_similarity(s1, s2, algorithm, config);
        });

    auto promise = worker->GetPromise();
    worker->Queue();

    return promise;

  } catch (const Napi::Error &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(e.Value());
    return deferred.Promise();
  } catch (const std::exception &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(Napi::Error::New(env, e.what()).Value());
    return deferred.Promise();
  }
}

Napi::Value
TextSimilarityAddon::CalculateDistanceAsync(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 2) {
      throw Napi::TypeError::New(
          env, "Expected at least 2 arguments: string1, string2");
    }

    if (!ValidateStringInput(info[0]) || !ValidateStringInput(info[1])) {
      throw Napi::TypeError::New(env, "Arguments must be strings");
    }

    std::string s1 = info[0].As<Napi::String>().Utf8Value();
    std::string s2 = info[1].As<Napi::String>().Utf8Value();

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 2) {
      algorithm = ExtractAlgorithmType(info[2]);
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 3 && info[3].IsObject()) {
      config = ExtractConfig(info[3].As<Napi::Object>());
    }

    auto worker = new AsyncWorker<Core::DistanceResult>(
        env, s1, s2, algorithm, config, [s1, s2, algorithm, config]() {
          return engine_->calculate_distance(s1, s2, algorithm, config);
        });

    auto promise = worker->GetPromise();
    worker->Queue();

    return promise;

  } catch (const Napi::Error &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(e.Value());
    return deferred.Promise();
  } catch (const std::exception &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(Napi::Error::New(env, e.what()).Value());
    return deferred.Promise();
  }
}

Napi::Value TextSimilarityAddon::CalculateSimilarityBatchAsync(
    const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 1 || !info[0].IsArray()) {
      throw Napi::TypeError::New(
          env, "Expected array of string pairs as first argument");
    }

    Napi::Array pairs_array = info[0].As<Napi::Array>();
    std::vector<std::pair<std::string, std::string>> pairs;

    for (uint32_t i = 0; i < pairs_array.Length(); ++i) {
      Napi::Value pair_value = pairs_array.Get(i);
      if (!pair_value.IsArray()) {
        throw Napi::TypeError::New(
            env, "Each element must be an array of two strings");
      }

      Napi::Array pair = pair_value.As<Napi::Array>();
      if (pair.Length() != 2) {
        throw Napi::TypeError::New(
            env, "Each pair must contain exactly two strings");
      }

      std::string s1 = pair.Get(0u).As<Napi::String>().Utf8Value();
      std::string s2 = pair.Get(1u).As<Napi::String>().Utf8Value();
      pairs.emplace_back(std::move(s1), std::move(s2));
    }

    Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein;
    if (info.Length() > 1) {
      algorithm = ExtractAlgorithmType(info[1]);
    }

    Core::AlgorithmConfig config{};
    if (info.Length() > 2 && info[2].IsObject()) {
      config = ExtractConfig(info[2].As<Napi::Object>());
    }

    auto worker = new BatchAsyncWorker(env, std::move(pairs), algorithm, config,
                                       engine_.get());
    auto promise = worker->GetPromise();
    worker->Queue();

    return promise;

  } catch (const Napi::Error &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(e.Value());
    return deferred.Promise();
  } catch (const std::exception &e) {
    auto deferred = Napi::Promise::Deferred::New(env);
    deferred.Reject(Napi::Error::New(env, e.what()).Value());
    return deferred.Promise();
  }
}

// Configuration and utility methods

Napi::Value
TextSimilarityAddon::SetGlobalConfiguration(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 1 || !info[0].IsObject()) {
      throw Napi::TypeError::New(
          env, "Expected configuration object as first argument");
    }

    auto config = ExtractConfig(info[0].As<Napi::Object>());
    engine_->set_global_configuration(config);

    return env.Undefined();

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Null();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value
TextSimilarityAddon::GetGlobalConfiguration(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    auto config = engine_->get_global_configuration();
    return ConfigToObject(env, config);

  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value
TextSimilarityAddon::GetSupportedAlgorithms(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    auto algorithms = engine_->get_supported_algorithms();
    Napi::Array result = Napi::Array::New(env, algorithms.size());

    for (size_t i = 0; i < algorithms.size(); ++i) {
      Napi::Object algo_info = Napi::Object::New(env);
      algo_info.Set("type",
                    Napi::Number::New(env, static_cast<int>(algorithms[i])));
      algo_info.Set("name", Napi::String::New(
                                env, Core::get_algorithm_name(algorithms[i])));
      result.Set(i, algo_info);
    }

    return result;

  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value
TextSimilarityAddon::GetMemoryUsage(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    auto usage = engine_->get_memory_usage();
    return Napi::Number::New(env, static_cast<double>(usage));

  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value TextSimilarityAddon::ClearCaches(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    engine_->clear_caches();
    return env.Undefined();

  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

// Helper functions

Core::AlgorithmType
TextSimilarityAddon::ExtractAlgorithmType(const Napi::Value &value) {
  if (value.IsNumber()) {
    int type_int = value.As<Napi::Number>().Int32Value();
    return static_cast<Core::AlgorithmType>(type_int);
  } else if (value.IsString()) {
    std::string type_str = value.As<Napi::String>().Utf8Value();
    auto parsed = Core::parse_algorithm_type(type_str);
    if (parsed.has_value()) {
      return *parsed;
    }
  }

  return Core::AlgorithmType::Levenshtein; // Default
}

Core::AlgorithmConfig
TextSimilarityAddon::ExtractConfig(const Napi::Object &config_obj) {
  Core::AlgorithmConfig config{};

  if (config_obj.Has("algorithm")) {
    config.algorithm = ExtractAlgorithmType(config_obj.Get("algorithm"));
  }

  if (config_obj.Has("preprocessing") &&
      config_obj.Get("preprocessing").IsNumber()) {
    int mode = config_obj.Get("preprocessing").As<Napi::Number>().Int32Value();
    config.preprocessing = static_cast<Core::PreprocessingMode>(mode);
  }

  if (config_obj.Has("caseSensitivity") &&
      config_obj.Get("caseSensitivity").IsNumber()) {
    int sensitivity =
        config_obj.Get("caseSensitivity").As<Napi::Number>().Int32Value();
    config.case_sensitivity = static_cast<Core::CaseSensitivity>(sensitivity);
  }

  if (config_obj.Has("ngramSize") && config_obj.Get("ngramSize").IsNumber()) {
    config.ngram_size =
        config_obj.Get("ngramSize").As<Napi::Number>().Uint32Value();
  }

  if (config_obj.Has("threshold") && config_obj.Get("threshold").IsNumber()) {
    config.threshold =
        config_obj.Get("threshold").As<Napi::Number>().DoubleValue();
  }

  if (config_obj.Has("alpha") && config_obj.Get("alpha").IsNumber()) {
    config.alpha = config_obj.Get("alpha").As<Napi::Number>().DoubleValue();
  }

  if (config_obj.Has("beta") && config_obj.Get("beta").IsNumber()) {
    config.beta = config_obj.Get("beta").As<Napi::Number>().DoubleValue();
  }

  if (config_obj.Has("prefixWeight") &&
      config_obj.Get("prefixWeight").IsNumber()) {
    config.prefix_weight =
        config_obj.Get("prefixWeight").As<Napi::Number>().DoubleValue();
  }

  if (config_obj.Has("prefixLength") &&
      config_obj.Get("prefixLength").IsNumber()) {
    config.prefix_length =
        config_obj.Get("prefixLength").As<Napi::Number>().Uint32Value();
  }

  if (config_obj.Has("maxStringLength") &&
      config_obj.Get("maxStringLength").IsNumber()) {
    config.max_string_length = static_cast<size_t>(
        config_obj.Get("maxStringLength").As<Napi::Number>().Int64Value());
  }

  return config;
}

Napi::Object
TextSimilarityAddon::ConfigToObject(Napi::Env env,
                                    const Core::AlgorithmConfig &config) {
  Napi::Object obj = Napi::Object::New(env);

  obj.Set("algorithm",
          Napi::Number::New(env, static_cast<int>(config.algorithm)));
  obj.Set("preprocessing",
          Napi::Number::New(env, static_cast<int>(config.preprocessing)));
  obj.Set("caseSensitivity",
          Napi::Number::New(env, static_cast<int>(config.case_sensitivity)));
  obj.Set("ngramSize", Napi::Number::New(env, config.ngram_size));

  if (config.threshold.has_value()) {
    obj.Set("threshold", Napi::Number::New(env, *config.threshold));
  }

  if (config.alpha.has_value()) {
    obj.Set("alpha", Napi::Number::New(env, *config.alpha));
  }

  if (config.beta.has_value()) {
    obj.Set("beta", Napi::Number::New(env, *config.beta));
  }

  if (config.prefix_weight.has_value()) {
    obj.Set("prefixWeight", Napi::Number::New(env, *config.prefix_weight));
  }

  if (config.prefix_length.has_value()) {
    obj.Set("prefixLength", Napi::Number::New(env, *config.prefix_length));
  }

  if (config.max_string_length.has_value()) {
    obj.Set(
        "maxStringLength",
        Napi::Number::New(env, static_cast<double>(*config.max_string_length)));
  }

  return obj;
}

Napi::Object
TextSimilarityAddon::ResultToObject(Napi::Env env,
                                    const Core::SimilarityResult &result) {
  Napi::Object obj = Napi::Object::New(env);

  if (result.is_success()) {
    obj.Set("success", Napi::Boolean::New(env, true));
    obj.Set("value", Napi::Number::New(env, result.value()));
  } else {
    obj.Set("success", Napi::Boolean::New(env, false));
    obj.Set("error", CreateError(env, result.error()).Value());
  }

  return obj;
}

Napi::Object
TextSimilarityAddon::ResultToObject(Napi::Env env,
                                    const Core::DistanceResult &result) {
  Napi::Object obj = Napi::Object::New(env);

  if (result.is_success()) {
    obj.Set("success", Napi::Boolean::New(env, true));
    obj.Set("value", Napi::Number::New(env, result.value()));
  } else {
    obj.Set("success", Napi::Boolean::New(env, false));
    obj.Set("error", CreateError(env, result.error()).Value());
  }

  return obj;
}

bool TextSimilarityAddon::ValidateStringInput(const Napi::Value &value) {
  return value.IsString();
}

bool TextSimilarityAddon::ValidateConfigInput(const Napi::Value &value) {
  return value.IsObject() || value.IsUndefined() || value.IsNull();
}

Napi::Error
TextSimilarityAddon::CreateError(Napi::Env env,
                                 const Core::SimilarityError &error) {
  return Napi::Error::New(env, error.message());
}

Napi::Error
TextSimilarityAddon::CreateValidationError(Napi::Env env,
                                           const std::string &message) {
  return Napi::Error::New(env, "Validation Error: " + message);
}

// BatchAsyncWorker implementation

void BatchAsyncWorker::Execute() {
  try {
    results_ = engine_->calculate_similarity_batch(pairs_, algorithm_, config_);
  } catch (const std::exception &e) {
    SetError(e.what());
  } catch (...) {
    SetError("Unknown error occurred in batch processing");
  }
}

void BatchAsyncWorker::OnOK() {
  Napi::Env env = Env();
  Napi::Array result_array = Napi::Array::New(env, results_.size());

  for (size_t i = 0; i < results_.size(); ++i) {
    if (results_[i].is_success()) {
      result_array.Set(i, Napi::Number::New(env, results_[i].value()));
    } else {
      auto error = TextSimilarityAddon::CreateError(env, results_[i].error());
      result_array.Set(i, error.Value());
    }
  }

  deferred_.Resolve(result_array);
}

void BatchAsyncWorker::OnError(const Napi::Error &error) {
  deferred_.Reject(error.Value());
}

// Utility function implementations

Napi::Value
TextSimilarityAddon::ParseAlgorithmType(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 1 || !info[0].IsString()) {
      throw Napi::Error::New(env, "Expected string argument");
    }

    std::string algorithm_name = info[0].As<Napi::String>().Utf8Value();

    // Convert to lowercase for comparison
    std::transform(algorithm_name.begin(), algorithm_name.end(),
                   algorithm_name.begin(), ::tolower);

    static const std::unordered_map<std::string, Core::AlgorithmType> name_map =
        {{"levenshtein", Core::AlgorithmType::Levenshtein},
         {"damerau-levenshtein", Core::AlgorithmType::DamerauLevenshtein},
         {"damerauLevenshtein", Core::AlgorithmType::DamerauLevenshtein},
         {"hamming", Core::AlgorithmType::Hamming},
         {"jaro", Core::AlgorithmType::Jaro},
         {"jaro-winkler", Core::AlgorithmType::JaroWinkler},
         {"jaroWinkler", Core::AlgorithmType::JaroWinkler},
         {"jaccard", Core::AlgorithmType::Jaccard},
         {"sorensen-dice", Core::AlgorithmType::SorensenDice},
         {"sorensenDice", Core::AlgorithmType::SorensenDice},
         {"dice", Core::AlgorithmType::SorensenDice},
         {"overlap", Core::AlgorithmType::Overlap},
         {"tversky", Core::AlgorithmType::Tversky},
         {"cosine", Core::AlgorithmType::Cosine},
         {"euclidean", Core::AlgorithmType::Euclidean},
         {"manhattan", Core::AlgorithmType::Manhattan},
         {"chebyshev", Core::AlgorithmType::Chebyshev}};

    auto it = name_map.find(algorithm_name);
    if (it != name_map.end()) {
      return Napi::Number::New(env, static_cast<int>(it->second));
    }

    return env.Undefined();

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value
TextSimilarityAddon::GetAlgorithmName(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  try {
    if (info.Length() < 1 || !info[0].IsNumber()) {
      throw Napi::Error::New(env, "Expected number argument");
    }

    int algorithm_type = info[0].As<Napi::Number>().Int32Value();

    static const std::unordered_map<int, std::string> type_map = {
        {static_cast<int>(Core::AlgorithmType::Levenshtein), "Levenshtein"},
        {static_cast<int>(Core::AlgorithmType::DamerauLevenshtein),
         "Damerau-Levenshtein"},
        {static_cast<int>(Core::AlgorithmType::Hamming), "Hamming"},
        {static_cast<int>(Core::AlgorithmType::Jaro), "Jaro"},
        {static_cast<int>(Core::AlgorithmType::JaroWinkler), "Jaro-Winkler"},
        {static_cast<int>(Core::AlgorithmType::Jaccard), "Jaccard"},
        {static_cast<int>(Core::AlgorithmType::SorensenDice), "Sorensen-Dice"},
        {static_cast<int>(Core::AlgorithmType::Overlap), "Overlap"},
        {static_cast<int>(Core::AlgorithmType::Tversky), "Tversky"},
        {static_cast<int>(Core::AlgorithmType::Cosine), "Cosine"},
        {static_cast<int>(Core::AlgorithmType::Euclidean), "Euclidean"},
        {static_cast<int>(Core::AlgorithmType::Manhattan), "Manhattan"},
        {static_cast<int>(Core::AlgorithmType::Chebyshev), "Chebyshev"}};

    auto it = type_map.find(algorithm_type);
    if (it != type_map.end()) {
      return Napi::String::New(env, it->second);
    }

    return env.Undefined();

  } catch (const Napi::Error &e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

} // namespace TextSimilarity::Bindings

// Module initialization
Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
  return TextSimilarity::Bindings::TextSimilarityAddon::Initialize(env,
                                                                   exports);
}

NODE_API_MODULE(text_similarity_node, InitModule)