#pragma once

#include "base_algorithm.hpp"
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TextSimilarity::Algorithms {

// Vector representation for cosine similarity calculations
template <typename T> class FrequencyVector {
public:
  using value_type = T;
  using frequency_type = uint32_t;
  using vector_map = std::unordered_map<T, frequency_type>;

  FrequencyVector() = default;

  template <typename Iterator> FrequencyVector(Iterator begin, Iterator end) {
    for (auto it = begin; it != end; ++it) {
      increment(*it);
    }
  }

  explicit FrequencyVector(const std::vector<T> &items)
      : FrequencyVector(items.begin(), items.end()) {}

  void increment(const T &item, frequency_type count = 1) {
    frequencies_[item] += count;
  }

  [[nodiscard]] frequency_type get_frequency(const T &item) const {
    auto it = frequencies_.find(item);
    return (it != frequencies_.end()) ? it->second : 0;
  }

  [[nodiscard]] size_t size() const noexcept { return frequencies_.size(); }
  [[nodiscard]] bool empty() const noexcept { return frequencies_.empty(); }

  [[nodiscard]] double magnitude() const {
    double sum_squares = 0.0;
    for (const auto &[item, freq] : frequencies_) {
      sum_squares += static_cast<double>(freq) * static_cast<double>(freq);
    }
    return std::sqrt(sum_squares);
  }

  [[nodiscard]] double dot_product(const FrequencyVector &other) const {
    double product = 0.0;

    // Iterate over smaller vector for efficiency
    const auto &smaller = (size() <= other.size()) ? *this : other;
    const auto &larger = (size() <= other.size()) ? other : *this;

    for (const auto &[item, freq] : smaller.frequencies_) {
      auto other_freq = larger.get_frequency(item);
      if (other_freq > 0) {
        product += static_cast<double>(freq) * static_cast<double>(other_freq);
      }
    }

    return product;
  }

  // Iterator support
  auto begin() const { return frequencies_.begin(); }
  auto end() const { return frequencies_.end(); }

  // Get all unique terms from both vectors
  static std::unordered_set<T> get_union_terms(const FrequencyVector &v1,
                                               const FrequencyVector &v2) {
    std::unordered_set<T> terms;

    for (const auto &[term, freq] : v1.frequencies_) {
      terms.insert(term);
    }

    for (const auto &[term, freq] : v2.frequencies_) {
      terms.insert(term);
    }

    return terms;
  }

private:
  vector_map frequencies_;
};

// Cosine similarity implementation
class CosineAlgorithm final : public BaseAlgorithm {
public:
  explicit CosineAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~CosineAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Cosine;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Cosine";
  }

protected:
  [[nodiscard]] Core::SimilarityResult
  compute_similarity_impl(const Core::UnicodeString &s1,
                          const Core::UnicodeString &s2,
                          const Core::AlgorithmConfig &config) const override;

  [[nodiscard]] Core::DistanceResult
  compute_distance_impl(const Core::UnicodeString &s1,
                        const Core::UnicodeString &s2,
                        const Core::AlgorithmConfig &config) const override;

  bool is_symmetric_impl() const noexcept override { return true; }

private:
  [[nodiscard]] double compute_cosine_similarity(
      const FrequencyVector<std::u32string> &vector1,
      const FrequencyVector<std::u32string> &vector2) const;

  // Optimized binary presence vector for character-based comparison
  [[nodiscard]] double
  compute_cosine_character_vectorization(const std::u32string &s1,
                                         const std::u32string &s2) const;

  // SIMD-optimized version for ASCII strings
  [[nodiscard]] double
  compute_cosine_simd(const std::string &s1, const std::string &s2,
                      const Core::AlgorithmConfig &config) const;

  [[nodiscard]] bool is_ascii_string(const std::string &str) const noexcept;
};

// Euclidean distance for vector-based comparison
class EuclideanAlgorithm final : public BaseAlgorithm {
public:
  explicit EuclideanAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~EuclideanAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Euclidean;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Euclidean";
  }

protected:
  [[nodiscard]] Core::SimilarityResult
  compute_similarity_impl(const Core::UnicodeString &s1,
                          const Core::UnicodeString &s2,
                          const Core::AlgorithmConfig &config) const override;

  [[nodiscard]] Core::DistanceResult
  compute_distance_impl(const Core::UnicodeString &s1,
                        const Core::UnicodeString &s2,
                        const Core::AlgorithmConfig &config) const override;

  bool is_symmetric_impl() const noexcept override { return true; }
  bool is_metric_impl() const noexcept override { return true; }

private:
  [[nodiscard]] double compute_euclidean_distance(
      const FrequencyVector<std::u32string> &vector1,
      const FrequencyVector<std::u32string> &vector2) const;

  [[nodiscard]] double distance_to_similarity(double distance) const noexcept;
};

// Manhattan distance (L1 norm)
class ManhattanAlgorithm final : public BaseAlgorithm {
public:
  explicit ManhattanAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~ManhattanAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Manhattan;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Manhattan";
  }

protected:
  [[nodiscard]] Core::SimilarityResult
  compute_similarity_impl(const Core::UnicodeString &s1,
                          const Core::UnicodeString &s2,
                          const Core::AlgorithmConfig &config) const override;

  [[nodiscard]] Core::DistanceResult
  compute_distance_impl(const Core::UnicodeString &s1,
                        const Core::UnicodeString &s2,
                        const Core::AlgorithmConfig &config) const override;

  bool is_symmetric_impl() const noexcept override { return true; }
  bool is_metric_impl() const noexcept override { return true; }

private:
  [[nodiscard]] double compute_manhattan_distance(
      const FrequencyVector<std::u32string> &vector1,
      const FrequencyVector<std::u32string> &vector2) const;

  [[nodiscard]] double distance_to_similarity(double distance) const noexcept;
};

// Chebyshev distance (L∞ norm)
class ChebyshevAlgorithm final : public BaseAlgorithm {
public:
  explicit ChebyshevAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~ChebyshevAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Chebyshev;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Chebyshev";
  }

protected:
  [[nodiscard]] Core::SimilarityResult
  compute_similarity_impl(const Core::UnicodeString &s1,
                          const Core::UnicodeString &s2,
                          const Core::AlgorithmConfig &config) const override;

  [[nodiscard]] Core::DistanceResult
  compute_distance_impl(const Core::UnicodeString &s1,
                        const Core::UnicodeString &s2,
                        const Core::AlgorithmConfig &config) const override;

  bool is_symmetric_impl() const noexcept override { return true; }
  bool is_metric_impl() const noexcept override { return true; }

private:
  [[nodiscard]] double compute_chebyshev_distance(
      const FrequencyVector<std::u32string> &vector1,
      const FrequencyVector<std::u32string> &vector2) const;

  [[nodiscard]] double distance_to_similarity(double distance) const noexcept;
};

} // namespace TextSimilarity::Algorithms