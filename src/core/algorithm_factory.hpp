#pragma once

#include "interfaces.hpp"
#include <functional>
#include <shared_mutex>
#include <unordered_map>

namespace TextSimilarity::Core {

// Algorithm factory implementation
class AlgorithmFactory final : public IAlgorithmFactory {
public:
  AlgorithmFactory() = default;
  ~AlgorithmFactory() override = default;

  // Non-copyable, non-movable (due to mutex)
  AlgorithmFactory(const AlgorithmFactory &) = delete;
  AlgorithmFactory &operator=(const AlgorithmFactory &) = delete;
  AlgorithmFactory(AlgorithmFactory &&) = delete;
  AlgorithmFactory &operator=(AlgorithmFactory &&) = delete;

  // IAlgorithmFactory implementation
  [[nodiscard]] std::unique_ptr<ISimilarityAlgorithm>
  create_algorithm(AlgorithmType type,
                   const AlgorithmConfig &config) const override;

  [[nodiscard]] std::vector<AlgorithmType>
  get_supported_algorithms() const noexcept override;
  [[nodiscard]] bool
  supports_algorithm(AlgorithmType type) const noexcept override;

  // Registration interface for algorithms
  using AlgorithmCreator = std::function<std::unique_ptr<ISimilarityAlgorithm>(
      const AlgorithmConfig &, std::shared_ptr<IMemoryPool>)>;

  void register_algorithm(AlgorithmType type, AlgorithmCreator creator);
  void unregister_algorithm(AlgorithmType type) noexcept;

  // Singleton access
  static AlgorithmFactory &instance();

  // Set default memory pool for all algorithms
  void set_default_memory_pool(std::shared_ptr<IMemoryPool> pool);
  [[nodiscard]] std::shared_ptr<IMemoryPool> get_default_memory_pool() const;

  // Initialize built-in algorithms
  void register_built_in_algorithms();

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<AlgorithmType, AlgorithmCreator> creators_;
  std::shared_ptr<IMemoryPool> default_memory_pool_;
};

// RAII helper for algorithm registration
class AlgorithmRegistration final {
public:
  AlgorithmRegistration(AlgorithmType type,
                        AlgorithmFactory::AlgorithmCreator creator)
      : type_(type) {
    AlgorithmFactory::instance().register_algorithm(type_, std::move(creator));
  }

  ~AlgorithmRegistration() {
    AlgorithmFactory::instance().unregister_algorithm(type_);
  }

  // Non-copyable, non-movable (global registration)
  AlgorithmRegistration(const AlgorithmRegistration &) = delete;
  AlgorithmRegistration &operator=(const AlgorithmRegistration &) = delete;
  AlgorithmRegistration(AlgorithmRegistration &&) = delete;
  AlgorithmRegistration &operator=(AlgorithmRegistration &&) = delete;

private:
  AlgorithmType type_;
};

// Convenience macro for algorithm registration
#define REGISTER_ALGORITHM_IMPL(AlgorithmClass, AlgorithmTypeEnum)             \
  namespace {                                                                  \
  static const AlgorithmRegistration AlgorithmClass##_registration{            \
      AlgorithmTypeEnum,                                                       \
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool)     \
          -> std::unique_ptr<ISimilarityAlgorithm> {                           \
        return std::make_unique<AlgorithmClass>(config, std::move(pool));      \
      }};                                                                      \
  }

// Helper functions for common operations
[[nodiscard]] std::unique_ptr<IAlgorithmFactory> create_algorithm_factory();
[[nodiscard]] std::vector<std::string> get_algorithm_names();
[[nodiscard]] std::string get_algorithm_name(AlgorithmType type);
[[nodiscard]] std::optional<AlgorithmType>
parse_algorithm_type(const std::string &name);

} // namespace TextSimilarity::Core