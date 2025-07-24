#pragma once

#include "../core/interfaces.hpp"
#include "../core/memory_pool.hpp"
#include <atomic>
#include <chrono>
#include <shared_mutex>

namespace TextSimilarity::Algorithms {

// Abstract base class for all similarity algorithms
class BaseAlgorithm : public Core::ISimilarityAlgorithm {
public:
  explicit BaseAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr);

  ~BaseAlgorithm() override = default;

  // ISimilarityAlgorithm implementation
  [[nodiscard]] Core::SimilarityResult
  calculate_similarity(const Core::UnicodeString &s1,
                       const Core::UnicodeString &s2) const override final;

  [[nodiscard]] Core::DistanceResult
  calculate_distance(const Core::UnicodeString &s1,
                     const Core::UnicodeString &s2) const override final;

  void update_configuration(const Core::AlgorithmConfig &config) override final;
  [[nodiscard]] Core::AlgorithmConfig get_configuration() const override final;

  [[nodiscard]] bool supports_early_termination() const noexcept override;
  [[nodiscard]] bool is_symmetric() const noexcept override;
  [[nodiscard]] bool is_metric() const noexcept override;

protected:
  // Abstract methods for derived classes to implement
  [[nodiscard]] virtual Core::SimilarityResult
  compute_similarity_impl(const Core::UnicodeString &s1,
                          const Core::UnicodeString &s2,
                          const Core::AlgorithmConfig &config) const = 0;

  [[nodiscard]] virtual Core::DistanceResult
  compute_distance_impl(const Core::UnicodeString &s1,
                        const Core::UnicodeString &s2,
                        const Core::AlgorithmConfig &config) const = 0;

  // Utility methods for derived classes
  [[nodiscard]] Core::UnicodeString
  preprocess_string(const Core::UnicodeString &input,
                    const Core::AlgorithmConfig &config) const;

  [[nodiscard]] std::vector<Core::UnicodeString>
  tokenize_string(const Core::UnicodeString &input,
                  const Core::AlgorithmConfig &config) const;

  [[nodiscard]] std::vector<std::u32string>
  generate_ngrams(const Core::UnicodeString &input, uint32_t n) const;

  // Memory management
  template <typename T>
  [[nodiscard]] std::unique_ptr<T[], std::function<void(T *)>>
  allocate_array(size_t count) const {
    if (!memory_pool_) {
      auto deleter = [](T *ptr) { delete[] ptr; };
      return {new T[count], deleter};
    }

    // Use memory pool for POD types only (like uint32_t)
    static_assert(std::is_trivial_v<T>,
                  "Memory pool arrays only support trivial types");

    size_t size = sizeof(T) * count;
    void *ptr = memory_pool_->allocate(size, alignof(T));
    T *arr = static_cast<T *>(ptr);

    auto deleter = [this, size](T *p) { memory_pool_->deallocate(p, size); };

    return {arr, deleter};
  }

  // Performance characteristics (to be overridden by derived classes)
  virtual bool supports_early_termination_impl() const noexcept {
    return false;
  }
  virtual bool is_symmetric_impl() const noexcept { return true; }
  virtual bool is_metric_impl() const noexcept { return false; }

  // Configuration validation
  virtual bool
  validate_configuration(const Core::AlgorithmConfig &config) const noexcept;

  // Quick answer optimization
  [[nodiscard]] std::optional<double>
  get_quick_similarity_answer(const Core::UnicodeString &s1,
                              const Core::UnicodeString &s2) const noexcept;

  [[nodiscard]] std::optional<uint32_t>
  get_quick_distance_answer(const Core::UnicodeString &s1,
                            const Core::UnicodeString &s2) const noexcept;

private:
  mutable std::shared_mutex config_mutex_;
  std::shared_ptr<Core::IMemoryPool> memory_pool_;
  Core::AlgorithmConfig config_;

  // Performance metrics
  mutable std::atomic<uint64_t> call_count_{0};
  mutable std::atomic<uint64_t> total_time_ns_{0};

  void update_metrics(std::chrono::nanoseconds duration) const noexcept;
};

// Template helper for algorithm registration
template <typename AlgorithmImpl> class AlgorithmRegistrar {
public:
  static bool register_algorithm() {
    // This would integrate with the factory system
    return true;
  }

private:
  static inline bool registered_ = register_algorithm();
};

// Macro for easy algorithm registration
#define REGISTER_ALGORITHM(AlgorithmClass)                                     \
  namespace {                                                                  \
  static const bool AlgorithmClass##_registered =                              \
      TextSimilarity::Algorithms::AlgorithmRegistrar<                          \
          AlgorithmClass>::register_algorithm();                               \
  }

} // namespace TextSimilarity::Algorithms