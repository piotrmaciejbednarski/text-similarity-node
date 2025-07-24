#pragma once

#include "../core/algorithm_factory.hpp"
#include "../core/interfaces.hpp"
#include <atomic>
#include <future>
#include <queue>
#include <thread>

namespace TextSimilarity::Engine {

// Async executor for non-blocking operations
class AsyncExecutor final : public Core::IAsyncExecutor {
public:
  explicit AsyncExecutor(
      size_t thread_count = std::thread::hardware_concurrency());
  ~AsyncExecutor() override;

  // Non-copyable, non-movable
  AsyncExecutor(const AsyncExecutor &) = delete;
  AsyncExecutor &operator=(const AsyncExecutor &) = delete;
  AsyncExecutor(AsyncExecutor &&) = delete;
  AsyncExecutor &operator=(AsyncExecutor &&) = delete;

  [[nodiscard]] Core::AsyncSimilarityResult calculate_similarity_async(
      std::unique_ptr<Core::ISimilarityAlgorithm> algorithm,
      Core::UnicodeString s1, Core::UnicodeString s2) override;

  [[nodiscard]] Core::AsyncDistanceResult calculate_distance_async(
      std::unique_ptr<Core::ISimilarityAlgorithm> algorithm,
      Core::UnicodeString s1, Core::UnicodeString s2) override;

  void shutdown() noexcept override;

private:
  struct Task {
    std::function<void()> work;

    Task(std::function<void()> w) : work(std::move(w)) {}
  };

  std::vector<std::thread> workers_;
  std::queue<Task> task_queue_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::atomic<bool> shutdown_{false};

  void worker_loop();
};

// Configuration manager implementation
class ConfigurationManager final : public Core::IConfigurationManager {
public:
  ConfigurationManager() = default;
  ~ConfigurationManager() override = default;

  void set_global_config(const Core::AlgorithmConfig &config) override;
  [[nodiscard]] Core::AlgorithmConfig get_global_config() const override;

  void set_algorithm_config(Core::AlgorithmType type,
                            const Core::AlgorithmConfig &config) override;
  [[nodiscard]] Core::AlgorithmConfig
  get_algorithm_config(Core::AlgorithmType type) const override;

  void reset_to_defaults() noexcept override;

private:
  mutable std::shared_mutex mutex_;
  Core::AlgorithmConfig global_config_;
  std::unordered_map<Core::AlgorithmType, Core::AlgorithmConfig>
      algorithm_configs_;

  [[nodiscard]] Core::AlgorithmConfig get_default_config() const noexcept;
};

// Main similarity engine implementation
class SimilarityEngine final : public Core::ISimilarityEngine {
public:
  explicit SimilarityEngine(
      std::unique_ptr<Core::IAlgorithmFactory> factory = nullptr,
      std::unique_ptr<Core::IAsyncExecutor> executor = nullptr,
      std::unique_ptr<Core::IConfigurationManager> config_manager = nullptr);

  ~SimilarityEngine() override;

  // Non-copyable, non-movable (due to complex internal state)
  SimilarityEngine(const SimilarityEngine &) = delete;
  SimilarityEngine &operator=(const SimilarityEngine &) = delete;
  SimilarityEngine(SimilarityEngine &&) = delete;
  SimilarityEngine &operator=(SimilarityEngine &&) = delete;

  // Synchronous operations
  [[nodiscard]] Core::SimilarityResult calculate_similarity(
      const std::string &s1, const std::string &s2,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      const Core::AlgorithmConfig &config = {}) override;

  [[nodiscard]] Core::DistanceResult calculate_distance(
      const std::string &s1, const std::string &s2,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      const Core::AlgorithmConfig &config = {}) override;

  // Asynchronous operations
  [[nodiscard]] Core::AsyncSimilarityResult calculate_similarity_async(
      std::string s1, std::string s2,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      Core::AlgorithmConfig config = {}) override;

  [[nodiscard]] Core::AsyncDistanceResult calculate_distance_async(
      std::string s1, std::string s2,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      Core::AlgorithmConfig config = {}) override;

  // Batch operations
  [[nodiscard]] std::vector<Core::SimilarityResult> calculate_similarity_batch(
      const std::vector<std::pair<std::string, std::string>> &pairs,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      const Core::AlgorithmConfig &config = {}) override;

  // Configuration management
  void set_global_configuration(const Core::AlgorithmConfig &config) override;
  [[nodiscard]] Core::AlgorithmConfig get_global_configuration() const override;

  // Algorithm information
  [[nodiscard]] std::vector<Core::AlgorithmType>
  get_supported_algorithms() const noexcept override;
  [[nodiscard]] bool
  supports_algorithm(Core::AlgorithmType type) const noexcept override;

  // Performance and diagnostics
  [[nodiscard]] size_t get_memory_usage() const noexcept override;
  void clear_caches() noexcept override;
  void shutdown() noexcept override;

  // Advanced batch operations with progress callback
  template <typename ProgressCallback>
  [[nodiscard]] std::vector<Core::SimilarityResult>
  calculate_similarity_batch_with_progress(
      const std::vector<std::pair<std::string, std::string>> &pairs,
      Core::AlgorithmType algorithm, const Core::AlgorithmConfig &config,
      ProgressCallback &&callback);

  // Parallel batch processing
  [[nodiscard]] std::future<std::vector<Core::SimilarityResult>>
  calculate_similarity_batch_parallel(
      std::vector<std::pair<std::string, std::string>> pairs,
      Core::AlgorithmType algorithm = Core::AlgorithmType::Levenshtein,
      Core::AlgorithmConfig config = {});

private:
  std::unique_ptr<Core::IAlgorithmFactory> factory_;
  std::unique_ptr<Core::IAsyncExecutor> executor_;
  std::unique_ptr<Core::IConfigurationManager> config_manager_;

  // Performance metrics
  mutable std::atomic<size_t> total_operations_{0};
  mutable std::atomic<size_t> cache_hits_{0};

  // Result caching (simple LRU-like cache)
  struct CacheEntry {
    std::string key;
    double result;
    std::chrono::steady_clock::time_point timestamp;
  };

  mutable std::mutex cache_mutex_;
  mutable std::unordered_map<std::string, CacheEntry> result_cache_;
  static constexpr size_t MAX_CACHE_SIZE = 10000;
  static constexpr std::chrono::minutes CACHE_TTL{5};

  // Helper methods
  [[nodiscard]] Core::AlgorithmConfig
  merge_configs(const Core::AlgorithmConfig &global,
                const Core::AlgorithmConfig &local,
                Core::AlgorithmType algorithm) const;

  [[nodiscard]] std::string
  create_cache_key(const std::string &s1, const std::string &s2,
                   Core::AlgorithmType algorithm,
                   const Core::AlgorithmConfig &config) const;

  [[nodiscard]] std::optional<double>
  get_cached_result(const std::string &key) const;
  void cache_result(const std::string &key, double result) const;
  void cleanup_cache() const;

  // Validation helpers
  [[nodiscard]] bool validate_input(const std::string &s1,
                                    const std::string &s2) const noexcept;
  [[nodiscard]] Core::SimilarityResult
  create_validation_error(const std::string &message) const;
};

// Factory function for creating configured engines
[[nodiscard]] std::unique_ptr<Core::ISimilarityEngine> create_similarity_engine(
    size_t thread_pool_size = std::thread::hardware_concurrency());

} // namespace TextSimilarity::Engine