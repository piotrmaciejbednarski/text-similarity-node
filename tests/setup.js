// Jest setup file
beforeAll(() => {
  // Ensure the native addon is built before running tests
  try {
    require("../index");
  } catch (error) {
    console.error("Failed to load native addon:", error.message);
    process.exit(1);
  }
});

// Custom matchers for better assertions
expect.extend({
  toBeWithinRange(received, min, max) {
    // Add small epsilon for floating point tolerance
    const epsilon = 1e-10;
    const pass = received >= min - epsilon && received <= max + epsilon;
    if (pass) {
      return {
        message: () =>
          `expected ${received} not to be within range ${min} - ${max}`,
        pass: true,
      };
    } else {
      return {
        message: () =>
          `expected ${received} to be within range ${min} - ${max}`,
        pass: false,
      };
    }
  },

  toBeCloseTo(received, expected, precision = 6) {
    const pass = Math.abs(received - expected) < 10 ** -precision;
    if (pass) {
      return {
        message: () => `expected ${received} not to be close to ${expected}`,
        pass: true,
      };
    } else {
      return {
        message: () =>
          `expected ${received} to be close to ${expected} (within ${10 ** -precision})`,
        pass: false,
      };
    }
  },
});
