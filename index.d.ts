/**
 *  High-performance and memory efficient native C++ text similarity algorithms for Node.js 
 *
 * @version 1.0.0
 * @author Piotr Bednarski <piotr.maciej.bednarski@gmail.com>
 * @license MIT
 */

declare module 'text-similarity-node' {
  // ============================================================================
  // ENUMS AND CONSTANTS
  // ============================================================================

  /**
   * Available text similarity algorithms
   */
  export enum AlgorithmType {
    /** Levenshtein edit distance - most common string distance metric */
    LEVENSHTEIN = 0,
    /** Damerau-Levenshtein with transposition support */
    DAMERAU_LEVENSHTEIN = 1,
    /** Hamming distance for equal-length strings */
    HAMMING = 2,
    /** Jaro similarity for fuzzy string matching */
    JARO = 3,
    /** Jaro-Winkler with prefix weighting */
    JARO_WINKLER = 4,
    /** Jaccard similarity coefficient */
    JACCARD = 5,
    /** Sørensen-Dice coefficient */
    SORENSEN_DICE = 6,
    /** Overlap coefficient (Szymkiewicz-Simpson index) */
    OVERLAP = 7,
    /** Tversky index with configurable alpha/beta weights */
    TVERSKY = 8,
    /** Cosine similarity using vector space model */
    COSINE = 9,
    /** Euclidean distance in vector space */
    EUCLIDEAN = 10,
    /** Manhattan distance (L1 norm) */
    MANHATTAN = 11,
    /** Chebyshev distance (L∞ norm) */
    CHEBYSHEV = 12,
  }

  /**
   * Text preprocessing modes
   */
  export enum PreprocessingMode {
    /** No preprocessing applied */
    NONE = 0,
    /** Character-level comparison */
    CHARACTER = 1,
    /** Word-level tokenization */
    WORD = 2,
    /** N-gram based tokenization */
    NGRAM = 3,
  }

  /**
   * Case sensitivity options
   */
  export enum CaseSensitivity {
    /** Case-sensitive comparison */
    SENSITIVE = 0,
    /** Case-insensitive comparison with Unicode support */
    INSENSITIVE = 1,
  }

  // ============================================================================
  // CONFIGURATION INTERFACES
  // ============================================================================

  /**
   * Configuration options for similarity algorithms
   */
  export interface AlgorithmConfig {
    /** Algorithm to use for calculation */
    algorithm?: AlgorithmType;

    /** Text preprocessing mode */
    preprocessing?: PreprocessingMode;

    /** Case sensitivity setting */
    caseSensitivity?: CaseSensitivity;

    /** N-gram size for n-gram based algorithms (default: 2) */
    ngramSize?: number;

    /** Early termination threshold for distance algorithms */
    threshold?: number;

    /** Alpha parameter for Tversky index (weight for first set) */
    alpha?: number;

    /** Beta parameter for Tversky index (weight for second set) */
    beta?: number;

    /** Prefix weight for Jaro-Winkler (0.0-0.25, default: 0.1) */
    prefixWeight?: number;

    /** Maximum prefix length for Jaro-Winkler (default: 4) */
    prefixLength?: number;
  }

  /**
   * Result wrapper for operations that may fail
   */
  export interface SimilarityResult {
    /** Whether the operation succeeded */
    success: boolean;

    /** Similarity value (0-1) if successful */
    value?: number;

    /** Error information if failed */
    error?: Error;
  }

  /**
   * Result wrapper for distance operations
   */
  export interface DistanceResult {
    /** Whether the operation succeeded */
    success: boolean;

    /** Distance value if successful */
    value?: number;

    /** Error information if failed */
    error?: Error;
  }

  /**
   * Algorithm information
   */
  export interface AlgorithmInfo {
    /** Algorithm type enum value */
    type: AlgorithmType;

    /** Human-readable algorithm name */
    name: string;
  }

  // ============================================================================
  // MAIN LIBRARY FUNCTIONS
  // ============================================================================

  /**
   * Calculate similarity between two strings (synchronous)
   *
   * @param s1 First string to compare
   * @param s2 Second string to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Similarity result object
   *
   * @example
   * ```typescript
   * import { calculateSimilarity, AlgorithmType } from 'text-similarity-node';
   *
   * const result = calculateSimilarity('hello', 'hallo', AlgorithmType.LEVENSHTEIN);
   * if (result.success) {
   *   console.log(`Similarity: ${result.value}`); // 0.8
   * }
   * ```
   */
  export function calculateSimilarity(
    s1: string,
    s2: string,
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): SimilarityResult;

  /**
   * Calculate distance between two strings (synchronous)
   *
   * @param s1 First string to compare
   * @param s2 Second string to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Distance result object
   *
   * @example
   * ```typescript
   * import { calculateDistance, AlgorithmType } from 'text-similarity-node';
   *
   * const result = calculateDistance('kitten', 'sitting');
   * if (result.success) {
   *   console.log(`Distance: ${result.value}`); // 3
   * }
   * ```
   */
  export function calculateDistance(
    s1: string,
    s2: string,
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): DistanceResult;

  /**
   * Calculate similarity for multiple string pairs (synchronous batch)
   *
   * @param pairs Array of string pairs to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Array of similarity results
   *
   * @example
   * ```typescript
   * import { calculateSimilarityBatch } from 'text-similarity-node';
   *
   * const pairs = [
   *   ['hello', 'hallo'],
   *   ['world', 'word'],
   *   ['test', 'text']
   * ];
   *
   * const results = calculateSimilarityBatch(pairs);
   * results.forEach((result, index) => {
   *   if (result.success) {
   *     console.log(`Pair ${index}: ${result.value}`);
   *   }
   * });
   * ```
   */
  export function calculateSimilarityBatch(
    pairs: [string, string][],
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): SimilarityResult[];

  // ============================================================================
  // ASYNCHRONOUS (PROMISE-BASED) API
  // ============================================================================

  /**
   * Calculate similarity between two strings (asynchronous)
   *
   * @param s1 First string to compare
   * @param s2 Second string to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Promise resolving to similarity value
   *
   * @example
   * ```typescript
   * import { calculateSimilarityAsync } from 'text-similarity-node';
   *
   * const similarity = await calculateSimilarityAsync('hello', 'hallo');
   * console.log(`Similarity: ${similarity}`); // 0.8
   * ```
   */
  export function calculateSimilarityAsync(
    s1: string,
    s2: string,
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): Promise<number>;

  /**
   * Calculate distance between two strings (asynchronous)
   *
   * @param s1 First string to compare
   * @param s2 Second string to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Promise resolving to distance value
   *
   * @example
   * ```typescript
   * import { calculateDistanceAsync } from 'text-similarity-node';
   *
   * const distance = await calculateDistanceAsync('kitten', 'sitting');
   * console.log(`Distance: ${distance}`); // 3
   * ```
   */
  export function calculateDistanceAsync(
    s1: string,
    s2: string,
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): Promise<number>;

  /**
   * Calculate similarity for multiple string pairs (asynchronous batch)
   *
   * @param pairs Array of string pairs to compare
   * @param algorithm Algorithm to use (default: Levenshtein)
   * @param config Additional configuration options
   * @returns Promise resolving to array of similarity values
   *
   * @example
   * ```typescript
   * import { calculateSimilarityBatchAsync } from 'text-similarity-node';
   *
   * const pairs = [
   *   ['hello', 'hallo'],
   *   ['world', 'word'],
   *   ['test', 'text']
   * ];
   *
   * const results = await calculateSimilarityBatchAsync(pairs);
   * results.forEach((similarity, index) => {
   *   console.log(`Pair ${index}: ${similarity}`);
   * });
   * ```
   */
  export function calculateSimilarityBatchAsync(
    pairs: [string, string][],
    algorithm?: AlgorithmType | string,
    config?: AlgorithmConfig
  ): Promise<number[]>;

  // ============================================================================
  // CONFIGURATION AND UTILITY FUNCTIONS
  // ============================================================================

  /**
   * Set global configuration for all operations
   *
   * @param config Global configuration object
   *
   * @example
   * ```typescript
   * import { setGlobalConfiguration, CaseSensitivity } from 'text-similarity-node';
   *
   * setGlobalConfiguration({
   *   caseSensitivity: CaseSensitivity.INSENSITIVE,
   *   ngramSize: 3
   * });
   * ```
   */
  export function setGlobalConfiguration(config: AlgorithmConfig): void;

  /**
   * Get current global configuration
   *
   * @returns Current global configuration
   */
  export function getGlobalConfiguration(): AlgorithmConfig;

  /**
   * Get list of supported algorithms
   *
   * @returns Array of algorithm information objects
   */
  export function getSupportedAlgorithms(): AlgorithmInfo[];

  /**
   * Get current memory usage of the library
   *
   * @returns Memory usage in bytes
   */
  export function getMemoryUsage(): number;

  /**
   * Clear internal result caches
   */
  export function clearCaches(): void;

  /**
   * Parse algorithm name string to enum value
   *
   * @param name Algorithm name (case-insensitive)
   * @returns Algorithm type enum value or undefined
   *
   * @example
   * ```typescript
   * import { parseAlgorithmType } from 'text-similarity-node';
   *
   * const algorithm = parseAlgorithmType('levenshtein');
   * // Returns AlgorithmType.LEVENSHTEIN
   * ```
   */
  export function parseAlgorithmType(name: string): AlgorithmType | undefined;

  /**
   * Get human-readable name for algorithm type
   *
   * @param type Algorithm type enum value
   * @returns Algorithm name string
   */
  export function getAlgorithmName(type: AlgorithmType): string;

  // ============================================================================
  // CONVENIENCE NAMESPACE API
  // ============================================================================

  /**
   * Convenience namespace for similarity calculations
   */
  export namespace similarity {
    /**
     * Levenshtein similarity (normalized edit distance)
     */
    function levenshtein(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Damerau-Levenshtein similarity with transposition support
     */
    function damerauLevenshtein(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Hamming similarity for equal-length strings
     */
    function hamming(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Jaro similarity for fuzzy matching
     */
    function jaro(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Jaro-Winkler similarity with prefix weighting
     */
    function jaroWinkler(
      s1: string,
      s2: string,
      caseSensitive?: boolean,
      prefixWeight?: number
    ): number;

    /**
     * Jaccard similarity coefficient
     */
    function jaccard(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;

    /**
     * Sørensen-Dice coefficient
     */
    function dice(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;

    /**
     * Cosine similarity using vector space model
     */
    function cosine(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;

    /**
     * Tversky index with configurable weights
     */
    function tversky(
      s1: string,
      s2: string,
      alpha: number,
      beta: number,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;
  }

  /**
   * Convenience namespace for distance calculations
   */
  export namespace distance {
    /**
     * Levenshtein edit distance
     */
    function levenshtein(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Damerau-Levenshtein distance with transposition support
     */
    function damerauLevenshtein(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Hamming distance for equal-length strings
     */
    function hamming(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Jaro distance (1 - Jaro similarity)
     */
    function jaro(s1: string, s2: string, caseSensitive?: boolean): number;

    /**
     * Euclidean distance in vector space
     */
    function euclidean(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;

    /**
     * Manhattan distance (L1 norm)
     */
    function manhattan(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;

    /**
     * Chebyshev distance (L∞ norm)
     */
    function chebyshev(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): number;
  }

  /**
   * Convenience namespace for asynchronous operations
   */
  export namespace async {
    /**
     * All similarity functions from the similarity namespace, but returning Promises
     */
    function levenshtein(s1: string, s2: string, caseSensitive?: boolean): Promise<number>;
    function damerauLevenshtein(s1: string, s2: string, caseSensitive?: boolean): Promise<number>;
    function hamming(s1: string, s2: string, caseSensitive?: boolean): Promise<number>;
    function jaro(s1: string, s2: string, caseSensitive?: boolean): Promise<number>;
    function jaroWinkler(
      s1: string,
      s2: string,
      caseSensitive?: boolean,
      prefixWeight?: number
    ): Promise<number>;
    function jaccard(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): Promise<number>;
    function dice(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): Promise<number>;
    function cosine(
      s1: string,
      s2: string,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): Promise<number>;
    function tversky(
      s1: string,
      s2: string,
      alpha: number,
      beta: number,
      useWords?: boolean,
      caseSensitive?: boolean,
      ngramSize?: number
    ): Promise<number>;
  }

  // ============================================================================
  // ADVANCED FEATURES
  // ============================================================================

  /**
   * Advanced batch processing with progress callback
   *
   * @param pairs Array of string pairs to process
   * @param algorithm Algorithm to use
   * @param config Algorithm configuration
   * @param onProgress Progress callback function
   * @returns Promise resolving to array of results
   */
  export function calculateSimilarityBatchWithProgress(
    pairs: [string, string][],
    algorithm: AlgorithmType,
    config: AlgorithmConfig,
    onProgress: (completed: number, total: number) => void
  ): Promise<number[]>;

  /**
   * Parallel batch processing using worker threads
   *
   * @param pairs Array of string pairs to process
   * @param algorithm Algorithm to use
   * @param config Algorithm configuration
   * @param maxWorkers Maximum number of worker threads (default: CPU cores)
   * @returns Promise resolving to array of results
   */
  export function calculateSimilarityBatchParallel(
    pairs: [string, string][],
    algorithm?: AlgorithmType,
    config?: AlgorithmConfig,
    maxWorkers?: number
  ): Promise<number[]>;

  // ============================================================================
  // ERROR TYPES
  // ============================================================================

  /**
   * Custom error types for text similarity operations
   */
  export class TextSimilarityError extends Error {
    readonly code: string;

    constructor(message: string, code: string);
  }

  export class InvalidInputError extends TextSimilarityError {
    constructor(message: string);
  }

  export class InvalidConfigurationError extends TextSimilarityError {
    constructor(message: string);
  }

  export class MemoryAllocationError extends TextSimilarityError {
    constructor(message: string);
  }

  export class UnicodeConversionError extends TextSimilarityError {
    constructor(message: string);
  }

  export class ComputationOverflowError extends TextSimilarityError {
    constructor(message: string);
  }

  export class ThreadingError extends TextSimilarityError {
    constructor(message: string);
  }

  // ============================================================================
  // MODULE METADATA
  // ============================================================================

  /**
   * Library version
   */
  export const VERSION: string;

  /**
   * Build information
   */
  export const BUILD_INFO: {
    version: string;
    buildDate: string;
    compiler: string;
    platform: string;
    features: string[];
  };

  /**
   * Performance characteristics of algorithms
   */
  export const ALGORITHM_PERFORMANCE: {
    [K in keyof typeof AlgorithmType]: {
      timeComplexity: string;
      spaceComplexity: string;
      isSymmetric: boolean;
      isMetric: boolean;
      supportsEarlyTermination: boolean;
      recommendedFor: string[];
    };
  };
}
