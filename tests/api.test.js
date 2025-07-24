const {
  calculateSimilarity,
  calculateDistance,
  calculateSimilarityBatch,
  calculateSimilarityAsync,
  calculateDistanceAsync,
  calculateSimilarityBatchAsync,
  setGlobalConfiguration,
  getGlobalConfiguration,
  getSupportedAlgorithms,
  getMemoryUsage,
  clearCaches,
  parseAlgorithmType,
  getAlgorithmName,
  AlgorithmType,
  PreprocessingMode,
  CaseSensitivity,
} = require("../index");

describe("text-similarity-node", () => {
  beforeEach(() => {
    // Reset configuration before each test
    setGlobalConfiguration({
      preprocessing: PreprocessingMode.CHARACTER,
      caseSensitivity: CaseSensitivity.SENSITIVE,
      ngramSize: 2,
    });
    clearCaches();
  });

  describe("Basic Similarity Functions", () => {
    test("calculateSimilarity - Levenshtein", () => {
      const result = calculateSimilarity(
        "hello",
        "hallo",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeCloseTo(0.8, 2);
      expect(result.error).toBeUndefined();
    });

    test("calculateSimilarity - Jaccard with n-grams", () => {
      const result = calculateSimilarity(
        "hello",
        "hallo",
        AlgorithmType.JACCARD,
        {
          preprocessing: PreprocessingMode.NGRAM,
          ngramSize: 2,
        },
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0);
      expect(result.value).toBeLessThanOrEqual(1);
    });

    test("calculateSimilarity - Cosine with words", () => {
      const result = calculateSimilarity(
        "hello world",
        "world hello",
        AlgorithmType.COSINE,
        {
          preprocessing: PreprocessingMode.WORD,
        },
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeCloseTo(1.0, 2); // Same words, different order
    });

    test("calculateSimilarity - Jaro-Winkler", () => {
      const result = calculateSimilarity(
        "martha",
        "marhta",
        AlgorithmType.JARO_WINKLER,
        {
          prefixWeight: 0.1,
          prefixLength: 4,
        },
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.9);
    });

    test("calculateSimilarity - Tversky with custom weights", () => {
      const result = calculateSimilarity(
        "hello",
        "hallo",
        AlgorithmType.TVERSKY,
        {
          alpha: 0.5,
          beta: 0.5,
          preprocessing: PreprocessingMode.NGRAM,
          ngramSize: 2,
        },
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0);
    });

    test("calculateDistance - Levenshtein", () => {
      const result = calculateDistance(
        "kitten",
        "sitting",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBe(3);
    });

    test("calculateDistance - Hamming", () => {
      const result = calculateDistance("hello", "hallo", AlgorithmType.HAMMING);

      expect(result.success).toBe(true);
      expect(result.value).toBe(1);
    });
  });

  describe("Batch Operations", () => {
    const testPairs = [
      ["hello", "hallo"],
      ["world", "word"],
      ["test", "text"],
      ["similar", "similarity"],
    ];

    test("calculateSimilarityBatch - synchronous", () => {
      const results = calculateSimilarityBatch(
        testPairs,
        AlgorithmType.LEVENSHTEIN,
      );

      expect(results).toHaveLength(testPairs.length);

      results.forEach((result, _index) => {
        expect(result.success).toBe(true);
        expect(result.value).toBeGreaterThan(0);
        expect(result.value).toBeLessThanOrEqual(1);
      });
    });

    test("calculateSimilarityBatch - with different algorithms", () => {
      const results = calculateSimilarityBatch(
        testPairs,
        AlgorithmType.JACCARD,
        {
          preprocessing: PreprocessingMode.NGRAM,
          ngramSize: 3,
        },
      );

      expect(results).toHaveLength(testPairs.length);
      results.forEach((result) => {
        expect(result.success).toBe(true);
        expect(typeof result.value).toBe("number");
      });
    });
  });

  describe("Asynchronous API", () => {
    test("calculateSimilarityAsync - Promise-based", async () => {
      const similarity = await calculateSimilarityAsync(
        "hello",
        "hallo",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(typeof similarity).toBe("number");
      expect(similarity).toBeCloseTo(0.8, 2);
    });

    test("calculateDistanceAsync - Promise-based", async () => {
      const distance = await calculateDistanceAsync(
        "kitten",
        "sitting",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(typeof distance).toBe("number");
      expect(distance).toBe(3);
    });

    test("calculateSimilarityBatchAsync - Promise-based batch", async () => {
      const testPairs = [
        ["hello", "hallo"],
        ["world", "word"],
      ];

      const results = await calculateSimilarityBatchAsync(
        testPairs,
        AlgorithmType.LEVENSHTEIN,
      );

      expect(Array.isArray(results)).toBe(true);
      expect(results).toHaveLength(testPairs.length);

      results.forEach((similarity) => {
        expect(typeof similarity).toBe("number");
        expect(similarity).toBeGreaterThan(0);
        expect(similarity).toBeLessThanOrEqual(1);
      });
    });

    test("calculateSimilarityAsync - error handling", async () => {
      // Test with very large strings that might cause memory issues
      const largeString1 = "a".repeat(1000000); // 1MB string
      const largeString2 = "b".repeat(1000000);

      await expect(
        calculateSimilarityAsync(largeString1, largeString2),
      ).rejects.toThrow();
    });
  });

  describe("Configuration Management", () => {
    test("setGlobalConfiguration and getGlobalConfiguration", () => {
      const config = {
        preprocessing: PreprocessingMode.WORD,
        caseSensitivity: CaseSensitivity.INSENSITIVE,
        ngramSize: 3,
      };

      setGlobalConfiguration(config);
      const retrievedConfig = getGlobalConfiguration();

      expect(retrievedConfig.preprocessing).toBe(PreprocessingMode.WORD);
      expect(retrievedConfig.caseSensitivity).toBe(CaseSensitivity.INSENSITIVE);
      expect(retrievedConfig.ngramSize).toBe(3);
    });

    test("Case insensitive comparison", () => {
      setGlobalConfiguration({
        caseSensitivity: CaseSensitivity.INSENSITIVE,
      });

      const result = calculateSimilarity(
        "Hello",
        "HELLO",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeCloseTo(1.0, 2);
    });
  });

  describe("Unicode Support", () => {
    test("Unicode characters - Basic Latin Extended", () => {
      const result = calculateSimilarity(
        "café",
        "cafe",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.5);
    });

    test("Unicode characters - Greek", () => {
      const result = calculateSimilarity(
        "αβγ",
        "αβδ",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.6);
    });

    test("Unicode characters - Emoji", () => {
      const result = calculateSimilarity(
        "hello 😀",
        "hello 😃",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.8);
    });

    test("Unicode case insensitive - Mixed scripts", () => {
      setGlobalConfiguration({
        caseSensitivity: CaseSensitivity.INSENSITIVE,
      });

      const result = calculateSimilarity(
        "Café",
        "CAFÉ",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeCloseTo(1.0, 2);
    });
  });

  describe("Algorithm Information", () => {
    test("getSupportedAlgorithms", () => {
      const algorithms = getSupportedAlgorithms();

      expect(Array.isArray(algorithms)).toBe(true);
      expect(algorithms.length).toBeGreaterThan(10);

      algorithms.forEach((algo) => {
        expect(algo).toHaveProperty("type");
        expect(algo).toHaveProperty("name");
        expect(typeof algo.type).toBe("number");
        expect(typeof algo.name).toBe("string");
      });
    });

    test("parseAlgorithmType and getAlgorithmName", () => {
      const levenshteinType = parseAlgorithmType("levenshtein");
      expect(levenshteinType).toBe(AlgorithmType.LEVENSHTEIN);

      const levenshteinName = getAlgorithmName(AlgorithmType.LEVENSHTEIN);
      expect(levenshteinName).toBe("Levenshtein");
    });

    test("Algorithm type constants", () => {
      expect(typeof AlgorithmType.LEVENSHTEIN).toBe("number");
      expect(typeof AlgorithmType.JACCARD).toBe("number");
      expect(typeof AlgorithmType.COSINE).toBe("number");
      expect(typeof AlgorithmType.JARO_WINKLER).toBe("number");
    });
  });

  describe("Performance and Memory", () => {
    test("getMemoryUsage", () => {
      const initialUsage = getMemoryUsage();
      expect(typeof initialUsage).toBe("number");
      expect(initialUsage).toBeGreaterThanOrEqual(0);

      // Perform some operations to increase memory usage
      calculateSimilarityBatch(
        [
          ["test1", "test2"],
          ["test3", "test4"],
          ["test5", "test6"],
        ],
        AlgorithmType.LEVENSHTEIN,
      );

      const afterUsage = getMemoryUsage();
      expect(afterUsage).toBeGreaterThanOrEqual(initialUsage);
    });

    test("clearCaches", () => {
      // Perform operations to populate cache
      for (let i = 0; i < 10; i++) {
        calculateSimilarity(
          `test${i}`,
          `test${i + 1}`,
          AlgorithmType.LEVENSHTEIN,
        );
      }

      const beforeClear = getMemoryUsage();
      clearCaches();
      const afterClear = getMemoryUsage();

      expect(afterClear).toBeLessThanOrEqual(beforeClear);
    });

    test("Performance - Large batch processing", () => {
      const largeBatch = [];
      for (let i = 0; i < 100; i++) {
        largeBatch.push([`string${i}`, `string${i + 1}`]);
      }

      const startTime = Date.now();
      const results = calculateSimilarityBatch(
        largeBatch,
        AlgorithmType.LEVENSHTEIN,
      );
      const endTime = Date.now();

      expect(results).toHaveLength(largeBatch.length);
      expect(endTime - startTime).toBeLessThan(1000); // Should complete in under 1 second
    });
  });

  describe("Error Handling", () => {
    test("Invalid algorithm type", () => {
      const result = calculateSimilarity("hello", "world", 999); // Invalid algorithm type

      expect(result.success).toBe(false);
      expect(result.error).toBeDefined();
    });

    test("Invalid configuration", () => {
      const result = calculateSimilarity(
        "hello",
        "world",
        AlgorithmType.TVERSKY,
        {
          // Missing required alpha and beta for Tversky
          preprocessing: PreprocessingMode.NGRAM,
        },
      );

      expect(result.success).toBe(false);
      expect(result.error).toBeDefined();
    });

    test("Empty string handling", () => {
      const result = calculateSimilarity("", "", AlgorithmType.LEVENSHTEIN);

      expect(result.success).toBe(true);
      expect(result.value).toBe(1.0); // Empty strings are identical
    });

    test("One empty string", () => {
      const result = calculateSimilarity(
        "hello",
        "",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBe(0.0); // Empty vs non-empty is completely different
    });

    test("Hamming distance with different length strings", () => {
      const result = calculateDistance("hello", "hi", AlgorithmType.HAMMING);

      expect(result.success).toBe(false);
      expect(result.error).toBeDefined();
      expect(result.error.message).toContain("equal-length");
    });
  });

  describe("Algorithm-Specific Tests", () => {
    test("Damerau-Levenshtein vs Levenshtein with transpositions", () => {
      const str1 = "abcdef";
      const str2 = "abcedf"; // transposition of 'd' and 'e'

      const levenshtein = calculateDistance(
        str1,
        str2,
        AlgorithmType.LEVENSHTEIN,
      );
      const damerau = calculateDistance(
        str1,
        str2,
        AlgorithmType.DAMERAU_LEVENSHTEIN,
      );

      expect(levenshtein.success).toBe(true);
      expect(damerau.success).toBe(true);

      // Damerau-Levenshtein should handle transposition better
      expect(damerau.value).toBeLessThanOrEqual(levenshtein.value);
    });

    test("Jaccard vs Sorensen-Dice comparison", () => {
      const str1 = "hello world";
      const str2 = "hello earth";

      const jaccard = calculateSimilarity(str1, str2, AlgorithmType.JACCARD, {
        preprocessing: PreprocessingMode.WORD,
      });
      const dice = calculateSimilarity(
        str1,
        str2,
        AlgorithmType.SORENSEN_DICE,
        {
          preprocessing: PreprocessingMode.WORD,
        },
      );

      expect(jaccard.success).toBe(true);
      expect(dice.success).toBe(true);

      // Dice typically gives higher values than Jaccard for same input
      expect(dice.value).toBeGreaterThanOrEqual(jaccard.value);
    });

    test("Cosine similarity - character vs word level", () => {
      const str1 = "hello world";
      const str2 = "world hello";

      const charLevel = calculateSimilarity(str1, str2, AlgorithmType.COSINE, {
        preprocessing: PreprocessingMode.CHARACTER,
      });
      const wordLevel = calculateSimilarity(str1, str2, AlgorithmType.COSINE, {
        preprocessing: PreprocessingMode.WORD,
      });

      expect(charLevel.success).toBe(true);
      expect(wordLevel.success).toBe(true);

      // Both should return exactly 1.0 (same words/characters, different order)
      expect(wordLevel.value).toBe(1.0);
      expect(charLevel.value).toBe(1.0);
    });
  });

  describe("Edge Cases and Stress Tests", () => {
    test("Very long strings", () => {
      const longStr1 = "a".repeat(10000);
      const longStr2 = `${"a".repeat(9999)}b`;

      const result = calculateSimilarity(
        longStr1,
        longStr2,
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeCloseTo(0.9999, 4);
    });

    test("Identical strings", () => {
      const result = calculateSimilarity(
        "identical",
        "identical",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBe(1.0);
    });

    test("Completely different strings", () => {
      const result = calculateSimilarity(
        "abcdef",
        "xyz123",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeLessThan(0.5);
    });

    test("Special characters and punctuation", () => {
      const result = calculateSimilarity(
        "Hello, world!",
        "Hello world",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.8);
    });

    test("Numbers and mixed content", () => {
      const result = calculateSimilarity(
        "Version 1.2.3",
        "Version 1.2.4",
        AlgorithmType.LEVENSHTEIN,
      );

      expect(result.success).toBe(true);
      expect(result.value).toBeGreaterThan(0.9);
    });
  });
});

describe("Performance Benchmarks", () => {
  const generateRandomString = (length) => {
    const chars =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    let result = "";
    for (let i = 0; i < length; i++) {
      result += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return result;
  };

  test("Benchmark - Small strings (10-50 chars)", () => {
    const pairs = [];
    for (let i = 0; i < 1000; i++) {
      pairs.push([
        generateRandomString(Math.floor(Math.random() * 40) + 10),
        generateRandomString(Math.floor(Math.random() * 40) + 10),
      ]);
    }

    const startTime = Date.now();
    const results = calculateSimilarityBatch(pairs, AlgorithmType.LEVENSHTEIN);
    const endTime = Date.now();

    expect(results).toHaveLength(pairs.length);
    expect(endTime - startTime).toBeLessThan(500); // Should be very fast

    console.log(
      `Small strings benchmark: ${endTime - startTime}ms for ${pairs.length} pairs`,
    );
  });

  test("Benchmark - Medium strings (100-500 chars)", () => {
    const pairs = [];
    for (let i = 0; i < 100; i++) {
      pairs.push([
        generateRandomString(Math.floor(Math.random() * 400) + 100),
        generateRandomString(Math.floor(Math.random() * 400) + 100),
      ]);
    }

    const startTime = Date.now();
    const results = calculateSimilarityBatch(pairs, AlgorithmType.LEVENSHTEIN);
    const endTime = Date.now();

    expect(results).toHaveLength(pairs.length);
    expect(endTime - startTime).toBeLessThan(2000); // Should complete in under 2 seconds

    console.log(
      `Medium strings benchmark: ${endTime - startTime}ms for ${pairs.length} pairs`,
    );
  });

  test("Benchmark - Async vs Sync performance", async () => {
    const testPairs = [];
    for (let i = 0; i < 50; i++) {
      testPairs.push([generateRandomString(100), generateRandomString(100)]);
    }

    // Sync benchmark
    const syncStart = process.hrtime.bigint();
    const syncResults = calculateSimilarityBatch(
      testPairs,
      AlgorithmType.LEVENSHTEIN,
    );
    const syncEnd = process.hrtime.bigint();

    // Async benchmark
    const asyncStart = process.hrtime.bigint();
    const asyncResults = await calculateSimilarityBatchAsync(
      testPairs,
      AlgorithmType.LEVENSHTEIN,
    );
    const asyncEnd = process.hrtime.bigint();

    expect(syncResults).toHaveLength(testPairs.length);
    expect(asyncResults).toHaveLength(testPairs.length);

    const syncTime = Number(syncEnd - syncStart) / 1000000; // Convert to milliseconds
    const asyncTime = Number(asyncEnd - asyncStart) / 1000000; // Convert to milliseconds

    console.log(
      `Sync time: ${syncTime.toFixed(2)}ms, Async time: ${asyncTime.toFixed(2)}ms`,
    );

    // Both should complete successfully - exact timing comparison is unreliable for fast operations
    expect(syncTime).toBeGreaterThan(0);
    expect(asyncTime).toBeGreaterThan(0);

    // Async might have some overhead, but both should be reasonably fast (under 100ms)
    expect(syncTime).toBeLessThan(100);
    expect(asyncTime).toBeLessThan(100);
  });
});
