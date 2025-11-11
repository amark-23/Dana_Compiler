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

# Directory to store all test outputs
OUTPUT_DIR = Path("test_results")


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

    # --- NEW ---
    # Create a subdirectory within OUTPUT_DIR for this specific test suite (e.g., "test_results/programs")
    suite_output_dir = OUTPUT_DIR / test_dir.name
    try:
        suite_output_dir.mkdir(parents=True, exist_ok=True)
        print(f"Saving test outputs to: {suite_output_dir.resolve()}")
    except OSError as e:
        print(f"{Colors.RED}Error: Could not create output directory '{suite_output_dir}'. {e}{Colors.NC}")
        return 1
    # --- END NEW ---

    for dana_file in test_files:
        print(f"\n--- Testing: {dana_file.name} ---")
        
        result_file = dana_file.with_suffix('.result')
        
        # --- MODIFIED ---
        # Save .actual files in the new output directory, not next to the test
        actual_file = suite_output_dir / dana_file.with_suffix('.dana.actual').name
        # --- END MODIFIED ---
        
        input_file = dana_file.with_suffix('.input') # <-- Check for .input file
        
        try:
            # --- THIS IS THE MODIFIED LOGIC ---

            # 1. Prepare the command with the .dana file as an argument
            command = [str(compiler), str(dana_file)]
            
            # 2. Check if an .input file exists to use for stdin
            stdin_data = None
            if input_file.is_file():
                with open(input_file, 'r') as f:
                    stdin_data = f.read()

            # 3. Run the compiler
            #    - 'command' now has the filename: ['./dana', '.../test.dana']
            #    - 'input' (for stdin) is now stdin_data (or None if no .input file)
            process = subprocess.run(
                command,
                input=stdin_data,       # <-- Pass .input content to stdin
                capture_output=True,
                text=True,
                timeout=30
            )

            # --- END OF MODIFIED LOGIC ---

            actual_output = process.stdout + process.stderr

            with open(actual_file, 'w') as f:
                f.write(actual_output)

            if result_file.is_file():
                with open(result_file, 'r') as f:
                    expected_output = f.read()
                
                if actual_output == expected_output:
                    print(f"{Colors.GREEN}PASS{Colors.NC}")
                    pass_count += 1
                    os.remove(actual_file) # Clean up successful .actual files
                else:
                    print(f"{Colors.RED}FAIL{Colors.NC}")
                    fail_count += 1
                    
                    diff = difflib.unified_diff(
                        expected_output.splitlines(keepends=True),
                        actual_output.splitlines(keepends=True),
                        fromfile=str(result_file.name),
                        tofile=str(actual_file.name),
                        lineterm=''
                    )
                    # Uncomment these lines if you want to see the diff in the console
                    print("--- Diff (Expected [-] vs Actual [+]) ---")
                    for line in diff:
                        print(line, end='')
                    print("\n----------------------------------------")
                    print(f"(Actual output saved in: {actual_file})")
            
            else:
                print(f"{Colors.YELLOW}WARNING:{Colors.NC} No result file found. Printing output:")
                print("----------------------------------------")
                print(actual_output.strip())
                print("----------------------------------------")
                warn_count += 1

        except Exception as e:
            print(f"{Colors.RED}ERROR during test run:{Colors.NC} {e}")
            fail_count += 1

    print("\n============================")
    print(f"{test_dir.name.upper()} Summary:")
    print(f"   {Colors.GREEN}PASS: {pass_count}{Colors.NC}")
    print(f"   {Colors.RED}FAIL: {fail_count}{Colors.NC}")
    print(f"   {Colors.YELLOW}WARN (No .result): {warn_count}{Colors.NC}")
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