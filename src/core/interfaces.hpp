#pragma once

#include "types.hpp"
#include <memory>
#include <shared_mutex>

namespace TextSimilarity::Core {

// Base interface for all similarity algorithms
class ISimilarityAlgorithm {
public:
  virtual ~ISimilarityAlgorithm() = default;

  // Pure virtual methods
  [[nodiscard]] virtual SimilarityResult
  calculate_similarity(const UnicodeString &s1,
                       const UnicodeString &s2) const = 0;

  [[nodiscard]] virtual DistanceResult
  calculate_distance(const UnicodeString &s1,
                     const UnicodeString &s2) const = 0;

  [[nodiscard]] virtual AlgorithmType get_algorithm_type() const noexcept = 0;
  [[nodiscard]] virtual std::string get_algorithm_name() const noexcept = 0;

  // Optional methods with default implementations
  [[nodiscard]] virtual double get_maximum_similarity() const noexcept {
    return 1.0;
  }
  [[nodiscard]] virtual uint32_t get_maximum_distance() const noexcept {
    return UINT32_MAX;
  }

  // Thread-safe configuration
  virtual void update_configuration(const AlgorithmConfig &config) = 0;
  [[nodiscard]] virtual AlgorithmConfig get_configuration() const = 0;

  // Performance hints
  [[nodiscard]] virtual bool supports_early_termination() const noexcept {
    return false;
  }
  [[nodiscard]] virtual bool is_symmetric() const noexcept { return true; }
  [[nodiscard]] virtual bool is_metric() const noexcept { return false; }
};

// Interface for preprocessing strategies
class IPreprocessor {
public:
  virtual ~IPreprocessor() = default;

  [[nodiscard]] virtual std::vector<UnicodeString>
  preprocess(const UnicodeString &text,
             const AlgorithmConfig &config) const = 0;

  [[nodiscard]] virtual PreprocessingMode get_mode() const noexcept = 0;
};

// Interface for text normalization strategies
class ITextNormalizer {
public:
  virtual ~ITextNormalizer() = default;

  [[nodiscard]] virtual UnicodeString
  normalize(const UnicodeString &text, const AlgorithmConfig &config) const = 0;
};

// Interface for memory pool management
class IMemoryPool {
public:
  virtual ~IMemoryPool() = default;

  [[nodiscard]] virtual void *
  allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
  virtual void deallocate(void *ptr, size_t size) noexcept = 0;
  virtual void reset() noexcept = 0;

  template <typename T, typename... Args>
  [[nodiscard]] std::unique_ptr<T, std::function<void(T *)>>
  create(Args &&...args) {
    void *ptr = allocate(sizeof(T), alignof(T));
    if (!ptr) {
      throw std::bad_alloc();
    }

    T *obj = new (ptr) T(std::forward<Args>(args)...);
    return {obj, [this](T *p) {
              p->~T();
              deallocate(p, sizeof(T));
            }};
  }
};

// Interface for algorithm factory
class IAlgorithmFactory {
public:
  virtual ~IAlgorithmFactory() = default;

  [[nodiscard]] virtual std::unique_ptr<ISimilarityAlgorithm>
  create_algorithm(AlgorithmType type, const AlgorithmConfig &config) const = 0;

  [[nodiscard]] virtual std::vector<AlgorithmType>
  get_supported_algorithms() const noexcept = 0;
  [[nodiscard]] virtual bool
  supports_algorithm(AlgorithmType type) const noexcept = 0;
};

// Interface for async execution
class IAsyncExecutor {
public:
  virtual ~IAsyncExecutor() = default;

  [[nodiscard]] virtual AsyncSimilarityResult
  calculate_similarity_async(std::unique_ptr<ISimilarityAlgorithm> algorithm,
                             UnicodeString s1, UnicodeString s2) = 0;

  [[nodiscard]] virtual AsyncDistanceResult
  calculate_distance_async(std::unique_ptr<ISimilarityAlgorithm> algorithm,
                           UnicodeString s1, UnicodeString s2) = 0;

  virtual void shutdown() noexcept = 0;
};

// Thread-safe configuration manager
class IConfigurationManager {
public:
  virtual ~IConfigurationManager() = default;

  virtual void set_global_config(const AlgorithmConfig &config) = 0;
  [[nodiscard]] virtual AlgorithmConfig get_global_config() const = 0;

  virtual void set_algorithm_config(AlgorithmType type,
                                    const AlgorithmConfig &config) = 0;
  [[nodiscard]] virtual AlgorithmConfig
  get_algorithm_config(AlgorithmType type) const = 0;

  virtual void reset_to_defaults() noexcept = 0;
};

// Interface for dependency injection container
class IDependencyContainer {
public:
  virtual ~IDependencyContainer() = default;

  template <typename Interface>
  [[nodiscard]] std::shared_ptr<Interface> resolve() {
    return std::static_pointer_cast<Interface>(resolve_impl(typeid(Interface)));
  }

  template <typename Interface, typename Implementation>
  void register_singleton() {
    register_singleton_impl(typeid(Interface), []() -> std::shared_ptr<void> {
      return std::make_shared<Implementation>();
    });
  }

  template <typename Interface, typename Implementation>
  void register_transient() {
    register_transient_impl(typeid(Interface), []() -> std::shared_ptr<void> {
      return std::make_shared<Implementation>();
    });
  }

private:
  virtual std::shared_ptr<void> resolve_impl(const std::type_info &type) = 0;
  virtual void
  register_singleton_impl(const std::type_info &type,
                          std::function<std::shared_ptr<void>()> factory) = 0;
  virtual void
  register_transient_impl(const std::type_info &type,
                          std::function<std::shared_ptr<void>()> factory) = 0;
};

// Main similarity engine interface
class ISimilarityEngine {
public:
  virtual ~ISimilarityEngine() = default;

  // Synchronous operations
  [[nodiscard]] virtual SimilarityResult
  calculate_similarity(const std::string &s1, const std::string &s2,
                       AlgorithmType algorithm = AlgorithmType::Levenshtein,
                       const AlgorithmConfig &config = {}) = 0;

  [[nodiscard]] virtual DistanceResult
  calculate_distance(const std::string &s1, const std::string &s2,
                     AlgorithmType algorithm = AlgorithmType::Levenshtein,
                     const AlgorithmConfig &config = {}) = 0;

  // Asynchronous operations
  [[nodiscard]] virtual AsyncSimilarityResult calculate_similarity_async(
      std::string s1, std::string s2,
      AlgorithmType algorithm = AlgorithmType::Levenshtein,
      AlgorithmConfig config = {}) = 0;

  [[nodiscard]] virtual AsyncDistanceResult
  calculate_distance_async(std::string s1, std::string s2,
                           AlgorithmType algorithm = AlgorithmType::Levenshtein,
                           AlgorithmConfig config = {}) = 0;

  // Batch operations
  [[nodiscard]] virtual std::vector<SimilarityResult>
  calculate_similarity_batch(
      const std::vector<std::pair<std::string, std::string>> &pairs,
      AlgorithmType algorithm = AlgorithmType::Levenshtein,
      const AlgorithmConfig &config = {}) = 0;

  // Configuration management
  virtual void set_global_configuration(const AlgorithmConfig &config) = 0;
  [[nodiscard]] virtual AlgorithmConfig get_global_configuration() const = 0;

  // Algorithm information
  [[nodiscard]] virtual std::vector<AlgorithmType>
  get_supported_algorithms() const noexcept = 0;
  [[nodiscard]] virtual bool
  supports_algorithm(AlgorithmType type) const noexcept = 0;

  // Performance and diagnostics
  [[nodiscard]] virtual size_t get_memory_usage() const noexcept = 0;
  virtual void clear_caches() noexcept = 0;
  virtual void shutdown() noexcept = 0;
};

} // namespace TextSimilarity::Core