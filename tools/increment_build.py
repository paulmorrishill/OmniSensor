#!/usr/bin/env python3
"""
Build Number Incrementer
Automatically increments the build number in version.h
"""

import os
import re
import sys
from pathlib import Path

def increment_build_number(version_file_path):
    """
    Increment the build number in the version.h file
    """
    if not os.path.exists(version_file_path):
        print(f"Error: Version file not found: {version_file_path}")
        return False
    
    try:
        # Read the current version file
        with open(version_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Find the current build number
        build_pattern = r'#define FIRMWARE_BUILD_NUMBER (\d+)'
        match = re.search(build_pattern, content)
        
        if not match:
            print("Error: Could not find FIRMWARE_BUILD_NUMBER in version.h")
            return False
        
        current_build = int(match.group(1))
        new_build = current_build + 1
        
        # Replace the build number
        new_content = re.sub(
            build_pattern,
            f'#define FIRMWARE_BUILD_NUMBER {new_build}',
            content
        )
        
        # Write the updated content back
        with open(version_file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"Build number incremented: {current_build} -> {new_build}")
        return True
        
    except Exception as e:
        print(f"Error updating build number: {e}")
        return False

def main():
    # Default path to version.h
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    version_file = project_root / "src" / "version.h"
    
    # Allow custom path as command line argument
    if len(sys.argv) > 1:
        version_file = Path(sys.argv[1])
    
    print(f"Incrementing build number in: {version_file}")
    
    if increment_build_number(version_file):
        print("Build number increment completed successfully!")
        sys.exit(0)
    else:
        print("Failed to increment build number!")
        sys.exit(1)

if __name__ == "__main__":
    main()