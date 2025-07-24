#include "token_based.hpp"
#include <algorithm>

namespace TextSimilarity::Algorithms {

// JaccardAlgorithm Implementation

Core::SimilarityResult JaccardAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    // Tokenize strings based on preprocessing mode
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    // Convert to appropriate representation
    if (config.preprocessing == Core::PreprocessingMode::Word) {
      // For word-based comparison, use set-based Jaccard
      std::unordered_set<std::u32string> set1, set2;

      for (const auto &token : tokens1) {
        set1.insert(token.unicode());
      }
      for (const auto &token : tokens2) {
        set2.insert(token.unicode());
      }

      double similarity = compute_jaccard_set(set1, set2);
      return Core::SimilarityResult{similarity};
    } else {
      // For character/n-gram based comparison, use multiset
      Counter<std::u32string> counter1, counter2;

      for (const auto &token : tokens1) {
        counter1.increment(token.unicode());
      }
      for (const auto &token : tokens2) {
        counter2.increment(token.unicode());
      }

      double similarity = compute_jaccard_multiset(counter1, counter2);
      return Core::SimilarityResult{similarity};
    }

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult JaccardAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  // Jaccard distance = 1 - Jaccard similarity
  double distance = 1.0 - similarity_result.value();

  // Convert to integer distance (0-1000 for precision)
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double JaccardAlgorithm::compute_jaccard_similarity(
    const std::vector<Core::UnicodeString> &tokens1,
    const std::vector<Core::UnicodeString> &tokens2, bool as_set) const {
  if (tokens1.empty() && tokens2.empty()) {
    return 1.0;
  }

  if (tokens1.empty() || tokens2.empty()) {
    return 0.0;
  }

  if (as_set) {
    std::unordered_set<std::u32string> set1, set2;

    for (const auto &token : tokens1) {
      set1.insert(token.unicode());
    }
    for (const auto &token : tokens2) {
      set2.insert(token.unicode());
    }

    return compute_jaccard_set(set1, set2);
  } else {
    Counter<std::u32string> counter1, counter2;

    for (const auto &token : tokens1) {
      counter1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      counter2.increment(token.unicode());
    }

    return compute_jaccard_multiset(counter1, counter2);
  }
}

double JaccardAlgorithm::compute_jaccard_multiset(
    const Counter<std::u32string> &counter1,
    const Counter<std::u32string> &counter2) const {
  if (counter1.empty() && counter2.empty()) {
    return 1.0;
  }

  if (counter1.empty() || counter2.empty()) {
    return 0.0;
  }

  auto intersection = counter1.intersect(counter2);
  auto union_counter = counter1.union_with(counter2);

  uint32_t intersection_size = intersection.total_count();
  uint32_t union_size = union_counter.total_count();

  if (union_size == 0) {
    return 0.0;
  }

  return static_cast<double>(intersection_size) /
         static_cast<double>(union_size);
}

double JaccardAlgorithm::compute_jaccard_set(
    const std::unordered_set<std::u32string> &set1,
    const std::unordered_set<std::u32string> &set2) const {
  if (set1.empty() && set2.empty()) {
    return 1.0;
  }

  if (set1.empty() || set2.empty()) {
    return 0.0;
  }

  // Calculate intersection
  size_t intersection_size = 0;
  const auto &smaller_set = (set1.size() <= set2.size()) ? set1 : set2;
  const auto &larger_set = (set1.size() <= set2.size()) ? set2 : set1;

  for (const auto &item : smaller_set) {
    if (larger_set.find(item) != larger_set.end()) {
      ++intersection_size;
    }
  }

  // Union size = size1 + size2 - intersection
  size_t union_size = set1.size() + set2.size() - intersection_size;

  if (union_size == 0) {
    return 0.0;
  }

  return static_cast<double>(intersection_size) /
         static_cast<double>(union_size);
}

// SorensenDiceAlgorithm Implementation

Core::SimilarityResult SorensenDiceAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    Counter<std::u32string> counter1, counter2;

    for (const auto &token : tokens1) {
      counter1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      counter2.increment(token.unicode());
    }

    double similarity = compute_dice_similarity(counter1, counter2);
    return Core::SimilarityResult{similarity};

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult SorensenDiceAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  double distance = 1.0 - similarity_result.value();
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double SorensenDiceAlgorithm::compute_dice_similarity(
    const Counter<std::u32string> &counter1,
    const Counter<std::u32string> &counter2) const {
  if (counter1.empty() && counter2.empty()) {
    return 1.0;
  }

  if (counter1.empty() || counter2.empty()) {
    return 0.0;
  }

  auto intersection = counter1.intersect(counter2);
  uint32_t intersection_size = intersection.total_count();
  uint32_t total_size = counter1.total_count() + counter2.total_count();

  if (total_size == 0) {
    return 0.0;
  }

  return (2.0 * static_cast<double>(intersection_size)) /
         static_cast<double>(total_size);
}

// OverlapAlgorithm Implementation

Core::SimilarityResult OverlapAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    Counter<std::u32string> counter1, counter2;

    for (const auto &token : tokens1) {
      counter1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      counter2.increment(token.unicode());
    }

    double similarity = compute_overlap_similarity(counter1, counter2);
    return Core::SimilarityResult{similarity};

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult OverlapAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  double distance = 1.0 - similarity_result.value();
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double OverlapAlgorithm::compute_overlap_similarity(
    const Counter<std::u32string> &counter1,
    const Counter<std::u32string> &counter2) const {
  if (counter1.empty() && counter2.empty()) {
    return 1.0;
  }

  if (counter1.empty() || counter2.empty()) {
    return 0.0;
  }

  auto intersection = counter1.intersect(counter2);
  uint32_t intersection_size = intersection.total_count();
  uint32_t min_size = std::min(counter1.total_count(), counter2.total_count());

  if (min_size == 0) {
    return 0.0;
  }

  return static_cast<double>(intersection_size) / static_cast<double>(min_size);
}

// TverskyAlgorithm Implementation

Core::SimilarityResult TverskyAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  if (!config.alpha.has_value() || !config.beta.has_value()) {
    return Core::SimilarityResult{Core::SimilarityError{
        Core::ErrorCode::InvalidConfiguration,
        "Tversky algorithm requires alpha and beta parameters"}};
  }

  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    Counter<std::u32string> counter1, counter2;

    for (const auto &token : tokens1) {
      counter1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      counter2.increment(token.unicode());
    }

    double similarity = compute_tversky_similarity(counter1, counter2,
                                                   *config.alpha, *config.beta);
    return Core::SimilarityResult{similarity};

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult TverskyAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  double distance = 1.0 - similarity_result.value();
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double TverskyAlgorithm::compute_tversky_similarity(
    const Counter<std::u32string> &counter1,
    const Counter<std::u32string> &counter2, double alpha, double beta) const {
  if (counter1.empty() && counter2.empty()) {
    return 1.0;
  }

  if (counter1.empty() || counter2.empty()) {
    return 0.0;
  }

  auto intersection = counter1.intersect(counter2);
  uint32_t intersection_size = intersection.total_count();

  // Calculate differences (elements in one set but not the other)
  uint32_t diff1 = counter1.total_count() - intersection_size; // |A - B|
  uint32_t diff2 = counter2.total_count() - intersection_size; // |B - A|

  double denominator = static_cast<double>(intersection_size) +
                       alpha * static_cast<double>(diff1) +
                       beta * static_cast<double>(diff2);

  if (denominator == 0.0) {
    return 0.0;
  }

  return static_cast<double>(intersection_size) / denominator;
}

} // namespace TextSimilarity::Algorithms