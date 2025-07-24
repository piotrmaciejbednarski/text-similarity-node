#include "vector_based.hpp"
#include <algorithm>
#include <array>
#include <unordered_map>
#include <unordered_set>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h> // For SIMD intrinsics (x86/x64 only)
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h> // For NEON intrinsics (ARM64/ARM32)
#endif

namespace TextSimilarity::Algorithms {

namespace {
// Helper to check if string is ASCII
bool is_ascii_char(char c) noexcept {
  return static_cast<unsigned char>(c) < 128;
}

// Fast character frequency counting for ASCII strings
std::array<uint32_t, 256> count_ascii_frequencies(const std::string &str) {
  std::array<uint32_t, 256> counts{};
  for (char c : str) {
    ++counts[static_cast<unsigned char>(c)];
  }
  return counts;
}
} // namespace

// CosineAlgorithm Implementation

Core::SimilarityResult CosineAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    // Special case: character vectorization mode for performance
    if (config.preprocessing == Core::PreprocessingMode::Character) {
      // Use optimized character vectorization
      if (is_ascii_string(s1.utf8()) && is_ascii_string(s2.utf8())) {
        double similarity = compute_cosine_simd(s1.utf8(), s2.utf8(), config);
        return Core::SimilarityResult{similarity};
      } else {
        double similarity =
            compute_cosine_character_vectorization(s1.unicode(), s2.unicode());
        return Core::SimilarityResult{similarity};
      }
    }

    // General token-based cosine similarity
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    FrequencyVector<std::u32string> vector1, vector2;

    for (const auto &token : tokens1) {
      vector1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      vector2.increment(token.unicode());
    }

    double similarity = compute_cosine_similarity(vector1, vector2);
    return Core::SimilarityResult{similarity};

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

Core::DistanceResult CosineAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto similarity_result = compute_similarity_impl(s1, s2, config);
  if (!similarity_result) {
    return Core::DistanceResult{similarity_result.error()};
  }

  // Cosine distance = 1 - cosine similarity
  double distance = 1.0 - similarity_result.value();
  uint32_t int_distance = static_cast<uint32_t>(std::round(distance * 1000.0));
  return Core::DistanceResult{int_distance};
}

double CosineAlgorithm::compute_cosine_similarity(
    const FrequencyVector<std::u32string> &vector1,
    const FrequencyVector<std::u32string> &vector2) const {
  if (vector1.empty() && vector2.empty()) {
    return 1.0;
  }

  if (vector1.empty() || vector2.empty()) {
    return 0.0;
  }

  // Check if frequency vectors are identical (avoids floating point errors)
  if (vector1.size() == vector2.size()) {
    bool identical = true;
    for (const auto &[term, freq] : vector1) {
      if (vector2.get_frequency(term) != freq) {
        identical = false;
        break;
      }
    }
    if (identical) {
      return 1.0;
    }
  }

  double magnitude1 = vector1.magnitude();
  double magnitude2 = vector2.magnitude();

  if (magnitude1 == 0.0 || magnitude2 == 0.0) {
    return 0.0;
  }

  double dot_product = vector1.dot_product(vector2);
  double similarity = dot_product / (magnitude1 * magnitude2);

  // Clamp to [0, 1] to handle floating point precision issues
  return std::max(0.0, std::min(1.0, similarity));
}

double CosineAlgorithm::compute_cosine_character_vectorization(
    const std::u32string &s1, const std::u32string &s2) const {
  if (s1.empty() && s2.empty()) {
    return 1.0;
  }

  if (s1.empty() || s2.empty()) {
    return 0.0;
  }

  // Use binary presence vectors for characters
  std::unordered_set<char32_t> chars1(s1.begin(), s1.end());
  std::unordered_set<char32_t> chars2(s2.begin(), s2.end());

  // Calculate intersection size
  size_t intersection_size = 0;
  const auto &smaller_set = (chars1.size() <= chars2.size()) ? chars1 : chars2;
  const auto &larger_set = (chars1.size() <= chars2.size()) ? chars2 : chars1;

  for (char32_t c : smaller_set) {
    if (larger_set.find(c) != larger_set.end()) {
      ++intersection_size;
    }
  }

  // Cosine similarity with binary vectors: |A ∩ B| / sqrt(|A| * |B|)
  double denominator = std::sqrt(static_cast<double>(chars1.size()) *
                                 static_cast<double>(chars2.size()));

  if (denominator == 0.0) {
    return 0.0;
  }

  return static_cast<double>(intersection_size) / denominator;
}

double CosineAlgorithm::compute_cosine_simd(
    const std::string &s1, const std::string &s2,
    const Core::AlgorithmConfig &config) const {
  if (s1.empty() && s2.empty()) {
    return 1.0;
  }

  if (s1.empty() || s2.empty()) {
    return 0.0;
  }

  // Count character frequencies
  auto freq1 = count_ascii_frequencies(s1);
  auto freq2 = count_ascii_frequencies(s2);

  // Apply case insensitive folding if needed
  if (config.case_sensitivity == Core::CaseSensitivity::Insensitive) {
    for (size_t i = 'A'; i <= 'Z'; ++i) {
      freq1[i + 32] += freq1[i]; // Add uppercase to lowercase
      freq1[i] = 0;              // Clear uppercase
      freq2[i + 32] += freq2[i];
      freq2[i] = 0;
    }
  }

  // Calculate dot product and magnitudes
  double dot_product = 0.0;
  double magnitude1_squared = 0.0;
  double magnitude2_squared = 0.0;

#ifdef __x86_64__
  // SIMD-optimized calculation for large vectors
  for (size_t i = 0; i < 256; i += 8) {
    __m256i v1 =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&freq1[i]));
    __m256i v2 =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&freq2[i]));

    // Convert to double for precision
    __m256d d1_lo = _mm256_cvtepi32_pd(_mm256_extracti128_si256(v1, 0));
    __m256d d1_hi = _mm256_cvtepi32_pd(_mm256_extracti128_si256(v1, 1));
    __m256d d2_lo = _mm256_cvtepi32_pd(_mm256_extracti128_si256(v2, 0));
    __m256d d2_hi = _mm256_cvtepi32_pd(_mm256_extracti128_si256(v2, 1));

    // Dot product
    __m256d dot_lo = _mm256_mul_pd(d1_lo, d2_lo);
    __m256d dot_hi = _mm256_mul_pd(d1_hi, d2_hi);

    // Magnitude squares
    __m256d mag1_lo = _mm256_mul_pd(d1_lo, d1_lo);
    __m256d mag1_hi = _mm256_mul_pd(d1_hi, d1_hi);
    __m256d mag2_lo = _mm256_mul_pd(d2_lo, d2_lo);
    __m256d mag2_hi = _mm256_mul_pd(d2_hi, d2_hi);

    // Horizontal sum using array extraction
    double dot_array_lo[4], dot_array_hi[4];
    double mag1_array_lo[4], mag1_array_hi[4];
    double mag2_array_lo[4], mag2_array_hi[4];

    _mm256_storeu_pd(dot_array_lo, dot_lo);
    _mm256_storeu_pd(dot_array_hi, dot_hi);
    _mm256_storeu_pd(mag1_array_lo, mag1_lo);
    _mm256_storeu_pd(mag1_array_hi, mag1_hi);
    _mm256_storeu_pd(mag2_array_lo, mag2_lo);
    _mm256_storeu_pd(mag2_array_hi, mag2_hi);

    for (int j = 0; j < 4; ++j) {
      dot_product += dot_array_lo[j] + dot_array_hi[j];
      magnitude1_squared += mag1_array_lo[j] + mag1_array_hi[j];
      magnitude2_squared += mag2_array_lo[j] + mag2_array_hi[j];
    }
  }
#else
  // Scalar fallback
  for (size_t i = 0; i < 256; ++i) {
    double f1 = static_cast<double>(freq1[i]);
    double f2 = static_cast<double>(freq2[i]);

    dot_product += f1 * f2;
    magnitude1_squared += f1 * f1;
    magnitude2_squared += f2 * f2;
  }
#endif

  double denominator = std::sqrt(magnitude1_squared * magnitude2_squared);

  if (denominator == 0.0) {
    return 0.0;
  }

  return std::max(0.0, std::min(1.0, dot_product / denominator));
}

bool CosineAlgorithm::is_ascii_string(const std::string &str) const noexcept {
  return std::all_of(str.begin(), str.end(), is_ascii_char);
}

// EuclideanAlgorithm Implementation

Core::SimilarityResult EuclideanAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  double similarity = distance_to_similarity(
      static_cast<double>(distance_result.value()) / 1000.0);
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult EuclideanAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    FrequencyVector<std::u32string> vector1, vector2;

    for (const auto &token : tokens1) {
      vector1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      vector2.increment(token.unicode());
    }

    double distance = compute_euclidean_distance(vector1, vector2);
    uint32_t int_distance =
        static_cast<uint32_t>(std::round(distance * 1000.0));
    return Core::DistanceResult{int_distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

double EuclideanAlgorithm::compute_euclidean_distance(
    const FrequencyVector<std::u32string> &vector1,
    const FrequencyVector<std::u32string> &vector2) const {
  // Get union of all terms
  auto all_terms =
      FrequencyVector<std::u32string>::get_union_terms(vector1, vector2);

  double sum_squared_differences = 0.0;

  for (const auto &term : all_terms) {
    double freq1 = static_cast<double>(vector1.get_frequency(term));
    double freq2 = static_cast<double>(vector2.get_frequency(term));
    double difference = freq1 - freq2;
    sum_squared_differences += difference * difference;
  }

  return std::sqrt(sum_squared_differences);
}

double
EuclideanAlgorithm::distance_to_similarity(double distance) const noexcept {
  // Convert distance to similarity using exponential decay
  return std::exp(-distance);
}

// ManhattanAlgorithm Implementation

Core::SimilarityResult ManhattanAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  double similarity = distance_to_similarity(
      static_cast<double>(distance_result.value()) / 1000.0);
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult ManhattanAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    FrequencyVector<std::u32string> vector1, vector2;

    for (const auto &token : tokens1) {
      vector1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      vector2.increment(token.unicode());
    }

    double distance = compute_manhattan_distance(vector1, vector2);
    uint32_t int_distance =
        static_cast<uint32_t>(std::round(distance * 1000.0));
    return Core::DistanceResult{int_distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

double ManhattanAlgorithm::compute_manhattan_distance(
    const FrequencyVector<std::u32string> &vector1,
    const FrequencyVector<std::u32string> &vector2) const {
  auto all_terms =
      FrequencyVector<std::u32string>::get_union_terms(vector1, vector2);

  double sum_absolute_differences = 0.0;

  for (const auto &term : all_terms) {
    double freq1 = static_cast<double>(vector1.get_frequency(term));
    double freq2 = static_cast<double>(vector2.get_frequency(term));
    sum_absolute_differences += std::abs(freq1 - freq2);
  }

  return sum_absolute_differences;
}

double
ManhattanAlgorithm::distance_to_similarity(double distance) const noexcept {
  return 1.0 / (1.0 + distance);
}

// ChebyshevAlgorithm Implementation

Core::SimilarityResult ChebyshevAlgorithm::compute_similarity_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  auto distance_result = compute_distance_impl(s1, s2, config);
  if (!distance_result) {
    return Core::SimilarityResult{distance_result.error()};
  }

  double similarity = distance_to_similarity(
      static_cast<double>(distance_result.value()) / 1000.0);
  return Core::SimilarityResult{similarity};
}

Core::DistanceResult ChebyshevAlgorithm::compute_distance_impl(
    const Core::UnicodeString &s1, const Core::UnicodeString &s2,
    const Core::AlgorithmConfig &config) const {
  try {
    auto tokens1 = tokenize_string(s1, config);
    auto tokens2 = tokenize_string(s2, config);

    FrequencyVector<std::u32string> vector1, vector2;

    for (const auto &token : tokens1) {
      vector1.increment(token.unicode());
    }
    for (const auto &token : tokens2) {
      vector2.increment(token.unicode());
    }

    double distance = compute_chebyshev_distance(vector1, vector2);
    uint32_t int_distance =
        static_cast<uint32_t>(std::round(distance * 1000.0));
    return Core::DistanceResult{int_distance};

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::ComputationOverflow, e.what()}};
  }
}

double ChebyshevAlgorithm::compute_chebyshev_distance(
    const FrequencyVector<std::u32string> &vector1,
    const FrequencyVector<std::u32string> &vector2) const {
  auto all_terms =
      FrequencyVector<std::u32string>::get_union_terms(vector1, vector2);

  double max_difference = 0.0;

  for (const auto &term : all_terms) {
    double freq1 = static_cast<double>(vector1.get_frequency(term));
    double freq2 = static_cast<double>(vector2.get_frequency(term));
    double difference = std::abs(freq1 - freq2);
    max_difference = std::max(max_difference, difference);
  }

  return max_difference;
}

double
ChebyshevAlgorithm::distance_to_similarity(double distance) const noexcept {
  return std::exp(-distance);
}

} // namespace TextSimilarity::Algorithms
