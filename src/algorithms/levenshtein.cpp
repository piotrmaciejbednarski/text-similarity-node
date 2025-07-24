#include "levenshtein.hpp"
#include <algorithm>
#include <limits>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h> // For SIMD intrinsics (x86/x64 only)
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h> // For NEON intrinsics (ARM64/ARM32)
#endif

namespace TextSimilarity::Algorithms {

namespace {
// SIMD-optimized character comparison for ASCII strings
inline bool is_ascii_char(char c) noexcept {
  return static_cast<unsigned char>(c) < 128;
}

// Fast ASCII case-insensitive comparison
inline bool ascii_chars_equal_ignore_case(char a, char b) noexcept {
  return (a | 0x20) == (b | 0x20);
}

// Unicode-aware character comparison
inline bool unicode_chars_equal(char32_t a, char32_t b,
                                bool case_sensitive) noexcept {
  if (case_sensitive) {
    return a == b;
  }

  // Fast path for ASCII
  if (a < 128 && b < 128) {
    return ascii_chars_equal_ignore_case(static_cast<char>(a),
                                         static_cast<char>(b));
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

// LevenshteinAlgorithm Implementation

Core::SimilarityResult LevenshteinAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  size_t max_len = std::max(s1.length(), s2.length());
  if (max_len == 0) {
    return Core::SimilarityResult{1.0};
  }

  double similarity = distance_to_similarity(distance_result.value(), max_len);
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult LevenshteinAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    // Handle empty strings
    if (s1.empty())
      return Core::DistanceResult{static_cast<uint32_t>(s2.length())};
    if (s2.empty())
      return Core::DistanceResult{static_cast<uint32_t>(s1.length())};

    // Quick identical check
    if (s1 == s2)
      return Core::DistanceResult{0U};

// For ASCII strings, use SIMD optimization (x86/x64 only)
#if defined(__x86_64__) || defined(__i386__)
    if (is_ascii_string(s1.utf8()) && is_ascii_string(s2.utf8())) {
      uint32_t distance = compute_distance_simd(s1.utf8(), s2.utf8(), config);
      return Core::DistanceResult{distance};
    }
#endif

    // Use Unicode-aware implementation
    uint32_t distance =
        compute_distance_optimized(s1.unicode(), s2.unicode(), config);
    return Core::DistanceResult{distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

uint32_t LevenshteinAlgorithm::compute_distance_optimized(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  // Use single-row optimization for memory efficiency
  if (config.threshold.has_value()) {
    return compute_distance_with_threshold(
        s1, s2, static_cast<uint32_t>(*config.threshold), config);
  }

  return compute_distance_single_row(s1, s2, config);
}

uint32_t LevenshteinAlgorithm::compute_distance_single_row(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  // Ensure s1 is the shorter string for memory optimization
  if (len1 > len2) {
    return compute_distance_single_row(s2, s1, config);
  }

  // Allocate single row + previous value
  auto current_row = allocate_array<uint32_t>(len1 + 1);
  if (!current_row) {
    throw std::bad_alloc();
  }

  // Initialize first row
  for (size_t i = 0; i <= len1; ++i) {
    current_row[i] = static_cast<uint32_t>(i);
  }

  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  // Process each row
  for (size_t j = 1; j <= len2; ++j) {
    uint32_t previous_diagonal = current_row[0];
    current_row[0] = static_cast<uint32_t>(j);

    for (size_t i = 1; i <= len1; ++i) {
      uint32_t previous_current = current_row[i];

      if (unicode_chars_equal(s1[i - 1], s2[j - 1], case_sensitive)) {
        current_row[i] = previous_diagonal;
      } else {
        current_row[i] = 1 + std::min({
                                 current_row[i],     // deletion
                                 current_row[i - 1], // insertion
                                 previous_diagonal   // substitution
                             });
      }

      previous_diagonal = previous_current;
    }
  }

  return current_row[len1];
}

uint32_t LevenshteinAlgorithm::compute_distance_with_threshold(
    const std::u32string &s1, const std::u32string &s2, uint32_t max_distance,
    const Core::AlgorithmConfig &config) const {
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  // Early termination if length difference exceeds threshold
  if (std::abs(static_cast<int64_t>(len1) - static_cast<int64_t>(len2)) >
      max_distance) {
    return max_distance + 1;
  }

  // Use band optimization - only compute within the threshold band
  const size_t band_width = max_distance + 1;
  auto current_row = allocate_array<uint32_t>(2 * band_width + 1);
  auto previous_row = allocate_array<uint32_t>(2 * band_width + 1);

  if (!current_row || !previous_row) {
    throw std::bad_alloc();
  }

  // Initialize with sentinel values
  std::fill_n(current_row.get(), 2 * band_width + 1, max_distance + 1);
  std::fill_n(previous_row.get(), 2 * band_width + 1, max_distance + 1);

  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  // Set initial values within band
  for (size_t i = 0; i <= std::min(band_width, len1); ++i) {
    previous_row[band_width + i] = static_cast<uint32_t>(i);
  }

  for (size_t j = 1; j <= len2; ++j) {
    std::fill_n(current_row.get(), 2 * band_width + 1, max_distance + 1);

    size_t min_i = (j > band_width) ? j - band_width : 1;
    size_t max_i = std::min(len1, j + band_width);

    if (j <= band_width) {
      current_row[band_width] = static_cast<uint32_t>(j);
    }

    bool found_valid = false;

    for (size_t i = min_i; i <= max_i; ++i) {
      size_t idx = band_width + i - j;

      if (unicode_chars_equal(s1[i - 1], s2[j - 1], case_sensitive)) {
        current_row[idx] = previous_row[idx];
      } else {
        uint32_t min_cost = max_distance + 1;

        // Check all three operations within bounds
        if (idx > 0)
          min_cost = std::min(min_cost, current_row[idx - 1] + 1); // insertion
        if (idx < 2 * band_width)
          min_cost = std::min(min_cost, previous_row[idx + 1] + 1); // deletion
        min_cost = std::min(min_cost, previous_row[idx] + 1); // substitution

        current_row[idx] = min_cost;
      }

      if (current_row[idx] <= max_distance) {
        found_valid = true;
      }
    }

    // Early termination if no valid values in current row
    if (!found_valid) {
      return max_distance + 1;
    }

    std::swap(current_row, previous_row);
  }

  uint32_t result = previous_row[band_width + len1 - len2];
  return std::min(result, max_distance + 1);
}

uint32_t LevenshteinAlgorithm::compute_distance_simd(
    const std::string &s1, const std::string &s2,
    const Core::AlgorithmConfig &config) const {
#if defined(__x86_64__) || defined(__i386__)
  // SIMD implementation for x86_64 platforms
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  if (len1 > len2) {
    return compute_distance_simd(s2, s1, config);
  }

  // SIMD implementation for ASCII strings
  auto current_row = allocate_array<uint32_t>(len1 + 1);
  if (!current_row) {
    throw std::bad_alloc();
  }

  // Initialize first row
  for (size_t i = 0; i <= len1; ++i) {
    current_row[i] = static_cast<uint32_t>(i);
  }

  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  for (size_t j = 1; j <= len2; ++j) {
    uint32_t previous_diagonal = current_row[0];
    current_row[0] = static_cast<uint32_t>(j);

    char c2 = case_sensitive ? s2[j - 1] : (s2[j - 1] | 0x20);

    for (size_t i = 1; i <= len1; ++i) {
      uint32_t previous_current = current_row[i];

      char c1 = case_sensitive ? s1[i - 1] : (s1[i - 1] | 0x20);

      if (c1 == c2) {
        current_row[i] = previous_diagonal;
      } else {
        current_row[i] = 1 + std::min({
                                 current_row[i],     // deletion
                                 current_row[i - 1], // insertion
                                 previous_diagonal   // substitution
                             });
      }

      previous_diagonal = previous_current;
    }
  }

  return current_row[len1];
#else
  // Fallback to Unicode implementation on non-x86 platforms
  return compute_distance_single_row(Core::UnicodeString(s1).unicode(),
                                     Core::UnicodeString(s2).unicode(), config);
#endif
}

bool LevenshteinAlgorithm::characters_equal(
    char32_t c1, char32_t c2,
    const Core::AlgorithmConfig &config) const noexcept {
  return unicode_chars_equal(
      c1, c2, config.case_sensitivity == Core::CaseSensitivity::Sensitive);
}

double
LevenshteinAlgorithm::distance_to_similarity(uint32_t distance,
                                             size_t max_length) const noexcept {
  if (max_length == 0)
    return 1.0;
  return 1.0 -
         (static_cast<double>(distance) / static_cast<double>(max_length));
}

bool LevenshteinAlgorithm::is_ascii_string(
    const std::string &str) const noexcept {
  return std::all_of(str.begin(), str.end(), is_ascii_char);
}

// DamerauLevenshteinAlgorithm Implementation

Core::SimilarityResult DamerauLevenshteinAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  size_t max_len = std::max(s1.length(), s2.length());
  double similarity = distance_to_similarity(distance_result.value(), max_len);
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult DamerauLevenshteinAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    if (s1.empty())
      return Core::DistanceResult{static_cast<uint32_t>(s2.length())};
    if (s2.empty())
      return Core::DistanceResult{static_cast<uint32_t>(s1.length())};
    if (s1 == s2)
      return Core::DistanceResult{0U};

    // Use OSA (Optimal String Alignment) by default for better performance
    uint32_t distance =
        compute_osa_distance(s1.unicode(), s2.unicode(), config);
    return Core::DistanceResult{distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

uint32_t DamerauLevenshteinAlgorithm::compute_osa_distance(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  const size_t len1 = s1.length();
  const size_t len2 = s2.length();

  // Allocate matrix
  auto matrix = allocate_array<uint32_t>((len1 + 1) * (len2 + 1));
  if (!matrix) {
    throw std::bad_alloc();
  }

  auto get_cell = [&](size_t i, size_t j) -> uint32_t & {
    return matrix[i * (len2 + 1) + j];
  };

  // Initialize first row and column
  for (size_t i = 0; i <= len1; ++i)
    get_cell(i, 0) = static_cast<uint32_t>(i);
  for (size_t j = 0; j <= len2; ++j)
    get_cell(0, j) = static_cast<uint32_t>(j);

  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  for (size_t i = 1; i <= len1; ++i) {
    for (size_t j = 1; j <= len2; ++j) {
      uint32_t cost =
          unicode_chars_equal(s1[i - 1], s2[j - 1], case_sensitive) ? 0 : 1;

      get_cell(i, j) = std::min({
          get_cell(i - 1, j) + 1,       // deletion
          get_cell(i, j - 1) + 1,       // insertion
          get_cell(i - 1, j - 1) + cost // substitution
      });

      // Transposition
      if (i > 1 && j > 1 &&
          unicode_chars_equal(s1[i - 1], s2[j - 2], case_sensitive) &&
          unicode_chars_equal(s1[i - 2], s2[j - 1], case_sensitive)) {
        get_cell(i, j) =
            std::min(get_cell(i, j), get_cell(i - 2, j - 2) + cost);
      }
    }
  }

  return get_cell(len1, len2);
}

uint32_t DamerauLevenshteinAlgorithm::compute_unrestricted_distance(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  // Full unrestricted Damerau-Levenshtein implementation
  // This is more complex and requires character alphabet tracking
  // For now, fall back to OSA
  return compute_osa_distance(s1, s2, config);
}

double DamerauLevenshteinAlgorithm::distance_to_similarity(
    uint32_t distance, size_t max_length) const noexcept {
  if (max_length == 0)
    return 1.0;
  return 1.0 -
         (static_cast<double>(distance) / static_cast<double>(max_length));
}

// HammingAlgorithm Implementation

uint32_t HammingAlgorithm::get_maximum_distance() const noexcept {
  return std::numeric_limits<uint32_t>::max();
}

Core::SimilarityResult HammingAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  if (s1.length() != s2.length()) {
    return Core::SimilarityResult{Core::SimilarityError{
        Core::ErrorCode::InvalidInput,
        "Hamming distance requires equal-length strings"}};
  }

  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  double similarity =
      distance_to_similarity(distance_result.value(), s1.length());
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult HammingAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  if (s1.length() != s2.length()) {
    return Core::DistanceResult{Core::SimilarityError{
        Core::ErrorCode::InvalidInput,
        "Hamming distance requires equal-length strings"}};
  }

  try {
    if (s1 == s2)
      return Core::DistanceResult{0U};

    // For ASCII strings, use SIMD optimization
    auto is_ascii_str = [](const std::string &str) {
      return std::all_of(str.begin(), str.end(), is_ascii_char);
    };

    if (is_ascii_str(s1.utf8()) && is_ascii_str(s2.utf8())) {
      uint32_t distance = compute_hamming_simd(s1.utf8(), s2.utf8(), config);
      return Core::DistanceResult{distance};
    }

    uint32_t distance =
        compute_hamming_distance(s1.unicode(), s2.unicode(), config);
    return Core::DistanceResult{distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

uint32_t HammingAlgorithm::compute_hamming_distance(
    const std::u32string &s1, const std::u32string &s2,
    const Core::AlgorithmConfig &config) const {
  uint32_t distance = 0;
  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  for (size_t i = 0; i < s1.length(); ++i) {
    if (!unicode_chars_equal(s1[i], s2[i], case_sensitive)) {
      ++distance;
    }
  }

  return distance;
}

uint32_t HammingAlgorithm::compute_hamming_simd(
    const std::string &s1, const std::string &s2,
    const Core::AlgorithmConfig &config) const {
  uint32_t distance = 0;
  bool case_sensitive =
      (config.case_sensitivity == Core::CaseSensitivity::Sensitive);

  // Simple scalar implementation for now
  // Could be optimized with SIMD for large strings
  for (size_t i = 0; i < s1.length(); ++i) {
    char c1 = case_sensitive ? s1[i] : (s1[i] | 0x20);
    char c2 = case_sensitive ? s2[i] : (s2[i] | 0x20);

    if (c1 != c2) {
      ++distance;
    }
  }

  return distance;
}

double HammingAlgorithm::distance_to_similarity(uint32_t distance,
                                                size_t length) const noexcept {
  if (length == 0)
    return 1.0;
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(length));
}

} // namespace TextSimilarity::Algorithms
