#!/usr/bin/env python3

import argparse
import os
import subprocess
from pathlib import Path
import sys
from PIL import Image

# --- CONFIGURATION ---
# The path to your executable. Assumes it's in the same directory.
MBGL_GLFW_EXECUTABLE = './mbgl-glfw'

# TODO: Fill this array with the names of your tests.
test_names = [
    "route_add_test",
    "route_add_traffic_test",
    "route_traffic_priority_test",
    "route_pick_test"
]
# -------------------

def parse_arguments():
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(description="Test runner for mbgl-glfw.")
    parser.add_argument(
        '--test-mode',
        required=True,
        choices=['gen', 'compare'],
        help='Set to "gen" to generate expected images, or "compare" to run comparisons.'
    )
    return parser.parse_args()

def compare_images(file1, file2):
    """
    Compares two images pixel by pixel.
    Returns True if they are identical, False otherwise.
    """
    try:
        img1 = Image.open(file1).convert('RGB')
        img2 = Image.open(file2).convert('RGB')

        if img1.size != img2.size:
            print(f"      - Mismatch: Image dimensions differ. {img1.size} vs {img2.size}")
            return False

        # Simple and direct pixel data comparison
        if list(img1.getdata()) != list(img2.getdata()):
            print(f"      - Mismatch: Pixel data differs.")
            return False

    except FileNotFoundError:
        print(f"      - Mismatch: Expected image {file1} or actual image {file2} not found.")
        return False
    except Exception as e:
        print(f"      - Error comparing images {file1} and {file2}: {e}")
        return False

    return True

def generate_html_report(results, report_path):
    """Generates an HTML file from the test results."""
    passed_count = sum(1 for r in results if r['status'] == 'PASSED')
    failed_count = len(results) - passed_count

    html_content = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-g">
        <title>Test Results</title>
        <style>
            body {{ font-family: sans-serif; margin: 2em; }}
            h1, h2 {{ color: #333; }}
            table {{ border-collapse: collapse; width: 60%; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
            th, td {{ border: 1px solid #ddd; padding: 12px; text-align: left; }}
            th {{ background-color: #f2f2f2; }}
            .status-passed {{ color: #28a745; font-weight: bold; }}
            .status-failed {{ color: #dc3545; font-weight: bold; }}
            .summary {{ margin-bottom: 2em; }}
        </style>
    </head>
    <body>
        <h1>mbgl-glfw Test Report</h1>
        <div class="summary">
            <h2>Summary</h2>
            <p><strong>Total Tests:</strong> {len(results)}</p>
            <p><strong class="status-passed">Passed:</strong> {passed_count}</p>
            <p><strong class="status-failed">Failed:</strong> {failed_count}</p>
        </div>
        <h2>Details</h2>
        <table>
            <thead>
                <tr>
                    <th>Test Name</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody>
    """

    for result in results:
        status_class = "status-passed" if result['status'] == 'PASSED' else "status-failed"
        html_content += f"""
                <tr>
                    <td>{result['name']}</td>
                    <td class="{status_class}">{result['status']}</td>
                </tr>
        """

    html_content += """
            </tbody>
        </table>
    </body>
    </html>
    """
    with open(report_path, 'w') as f:
        f.write(html_content)
    print(f"\n✅ HTML report generated at: {report_path}")


def main():
    """Main script execution function."""
    args = parse_arguments()

    # 1. Determine the root directory for tests
    root_dir = Path(os.getenv('GLFW_TEST_DIR', '.'))
    glfw_test_dir = root_dir / 'glfw_test'
    print(f"ℹ️  Using test root directory: {glfw_test_dir.resolve()}")

    if not Path(MBGL_GLFW_EXECUTABLE).is_file():
        print(f"❌ Error: Executable not found at '{MBGL_GLFW_EXECUTABLE}'")
        sys.exit(1)

    if not test_names:
        print("⚠️  Warning: The 'test_names' array is empty. Please add test names to the script.")
        return

    # 2. Handle 'gen' mode
    if args.test_mode == 'gen':
        print("\n🚀 Starting 'gen' mode: Generating expected images...")
        expected_root_dir = glfw_test_dir / 'expected'

        for test_name in test_names:
            output_dir = expected_root_dir / test_name
            output_dir.mkdir(parents=True, exist_ok=True)
            print(f"   Running test '{test_name}'...")

            command = [
                MBGL_GLFW_EXECUTABLE,
                '--test-name', test_name,
                '--test-mode', 'gen',
                '--test-dir', str(output_dir.resolve())
            ]

            subprocess.run(command, check=True)
        print("\n✅ 'gen' mode complete. Expected images are in the 'glfw_test/expected' directory.")

    # 3. Handle 'compare' mode
    elif args.test_mode == 'compare':
        print("\n🚀 Starting 'compare' mode: Generating and comparing images...")
        results = []
        expected_root_dir = glfw_test_dir / 'expected'
        compare_root_dir = glfw_test_dir / 'compare'

        for test_name in test_names:
            print(f"\n   Running test '{test_name}'...")
            actual_dir = compare_root_dir / test_name
            expected_dir = expected_root_dir / test_name

            # Create directory for the actual results
            actual_dir.mkdir(parents=True, exist_ok=True)

            # Run the test to generate the 'compare' images
            command = [
                MBGL_GLFW_EXECUTABLE,
                '--test-name', test_name,
                '--test-mode', 'compare',
                '--test-dir', str(actual_dir.resolve())
            ]
            subprocess.run(command, check=True)

            # Compare results
            print(f"   Comparing results for '{test_name}'...")
            if not expected_dir.is_dir():
                print(f"      - FAILED: Expected directory '{expected_dir}' not found. Run --test-mode gen first.")
                results.append({'name': test_name, 'status': 'FAILED'})
                continue

            # Find all expected images and compare them one by one
            expected_images = sorted(list(expected_dir.glob('*.png')))
            if not expected_images:
                print(f"      - FAILED: No expected PNG images found in '{expected_dir}'.")
                results.append({'name': test_name, 'status': 'FAILED'})
                continue

            test_passed = True
            for expected_image_path in expected_images:
                actual_image_path = actual_dir / expected_image_path.name
                print(f"      - Comparing: {expected_image_path.name}")
                if not compare_images(expected_image_path, actual_image_path):
                    test_passed = False
                    break # A single failed image fails the whole test case

            if test_passed:
                print(f"   ✨ PASSED: '{test_name}'")
                results.append({'name': test_name, 'status': 'PASSED'})
            else:
                print(f"   ❌ FAILED: '{test_name}'")
                results.append({'name': test_name, 'status': 'FAILED'})

        # 4. Generate HTML Report
        report_path = glfw_test_dir / 'results.html'
        generate_html_report(results, report_path)

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"\n❌ Error: The 'mbgl-glfw' process failed with exit code {e.returncode}.")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ An unexpected error occurred: {e}")
        sys.exit(1)
