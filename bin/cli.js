#!/usr/bin/env node

/**
 * text-similarity-node CLI
 * Compare text strings using high-performance similarity algorithms directly from your terminal.
 *
 * @version 1.2.0
 * @license MIT
 */

const { Command } = require('commander');
const path = require('node:path');
const fs = require('node:fs');

const lib = require(path.resolve(__dirname, '..', 'index.js'));

const ALGORITHM_CHOICES = [
  'levenshtein',
  'damerau-levenshtein',
  'hamming',
  'jaro',
  'jaro-winkler',
  'jaccard',
  'sorensen-dice',
  'overlap',
  'tversky',
  'cosine',
  'euclidean',
  'manhattan',
  'chebyshev',
];

const PREPROCESSING_CHOICES = ['none', 'character', 'word', 'ngram'];

/**
 * Map a CLI algorithm name to the AlgorithmType enum value.
 */
function resolveAlgorithm(name) {
  const map = {
    levenshtein: lib.AlgorithmType.LEVENSHTEIN,
    'damerau-levenshtein': lib.AlgorithmType.DAMERAU_LEVENSHTEIN,
    hamming: lib.AlgorithmType.HAMMING,
    jaro: lib.AlgorithmType.JARO,
    'jaro-winkler': lib.AlgorithmType.JARO_WINKLER,
    jaccard: lib.AlgorithmType.JACCARD,
    'sorensen-dice': lib.AlgorithmType.SORENSEN_DICE,
    overlap: lib.AlgorithmType.OVERLAP,
    tversky: lib.AlgorithmType.TVERSKY,
    cosine: lib.AlgorithmType.COSINE,
    euclidean: lib.AlgorithmType.EUCLIDEAN,
    manhattan: lib.AlgorithmType.MANHATTAN,
    chebyshev: lib.AlgorithmType.CHEBYSHEV,
  };
  return map[name];
}

/**
 * Map a CLI preprocessing name to the PreprocessingMode enum value.
 */
function resolvePreprocessing(name) {
  const map = {
    none: lib.PreprocessingMode.NONE,
    character: lib.PreprocessingMode.CHARACTER,
    word: lib.PreprocessingMode.WORD,
    ngram: lib.PreprocessingMode.NGRAM,
  };
  return map[name];
}

/**
 * Build the options object from CLI flags.
 */
function buildOptions(opts) {
  const config = {};

  if (opts.preprocessing) {
    config.preprocessing = resolvePreprocessing(opts.preprocessing);
  }

  config.caseSensitivity = opts.ignoreCase
    ? lib.CaseSensitivity.INSENSITIVE
    : lib.CaseSensitivity.SENSITIVE;

  if (opts.ngramSize != null) {
    config.ngramSize = Number(opts.ngramSize);
  }

  if (opts.threshold != null) {
    config.threshold = Number(opts.threshold);
  }

  if (opts.alpha != null) {
    config.alpha = Number(opts.alpha);
  }

  if (opts.beta != null) {
    config.beta = Number(opts.beta);
  }

  if (opts.prefixWeight != null) {
    config.prefixWeight = Number(opts.prefixWeight);
  }

  return config;
}

/**
 * Format output based on the chosen format.
 */
function formatOutput(data, format) {
  if (format === 'json') {
    console.log(JSON.stringify(data, null, 2));
  } else {
    if (data.value !== undefined) {
      console.log(data.value);
    } else {
      console.log(data);
    }
  }
}

// ─── Program ──────────────────────────────────────────────────────────

const program = new Command();

program
  .name('text-similarity')
  .description(
    'High-performance text similarity algorithms for the terminal.\nCompare strings using Levenshtein, Jaro-Winkler, Cosine, and more.',
  )
  .version(lib.VERSION, '-v, --version');

// ─── similarity command ───────────────────────────────────────────────

program
  .command('similarity')
  .description('Calculate the similarity score (0–1) between two strings')
  .argument('<string1>', 'first string to compare')
  .argument('<string2>', 'second string to compare')
  .option(
    '-a, --algorithm <name>',
    `algorithm to use (${ALGORITHM_CHOICES.join(', ')})`,
    'levenshtein',
  )
  .option('-p, --preprocessing <mode>', `preprocessing mode (${PREPROCESSING_CHOICES.join(', ')})`)
  .option('-i, --ignore-case', 'perform case-insensitive comparison')
  .option('-n, --ngram-size <size>', 'n-gram size for n-gram based algorithms', '2')
  .option('--threshold <value>', 'early termination threshold')
  .option('--alpha <value>', 'alpha weight for Tversky index')
  .option('--beta <value>', 'beta weight for Tversky index')
  .option('--prefix-weight <value>', 'prefix weight for Jaro-Winkler (0.0–0.25)')
  .option('-f, --format <type>', 'output format: plain, json', 'plain')
  .action((string1, string2, opts) => {
    const algorithmType = resolveAlgorithm(opts.algorithm);
    if (algorithmType === undefined) {
      console.error(`Error: Unknown algorithm "${opts.algorithm}".`);
      console.error(`Available algorithms: ${ALGORITHM_CHOICES.join(', ')}`);
      process.exit(1);
    }

    const config = buildOptions(opts);
    const result = lib.calculateSimilarity(string1, string2, algorithmType, config);

    if (!result.success) {
      console.error(`Error: ${result.error || 'Calculation failed.'}`);
      process.exit(1);
    }

    formatOutput(result, opts.format);
  });

// ─── distance command ─────────────────────────────────────────────────

program
  .command('distance')
  .description('Calculate the distance between two strings')
  .argument('<string1>', 'first string to compare')
  .argument('<string2>', 'second string to compare')
  .option(
    '-a, --algorithm <name>',
    `algorithm to use (${ALGORITHM_CHOICES.join(', ')})`,
    'levenshtein',
  )
  .option('-p, --preprocessing <mode>', `preprocessing mode (${PREPROCESSING_CHOICES.join(', ')})`)
  .option('-i, --ignore-case', 'perform case-insensitive comparison')
  .option('-n, --ngram-size <size>', 'n-gram size for n-gram based algorithms', '2')
  .option('--threshold <value>', 'early termination threshold')
  .option('--alpha <value>', 'alpha weight for Tversky index')
  .option('--beta <value>', 'beta weight for Tversky index')
  .option('--prefix-weight <value>', 'prefix weight for Jaro-Winkler (0.0–0.25)')
  .option('-f, --format <type>', 'output format: plain, json', 'plain')
  .action((string1, string2, opts) => {
    const algorithmType = resolveAlgorithm(opts.algorithm);
    if (algorithmType === undefined) {
      console.error(`Error: Unknown algorithm "${opts.algorithm}".`);
      console.error(`Available algorithms: ${ALGORITHM_CHOICES.join(', ')}`);
      process.exit(1);
    }

    const config = buildOptions(opts);
    const result = lib.calculateDistance(string1, string2, algorithmType, config);

    if (!result.success) {
      console.error(`Error: ${result.error || 'Calculation failed.'}`);
      process.exit(1);
    }

    formatOutput(result, opts.format);
  });

// ─── batch command ────────────────────────────────────────────────────

program
  .command('batch')
  .description('Process multiple string pairs from a JSON file')
  .argument('<file>', 'path to JSON file with array of [string1, string2] pairs')
  .option(
    '-a, --algorithm <name>',
    `algorithm to use (${ALGORITHM_CHOICES.join(', ')})`,
    'levenshtein',
  )
  .option('-m, --mode <type>', 'calculation mode: similarity, distance', 'similarity')
  .option('-p, --preprocessing <mode>', `preprocessing mode (${PREPROCESSING_CHOICES.join(', ')})`)
  .option('-i, --ignore-case', 'perform case-insensitive comparison')
  .option('-n, --ngram-size <size>', 'n-gram size for n-gram based algorithms', '2')
  .option('--alpha <value>', 'alpha weight for Tversky index')
  .option('--beta <value>', 'beta weight for Tversky index')
  .option('--prefix-weight <value>', 'prefix weight for Jaro-Winkler (0.0–0.25)')
  .option('-f, --format <type>', 'output format: plain, json', 'plain')
  .action((file, opts) => {
    const algorithmType = resolveAlgorithm(opts.algorithm);
    if (algorithmType === undefined) {
      console.error(`Error: Unknown algorithm "${opts.algorithm}".`);
      process.exit(1);
    }

    let pairs;
    try {
      const raw = fs.readFileSync(path.resolve(file), 'utf-8');
      pairs = JSON.parse(raw);
    } catch (err) {
      console.error(`Error reading file: ${err.message}`);
      process.exit(1);
    }

    if (!Array.isArray(pairs) || !pairs.every((p) => Array.isArray(p) && p.length === 2)) {
      console.error('Error: File must contain a JSON array of [string1, string2] pairs.');
      process.exit(1);
    }

    const config = buildOptions(opts);
    const results = lib.calculateSimilarityBatch(pairs, algorithmType, config);

    if (opts.format === 'json') {
      const output = pairs.map((pair, i) => ({
        string1: pair[0],
        string2: pair[1],
        algorithm: opts.algorithm,
        [opts.mode]: results[i].success ? results[i].value : null,
      }));
      console.log(JSON.stringify(output, null, 2));
    } else {
      for (let i = 0; i < pairs.length; i++) {
        const val = results[i].success ? results[i].value : 'error';
        console.log(`"${pairs[i][0]}" <-> "${pairs[i][1]}"  =>  ${val}`);
      }
    }
  });

// ─── algorithms command ───────────────────────────────────────────────

program
  .command('algorithms')
  .description('List all available algorithms with descriptions')
  .option('-f, --format <type>', 'output format: plain, json', 'plain')
  .action((opts) => {
    const algorithms = [
      {
        name: 'levenshtein',
        description: 'Classic edit distance – insertions, deletions, substitutions',
        type: 'edit-based',
      },
      {
        name: 'damerau-levenshtein',
        description: 'Edit distance with transposition support',
        type: 'edit-based',
      },
      {
        name: 'hamming',
        description: 'Distance for equal-length strings',
        type: 'edit-based',
      },
      { name: 'jaro', description: 'Fuzzy matching optimized for short strings', type: 'edit-based' },
      {
        name: 'jaro-winkler',
        description: 'Jaro with prefix weighting bonus',
        type: 'edit-based',
      },
      {
        name: 'jaccard',
        description: 'Set intersection over union',
        type: 'token-based',
      },
      {
        name: 'sorensen-dice',
        description: 'Harmonic mean of precision and recall',
        type: 'token-based',
      },
      {
        name: 'overlap',
        description: 'Overlap coefficient (Szymkiewicz-Simpson)',
        type: 'token-based',
      },
      {
        name: 'tversky',
        description: 'Configurable alpha/beta asymmetric similarity',
        type: 'token-based',
      },
      {
        name: 'cosine',
        description: 'Angular distance in vector space',
        type: 'vector-based',
      },
      {
        name: 'euclidean',
        description: 'Euclidean (L2) distance in vector space',
        type: 'vector-based',
      },
      {
        name: 'manhattan',
        description: 'Manhattan (L1) distance in vector space',
        type: 'vector-based',
      },
      {
        name: 'chebyshev',
        description: 'Chebyshev (L∞) distance in vector space',
        type: 'vector-based',
      },
    ];

    if (opts.format === 'json') {
      console.log(JSON.stringify(algorithms, null, 2));
      return;
    }

    const maxName = Math.max(...algorithms.map((a) => a.name.length));

    let currentType = '';
    for (const algo of algorithms) {
      if (algo.type !== currentType) {
        currentType = algo.type;
        console.log(`\n  ${currentType.toUpperCase()}`);
      }
      console.log(`    ${algo.name.padEnd(maxName + 2)} ${algo.description}`);
    }
    console.log();
  });

program.parse();
