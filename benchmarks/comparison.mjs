import os from "node:os";
import fastLevenshtein from "fast-levenshtein";
import fastestLevenshtein from "fastest-levenshtein";
import jsLevenshtein from "js-levenshtein";
import natural from "natural";
import similarity from "similarity";
import stringComparison from "string-comparison";
import stringSimilarity from "string-similarity";
import talisman from "talisman/metrics/levenshtein.js";
import { Bench } from "tinybench";
import textSimilarity from "../index.js";

class MemoryMonitor {
  formatBytes(bytes) {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(Math.abs(bytes)) / Math.log(k));
    return `${parseFloat((bytes / k ** i).toFixed(2))} ${sizes[i]}`;
  }

  measureMemoryUsage(fn) {
    if (global.gc) global.gc();
    const startMem = process.memoryUsage();
    fn(); // Run once for memory allocation
    if (global.gc) global.gc();
    const endMem = process.memoryUsage();
    return {
      heapDelta: endMem.heapUsed - startMem.heapUsed,
      rssDelta: endMem.rss - startMem.rss,
    };
  }
}

const monitor = new MemoryMonitor();

const testData = {
  veryShort: { s1: "a", s2: "b" },
  short: { s1: "hello", s2: "hallo" },
  medium: { s1: "information retrieval", s2: "information extraction" },
  long: {
    s1: "The quick brown fox jumps over the lazy dog and runs through the forest",
    s2: "The quick brown fox jumped over the lazy dog and ran through the woods",
  },
  veryLong: {
    s1: "Lorem ipsum ".repeat(100).trim(),
    s2: "Lorem ipsum ".repeat(98).trim(),
  },
};

function normalizeResult(result) {
  if (typeof result === "number") return result.toFixed(4);
  return String(result);
}

async function runBenchmark(groupName, libraries, timeMs = 1000) {
  console.log(`\n${groupName} Performance`);
  console.log("=".repeat(groupName.length + 12));

  const bench = new Bench({ time: timeMs });

  for (const [name, fn] of libraries) {
    bench.add(name, fn);
  }

  // Warm-up run
  for (const [, fn] of libraries) {
    try {
      fn();
    } catch (_err) {
      // Ignore warmup errors
    }
  }

  await bench.run();

  console.table(bench.table());

  // Memory usage measurement
  const memoryResults = [];
  for (const [name, fn] of libraries) {
    try {
      const mem = monitor.measureMemoryUsage(fn);
      const result = normalizeResult(fn());
      memoryResults.push({
        Library: name,
        "Heap Delta": monitor.formatBytes(mem.heapDelta),
        "RSS Delta": monitor.formatBytes(mem.rssDelta),
        Result: result,
      });
    } catch (error) {
      memoryResults.push({
        Library: name,
        "Heap Delta": "ERROR",
        "RSS Delta": "ERROR",
        Result: error.message,
      });
    }
  }

  console.log("\nMemory Usage:");
  console.table(memoryResults);

  // Winner logic (based on ops/sec or avgTime)
  if (bench.tasks.length > 0) {
    const results = bench.tasks.map((task) => ({
      name: task.name,
      opsPerSec: task.result?.hz || task.result?.opsPerSec,
      avgTime: task.result?.mean ? task.result.mean * 1000 : Infinity, // ms
    }));

    const sortedByOps = [...results].sort(
      (a, b) => (b.opsPerSec || 0) - (a.opsPerSec || 0),
    );
    const sortedByTime = [...results].sort((a, b) => a.avgTime - b.avgTime);

    if (sortedByOps[0]?.opsPerSec) {
      console.log(
        `Winner (ops/sec): ${sortedByOps[0].name} with ${sortedByOps[0].opsPerSec.toLocaleString()} ops/s`,
      );
    } else if (sortedByTime[0] && sortedByTime[0].avgTime !== Infinity) {
      console.log(
        `Winner (latency): ${sortedByTime[0].name} with ${sortedByTime[0].avgTime.toFixed(4)} ms`,
      );
    } else {
      console.log(
        "Winner: Could not determine due to missing benchmark results",
      );
    }
  }
}

// Individual benchmarks
async function benchmarkLevenshtein() {
  const { s1, s2 } = testData.medium;

  const libraries = [
    [
      "text-similarity-node",
      () => textSimilarity.similarity.levenshtein(s1, s2),
    ],
    [
      "string-comparison",
      () => stringComparison.levenshtein.similarity(s1, s2),
    ],
    ["similarity", () => similarity(s1, s2)],
    [
      "natural",
      () => {
        const distance = natural.LevenshteinDistance(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "fast-levenshtein",
      () => {
        const distance = fastLevenshtein.get(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "js-levenshtein",
      () => {
        const distance = jsLevenshtein(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "fastest-levenshtein",
      () => {
        const distance = fastestLevenshtein.distance(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "talisman",
      () => {
        const distance = talisman(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
  ];

  await runBenchmark("Levenshtein Distance", libraries);
}

async function benchmarkJaroWinkler() {
  const { s1, s2 } = testData.medium;

  const libraries = [
    [
      "text-similarity-node",
      () => textSimilarity.similarity.jaroWinkler(s1, s2),
    ],
    [
      "string-comparison",
      () => stringComparison.jaroWinkler.similarity(s1, s2),
    ],
    ["natural", () => natural.JaroWinklerDistance(s1, s2)],
  ];

  await runBenchmark("Jaro-Winkler Distance", libraries);
}

async function benchmarkJaccard() {
  const { s1, s2 } = testData.medium;

  const libraries = [
    [
      "text-similarity-node (char)",
      () => textSimilarity.similarity.jaccard(s1, s2, false),
    ],
    [
      "text-similarity-node (word)",
      () => textSimilarity.similarity.jaccard(s1, s2, true),
    ],
    [
      "string-comparison",
      () => stringComparison.jaccardIndex.similarity(s1, s2),
    ],
  ];

  await runBenchmark("Jaccard Similarity", libraries);
}

async function benchmarkCosine() {
  const { s1, s2 } = testData.medium;

  const libraries = [
    [
      "text-similarity-node (char)",
      () => textSimilarity.similarity.cosine(s1, s2, false),
    ],
    [
      "text-similarity-node (word)",
      () => textSimilarity.similarity.cosine(s1, s2, true),
    ],
    ["string-comparison", () => stringComparison.cosine.similarity(s1, s2)],
  ];

  await runBenchmark("Cosine Similarity", libraries);
}

async function benchmarkDice() {
  const { s1, s2 } = testData.medium;

  const libraries = [
    [
      "text-similarity-node (char)",
      () => textSimilarity.similarity.dice(s1, s2, false),
    ],
    [
      "text-similarity-node (word)",
      () => textSimilarity.similarity.dice(s1, s2, true),
    ],
    ["string-similarity", () => stringSimilarity.compareTwoStrings(s1, s2)],
    [
      "string-comparison",
      () => stringComparison.diceCoefficient.similarity(s1, s2),
    ],
  ];

  await runBenchmark("Dice Coefficient", libraries);
}

async function benchmarkStringLengths() {
  console.log("\nString Length Performance Impact");
  console.log("===============================");

  for (const [lengthName, data] of Object.entries(testData)) {
    const { s1, s2 } = data;
    console.log(
      `\n${lengthName.toUpperCase()} strings (${s1.length}/${s2.length} chars):`,
    );

    const time = lengthName === "veryLong" ? 500 : 1000; // Less time for very long strings

    const libraries = [
      [
        "text-similarity-node",
        () => textSimilarity.similarity.levenshtein(s1, s2),
      ],
      ["similarity", () => similarity(s1, s2)],
    ];

    await runBenchmark(`Length Impact: ${lengthName}`, libraries, data, time);
  }
}

async function overallMemoryComparison() {
  const { s1, s2 } = testData.long;

  const libraries = [
    [
      "text-similarity-node",
      () => textSimilarity.similarity.levenshtein(s1, s2),
    ],
    [
      "string-comparison",
      () => stringComparison.levenshtein.similarity(s1, s2),
    ],
    ["similarity", () => similarity(s1, s2)],
    [
      "natural",
      () => {
        const distance = natural.LevenshteinDistance(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "fast-levenshtein",
      () => {
        const distance = fastLevenshtein.get(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "js-levenshtein",
      () => {
        const distance = jsLevenshtein(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "fastest-levenshtein",
      () => {
        const distance = fastestLevenshtein.distance(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
    [
      "talisman",
      () => {
        const distance = talisman(s1, s2);
        return 1 - distance / Math.max(s1.length, s2.length);
      },
    ],
  ];

  await runBenchmark("Overall Memory Usage Comparison", libraries);
}

function printSystemInfo() {
  console.log("String Similarity Library Comparison Benchmark");
  console.log("==============================================");
  console.log(`Node.js version: ${process.version}`);
  console.log(`Platform: ${process.platform} ${process.arch}`);
  console.log(`CPU cores: ${os.cpus().length}`);
  console.log(`Total memory: ${monitor.formatBytes(os.totalmem())}`);

  if (global.gc) {
    console.log("Garbage collection: ENABLED");
  } else {
    console.log(
      "Garbage collection: DISABLED (run with --expose-gc for better accuracy)",
    );
  }

  const memUsage = process.memoryUsage();
  console.log("\nInitial Process Memory Usage:");
  console.log(`  RSS: ${monitor.formatBytes(memUsage.rss)}`);
  console.log(`  Heap Used: ${monitor.formatBytes(memUsage.heapUsed)}`);
}

async function main() {
  printSystemInfo();

  try {
    await benchmarkLevenshtein();
    await benchmarkJaroWinkler();
    await benchmarkJaccard();
    await benchmarkCosine();
    await benchmarkDice();
    await benchmarkStringLengths();
    await overallMemoryComparison();

    console.log("\n\nBenchmark completed successfully.");
  } catch (error) {
    console.error("Benchmark failed:", error);
    process.exit(1);
  }
}

// Start execution
main().catch((err) => {
  console.error("Benchmark failed with error:", err);
  process.exit(1);
});
