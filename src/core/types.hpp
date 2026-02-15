#pragma once

#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace TextSimilarity::Core {

// Strongly-typed enums
enum class AlgorithmType : uint8_t {
  Levenshtein = 0,
  DamerauLevenshtein,
  Hamming,
  Jaro,
  JaroWinkler,
  Jaccard,
  SorensenDice,
  Overlap,
  Tversky,
  Cosine,
  Euclidean,
  Manhattan,
  Chebyshev
};

enum class PreprocessingMode : uint8_t {
  None = 0,  // No preprocessing
  Character, // Character-level comparison (qval=1)
  Word,      // Word-level comparison (qval=None)
  NGram      // N-gram based comparison (qval>1)
};

enum class NormalizationMode : uint8_t {
  None = 0,  // Raw scores
  Distance,  // 0-1 normalized distance
  Similarity // 0-1 normalized similarity
};

enum class CaseSensitivity : uint8_t { Sensitive = 0, Insensitive };

// Configuration structures with RAII principles
struct AlgorithmConfig {
  AlgorithmType algorithm{AlgorithmType::Levenshtein};
  PreprocessingMode preprocessing{PreprocessingMode::Character};
  NormalizationMode normalization{NormalizationMode::Similarity};
  CaseSensitivity case_sensitivity{CaseSensitivity::Sensitive};
  uint32_t ngram_size{2U};

  // Algorithm-specific parameters
  std::optional<double> threshold{};       // For early termination
  std::optional<double> alpha{};           // Tversky alpha
  std::optional<double> beta{};            // Tversky beta
  std::optional<double> prefix_weight{};   // Jaro-Winkler prefix weight
  std::optional<uint32_t> prefix_length{}; // Jaro-Winkler prefix length
  std::optional<size_t> max_string_length{}; // Max input string length (bytes)
};

// Result types with comprehensive error handling
enum class ErrorCode : uint8_t {
  Success = 0,
  InvalidInput,
  InvalidConfiguration,
  MemoryAllocation,
  UnicodeConversion,
  ComputationOverflow,
  ThreadingError,
  Unknown
};

class SimilarityError final {
public:
  explicit SimilarityError(ErrorCode code, std::string message = "")
      : code_(code), message_(std::move(message)) {}

  [[nodiscard]] ErrorCode code() const noexcept { return code_; }
  [[nodiscard]] const std::string &message() const noexcept { return message_; }

private:
  ErrorCode code_;
  std::string message_;
};

// Result wrapper with error handling
template <typename T> class Result final {
public:
  // Default constructor (for uninitialized state)
  Result() : value_(std::nullopt), error_(std::nullopt) {}

  // Success constructor
  explicit Result(T value) : value_(std::move(value)), error_(std::nullopt) {}

  // Error constructor
  explicit Result(SimilarityError error)
      : value_(std::nullopt), error_(std::move(error)) {}

  [[nodiscard]] bool is_success() const noexcept { return value_.has_value(); }
  [[nodiscard]] bool is_error() const noexcept { return error_.has_value(); }

  [[nodiscard]] const T &value() const {
    if (!value_.has_value()) {
      throw std::runtime_error("Attempting to access value from error result");
    }
    return *value_;
  }

  [[nodiscard]] const SimilarityError &error() const {
    if (!error_.has_value()) {
      throw std::runtime_error(
          "Attempting to access error from success result");
    }
    return *error_;
  }

  // Conversion operators for convenience
  explicit operator bool() const noexcept { return is_success(); }

private:
  std::optional<T> value_;
  std::optional<SimilarityError> error_;
};

using SimilarityResult = Result<double>;
using DistanceResult = Result<uint32_t>;

// Unicode-safe string types
class UnicodeString final {
public:
  explicit UnicodeString(std::string utf8_string);
  explicit UnicodeString(std::u32string unicode_string);

  [[nodiscard]] const std::string &utf8() const noexcept {
    return utf8_string_;
  }
  [[nodiscard]] const std::u32string &unicode() const noexcept {
    return unicode_string_;
  }
  [[nodiscard]] size_t length() const noexcept {
    return unicode_string_.length();
  }
  [[nodiscard]] bool empty() const noexcept { return unicode_string_.empty(); }

  // Case conversion methods
  [[nodiscard]] UnicodeString to_lower() const;
  [[nodiscard]] UnicodeString to_upper() const;

  // Comparison operators
  bool operator==(const UnicodeString &other) const noexcept;
  bool operator!=(const UnicodeString &other) const noexcept;

private:
  std::string utf8_string_;
  std::u32string unicode_string_;

  void validate_and_convert();
};

// Memory-safe containers
template <typename T> using SafeVector = std::vector<T>;

template <typename K, typename V> using SafeMap = std::unordered_map<K, V>;

// Thread-safe async types
template <typename T> using AsyncResult = std::future<Result<T>>;

using AsyncSimilarityResult = AsyncResult<double>;
using AsyncDistanceResult = AsyncResult<uint32_t>;

// Callback types for Node.js integration
using SimilarityCallback = std::function<void(const SimilarityResult &)>;
using DistanceCallback = std::function<void(const DistanceResult &)>;

} // namespace TextSimilarity::Core