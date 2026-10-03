"""
Quick fix for KAIN-generated shader includes
Adds 'Shaders/' prefix to all header includes in shader .cpp files
"""

import os
import re

# Shader files to fix
shader_files = [
    "Source/Materialize/Private/Shaders/GlossyClearCoat.cpp",
    "Source/Materialize/Private/Shaders/GlossyDualLobe.cpp",
    "Source/Materialize/Private/Shaders/GlossySubsurface.cpp",
    "Source/Materialize/Private/Shaders/MaterializeFresnelSchlick.cpp",
    "Source/Materialize/Private/Shaders/MaterializeGGXDistribution.cpp",
    "Source/Materialize/Private/Shaders/MaterializeSmithVisibility.cpp",
    "Source/Materialize/Private/Shaders/MetalAnisotropicSpecular.cpp",
    "Source/Materialize/Private/Shaders/MetalFresnelRim.cpp",
    "Source/Materialize/Private/Shaders/ToonCelShading.cpp",
    "Source/Materialize/Private/Shaders/ToonConfigurableBands.cpp",
    "Source/Materialize/Private/Shaders/ToonOutlineDetection.cpp",
    "Source/Materialize/Private/Shaders/ToonRimLight.cpp",
    "Source/Materialize/Private/Shaders/ToonSpecular.cpp",
]

for filepath in shader_files:
    if not os.path.exists(filepath):
        print(f"SKIP: {filepath} (not found)")
        continue
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Fix the include - add Shaders/ prefix if not already there
    # Pattern: #include "FileName.h" -> #include "Shaders/FileName.h"
    pattern = r'#include "([A-Z][a-zA-Z]+\.h)"'
    replacement = r'#include "Shaders/\1"'
    
    new_content = re.sub(pattern, replacement, content, count=1)  # Only fix first include
    
    if new_content != content:
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"FIXED: {filepath}")
    else:
        print(f"OK: {filepath}")

print("\nAll shader includes fixed!")
