#include "algorithm_factory.hpp"
#include "../algorithms/levenshtein.hpp"
#include "../algorithms/phonetic.hpp"
#include "../algorithms/token_based.hpp"
#include "../algorithms/vector_based.hpp"
#include "memory_pool.hpp"

namespace TextSimilarity::Core {

std::unique_ptr<ISimilarityAlgorithm>
AlgorithmFactory::create_algorithm(AlgorithmType type,
                                   const AlgorithmConfig &config) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  auto it = creators_.find(type);
  if (it == creators_.end()) {
    throw std::invalid_argument("Unsupported algorithm type: " +
                                std::to_string(static_cast<int>(type)));
  }

  // Use provided memory pool or default
  auto memory_pool = default_memory_pool_;
  if (!memory_pool) {
    memory_pool = create_memory_pool(1024 * 1024); // 1MB default
  }

  return it->second(config, memory_pool);
}

std::vector<AlgorithmType>
AlgorithmFactory::get_supported_algorithms() const noexcept {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  std::vector<AlgorithmType> algorithms;
  algorithms.reserve(creators_.size());

  for (const auto &[type, creator] : creators_) {
    algorithms.push_back(type);
  }

  return algorithms;
}

bool AlgorithmFactory::supports_algorithm(AlgorithmType type) const noexcept {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return creators_.find(type) != creators_.end();
}

void AlgorithmFactory::register_algorithm(AlgorithmType type,
                                          AlgorithmCreator creator) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  creators_[type] = std::move(creator);
}

void AlgorithmFactory::unregister_algorithm(AlgorithmType type) noexcept {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  creators_.erase(type);
}

AlgorithmFactory &AlgorithmFactory::instance() {
  static AlgorithmFactory factory;
  static bool initialized = false;

  if (!initialized) {
    factory.register_built_in_algorithms();
    initialized = true;
  }

  return factory;
}

void AlgorithmFactory::set_default_memory_pool(
    std::shared_ptr<IMemoryPool> pool) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  default_memory_pool_ = std::move(pool);
}

std::shared_ptr<IMemoryPool> AlgorithmFactory::get_default_memory_pool() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return default_memory_pool_;
}

void AlgorithmFactory::register_built_in_algorithms() {
  using namespace TextSimilarity::Algorithms;

  // Edit-based algorithms
  register_algorithm(
      AlgorithmType::Levenshtein,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<LevenshteinAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::DamerauLevenshtein,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<DamerauLevenshteinAlgorithm>(config,
                                                             std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Hamming,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<HammingAlgorithm>(config, std::move(pool));
      });

  // Phonetic algorithms
  register_algorithm(
      AlgorithmType::Jaro,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<JaroAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::JaroWinkler,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<JaroWinklerAlgorithm>(config, std::move(pool));
      });

  // Token-based algorithms
  register_algorithm(
      AlgorithmType::Jaccard,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<JaccardAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::SorensenDice,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<SorensenDiceAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Overlap,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<OverlapAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Tversky,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<TverskyAlgorithm>(config, std::move(pool));
      });

  // Vector-based algorithms
  register_algorithm(
      AlgorithmType::Cosine,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<CosineAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Euclidean,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<EuclideanAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Manhattan,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<ManhattanAlgorithm>(config, std::move(pool));
      });

  register_algorithm(
      AlgorithmType::Chebyshev,
      [](const AlgorithmConfig &config, std::shared_ptr<IMemoryPool> pool) {
        return std::make_unique<ChebyshevAlgorithm>(config, std::move(pool));
      });
}

// Helper functions implementation

std::unique_ptr<IAlgorithmFactory> create_algorithm_factory() {
  auto factory = std::make_unique<AlgorithmFactory>();
  // Initialize built-in algorithms
  factory->register_built_in_algorithms();
  return factory;
}

std::vector<std::string> get_algorithm_names() {
  auto &factory = AlgorithmFactory::instance();
  auto algorithms = factory.get_supported_algorithms();

  std::vector<std::string> names;
  names.reserve(algorithms.size());

  for (auto type : algorithms) {
    names.push_back(get_algorithm_name(type));
  }

  return names;
}

std::string get_algorithm_name(AlgorithmType type) {
  switch (type) {
  case AlgorithmType::Levenshtein:
    return "Levenshtein";
  case AlgorithmType::DamerauLevenshtein:
    return "Damerau-Levenshtein";
  case AlgorithmType::Hamming:
    return "Hamming";
  case AlgorithmType::Jaro:
    return "Jaro";
  case AlgorithmType::JaroWinkler:
    return "Jaro-Winkler";
  case AlgorithmType::Jaccard:
    return "Jaccard";
  case AlgorithmType::SorensenDice:
    return "Sorensen-Dice";
  case AlgorithmType::Overlap:
    return "Overlap";
  case AlgorithmType::Tversky:
    return "Tversky";
  case AlgorithmType::Cosine:
    return "Cosine";
  case AlgorithmType::Euclidean:
    return "Euclidean";
  case AlgorithmType::Manhattan:
    return "Manhattan";
  case AlgorithmType::Chebyshev:
    return "Chebyshev";
  default:
    return "Unknown";
  }
}

std::optional<AlgorithmType> parse_algorithm_type(const std::string &name) {
  static const std::unordered_map<std::string, AlgorithmType> name_to_type = {
      {"levenshtein", AlgorithmType::Levenshtein},
      {"damerau-levenshtein", AlgorithmType::DamerauLevenshtein},
      {"hamming", AlgorithmType::Hamming},
      {"jaro", AlgorithmType::Jaro},
      {"jaro-winkler", AlgorithmType::JaroWinkler},
      {"jaccard", AlgorithmType::Jaccard},
      {"sorensen-dice", AlgorithmType::SorensenDice},
      {"overlap", AlgorithmType::Overlap},
      {"tversky", AlgorithmType::Tversky},
      {"cosine", AlgorithmType::Cosine},
      {"euclidean", AlgorithmType::Euclidean},
      {"manhattan", AlgorithmType::Manhattan},
      {"chebyshev", AlgorithmType::Chebyshev}};

  // Convert to lowercase for case-insensitive matching
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 [](char c) { return std::tolower(c); });

  auto it = name_to_type.find(lower_name);
  if (it != name_to_type.end()) {
    return it->second;
  }

  return std::nullopt;
}

} // namespace TextSimilarity::Core