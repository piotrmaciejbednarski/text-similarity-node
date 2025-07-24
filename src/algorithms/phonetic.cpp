#include "phonetic.hpp"
#include <algorithm>

namespace TextSimilarity::Algorithms {

namespace {
// Unicode-aware character equality check
bool unicode_chars_equal(char32_t a, char32_t b, bool case_sensitive) noexcept {
  if (case_sensitive) {
    return a == b;
  }

  // Fast path for ASCII
  if (a < 128 && b < 128) {
    char ca = static_cast<char>(a);
    char cb = static_cast<char>(b);
    return (ca | 0x20) == (cb | 0x20);
  }

  // Unicode case folding (simplified)
  auto fold = [](char32_t c) -> char32_t {
    if (c >= 'A' && c <= 'Z')
      return c + 32;
    if (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7)
      return c + 32;
    if (c >= 0x0391 && c <= 0x03A9)
      return c + 32;
    if (c >= 0x0410 && c <= 0x042F)
      return c + 32;
    return c;
  };

  return fold(a) == fold(b);
}
} // namespace

// JaroAlgorithm Implementation

Core::SimilarityResult JaroAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    if (s1.empty() && s2.empty()) {
      return Core::SimilarityResult{1.0};
    }

    if (s1.empty() || s2.empty()) {
      return Core::SimilarityResult{0.0};
    }

    double similarity =
        compute_jaro_similarity(s1.unicode(), s2.unicode(), config);
    return Core::SimilarityResult{similarity};

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult JaroAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  // Jaro distance = 1 - Jaro similarity
  double distance = 1.0 - similarity_result.value();
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double JaroAlgorithm::compute_jaro_similarity(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  if (len1 == 0 && len2 == 0) {
    return 1.0;
  }

  if (len1 == 0 || len2 == 0) {
    return 0.0;
  }

  // Find matches
  auto match_info = find_matches(s1, s2, config);

  if (match_info.match_count == 0) {
    return 0.0;
  }

  // Jaro formula: (m/|s1| + m/|s2| + (m-t)/m) / 3
  // where m = matches, t = transpositions
  double m = static_cast<double>(match_info.match_count);
  double jaro =
      (m / static_cast<double>(len1) + m / static_cast<double>(len2) +
       (m - static_cast<double>(match_info.transposition_count)) / m) /
      3.0;

  return std::max(0.0, std::min(1.0, jaro));
}

JaroAlgorithm::MatchInfo
JaroAlgorithm::find_matches(const std::u32string &s1, const std::u32string &s2,
                            const Core::AlgorithmConfig &config) const {
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  MatchInfo info(len1, len2);

  // Calculate matching window - following Python implementation
  size_t search_range = std::max(len1, len2);
  search_range = (search_range / 2);
  if (search_range > 0) {
    search_range -= 1;
  }

  // Find matches within the window - exactly like Python implementation
  for (size_t i = 0; i < len1; ++i) {
    size_t low = (i >= search_range) ? i - search_range : 0;
    size_t hi = std::min(i + search_range, len2 - 1);

    for (size_t j = low; j <= hi; ++j) {
      if (info.s2_matches[j] || !characters_match(s1[i], s2[j], config)) {
        continue;
      }

      // Found a match
      info.s1_matches[i] = true;
      info.s2_matches[j] = true;
      ++info.match_count;
      break;
    }
  }

  // Count transpositions - following Python implementation exactly
  size_t k = 0;
  for (size_t i = 0; i < len1; ++i) {
    if (info.s1_matches[i]) {
      // Find next matched position in s2
      while (k < len2 && !info.s2_matches[k]) {
        ++k;
      }
      if (k < len2) {
        if (!characters_match(s1[i], s2[k], config)) {
          ++info.transposition_count;
        }
        ++k;
      }
    }
  }

  // Transpositions are counted as pairs, so divide by 2
  info.transposition_count /= 2;

  return info;
}

bool JaroAlgorithm::characters_match(
    char32_t c1, char32_t c2,
    const Core::AlgorithmConfig &config) const noexcept {
  return unicode_chars_equal(
      c1, c2, config.case_sensitivity == Core::CaseSensitivity::Sensitive);
}

// JaroWinklerAlgorithm Implementation

double JaroWinklerAlgorithm::compute_jaro_similarity(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  // First calculate Jaro similarity
  double jaro_sim = JaroAlgorithm::compute_jaro_similarity(s1, s2, config);

  // Only apply Winkler modification if Jaro similarity is above threshold
  // Typically 0.7, but configurable via threshold parameter
  double threshold = config.threshold.value_or(0.7);

  if (jaro_sim < threshold) {
    return jaro_sim;
  }

  // Calculate common prefix length (up to 4 characters)
  size_t prefix_length = calculate_common_prefix_length(
      s1, s2, config, config.prefix_length.value_or(4));

  if (prefix_length == 0) {
    return jaro_sim;
  }

  // Apply Winkler modification: jaro + (prefix_length * p * (1 - jaro))
  // where p is the scaling factor (default 0.1, max 0.25)
  double p = get_prefix_weight(config);
  double jaro_winkler =
      jaro_sim + (static_cast<double>(prefix_length) * p * (1.0 - jaro_sim));

  return std::max(0.0, std::min(1.0, jaro_winkler));
}

size_t JaroWinklerAlgorithm::calculate_common_prefix_length(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config, size_t max_length) const {
  size_t min_len = std::min({s1.length(), s2.length(), max_length});
  size_t prefix_length = 0;

  for (size_t i = 0; i < min_len; ++i) {
    if (characters_match(s1[i], s2[i], config)) {
      ++prefix_length;
    } else {
      break;
    }
  }

  return prefix_length;
}

double JaroWinklerAlgorithm::get_prefix_weight(
    const Core::AlgorithmConfig &config) const noexcept {
  if (config.prefix_weight.has_value()) {
    // Clamp to valid range [0.0, 0.25]
    double weight = *config.prefix_weight;
    return std::max(0.0, std::min(0.25, weight));
  }

  return 0.1; // Default Winkler scaling factor
}

} // namespace TextSimilarity::Algorithms