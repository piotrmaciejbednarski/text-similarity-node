#pragma once

#include "base_algorithm.hpp"
#include <cmath>
#include <vector>

namespace TextSimilarity::Algorithms {

// Jaro similarity implementation
class JaroAlgorithm : public BaseAlgorithm {
public:
  explicit JaroAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : BaseAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~JaroAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::Jaro;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Jaro";
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

protected:
  // Virtual to allow Jaro-Winkler to override
  [[nodiscard]] virtual double
  compute_jaro_similarity(const std::u32string &s1, const std::u32string &s2,
                          const Core::AlgorithmConfig &config) const;

protected:
  struct MatchInfo {
    std::vector<bool> s1_matches;
    std::vector<bool> s2_matches;
    size_t match_count;
    size_t transposition_count;

    MatchInfo(size_t s1_len, size_t s2_len)
        : s1_matches(s1_len, false), s2_matches(s2_len, false), match_count(0),
          transposition_count(0) {}
  };

  [[nodiscard]] MatchInfo
  find_matches(const std::u32string &s1, const std::u32string &s2,
               const Core::AlgorithmConfig &config) const;

  [[nodiscard]] bool
  characters_match(char32_t c1, char32_t c2,
                   const Core::AlgorithmConfig &config) const noexcept;
};

// Jaro-Winkler similarity implementation (extends Jaro)
class JaroWinklerAlgorithm final : public JaroAlgorithm {
public:
  explicit JaroWinklerAlgorithm(
      Core::AlgorithmConfig config = {},
      std::shared_ptr<Core::IMemoryPool> memory_pool = nullptr)
      : JaroAlgorithm(std::move(config), std::move(memory_pool)) {}

  ~JaroWinklerAlgorithm() override = default;

  [[nodiscard]] Core::AlgorithmType
  get_algorithm_type() const noexcept override {
    return Core::AlgorithmType::JaroWinkler;
  }

  [[nodiscard]] std::string get_algorithm_name() const noexcept override {
    return "Jaro-Winkler";
  }

protected:
  [[nodiscard]] double
  compute_jaro_similarity(const std::u32string &s1, const std::u32string &s2,
                          const Core::AlgorithmConfig &config) const override;

private:
  [[nodiscard]] size_t calculate_common_prefix_length(
      const std::u32string &s1, const std::u32string &s2,
      const Core::AlgorithmConfig &config, size_t max_length = 4) const;

  [[nodiscard]] double
  get_prefix_weight(const Core::AlgorithmConfig &config) const noexcept;
};

} // namespace TextSimilarity::Algorithms