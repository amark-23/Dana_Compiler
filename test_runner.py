import sys
import os
import subprocess
import difflib
from pathlib import Path

class Colors:
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[0;33m'
    NC = '\033[0m'


def run_tests(test_dir_path, compiler_path):
    """
    Runs all .dana files in a directory, comparing their output to .result files.
    """
    test_dir = Path(test_dir_path).resolve()
    compiler = Path(compiler_path).resolve()

    if not test_dir.is_dir():
        print(f"{Colors.RED}Error: Test directory '{test_dir}' not found.{Colors.NC}")
        return 1
    
    if not compiler.is_file():
        print(f"{Colors.RED}Error: Compiler '{compiler}' not found.{Colors.NC}")
        return 1

    pass_count = 0
    fail_count = 0
    warn_count = 0

    test_files = sorted(list(test_dir.glob('*.dana')))
    if not test_files:
        print(f"{Colors.YELLOW}No test files found in '{test_dir}'.{Colors.NC}")
        return 0

    for dana_file in test_files:
        print(f"\n--- Testing: {dana_file} ---")
        
        result_file = dana_file.with_suffix('.result')
        actual_file = dana_file.with_suffix('.dana.actual')
        
        try:
            with open(dana_file, 'r') as stdin_file:
                process = subprocess.run(
                    [str(compiler)],
                    stdin=stdin_file,
                    capture_output=True,
                    text=True,
                    timeout=5
                )

            actual_output = process.stdout + process.stderr

            with open(actual_file, 'w') as f:
                f.write(actual_output)

            if result_file.is_file():
                with open(result_file, 'r') as f:
                    expected_output = f.read()
                
                if actual_output == expected_output:
                    print(f"{Colors.GREEN}PASS{Colors.NC}")
                    pass_count += 1
                    os.remove(actual_file)
                else:
                    print(f"{Colors.RED}FAIL{Colors.NC}")
                    fail_count += 1
                    
                    diff = difflib.unified_diff(
                        expected_output.splitlines(keepends=True),
                        actual_output.splitlines(keepends=True),
                        fromfile=str(result_file),
                        tofile=str(actual_file),
                        lineterm=''
                    )
                    #print("--- Diff (Expected [-] vs Actual [+]) ---")
                    #for line in diff:
                    #    print(line, end='')
                    #print("\n----------------------------------------")
                    print(f"(Actual output saved in: {actual_file})")
            
            else:
                print(f"{Colors.YELLOW}WARNING:{Colors.NC} No result file found. Printing output:")
                print(actual_output.strip())
                warn_count += 1

        except Exception as e:
            print(f"{Colors.RED}ERROR during test run:{Colors.NC} {e}")
            fail_count += 1

    print("\n============================")
    print(f"{test_dir.name.upper()} Summary:")
    print(f"  {Colors.GREEN}PASS: {pass_count}{Colors.NC}")
    print(f"  {Colors.RED}FAIL: {fail_count}{Colors.NC}")
    print(f"  {Colors.YELLOW}WARN (No .result): {warn_count}{Colors.NC}")
    print("============================")
    
    return fail_count

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 test_runner.py <test_directory> <compiler_executable>")
        sys.exit(1)
        
    test_dir = sys.argv[1]
    compiler_path = sys.argv[2]
    
    failures = run_tests(test_dir, compiler_path)
    if failures > 0:
        sys.exit(1)