#!/usr/bin/env python3.11

"""
Testing of text-similarity-node against textdistance reference
"""

import json
import subprocess
import sys
import os
import time
from typing import Dict, List, Tuple, Any, Optional
from dataclasses import dataclass
from pathlib import Path


# Color definitions for terminal output
class Colors:
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[0;33m"
    BLUE = "\033[0;34m"
    MAGENTA = "\033[0;35m"
    CYAN = "\033[0;36m"
    WHITE = "\033[0;37m"
    BOLD = "\033[1m"
    NC = "\033[0m"  # No Color


@dataclass
class TestResult:
    test_case: Tuple[str, str]
    status: str
    node_value: Optional[float] = None
    textdist_value: Optional[float] = None
    error: Optional[str] = None
    difference: Optional[float] = None


@dataclass
class AlgorithmResult:
    algorithm: str
    category: str
    total: int
    passed: int
    failed: int
    results: List[TestResult]


class TestStatistics:
    def __init__(self):
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.skipped_tests = 0
        self.algorithm_results: List[AlgorithmResult] = []

    def add_result(self, result: AlgorithmResult):
        self.algorithm_results.append(result)
        self.total_tests += result.total
        self.passed_tests += result.passed
        self.failed_tests += result.failed

    def add_skip(self):
        self.skipped_tests += 1
        self.total_tests += 1


class AlgorithmVerifier:
    def __init__(self):
        self.stats = TestStatistics()
        self.script_dir = Path(__file__).parent
        self.root_dir = self.script_dir.parent
        self.temp_dir = self.script_dir / "tmp"
        self.log_file = self.temp_dir / "verification.log"

        # Test data
        self.test_cases = {
            "basic": [
                ("hello", "hello"),
                ("hello", "hallo"),
                ("kitten", "sitting"),
                ("martha", "marhta"),
                ("", ""),
                ("", "hello"),
                ("hello", ""),
            ],
            "unicode": [
                ("café", "cafe"),
                ("αβγ", "αβδ"),
                ("日本語", "日本"),
                ("привет", "превет"),
            ],
            "complex": [
                ("the quick brown fox", "the brown fox jumps"),
                ("DIXON", "DICKSONX"),
                ("algorithm", "logarithm"),
                ("machine learning", "deep learning"),
                ("abcdef", "uvwxyz"),
            ],
            "edge_cases": [
                ("a", "b"),
                ("aa", "aaa"),
                ("hello world", "world hello"),
                ("one two three", "three two one"),
            ],
        }

    def log(self, message: str):
        """Log message to file with timestamp"""
        if self.log_file.exists():
            with open(self.log_file, "a") as f:
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                f.write(f"{timestamp} - {message}\n")

    def print_header(self, text: str):
        """Print colored header"""
        print(f"{Colors.BOLD}{Colors.CYAN}{text}{Colors.NC}")
        print(f"{Colors.CYAN}{'=' * len(text)}{Colors.NC}")
        self.log(f"HEADER: {text}")

    def print_subheader(self, text: str):
        """Print colored subheader"""
        print(f"{Colors.BOLD}{Colors.BLUE}{text}{Colors.NC}")
        print(f"{Colors.BLUE}{'-' * len(text)}{Colors.NC}")
        self.log(f"SUBHEADER: {text}")

    def print_success(self, text: str):
        """Print success message"""
        print(f"{Colors.GREEN}[PASS]{Colors.NC} {text}")
        self.log(f"PASS: {text}")

    def print_failure(self, text: str):
        """Print failure message"""
        print(f"{Colors.RED}[FAIL]{Colors.NC} {text}")
        self.log(f"FAIL: {text}")

    def print_warning(self, text: str):
        """Print warning message"""
        print(f"{Colors.YELLOW}[WARN]{Colors.NC} {text}")
        self.log(f"WARN: {text}")

    def print_info(self, text: str):
        """Print info message"""
        print(f"{Colors.CYAN}[INFO]{Colors.NC} {text}")
        self.log(f"INFO: {text}")

    def print_skip(self, text: str):
        """Print skip message"""
        print(f"{Colors.YELLOW}[SKIP]{Colors.NC} {text}")
        self.log(f"SKIP: {text}")

    def setup_environment(self) -> bool:
        """Setup test environment"""
        self.print_header("Environment Setup")

        # Create temp directory
        self.temp_dir.mkdir(exist_ok=True)

        # Clear log file
        with open(self.log_file, "w") as f:
            f.write("")

        # Check Python version
        python_version = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
        self.print_info(f"Using Python {python_version}")

        # Check if we're in the right directory
        if not (self.root_dir / "package.json").exists():
            self.print_failure("Not in text-similarity-node project directory")
            return False

        # Check textdistance availability
        try:
            import textdistance

            self.print_success("textdistance library imported successfully")
        except ImportError:
            self.print_failure(
                "textdistance library not available. Install with: pip install textdistance"
            )
            return False

        # Check if Node.js project is built
        native_module = (
            self.root_dir / "build" / "Release" / "text_similarity_node.node"
        )
        if not native_module.exists():
            self.print_warning("Native module not found, attempting to build...")
            try:
                result = subprocess.run(
                    ["npm", "run", "build"],
                    cwd=self.root_dir,
                    capture_output=True,
                    text=True,
                )
                if result.returncode == 0:
                    self.print_success("Native module built successfully")
                else:
                    self.print_failure(
                        f"Failed to build native module: {result.stderr}"
                    )
                    return False
            except Exception as e:
                self.print_failure(f"Build failed: {e}")
                return False

        self.print_success("Environment setup complete")
        print()
        return True

    def run_node_test(
        self,
        str1: str,
        str2: str,
        algorithm: str,
        test_type: str = "distance",
        **options,
    ) -> Dict[str, Any]:
        """Run Node.js test for given string pair and algorithm"""

        # Prepare options string
        options_str = ""
        if options:
            opts = []
            for key, value in options.items():
                if isinstance(value, str) and value.startswith("PreprocessingMode."):
                    opts.append(f"{key}: lib.{value}")
                elif isinstance(value, str):
                    opts.append(f'{key}: "{value}"')
                else:
                    opts.append(f"{key}: {json.dumps(value)}")
            if opts:
                options_str = "{" + ", ".join(opts) + "}"
            else:
                options_str = "{}"
        else:
            options_str = "{}"
        # Create Node.js script
        if test_type == "distance":
            node_script = f'''
            const lib = require('{self.root_dir}/index.js');
            try {{
                const result = lib.calculateDistance("{str1}", "{str2}", lib.AlgorithmType.{algorithm});
                console.log(JSON.stringify(result));
            }} catch (error) {{
                console.log(JSON.stringify({{success: false, error: error.message}}));
            }}
            '''
        else:  # similarity
            node_script = f'''
            const lib = require('{self.root_dir}/index.js');
            try {{
                const options = {options_str};
                const result = lib.calculateSimilarity("{str1}", "{str2}", lib.AlgorithmType.{algorithm}, options);
                console.log(JSON.stringify(result));
            }} catch (error) {{
                console.log(JSON.stringify({{success: false, error: error.message}}));
            }}
            '''

        try:
            result = subprocess.run(
                ["node", "-e", node_script],
                cwd=self.root_dir,
                capture_output=True,
                text=True,
            )

            if result.returncode != 0:
                return {
                    "success": False,
                    "error": f"Node process failed: {result.stderr}",
                }

            return json.loads(result.stdout)

        except Exception as e:
            return {"success": False, "error": f"Execution failed: {str(e)}"}

    def test_algorithm_distance(
        self, textdist_name: str, node_algorithm: str, test_category: str
    ) -> AlgorithmResult:
        """Test distance-based algorithm"""
        import textdistance

        test_cases = self.test_cases[test_category]
        results = []

        for str1, str2 in test_cases:
            # Run Node.js test
            node_result = self.run_node_test(str1, str2, node_algorithm, "distance")

            if not node_result.get("success", False):
                results.append(
                    TestResult(
                        test_case=(str1, str2),
                        status="ERROR",
                        error=node_result.get("error", "Unknown error"),
                    )
                )
                continue

            node_value = node_result["value"]

            # Get textdistance result
            try:
                textdist_func = getattr(textdistance, textdist_name)
                textdist_value = textdist_func(str1, str2)
            except Exception as e:
                results.append(
                    TestResult(
                        test_case=(str1, str2),
                        status="ERROR",
                        error=f"textdistance error: {str(e)}",
                    )
                )
                continue

            # Compare results
            if node_value == textdist_value:
                status = "PASS"
            else:
                status = "FAIL"

            results.append(
                TestResult(
                    test_case=(str1, str2),
                    status=status,
                    node_value=node_value,
                    textdist_value=textdist_value,
                    difference=abs(node_value - textdist_value)
                    if isinstance(node_value, (int, float))
                    and isinstance(textdist_value, (int, float))
                    else None,
                )
            )

        # Calculate statistics
        passed = len([r for r in results if r.status == "PASS"])
        failed = len([r for r in results if r.status == "FAIL"])
        total = len(results)

        return AlgorithmResult(
            algorithm=textdist_name,
            category=test_category,
            total=total,
            passed=passed,
            failed=failed,
            results=results,
        )

    def test_algorithm_similarity(
        self,
        textdist_name: str,
        node_algorithm: str,
        test_category: str,
        use_ngrams: bool = False,
        **options,
    ) -> AlgorithmResult:
        """Test similarity-based algorithm"""
        import textdistance

        test_cases = self.test_cases[test_category]
        results = []

        for str1, str2 in test_cases:
            # Run Node.js test
            node_result = self.run_node_test(
                str1, str2, node_algorithm, "similarity", **options
            )

            if not node_result.get("success", False):
                results.append(
                    TestResult(
                        test_case=(str1, str2),
                        status="ERROR",
                        error=node_result.get("error", "Unknown error"),
                    )
                )
                continue

            node_value = node_result["value"]

            # Get textdistance result
            try:
                if use_ngrams:
                    # Use qval=2 for n-grams
                    textdist_class = getattr(textdistance, textdist_name.title())
                    textdist_func = textdist_class(qval=2)
                    textdist_value = textdist_func(str1, str2)
                else:
                    # Use default qval=1 for characters
                    textdist_func = getattr(textdistance, textdist_name)
                    textdist_value = textdist_func(str1, str2)
            except Exception as e:
                results.append(
                    TestResult(
                        test_case=(str1, str2),
                        status="ERROR",
                        error=f"textdistance error: {str(e)}",
                    )
                )
                continue

            # Compare results with tolerance for floating point
            tolerance = 1e-6
            if isinstance(node_value, (int, float)) and isinstance(
                textdist_value, (int, float)
            ):
                diff = abs(node_value - textdist_value)
                status = "PASS" if diff <= tolerance else "FAIL"
            else:
                status = "PASS" if node_value == textdist_value else "FAIL"

            results.append(
                TestResult(
                    test_case=(str1, str2),
                    status=status,
                    node_value=node_value,
                    textdist_value=textdist_value,
                    difference=abs(node_value - textdist_value)
                    if isinstance(node_value, (int, float))
                    and isinstance(textdist_value, (int, float))
                    else None,
                )
            )

        # Calculate statistics
        passed = len([r for r in results if r.status == "PASS"])
        failed = len([r for r in results if r.status == "FAIL"])
        total = len(results)

        return AlgorithmResult(
            algorithm=f"{textdist_name}",
            category=test_category,
            total=total,
            passed=passed,
            failed=failed,
            results=results,
        )

    def display_algorithm_result(self, result: AlgorithmResult):
        """Display results for an algorithm"""
        if result.failed == 0:
            self.print_success(
                f"{result.algorithm} ({result.category}): {result.passed}/{result.total} tests passed"
            )
        else:
            self.print_failure(
                f"{result.algorithm} ({result.category}): {result.passed}/{result.total} tests passed, {result.failed} failed"
            )

            # Show first few failed test details
            failed_results = [r for r in result.results if r.status == "FAIL"]
            for i, failed in enumerate(failed_results[:3]):
                print(
                    f"  {failed.test_case} - Node: {failed.node_value}, textdistance: {failed.textdist_value}"
                )

            if len(failed_results) > 3:
                print(f"  ... and {len(failed_results) - 3} more failures")

    def test_edit_based_algorithms(self):
        """Test edit-based distance algorithms"""
        self.print_subheader("Edit-Based Distance Algorithms")

        algorithms = [
            ("levenshtein", "LEVENSHTEIN"),
            ("damerau_levenshtein", "DAMERAU_LEVENSHTEIN"),
            ("hamming", "HAMMING"),
        ]

        categories = ["basic", "unicode", "complex"]

        for textdist_name, node_name in algorithms:
            for category in categories:
                # Skip hamming for different length strings
                if textdist_name == "hamming" and category == "complex":
                    self.print_skip(
                        f"hamming ({category}) - Different length strings not supported"
                    )
                    self.stats.add_skip()
                    continue

                result = self.test_algorithm_distance(
                    textdist_name, node_name, category
                )
                self.display_algorithm_result(result)
                self.stats.add_result(result)

        print()

    def test_phonetic_algorithms(self):
        """Test phonetic similarity algorithms"""
        self.print_subheader("Phonetic Similarity Algorithms")

        algorithms = [("jaro", "JARO"), ("jaro_winkler", "JARO_WINKLER")]

        categories = ["basic", "unicode", "complex", "edge_cases"]

        for textdist_name, node_name in algorithms:
            for category in categories:
                result = self.test_algorithm_similarity(
                    textdist_name,
                    node_name,
                    category,
                    use_ngrams=False,
                    preprocessing="PreprocessingMode.CHARACTER",
                )
                self.display_algorithm_result(result)
                self.stats.add_result(result)

        print()

    def test_token_based_algorithms(self):
        """Test token-based similarity algorithms"""
        self.print_subheader("Token-Based Similarity Algorithms")

        algorithms = [
            ("jaccard", "JACCARD"),
            ("sorensen", "SORENSEN_DICE"),
            ("overlap", "OVERLAP"),
            ("cosine", "COSINE"),
        ]

        categories = ["basic", "unicode", "complex"]

        # Test character-level
        self.print_info("Testing character-level tokenization...")
        for textdist_name, node_name in algorithms:
            for category in categories:
                result = self.test_algorithm_similarity(
                    textdist_name,
                    node_name,
                    category,
                    use_ngrams=False,
                    preprocessing="PreprocessingMode.CHARACTER",
                )
                result.algorithm = f"{textdist_name}-char"
                self.display_algorithm_result(result)
                self.stats.add_result(result)

        # Test n-gram level
        self.print_info("Testing n-gram tokenization...")
        for textdist_name, node_name in algorithms:
            for category in categories:
                result = self.test_algorithm_similarity(
                    textdist_name,
                    node_name,
                    category,
                    use_ngrams=True,
                    preprocessing="PreprocessingMode.NGRAM",
                    ngramSize=2,
                )
                result.algorithm = f"{textdist_name}-ngram"
                self.display_algorithm_result(result)
                self.stats.add_result(result)

        print()

    def test_tversky_algorithm(self):
        """Test Tversky algorithm with parameter validation"""
        self.print_subheader("Tversky Algorithm (Parameter Validation)")

        import textdistance

        test_cases = [("hello", "hallo"), ("kitten", "sitting")]

        param_combinations = [
            (1.0, 1.0),  # Should equal Jaccard
            (0.5, 0.5),  # Balanced
            (1.0, 0.0),  # Focus on first string
            (0.0, 1.0),  # Focus on second string
        ]

        results = []

        for str1, str2 in test_cases:
            for alpha, beta in param_combinations:
                # Node.js result
                node_result = self.run_node_test(
                    str1,
                    str2,
                    "TVERSKY",
                    "similarity",
                    preprocessing="PreprocessingMode.CHARACTER",
                    alpha=alpha,
                    beta=beta,
                )

                if not node_result.get("success", False):
                    results.append(
                        TestResult(
                            test_case=(f"{str1}, {str2}, α={alpha}, β={beta}",),
                            status="ERROR",
                            error=node_result.get("error", "Unknown error"),
                        )
                    )
                    continue

                node_value = node_result["value"]

                # textdistance result
                try:
                    tversky_instance = textdistance.Tversky(ks=[alpha, beta])
                    textdist_value = tversky_instance(str1, str2)
                except Exception as e:
                    results.append(
                        TestResult(
                            test_case=(f"{str1}, {str2}, α={alpha}, β={beta}",),
                            status="ERROR",
                            error=f"textdistance error: {str(e)}",
                        )
                    )
                    continue

                # Compare results
                tolerance = 1e-6
                diff = abs(node_value - textdist_value)
                status = "PASS" if diff <= tolerance else "FAIL"

                results.append(
                    TestResult(
                        test_case=(f"{str1}, {str2}, α={alpha}, β={beta}",),
                        status=status,
                        node_value=node_value,
                        textdist_value=textdist_value,
                        difference=diff,
                    )
                )

        # Create algorithm result
        passed = len([r for r in results if r.status == "PASS"])
        failed = len([r for r in results if r.status == "FAIL"])
        total = len(results)

        tversky_result = AlgorithmResult(
            algorithm="tversky",
            category="parameters",
            total=total,
            passed=passed,
            failed=failed,
            results=results,
        )

        self.display_algorithm_result(tversky_result)
        self.stats.add_result(tversky_result)
        print()

    def run_performance_tests(self):
        """Run performance validation tests"""
        self.print_subheader("Performance Validation")

        # Create performance test script
        performance_script = f"""
const lib = require('{self.root_dir}/index.js');

function generateRandomString(length) {{
    const chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    let result = '';
    for (let i = 0; i < length; i++) {{
        result += chars.charAt(Math.floor(Math.random() * chars.length));
    }}
    return result;
}}

function runPerformanceTest() {{
    const pairs = [];
    for (let i = 0; i < 100; i++) {{
        pairs.push([
            generateRandomString(50),
            generateRandomString(50)
        ]);
    }}
    
    const algorithms = ['LEVENSHTEIN', 'JARO', 'JACCARD', 'COSINE'];
    const results = [];
    
    algorithms.forEach(algo => {{
        const startTime = process.hrtime.bigint();
        
        pairs.forEach(([str1, str2]) => {{
            lib.calculateSimilarity(str1, str2, lib.AlgorithmType[algo]);
        }});
        
        const endTime = process.hrtime.bigint();
        const timeMs = Number(endTime - startTime) / 1000000;
        const opsPerSec = Math.round(pairs.length / (timeMs / 1000));
        
        results.push({{
            algorithm: algo,
            timeMs: timeMs.toFixed(2),
            opsPerSec: opsPerSec
        }});
    }});
    
    console.log(JSON.stringify(results));
}}

runPerformanceTest();
"""

        try:
            result = subprocess.run(
                ["node", "-e", performance_script],
                cwd=self.root_dir,
                capture_output=True,
                text=True,
            )

            if result.returncode == 0:
                perf_results = json.loads(result.stdout)
                self.print_success("Performance tests completed")

                for perf in perf_results:
                    print(
                        f"  {perf['algorithm']:<20} {perf['timeMs']:>8} ms  {perf['opsPerSec']:>10} ops/sec"
                    )

                # Add to statistics
                self.stats.total_tests += 1
                self.stats.passed_tests += 1
            else:
                self.print_failure("Performance tests failed")
                self.stats.total_tests += 1
                self.stats.failed_tests += 1

        except Exception as e:
            self.print_failure(f"Performance test execution failed: {e}")
            self.stats.total_tests += 1
            self.stats.failed_tests += 1

        print()

    def generate_report(self):
        """Generate final test report"""
        self.print_header("Test Report Summary")

        success_rate = (
            (self.stats.passed_tests * 100) // self.stats.total_tests
            if self.stats.total_tests > 0
            else 0
        )

        print(f"{'Total Tests:':<20} {self.stats.total_tests}")
        print(f"{'Passed:':<20} {Colors.GREEN}{self.stats.passed_tests}{Colors.NC}")
        print(f"{'Failed:':<20} {Colors.RED}{self.stats.failed_tests}{Colors.NC}")
        print(f"{'Skipped:':<20} {Colors.YELLOW}{self.stats.skipped_tests}{Colors.NC}")
        print(f"{'Success Rate:':<20} {success_rate}%")

        print()

        if self.stats.failed_tests == 0:
            self.print_success(
                "All algorithm implementations are verified against textdistance reference"
            )
            print(f"{Colors.BOLD}{Colors.GREEN}VERIFICATION PASSED{Colors.NC}")
        else:
            self.print_warning(
                "Some algorithms show differences from textdistance reference"
            )
            print(
                f"{Colors.BOLD}{Colors.YELLOW}VERIFICATION COMPLETED WITH WARNINGS{Colors.NC}"
            )

        print()
        self.print_info(f"Detailed log available at: {self.log_file}")
        self.print_info(f"Temporary files available at: {self.temp_dir}")

    def run_all_tests(self):
        """Run all verification tests"""
        self.print_header("Text Similarity Algorithm Verification")
        self.print_info(
            "Comparing text-similarity-node with textdistance reference library"
        )
        self.print_info(f"Start time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        print()

        # Setup environment
        if not self.setup_environment():
            return False

        # Run all test suites
        self.test_edit_based_algorithms()
        self.test_phonetic_algorithms()
        self.test_token_based_algorithms()
        self.test_tversky_algorithm()

        # Performance validation
        self.run_performance_tests()

        # Generate final report
        self.generate_report()

        return self.stats.failed_tests == 0


def main():
    """Main execution function"""
    verifier = AlgorithmVerifier()
    success = verifier.run_all_tests()

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
