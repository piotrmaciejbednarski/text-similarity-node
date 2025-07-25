/**
 * text-similarity-node
 * High-performance and memory efficient native C++ text similarity algorithms for Node.js
 *
 * @version 1.0.1
 * @author Piotr Bednarski <piotr.maciej.bednarski@gmail.com>
 * @license MIT
 */

// Load the native addon - prebuilds are handled by node-gyp-build
const addon = require("node-gyp-build")(__dirname);

// Export the Modern API directly
module.exports = {
  // Main API functions
  calculateSimilarity: addon.calculateSimilarity,
  calculateDistance: addon.calculateDistance,
  calculateSimilarityBatch: addon.calculateSimilarityBatch,

  // Asynchronous API
  calculateSimilarityAsync: addon.calculateSimilarityAsync,
  calculateDistanceAsync: addon.calculateDistanceAsync,
  calculateSimilarityBatchAsync: addon.calculateSimilarityBatchAsync,

  // Configuration management
  setGlobalConfiguration: addon.setGlobalConfiguration,
  getGlobalConfiguration: addon.getGlobalConfiguration,
  getSupportedAlgorithms: addon.getSupportedAlgorithms,
  getMemoryUsage: addon.getMemoryUsage,
  clearCaches: addon.clearCaches,

  // Utility functions
  parseAlgorithmType: addon.parseAlgorithmType,
  getAlgorithmName: addon.getAlgorithmName,

  // Algorithm type constants
  AlgorithmType: addon.AlgorithmType,
  PreprocessingMode: addon.PreprocessingMode,
  CaseSensitivity: addon.CaseSensitivity,

  // Convenience namespaces for backward compatibility and ease of use
  similarity: {
    levenshtein: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.LEVENSHTEIN,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : 0;
    },

    damerauLevenshtein: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.DAMERAU_LEVENSHTEIN,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : 0;
    },

    hamming: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.HAMMING,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : 0;
    },

    jaro: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.JARO,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : 0;
    },

    jaroWinkler: (s1, s2, caseSensitive = true, prefixWeight = 0.1) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.JARO_WINKLER,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          prefixWeight: prefixWeight,
        },
      );
      return result.success ? result.value : 0;
    },

    jaccard: (
      s1,
      s2,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.JACCARD,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value : 0;
    },

    dice: (s1, s2, useWords = false, caseSensitive = true, ngramSize = 2) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.SORENSEN_DICE,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value : 0;
    },

    cosine: (s1, s2, useWords = false, caseSensitive = true, ngramSize = 2) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.COSINE,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value : 0;
    },

    tversky: (
      s1,
      s2,
      alpha,
      beta,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) => {
      const result = addon.calculateSimilarity(
        s1,
        s2,
        addon.AlgorithmType.TVERSKY,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
          alpha: alpha,
          beta: beta,
        },
      );
      return result.success ? result.value : 0;
    },
  },

  distance: {
    levenshtein: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.LEVENSHTEIN,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : Infinity;
    },

    damerauLevenshtein: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.DAMERAU_LEVENSHTEIN,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : Infinity;
    },

    hamming: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.HAMMING,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      );
      return result.success ? result.value : Infinity;
    },

    jaro: (s1, s2, caseSensitive = true) => {
      const result = addon.calculateDistance(s1, s2, addon.AlgorithmType.JARO, {
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
      });
      return result.success ? result.value / 1000.0 : 1.0; // Convert from integer scale
    },

    euclidean: (
      s1,
      s2,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.EUCLIDEAN,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value / 1000.0 : Infinity; // Convert from integer scale
    },

    manhattan: (
      s1,
      s2,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.MANHATTAN,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value / 1000.0 : Infinity; // Convert from integer scale
    },

    chebyshev: (
      s1,
      s2,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) => {
      const result = addon.calculateDistance(
        s1,
        s2,
        addon.AlgorithmType.CHEBYSHEV,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      );
      return result.success ? result.value / 1000.0 : Infinity; // Convert from integer scale
    },
  },

  // Asynchronous convenience API
  async: {
    levenshtein: (s1, s2, caseSensitive = true) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.LEVENSHTEIN, {
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
      }),

    damerauLevenshtein: (s1, s2, caseSensitive = true) =>
      addon.calculateSimilarityAsync(
        s1,
        s2,
        addon.AlgorithmType.DAMERAU_LEVENSHTEIN,
        {
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
        },
      ),

    hamming: (s1, s2, caseSensitive = true) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.HAMMING, {
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
      }),

    jaro: (s1, s2, caseSensitive = true) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.JARO, {
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
      }),

    jaroWinkler: (s1, s2, caseSensitive = true, prefixWeight = 0.1) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.JARO_WINKLER, {
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
        prefixWeight: prefixWeight,
      }),

    jaccard: (s1, s2, useWords = false, caseSensitive = true, ngramSize = 2) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.JACCARD, {
        preprocessing: useWords
          ? addon.PreprocessingMode.WORD
          : addon.PreprocessingMode.NGRAM,
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
        ngramSize: ngramSize,
      }),

    dice: (s1, s2, useWords = false, caseSensitive = true, ngramSize = 2) =>
      addon.calculateSimilarityAsync(
        s1,
        s2,
        addon.AlgorithmType.SORENSEN_DICE,
        {
          preprocessing: useWords
            ? addon.PreprocessingMode.WORD
            : addon.PreprocessingMode.NGRAM,
          caseSensitivity: caseSensitive
            ? addon.CaseSensitivity.SENSITIVE
            : addon.CaseSensitivity.INSENSITIVE,
          ngramSize: ngramSize,
        },
      ),

    cosine: (s1, s2, useWords = false, caseSensitive = true, ngramSize = 2) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.COSINE, {
        preprocessing: useWords
          ? addon.PreprocessingMode.WORD
          : addon.PreprocessingMode.NGRAM,
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
        ngramSize: ngramSize,
      }),

    tversky: (
      s1,
      s2,
      alpha,
      beta,
      useWords = false,
      caseSensitive = true,
      ngramSize = 2,
    ) =>
      addon.calculateSimilarityAsync(s1, s2, addon.AlgorithmType.TVERSKY, {
        preprocessing: useWords
          ? addon.PreprocessingMode.WORD
          : addon.PreprocessingMode.NGRAM,
        caseSensitivity: caseSensitive
          ? addon.CaseSensitivity.SENSITIVE
          : addon.CaseSensitivity.INSENSITIVE,
        ngramSize: ngramSize,
        alpha: alpha,
        beta: beta,
      }),
  },

  // Version and build information
  VERSION: "1.0.1",
  BUILD_INFO: {
    version: "1.0.1",
    buildDate: new Date().toISOString(),
    compiler: "Modern C++17/20",
    platform: process.platform,
    features: [
      "Unicode/UTF-8 Support",
      "SIMD Optimizations",
      "Memory Pooling",
      "Thread Safety",
      "Promise/Async API",
      "13+ Algorithms",
      "MISRA C++ Compliant",
      "Enterprise Ready",
    ],
  },
};

// Add metadata about algorithm performance characteristics
module.exports.ALGORITHM_PERFORMANCE = {
  LEVENSHTEIN: {
    timeComplexity: "O(m*n)",
    spaceComplexity: "O(min(m,n))",
    isSymmetric: true,
    isMetric: true,
    supportsEarlyTermination: true,
    recommendedFor: [
      "General text comparison",
      "Spell checking",
      "DNA sequences",
    ],
  },
  DAMERAU_LEVENSHTEIN: {
    timeComplexity: "O(m*n)",
    spaceComplexity: "O(m*n)",
    isSymmetric: true,
    isMetric: true,
    supportsEarlyTermination: true,
    recommendedFor: ["Typo detection", "OCR correction", "Keyboard errors"],
  },
  HAMMING: {
    timeComplexity: "O(n)",
    spaceComplexity: "O(1)",
    isSymmetric: true,
    isMetric: true,
    supportsEarlyTermination: false,
    recommendedFor: ["Equal-length strings", "Binary data", "Fast comparisons"],
  },
  JARO: {
    timeComplexity: "O(m*n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: true,
    isMetric: false,
    supportsEarlyTermination: false,
    recommendedFor: ["Names", "Short strings", "Fuzzy matching"],
  },
  JARO_WINKLER: {
    timeComplexity: "O(m*n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: true,
    isMetric: false,
    supportsEarlyTermination: false,
    recommendedFor: [
      "Names with common prefixes",
      "Person matching",
      "Address matching",
    ],
  },
  JACCARD: {
    timeComplexity: "O(m+n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: true,
    isMetric: true,
    supportsEarlyTermination: false,
    recommendedFor: [
      "Document similarity",
      "Set comparison",
      "N-gram analysis",
    ],
  },
  SORENSEN_DICE: {
    timeComplexity: "O(m+n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: true,
    isMetric: false,
    supportsEarlyTermination: false,
    recommendedFor: [
      "Document similarity",
      "Biomedical texts",
      "Overlap analysis",
    ],
  },
  COSINE: {
    timeComplexity: "O(m+n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: true,
    isMetric: false,
    supportsEarlyTermination: false,
    recommendedFor: [
      "Document similarity",
      "Vector comparison",
      "Information retrieval",
    ],
  },
  TVERSKY: {
    timeComplexity: "O(m+n)",
    spaceComplexity: "O(m+n)",
    isSymmetric: false,
    isMetric: false,
    supportsEarlyTermination: false,
    recommendedFor: [
      "Asymmetric similarity",
      "Prototype matching",
      "Custom weighting",
    ],
  },
};
