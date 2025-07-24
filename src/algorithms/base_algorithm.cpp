#include "base_algorithm.hpp"
#include <chrono>
#include <regex>
#include <sstream>

namespace TextSimilarity::Algorithms {

BaseAlgorithm::BaseAlgorithm(Core::AlgorithmConfig config,
                             std::shared_ptr<Core::IMemoryPool> memory_pool)
    : memory_pool_(std::move(memory_pool)), config_(std::move(config)) {
  if (!validate_configuration(config_)) {
    throw std::invalid_argument("Invalid algorithm configuration");
  }
}

Core::SimilarityResult
BaseAlgorithm::calculate_similarity(const Core::UnicodeString &s1,
                                    const Core::UnicodeString &s2) const {
  auto start = std::chrono::high_resolution_clock::now();

  try {
    // Quick answer optimization
    if (auto quick_result = get_quick_similarity_answer(s1, s2)) {
      update_metrics(std::chrono::high_resolution_clock::now() - start);
      return Core::SimilarityResult{*quick_result};
    }

    // Get current configuration thread-safely
    Core::AlgorithmConfig current_config;
    {
      std::shared_lock<std::shared_mutex> lock(config_mutex_);
      current_config = config_;
    }

    // Preprocess strings
    auto processed_s1 = preprocess_string(s1, current_config);
    auto processed_s2 = preprocess_string(s2, current_config);

    // Delegate to implementation
    auto result =
        compute_similarity_impl(processed_s1, processed_s2, current_config);

    update_metrics(std::chrono::high_resolution_clock::now() - start);
    return result;

  } catch (const std::exception &e) {
    return Core::SimilarityResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}};
  }
}

Core::DistanceResult
BaseAlgorithm::calculate_distance(const Core::UnicodeString &s1,
                                  const Core::UnicodeString &s2) const {
  auto start = std::chrono::high_resolution_clock::now();

  try {
    // Quick answer optimization
    if (auto quick_result = get_quick_distance_answer(s1, s2)) {
      update_metrics(std::chrono::high_resolution_clock::now() - start);
      return Core::DistanceResult{*quick_result};
    }

    // Get current configuration thread-safely
    Core::AlgorithmConfig current_config;
    {
      std::shared_lock<std::shared_mutex> lock(config_mutex_);
      current_config = config_;
    }

    // Preprocess strings
    auto processed_s1 = preprocess_string(s1, current_config);
    auto processed_s2 = preprocess_string(s2, current_config);

    // Delegate to implementation
    auto result =
        compute_distance_impl(processed_s1, processed_s2, current_config);

    update_metrics(std::chrono::high_resolution_clock::now() - start);
    return result;

  } catch (const std::exception &e) {
    return Core::DistanceResult{
        Core::SimilarityError{Core::ErrorCode::Unknown, e.what()}};
  }
}

void BaseAlgorithm::update_configuration(const Core::AlgorithmConfig &config) {
  if (!validate_configuration(config)) {
    throw std::invalid_argument("Invalid configuration provided");
  }

  std::unique_lock<std::shared_mutex> lock(config_mutex_);
  config_ = config;
}

Core::AlgorithmConfig BaseAlgorithm::get_configuration() const {
  std::shared_lock<std::shared_mutex> lock(config_mutex_);
  return config_;
}

bool BaseAlgorithm::supports_early_termination() const noexcept {
  return supports_early_termination_impl();
}

bool BaseAlgorithm::is_symmetric() const noexcept {
  return is_symmetric_impl();
}

bool BaseAlgorithm::is_metric() const noexcept { return is_metric_impl(); }

Core::UnicodeString
BaseAlgorithm::preprocess_string(const Core::UnicodeString &input,
                                 const Core::AlgorithmConfig &config) const {
  if (input.empty()) {
    return input;
  }

  // Apply case sensitivity
  Core::UnicodeString result = input;
  if (config.case_sensitivity == Core::CaseSensitivity::Insensitive) {
    result = result.to_lower();
  }

  return result;
}

std::vector<Core::UnicodeString>
BaseAlgorithm::tokenize_string(const Core::UnicodeString &input,
                               const Core::AlgorithmConfig &config) const {
  std::vector<Core::UnicodeString> tokens;

  switch (config.preprocessing) {
  case Core::PreprocessingMode::Character: {
    // Character-level tokenization
    const auto &unicode_str = input.unicode();
    tokens.reserve(unicode_str.length());

    for (char32_t c : unicode_str) {
      tokens.emplace_back(std::u32string(1, c));
    }
    break;
  }

  case Core::PreprocessingMode::Word: {
    // Word-level tokenization using regex
    std::regex word_regex(R"(\b\w+\b)");
    const std::string &utf8_str = input.utf8();

    std::sregex_iterator iter(utf8_str.begin(), utf8_str.end(), word_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
      tokens.emplace_back(iter->str());
    }
    break;
  }

  case Core::PreprocessingMode::NGram: {
    // N-gram tokenization
    auto ngrams = generate_ngrams(input, config.ngram_size);
    tokens.reserve(ngrams.size());

    for (const auto &ngram : ngrams) {
      tokens.emplace_back(ngram);
    }
    break;
  }

  case Core::PreprocessingMode::None:
  default:
    // No tokenization, return original string
    tokens.push_back(input);
    break;
  }

  return tokens;
}

std::vector<std::u32string>
BaseAlgorithm::generate_ngrams(const Core::UnicodeString &input,
                               uint32_t n) const {
  std::vector<std::u32string> ngrams;

  if (n == 0 || input.empty()) {
    return ngrams;
  }

  const auto &unicode_str = input.unicode();
  const size_t len = unicode_str.length();

  if (len < n) {
    // String is shorter than n-gram size, return the whole string
    ngrams.push_back(unicode_str);
    return ngrams;
  }

  // Generate n-grams
  ngrams.reserve(len - n + 1);
  for (size_t i = 0; i <= len - n; ++i) {
    ngrams.push_back(unicode_str.substr(i, n));
  }

  return ngrams;
}

bool BaseAlgorithm::validate_configuration(
    const Core::AlgorithmConfig &config) const noexcept {
  // Basic validation
  if (config.ngram_size == 0) {
    return false;
  }

  // Algorithm-specific validation for Tversky parameters
  if (config.algorithm == Core::AlgorithmType::Tversky) {
    if (!config.alpha.has_value() || !config.beta.has_value()) {
      return false;
    }
    if (*config.alpha < 0.0 || *config.beta < 0.0) {
      return false;
    }
  }

  // Jaro-Winkler specific validation
  if (config.algorithm == Core::AlgorithmType::JaroWinkler) {
    if (config.prefix_weight.has_value() &&
        (*config.prefix_weight < 0.0 || *config.prefix_weight > 0.25)) {
      return false;
    }
    if (config.prefix_length.has_value() && *config.prefix_length > 4) {
      return false;
    }
  }

  // Threshold validation
  if (config.threshold.has_value() && *config.threshold < 0.0) {
    return false;
  }

  return true;
}

std::optional<double> BaseAlgorithm::get_quick_similarity_answer(
    const Core::UnicodeString &s1,
    const Core::UnicodeString &s2) const noexcept {
  // Empty strings
  if (s1.empty() && s2.empty()) {
    return 1.0;
  }

  if (s1.empty() || s2.empty()) {
    return 0.0;
  }

  // Identical strings
  if (s1 == s2) {
    return 1.0;
  }

  // For case-insensitive comparison, check if they're the same when lowercased
  Core::AlgorithmConfig current_config;
  {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    current_config = config_;
  }

  if (current_config.case_sensitivity == Core::CaseSensitivity::Insensitive) {
    try {
      if (s1.to_lower() == s2.to_lower()) {
        return 1.0;
      }
    } catch (...) {
      // If case conversion fails, continue with normal processing
    }
  }

  return std::nullopt;
}

std::optional<uint32_t> BaseAlgorithm::get_quick_distance_answer(
    const Core::UnicodeString &s1,
    const Core::UnicodeString &s2) const noexcept {
  // Empty strings
  if (s1.empty() && s2.empty()) {
    return 0U;
  }

  if (s1.empty()) {
    return static_cast<uint32_t>(s2.length());
  }

  if (s2.empty()) {
    return static_cast<uint32_t>(s1.length());
  }

  // Identical strings
  if (s1 == s2) {
    return 0U;
  }

  // For case-insensitive comparison, check if they're the same when lowercased
  Core::AlgorithmConfig current_config;
  {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    current_config = config_;
  }

  if (current_config.case_sensitivity == Core::CaseSensitivity::Insensitive) {
    try {
      if (s1.to_lower() == s2.to_lower()) {
        return 0U;
      }
    } catch (...) {
      // If case conversion fails, continue with normal processing
    }
  }

  return std::nullopt;
}

void BaseAlgorithm::update_metrics(
    std::chrono::nanoseconds duration) const noexcept {
  call_count_.fetch_add(1, std::memory_order_relaxed);
  total_time_ns_.fetch_add(duration.count(), std::memory_order_relaxed);
}

} // namespace TextSimilarity::Algorithms