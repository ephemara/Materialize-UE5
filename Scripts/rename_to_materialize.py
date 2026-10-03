#!/usr/bin/env python3
"""
Automated renaming script to transform KSample plugin to Materialize plugin.

This script performs comprehensive renaming across:
- File and directory names
- C++ class names (UKSample*, FKSample*, SKSample*, etc.)
- UI strings and labels
- Shader virtual paths
- Log categories
- Configuration keys
- Asset prefixes

Usage:
    python Scripts/rename_to_materialize.py [--dry-run] [--verbose]
"""

import os
import re
import sys
import argparse
import shutil
from pathlib import Path
from typing import List, Tuple, Set

# Root directory of the plugin (parent of Scripts/)
PLUGIN_ROOT = Path(__file__).parent.parent

# File extensions to process for text replacement
TEXT_FILE_EXTENSIONS = {
    '.cpp', '.h', '.cs', '.uplugin', '.usf', '.ush', 
    '.ini', '.txt', '.md', '.json', '.xml'
}

# Directories to skip
SKIP_DIRECTORIES = {
    '.git', '.vs', 'Binaries', 'Intermediate', 'Saved', 
    'Epic_Src', '.kiro', '.vscode'
}

# Text replacement patterns (regex pattern, replacement)
# Order matters - more specific patterns should come first
TEXT_REPLACEMENTS = [
    # C++ class prefixes
    (r'\bUKSample', 'UMaterialize'),
    (r'\bFKSample', 'FMaterialize'),
    (r'\bSKSample', 'SMaterialize'),
    (r'\bEKSample', 'EMaterialize'),
    (r'\bIKSample', 'IMaterialize'),
    (r'\bAKSample', 'AMaterialize'),
    
    # File and module names
    (r'\bKSample\.Build\.cs\b', 'Materialize.Build.cs'),
    (r'\bKSample\.uplugin\b', 'Materialize.uplugin'),
    (r'\bKSample\.h\b', 'Materialize.h'),
    (r'\bKSample\.cpp\b', 'Materialize.cpp'),
    
    # Shader virtual paths
    (r'/Plugin/KSample/', '/Plugin/Materialize/'),
    (r'"/KSample/', '"/Materialize/'),
    
    # Log categories
    (r'\bLogKSample\b', 'LogMaterialize'),
    
    # Config sections and keys
    (r'\[KSample\]', '[Materialize]'),
    (r'\bKSample\.', 'Materialize.'),
    
    # Asset prefixes
    (r'\bKS_', 'MAT_'),
    
    # UI strings - handle various formats
    (r'\bK-Sample\b', 'Materialize'),
    (r'\bKSample\b', 'Materialize'),
    
    # Macro definitions
    (r'\bKSAMPLE_API\b', 'MATERIALIZE_API'),
    (r'\bKSAMPLE_', 'MATERIALIZE_'),
    
    # Include guards
    (r'\b__KSAMPLE_', '__MATERIALIZE_'),
    (r'\bKSAMPLE_.*_H\b', lambda m: m.group(0).replace('KSAMPLE', 'MATERIALIZE')),
]

# File name replacement patterns
FILE_NAME_REPLACEMENTS = [
    ('KSample', 'Materialize'),
    ('KS_', 'MAT_'),
]


class RenamingStats:
    """Track statistics about the renaming operation."""
    def __init__(self):
        self.files_renamed = 0
        self.dirs_renamed = 0
        self.files_modified = 0
        self.replacements_made = 0
        self.errors = []
    
    def print_summary(self):
        """Print a summary of the renaming operation."""
        print("\n" + "="*60)
        print("RENAMING SUMMARY")
        print("="*60)
        print(f"Files renamed:        {self.files_renamed}")
        print(f"Directories renamed:  {self.dirs_renamed}")
        print(f"Files modified:       {self.files_modified}")
        print(f"Text replacements:    {self.replacements_made}")
        if self.errors:
            print(f"\nErrors encountered:   {len(self.errors)}")
            for error in self.errors[:10]:  # Show first 10 errors
                print(f"  - {error}")
            if len(self.errors) > 10:
                print(f"  ... and {len(self.errors) - 10} more")
        print("="*60)


def should_skip_path(path: Path) -> bool:
    """Check if a path should be skipped during processing."""
    parts = path.parts
    return any(skip_dir in parts for skip_dir in SKIP_DIRECTORIES)


def get_new_filename(old_name: str) -> str:
    """Generate new filename by applying replacement patterns."""
    new_name = old_name
    for old_pattern, new_pattern in FILE_NAME_REPLACEMENTS:
        new_name = new_name.replace(old_pattern, new_pattern)
    return new_name


def replace_text_in_file(file_path: Path, dry_run: bool, verbose: bool) -> int:
    """
    Replace text patterns in a single file.
    Returns the number of replacements made.
    """
    try:
        # Read file content
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        original_content = content
        replacements = 0
        
        # Apply all text replacement patterns
        for pattern, replacement in TEXT_REPLACEMENTS:
            if callable(replacement):
                # Handle lambda replacements
                new_content, count = re.subn(pattern, replacement, content)
            else:
                new_content, count = re.subn(pattern, replacement, content)
            
            if count > 0:
                replacements += count
                content = new_content
                if verbose:
                    print(f"    {pattern} -> {replacement}: {count} replacements")
        
        # Write back if changes were made
        if content != original_content:
            if not dry_run:
                with open(file_path, 'w', encoding='utf-8', newline='') as f:
                    f.write(content)
            return replacements
        
        return 0
    
    except Exception as e:
        raise Exception(f"Error processing {file_path}: {str(e)}")


def rename_files_in_directory(directory: Path, dry_run: bool, verbose: bool, stats: RenamingStats):
    """
    Recursively rename files in a directory.
    Process files bottom-up to avoid path issues.
    """
    # Collect all files and directories first
    items_to_process = []
    
    for root, dirs, files in os.walk(directory, topdown=False):
        root_path = Path(root)
        
        # Skip excluded directories
        if should_skip_path(root_path):
            continue
        
        # Process files
        for filename in files:
            file_path = root_path / filename
            new_filename = get_new_filename(filename)
            
            if new_filename != filename:
                items_to_process.append(('file', file_path, new_filename))
        
        # Process directories
        for dirname in dirs:
            dir_path = root_path / dirname
            new_dirname = get_new_filename(dirname)
            
            if new_dirname != dirname and not should_skip_path(dir_path):
                items_to_process.append(('dir', dir_path, new_dirname))
    
    # Execute renames
    for item_type, old_path, new_name in items_to_process:
        new_path = old_path.parent / new_name
        
        if verbose or dry_run:
            print(f"{'[DRY RUN] ' if dry_run else ''}Rename {item_type}: {old_path.relative_to(PLUGIN_ROOT)} -> {new_name}")
        
        if not dry_run:
            try:
                old_path.rename(new_path)
                if item_type == 'file':
                    stats.files_renamed += 1
                else:
                    stats.dirs_renamed += 1
            except Exception as e:
                error_msg = f"Failed to rename {old_path}: {str(e)}"
                stats.errors.append(error_msg)
                print(f"ERROR: {error_msg}")


def replace_text_in_files(directory: Path, dry_run: bool, verbose: bool, stats: RenamingStats):
    """
    Recursively replace text in all relevant files.
    """
    for root, dirs, files in os.walk(directory):
        root_path = Path(root)
        
        # Skip excluded directories
        if should_skip_path(root_path):
            continue
        
        # Remove excluded directories from traversal
        dirs[:] = [d for d in dirs if not should_skip_path(root_path / d)]
        
        for filename in files:
            file_path = root_path / filename
            
            # Check if file extension should be processed
            if file_path.suffix.lower() not in TEXT_FILE_EXTENSIONS:
                continue
            
            try:
                replacements = replace_text_in_file(file_path, dry_run, verbose)
                
                if replacements > 0:
                    stats.files_modified += 1
                    stats.replacements_made += replacements
                    
                    if verbose or dry_run:
                        rel_path = file_path.relative_to(PLUGIN_ROOT)
                        print(f"{'[DRY RUN] ' if dry_run else ''}Modified: {rel_path} ({replacements} replacements)")
            
            except Exception as e:
                error_msg = str(e)
                stats.errors.append(error_msg)
                print(f"ERROR: {error_msg}")


def create_backup(dry_run: bool):
    """Create a backup of the plugin directory before renaming."""
    if dry_run:
        print("[DRY RUN] Would create backup: KSample_backup.zip")
        return
    
    backup_path = PLUGIN_ROOT.parent / "KSample_backup"
    
    print(f"Creating backup at {backup_path}...")
    
    try:
        if backup_path.exists():
            shutil.rmtree(backup_path)
        
        # Copy entire plugin directory
        shutil.copytree(
            PLUGIN_ROOT, 
            backup_path,
            ignore=shutil.ignore_patterns(
                '.git', 'Binaries', 'Intermediate', 'Saved', 
                '*.zip', '__pycache__', '*.pyc'
            )
        )
        
        print(f"Backup created successfully at {backup_path}")
    
    except Exception as e:
        print(f"ERROR: Failed to create backup: {str(e)}")
        print("Aborting renaming operation for safety.")
        sys.exit(1)


def main():
    """Main entry point for the renaming script."""
    parser = argparse.ArgumentParser(
        description='Rename KSample plugin to Materialize plugin'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be renamed without making changes'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Show detailed output for each operation'
    )
    parser.add_argument(
        '--no-backup',
        action='store_true',
        help='Skip creating backup (not recommended)'
    )
    
    args = parser.parse_args()
    
    print("="*60)
    print("KSample -> Materialize Renaming Script")
    print("="*60)
    print(f"Plugin root: {PLUGIN_ROOT}")
    print(f"Mode: {'DRY RUN' if args.dry_run else 'LIVE'}")
    print(f"Verbose: {args.verbose}")
    print("="*60)
    
    if not args.dry_run and not args.no_backup:
        response = input("\nCreate backup before renaming? (recommended) [Y/n]: ")
        if response.lower() != 'n':
            create_backup(args.dry_run)
    
    if not args.dry_run:
        response = input("\nProceed with renaming? This will modify files! [y/N]: ")
        if response.lower() != 'y':
            print("Aborted by user.")
            sys.exit(0)
    
    stats = RenamingStats()
    
    print("\nPhase 1: Replacing text in files...")
    replace_text_in_files(PLUGIN_ROOT, args.dry_run, args.verbose, stats)
    
    print("\nPhase 2: Renaming files and directories...")
    rename_files_in_directory(PLUGIN_ROOT, args.dry_run, args.verbose, stats)
    
    stats.print_summary()
    
    if not args.dry_run:
        print("\n" + "="*60)
        print("NEXT STEPS:")
        print("="*60)
        print("1. Review the changes in your version control system")
        print("2. Clean the build directory:")
        print("   - Delete Binaries/ and Intermediate/ folders")
        print("3. Rebuild the plugin:")
        print("   - Run UnrealBuildTool with the new module name")
        print("4. Test loading old KSample assets for backward compatibility")
        print("5. Verify all UI displays 'Materialize' branding")
        print("="*60)
    
    return 0 if not stats.errors else 1


if __name__ == '__main__':
    sys.exit(main())
