#!/usr/bin/env python3
"""
PlatformIO Pre-Build Script
Automatically generates HTML constants before building
"""

import os
import sys
import subprocess
from pathlib import Path

# Add the project root to the path so we can import our modules
# Handle case where __file__ might not be defined (PlatformIO execution)
try:
    project_root = Path(__file__).parent.parent
except NameError:
    # Fallback to current working directory
    project_root = Path.cwd()
sys.path.insert(0, str(project_root))

def run_html_generator():
    """
    Run the HTML to header generator
    """
    print("=" * 50)
    print("Generating HTML constants...")
    print("=" * 50)
    
    # Define paths
    html_dir = project_root / "html"
    output_header = project_root / "src" / "html_constants.h"
    generator_script = project_root / "tools" / "html_to_header.py"
    
    # Find all HTML files
    html_files = []
    if html_dir.exists():
        html_files = list(html_dir.glob("*.html"))
    
    if not html_files:
        print("No HTML files found in html/ directory")
        return True
    
    # Build command
    cmd = [sys.executable, str(generator_script), str(output_header)]
    cmd.extend([str(f) for f in html_files])
    
    try:
        # Run the generator
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=str(project_root))
        
        if result.returncode == 0:
            print(result.stdout)
            print("HTML constants generated successfully!")
            return True
        else:
            print("Error generating HTML constants:")
            print(result.stderr)
            return False
            
    except Exception as e:
        print(f"Failed to run HTML generator: {e}")
        return False

def main():
    """
    Main pre-build function called by PlatformIO
    """
    print("Running pre-build script...")
    
    if not run_html_generator():
        print("Pre-build failed!")
        sys.exit(1)
    
    print("Pre-build completed successfully!")

# PlatformIO calls this function
if __name__ == "__main__":
    main()