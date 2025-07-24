#include "similarity_engine.hpp"
#include "../core/dependency_container.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace TextSimilarity::Engine {

// AsyncExecutor Implementation

AsyncExecutor::AsyncExecutor(size_t thread_count) {
  thread_count = std::max(static_cast<size_t>(1), thread_count);
  workers_.reserve(thread_count);

  for (size_t i = 0; i < thread_count; ++i) {
    workers_.emplace_back(&AsyncExecutor::worker_loop, this);
  }
}

AsyncExecutor::~AsyncExecutor() { shutdown(); }

Core::AsyncSimilarityResult AsyncExecutor::calculate_similarity_async(
    std::unique_ptr<Core::ISimilarityAlgorithm> algorithm,
    Core::UnicodeString s1, Core::UnicodeString s2) {
  auto promise = std::make_shared<std::promise<Core::Result<double>>>();
  auto future = promise->get_future();

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (shutdown_.load()) {
      promise->set_value(Core::SimilarityResult{Core::SimilarityError{
          Core::ErrorCode::ThreadingError, "Executor is shutting down"}});
      return future;
    }

    task_queue_.emplace([alg = std::shared_ptr<Core::ISimilarityAlgorithm>(
                             std::move(algorithm)),
                         s1, s2, promise]() {
      try {
        auto result = alg->calculate_similarity(s1, s2);
        promise->set_value(std::move(result));
      } catch (const std::exception &e) {
        promise->set_value(Core::SimilarityResult{
            Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}});
      } catch (...) {
        promise->set_value(Core::SimilarityResult{Core::SimilarityError{
            Core::ErrorCode::Unknown, "Unknown error occurred"}});
      }
    });
  }

  cv_.notify_one();
  return future;
}

Core::AsyncDistanceResult AsyncExecutor::calculate_distance_async(
    std::unique_ptr<Core::ISimilarityAlgorithm> algorithm,
    Core::UnicodeString s1, Core::UnicodeString s2) {
  auto promise = std::make_shared<std::promise<Core::Result<uint32_t>>>();
  auto future = promise->get_future();

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (shutdown_.load()) {
      promise->set_value(Core::DistanceResult{Core::SimilarityError{
          Core::ErrorCode::ThreadingError, "Executor is shutting down"}});
      return future;
    }

    task_queue_.emplace([alg = std::shared_ptr<Core::ISimilarityAlgorithm>(
                             std::move(algorithm)),
                         s1, s2, promise]() {
      try {
        auto result = alg->calculate_distance(s1, s2);
        promise->set_value(std::move(result));
      } catch (const std::exception &e) {
        promise->set_value(Core::DistanceResult{
            Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}});
      } catch (...) {
        promise->set_value(Core::DistanceResult{Core::SimilarityError{
            Core::ErrorCode::Unknown, "Unknown error occurred"}});
      }
    });
  }

  cv_.notify_one();
  return future;
}

void AsyncExecutor::shutdown() noexcept {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    shutdown_.store(true);
  }

  cv_.notify_all();

  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  workers_.clear();
}

void AsyncExecutor::worker_loop() {
  while (!shutdown_.load()) {
    Task task{[]() {}};

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      cv_.wait(lock,
               [this] { return shutdown_.load() || !task_queue_.empty(); });

      if (shutdown_.load()) {
        break;
      }

      if (!task_queue_.empty()) {
        task = std::move(task_queue_.front());
        task_queue_.pop();
      }
    }

    if (task.work) {
      task.work();
    }
  }
}

// ConfigurationManager Implementation

void ConfigurationManager::set_global_config(
    const Core::AlgorithmConfig &config) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  global_config_ = config;
}

Core::AlgorithmConfig ConfigurationManager::get_global_config() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return global_config_;
}

void ConfigurationManager::set_algorithm_config(
    Core::AlgorithmType type, const Core::AlgorithmConfig &config) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  algorithm_configs_[type] = config;
}

Core::AlgorithmConfig
ConfigurationManager::get_algorithm_config(Core::AlgorithmType type) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  auto it = algorithm_configs_.find(type);
  if (it != algorithm_configs_.end()) {
    return it->second;
  }

  return get_default_config();
}

void ConfigurationManager::reset_to_defaults() noexcept {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  global_config_ = get_default_config();
  algorithm_configs_.clear();
}

Core::AlgorithmConfig
ConfigurationManager::get_default_config() const noexcept {
  Core::AlgorithmConfig config;
  config.algorithm = Core::AlgorithmType::Levenshtein;
  config.preprocessing = Core::PreprocessingMode::Character;
  config.normalization = Core::NormalizationMode::Similarity;
  config.case_sensitivity = Core::CaseSensitivity::Sensitive;
  config.ngram_size = 2;
  return config;
}

// SimilarityEngine Implementation

SimilarityEngine::SimilarityEngine(
    std::unique_ptr<Core::IAlgorithmFactory> factory,
    std::unique_ptr<Core::IAsyncExecutor> executor,
    std::unique_ptr<Core::IConfigurationManager> config_manager)
    : factory_(factory ? std::move(factory)
                       : std::make_unique<Core::AlgorithmFactory>()),
      executor_(executor ? std::move(executor)
                         : std::make_unique<AsyncExecutor>()),
      config_manager_(config_manager
                          ? std::move(config_manager)
                          : std::make_unique<ConfigurationManager>()) {}

SimilarityEngine::~SimilarityEngine() { shutdown(); }

Core::SimilarityResult SimilarityEngine::calculate_similarity(
    const std::string &s1, const std::string &s2, Core::AlgorithmType algorithm,
    const Core::AlgorithmConfig &config) {
  total_operations_.fetch_add(1, std::memory_order_relaxed);

  // Input validation
  if (!validate_input(s1, s2)) {
    return create_validation_error("Invalid input strings");
  }

  try {
    // Merge configurations
    auto global_config = config_manager_->get_global_config();
    auto algorithm_config = config_manager_->get_algorithm_config(algorithm);
    auto final_config =
        merge_configs(global_config, algorithm_config, algorithm);

    // Override with provided config
    if (config.algorithm != Core::AlgorithmType::Levenshtein ||
        config.preprocessing != Core::PreprocessingMode::None) {
      final_config = merge_configs(final_config, config, algorithm);
    }

    // Check cache
    auto cache_key = create_cache_key(s1, s2, algorithm, final_config);
    if (auto cached = get_cached_result(cache_key)) {
      cache_hits_.fetch_add(1, std::memory_order_relaxed);
      return Core::SimilarityResult{*cached};
    }

    // Create Unicode strings
    Core::UnicodeString us1(s1);
    Core::UnicodeString us2(s2);

    // Create algorithm instance
    auto algo = factory_->create_algorithm(algorithm, final_config);

    // Perform calculation
    auto result = algo->calculate_similarity(us1, us2);

    // Cache result if successful
    if (result.is_success()) {
      cache_result(cache_key, result.value());
    }

    return result;

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}};
  }
}

Core::DistanceResult SimilarityEngine::calculate_distance(
    const std::string &s1, const std::string &s2, Core::AlgorithmType algorithm,
    const Core::AlgorithmConfig &config) {
  total_operations_.fetch_add(1, std::memory_order_relaxed);

  if (!validate_input(s1, s2)) {
    return Core::DistanceResult{Core::SimilarityError{
        Core::ErrorCode::InvalidInput, "Invalid input strings"}};
  }

  try {
    auto global_config = config_manager_->get_global_config();
    auto algorithm_config = config_manager_->get_algorithm_config(algorithm);
    auto final_config =
        merge_configs(global_config, algorithm_config, algorithm);

    if (config.algorithm != Core::AlgorithmType::Levenshtein ||
        config.preprocessing != Core::PreprocessingMode::None) {
      final_config = merge_configs(final_config, config, algorithm);
    }

    Core::UnicodeString us1(s1);
    Core::UnicodeString us2(s2);

    auto algo = factory_->create_algorithm(algorithm, final_config);
    return algo->calculate_distance(us1, us2);

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}};
  }
}

Core::AsyncSimilarityResult
SimilarityEngine::calculate_similarity_async(std::string s1, std::string s2,
                                             Core::AlgorithmType algorithm,
                                             Core::AlgorithmConfig config) {
  try {
    if (!validate_input(s1, s2)) {
      auto promise = std::promise<Core::Result<double>>();
      promise.set_value(create_validation_error("Invalid input strings"));
      return promise.get_future();
    }

    auto global_config = config_manager_->get_global_config();
    auto algorithm_config = config_manager_->get_algorithm_config(algorithm);
    auto final_config =
        merge_configs(global_config, algorithm_config, algorithm);
    final_config = merge_configs(final_config, config, algorithm);

    Core::UnicodeString us1(std::move(s1));
    Core::UnicodeString us2(std::move(s2));

    auto algo = factory_->create_algorithm(algorithm, final_config);

    return executor_->calculate_similarity_async(
        std::move(algo), std::move(us1), std::move(us2));

  } catch (const std::exception &e) {
    auto promise = std::promise<Core::Result<double>>();
    promise.set_value(Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}});
    return promise.get_future();
  }
}

Core::AsyncDistanceResult
SimilarityEngine::calculate_distance_async(std::string s1, std::string s2,
                                           Core::AlgorithmType algorithm,
                                           Core::AlgorithmConfig config) {
  try {
    if (!validate_input(s1, s2)) {
      auto promise = std::promise<Core::Result<uint32_t>>();
      promise.set_value(Core::DistanceResult{Core::SimilarityError{
          Core::ErrorCode::InvalidInput, "Invalid input strings"}});
      return promise.get_future();
    }

    auto global_config = config_manager_->get_global_config();
    auto algorithm_config = config_manager_->get_algorithm_config(algorithm);
    auto final_config =
        merge_configs(global_config, algorithm_config, algorithm);
    final_config = merge_configs(final_config, config, algorithm);

    Core::UnicodeString us1(std::move(s1));
    Core::UnicodeString us2(std::move(s2));

    auto algo = factory_->create_algorithm(algorithm, final_config);

    return executor_->calculate_distance_async(std::move(algo), std::move(us1),
                                               std::move(us2));

  } catch (const std::exception &e) {
    auto promise = std::promise<Core::Result<uint32_t>>();
    promise.set_value(Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}});
    return promise.get_future();
  }
}

std::vector<Core::SimilarityResult>
SimilarityEngine::calculate_similarity_batch(
    const std::vector<std::pair<std::string, std::string>> &pairs,
    Core::AlgorithmType algorithm, const Core::AlgorithmConfig &config) {
  std::vector<Core::SimilarityResult> results;
  results.reserve(pairs.size());

  for (const auto &[s1, s2] : pairs) {
    results.push_back(calculate_similarity(s1, s2, algorithm, config));
  }

  return results;
}

void SimilarityEngine::set_global_configuration(
    const Core::AlgorithmConfig &config) {
  config_manager_->set_global_config(config);
  clear_caches(); // Clear cache as configuration changed
}

Core::AlgorithmConfig SimilarityEngine::get_global_configuration() const {
  return config_manager_->get_global_config();
}

std::vector<Core::AlgorithmType>
SimilarityEngine::get_supported_algorithms() const noexcept {
  return factory_->get_supported_algorithms();
}

bool SimilarityEngine::supports_algorithm(
    Core::AlgorithmType type) const noexcept {
  return factory_->supports_algorithm(type);
}

size_t SimilarityEngine::get_memory_usage() const noexcept {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  return result_cache_.size() * (sizeof(CacheEntry) + 200); // Rough estimate
}

void SimilarityEngine::clear_caches() noexcept {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  result_cache_.clear();
}

void SimilarityEngine::shutdown() noexcept {
  if (executor_) {
    executor_->shutdown();
  }
  clear_caches();
}

std::future<std::vector<Core::SimilarityResult>>
SimilarityEngine::calculate_similarity_batch_parallel(
    std::vector<std::pair<std::string, std::string>> pairs,
    Core::AlgorithmType algorithm, Core::AlgorithmConfig config) {
  return std::async(std::launch::async, [this, pairs = std::move(pairs),
                                         algorithm, config]() {
    // Use a thread pool to parallelize the work
    std::vector<std::future<Core::SimilarityResult>> futures;
    futures.reserve(pairs.size());

    for (const auto &pair : pairs) {
      const auto &s1 = pair.first;
      const auto &s2 = pair.second;
      futures.emplace_back(
          std::async(std::launch::async, [this, s1, s2, algorithm, config]() {
            return calculate_similarity(s1, s2, algorithm, config);
          }));
    }

    // Collect results
    std::vector<Core::SimilarityResult> results;
    results.reserve(futures.size());

    for (auto &future : futures) {
      results.push_back(future.get());
    }

    return results;
  });
}

// Private helper methods

Core::AlgorithmConfig
SimilarityEngine::merge_configs(const Core::AlgorithmConfig &global,
                                const Core::AlgorithmConfig &local,
                                Core::AlgorithmType algorithm) const {
  Core::AlgorithmConfig merged = global;

  // Override with local config values
  if (local.algorithm != Core::AlgorithmType::Levenshtein) {
    merged.algorithm = local.algorithm;
  } else {
    merged.algorithm = algorithm;
  }

  if (local.preprocessing != Core::PreprocessingMode::None) {
    merged.preprocessing = local.preprocessing;
  }

  if (local.normalization != Core::NormalizationMode::None) {
    merged.normalization = local.normalization;
  }

  if (local.case_sensitivity != Core::CaseSensitivity::Sensitive) {
    merged.case_sensitivity = local.case_sensitivity;
  }

  if (local.ngram_size != 2) {
    merged.ngram_size = local.ngram_size;
  }

  // Optional parameters
  if (local.threshold.has_value()) {
    merged.threshold = local.threshold;
  }

  if (local.alpha.has_value()) {
    merged.alpha = local.alpha;
  }

  if (local.beta.has_value()) {
    merged.beta = local.beta;
  }

  if (local.prefix_weight.has_value()) {
    merged.prefix_weight = local.prefix_weight;
  }

  if (local.prefix_length.has_value()) {
    merged.prefix_length = local.prefix_length;
  }

  return merged;
}

std::string
SimilarityEngine::create_cache_key(const std::string &s1, const std::string &s2,
                                   Core::AlgorithmType algorithm,
                                   const Core::AlgorithmConfig &config) const {
  std::ostringstream oss;
  oss << static_cast<int>(algorithm) << "|"
      << static_cast<int>(config.preprocessing) << "|"
      << static_cast<int>(config.case_sensitivity) << "|" << config.ngram_size
      << "|" << s1 << "|" << s2;
  return oss.str();
}

std::optional<double>
SimilarityEngine::get_cached_result(const std::string &key) const {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  auto it = result_cache_.find(key);
  if (it != result_cache_.end()) {
    auto now = std::chrono::steady_clock::now();
    if (now - it->second.timestamp < CACHE_TTL) {
      return it->second.result;
    } else {
      // Expired entry
      result_cache_.erase(it);
    }
  }

  return std::nullopt;
}

void SimilarityEngine::cache_result(const std::string &key,
                                    double result) const {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  // Simple LRU-like eviction
  if (result_cache_.size() >= MAX_CACHE_SIZE) {
    cleanup_cache();
  }

  result_cache_[key] =
      CacheEntry{key, result, std::chrono::steady_clock::now()};
}

void SimilarityEngine::cleanup_cache() const {
  auto now = std::chrono::steady_clock::now();

  // Remove expired entries
  for (auto it = result_cache_.begin(); it != result_cache_.end();) {
    if (now - it->second.timestamp >= CACHE_TTL) {
      it = result_cache_.erase(it);
    } else {
      ++it;
    }
  }

  // If still too full, remove oldest entries
  if (result_cache_.size() >= MAX_CACHE_SIZE) {
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>>
        timestamps;
    timestamps.reserve(result_cache_.size());

    for (const auto &[key, entry] : result_cache_) {
      timestamps.emplace_back(entry.timestamp, key);
    }

    std::sort(timestamps.begin(), timestamps.end());

    size_t to_remove = result_cache_.size() - (MAX_CACHE_SIZE / 2);
    for (size_t i = 0; i < to_remove && i < timestamps.size(); ++i) {
      result_cache_.erase(timestamps[i].second);
    }
  }
}

bool SimilarityEngine::validate_input(const std::string &s1,
                                      const std::string &s2) const noexcept {
  // Basic validation - strings shouldn't be too large
  constexpr size_t MAX_STRING_LENGTH = 100000; // 100KB limit

  return s1.length() <= MAX_STRING_LENGTH && s2.length() <= MAX_STRING_LENGTH;
}

Core::SimilarityResult
SimilarityEngine::create_validation_error(const std::string &message) const {
  return Core::SimilarityResult{
      Core::SimilarityError{Core::ErrorCode::InvalidInput, message}};
}

// Factory function

std::unique_ptr<Core::ISimilarityEngine>
create_similarity_engine(size_t thread_pool_size) {
  auto factory = Core::create_algorithm_factory();
  auto executor = std::make_unique<AsyncExecutor>(thread_pool_size);
  auto config_manager = std::make_unique<ConfigurationManager>();

  return std::make_unique<SimilarityEngine>(
      std::move(factory), std::move(executor), std::move(config_manager));
}

} // namespace TextSimilarity::Engine