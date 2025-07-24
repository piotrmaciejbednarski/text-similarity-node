#pragma once

#include "base_algorithm.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace TextSimilarity::Algorithms {

class LevenshteinAlgorithm final : public BaseAlgorithm {
public:
  explicit LevenshteinAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~LevenshteinAlgorithm() override = default;

  // ISimilarityAlgorithm implementation
  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Levenshtein;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Levenshtein";
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

  // Performance characteristics
  bool supports_early_termination_impl() const noexcept override {
    return true;
  }
  bool is_symmetric_impl() const noexcept override { return true; }
  bool is_metric_impl() const noexcept override { return true; }

private:
  // Optimized implementations
  [[nodiscard]] uint32_t
  compute_distance_optimized(const std::u32string &s1, const std::u32string &s2,
                             const Core::AlgorithmConfig &config) const;

  // Memory-efficient single-row implementation
  [[nodiscard]] uint32_t
  compute_distance_single_row(const std::u32string &s1,
                              const std::u32string &s2,
                              const Core::AlgorithmConfig &config) const;

  // Early termination implementation
  [[nodiscard]] uint32_t compute_distance_with_threshold(
      const std::u32string &s1, const std::u32string &s2, uint32_t max_distance,
      const Core::AlgorithmConfig &config) const;

  // SIMD-optimized implementation for ASCII strings
  [[nodiscard]] uint32_t
  compute_distance_simd(const std::string &s1, const std::string &s2,
                        const Core::AlgorithmConfig &config) const;

  // Character comparison with case sensitivity
  [[nodiscard]] bool
  characters_equal(char32_t c1, char32_t c2,
                   const Core::AlgorithmConfig &config) const noexcept;

  // Similarity calculation from distance
  [[nodiscard]] double distance_to_similarity(uint32_t distance,
                                              size_t max_length) const noexcept;

  // ASCII optimization detection
  [[nodiscard]] bool is_ascii_string(const std::string &str) const noexcept;
};

// Damerau-Levenshtein with transposition support
class DamerauLevenshteinAlgorithm final : public BaseAlgorithm {
public:
  explicit DamerauLevenshteinAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~DamerauLevenshteinAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::DamerauLevenshtein;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Damerau-Levenshtein";
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

  bool supports_early_termination_impl() const noexcept override {
    return true;
  }
  bool is_symmetric_impl() const noexcept override { return true; }
  bool is_metric_impl() const noexcept override { return true; }

private:
  // Restricted Damerau-Levenshtein (Optimal String Alignment)
  [[nodiscard]] uint32_t
  compute_osa_distance(const std::u32string &s1, const std::u32string &s2,
                       const Core::AlgorithmConfig &config) const;

  // Unrestricted Damerau-Levenshtein (true metric)
  [[nodiscard]] uint32_t
  compute_unrestricted_distance(const std::u32string &s1,
                                const std::u32string &s2,
                                const Core::AlgorithmConfig &config) const;

  [[nodiscard]] double distance_to_similarity(uint32_t distance,
                                              size_t max_length) const noexcept;
};

// Hamming distance for equal-length strings
class HammingAlgorithm final : public BaseAlgorithm {
public:
  explicit HammingAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~HammingAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Hamming;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Hamming";
  }

  [[nodiscard]] uint32_t get_maximum_distance() const noexcept override;

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
  [[nodiscard]] uint32_t
  compute_hamming_distance(const std::u32string &s1, const std::u32string &s2,
                           const Core::AlgorithmConfig &config) const;

  // SIMD-optimized version for ASCII
  [[nodiscard]] uint32_t
  compute_hamming_simd(const std::string &s1, const std::string &s2,
                       const Core::AlgorithmConfig &config) const;

  [[nodiscard]] double distance_to_similarity(uint32_t distance,
                                              size_t length) const noexcept;
};

} // namespace TextSimilarity::Algorithms