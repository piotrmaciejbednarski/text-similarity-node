#pragma once

#include "base_algorithm.hpp"
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TextSimilarity::Algorithms {

// Counter-based frequency map for token operations
template <typename T> class Counter {
public:
  using value_type = T;
  using count_type = uint32_t;
  using map_type = std::unordered_map<T, count_type>;

  Counter() = default;

  template <typename Iterator> Counter(Iterator begin, Iterator end) {
    for (auto it = begin; it != end; ++it) {
      increment(*it);
    }
  }

  explicit Counter(const std::vector<T> &items)
      : Counter(items.begin(), items.end()) {}

  void increment(const T &item, count_type count = 1) {
    counts_[item] += count;
  }

  [[nodiscard]] count_type get_count(const T &item) const {
    auto it = counts_.find(item);
    return (it != counts_.end()) ? it->second : 0;
  }

  [[nodiscard]] size_t size() const noexcept { return counts_.size(); }
  [[nodiscard]] bool empty() const noexcept { return counts_.empty(); }

  [[nodiscard]] count_type total_count() const noexcept {
    count_type total = 0;
    for (const auto &pair : counts_) {
      total += pair.second;
    }
    return total;
  }

  // Set operations
  Counter intersect(const Counter &other) const {
    Counter result;
    for (const auto &[item, count] : counts_) {
      count_type other_count = other.get_count(item);
      if (other_count > 0) {
        result.counts_[item] = std::min(count, other_count);
      }
    }
    return result;
  }

  Counter union_with(const Counter &other) const {
    Counter result = *this;
    for (const auto &[item, count] : other.counts_) {
      result.counts_[item] = std::max(result.get_count(item), count);
    }
    return result;
  }

  Counter sum_with(const Counter &other) const {
    Counter result = *this;
    for (const auto &[item, count] : other.counts_) {
      result.counts_[item] += count;
    }
    return result;
  }

  // Iterator support
  auto begin() const { return counts_.begin(); }
  auto end() const { return counts_.end(); }

private:
  map_type counts_;
};

// Jaccard similarity implementation
class JaccardAlgorithm final : public BaseAlgorithm {
public:
  explicit JaccardAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~JaccardAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Jaccard;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Jaccard";
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
  [[nodiscard]] double
  compute_jaccard_similarity(const std::vector<Core::UnicodeString> &tokens1,
                             const std::vector<Core::UnicodeString> &tokens2,
                             bool as_set = false) const;

  [[nodiscard]] double
  compute_jaccard_multiset(const Counter<std::u32string> &counter1,
                           const Counter<std::u32string> &counter2) const;

  [[nodiscard]] double
  compute_jaccard_set(const std::unordered_set<std::u32string> &set1,
                      const std::unordered_set<std::u32string> &set2) const;
};

// Sorensen-Dice similarity
class SorensenDiceAlgorithm final : public BaseAlgorithm {
public:
  explicit SorensenDiceAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~SorensenDiceAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::SorensenDice;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Sorensen-Dice";
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
  [[nodiscard]] double
  compute_dice_similarity(const Counter<std::u32string> &counter1,
                          const Counter<std::u32string> &counter2) const;
};

// Overlap coefficient
class OverlapAlgorithm final : public BaseAlgorithm {
public:
  explicit OverlapAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~OverlapAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Overlap;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Overlap";
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
  [[nodiscard]] double
  compute_overlap_similarity(const Counter<std::u32string> &counter1,
                             const Counter<std::u32string> &counter2) const;
};

// Tversky index (generalized Jaccard)
class TverskyAlgorithm final : public BaseAlgorithm {
public:
  explicit TverskyAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~TverskyAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Tversky;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Tversky";
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

  // Tversky is not symmetric unless alpha == beta
  bool is_symmetric_impl() const noexcept override { return false; }

private:
  [[nodiscard]] double
  compute_tversky_similarity(const Counter<std::u32string> &counter1,
                             const Counter<std::u32string> &counter2,
                             double alpha, double beta) const;
};

} // namespace TextSimilarity::Algorithms