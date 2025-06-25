#!/usr/bin/env python3
"""
Setup script for Serial Monitor
Installs dependencies and creates shortcuts
"""

import subprocess
import sys
import os

def install_requirements():
    """Install required packages"""
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        print("[OK] Dependencies installed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Failed to install dependencies: {e}")
        return False

def create_batch_files():
    """Create Windows batch files for easy launching"""
    if os.name == 'nt':  # Windows
        # GUI version
        with open("run_gui.bat", "w") as f:
            f.write("@echo off\n")
            f.write("cd /d %~dp0\n")
            f.write("python gui_monitor.py\n")
        
        print("[OK] Created batch file: run_gui.bat")
    else:
        # Unix/Linux shell scripts
        with open("run_gui.sh", "w") as f:
            f.write("#!/bin/bash\n")
            f.write("cd \"$(dirname \"$0\")\"\n")
            f.write("python3 gui_monitor.py\n")
        
        # Make executable
        os.chmod("run_gui.sh", 0o755)
        
        print("[OK] Created shell script: run_gui.sh")

def main():
    print("Setting up Serial Monitor...")
    print("=" * 40)
    
    if install_requirements():
        create_batch_files()
        print("\n[OK] Setup complete!")
        print("\nUsage:")
        if os.name == 'nt':
            print("  - Double-click run_gui.bat to start")
        else:
            print("  - Run ./run_gui.sh to start")
        print("  - Or run directly: python gui_monitor.py")
    else:
        print("\n[ERROR] Setup failed!")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())