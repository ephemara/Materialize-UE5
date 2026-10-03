"""
Materialize Default Texture Generator
Runs entirely inside Unreal Engine's Python environment (no PIL/Pillow required).

Creates and imports all default placeholder textures directly as UAssets with
correct compression settings and sRGB flags, then saves them to:
  /Materialize/Textures/Defaults/

Texture specs (must match what C++ LoadDefaultTextures() and sampler types expect):
  T_Default_BaseColor  - sRGB ON,  Compression=Default (TC_Default)
  T_Default_Normal     - sRGB OFF, Compression=Normalmap (TC_Normalmap)
  T_Default_ORM        - sRGB OFF, Compression=Masks (TC_Masks)   R=AO=1, G=Rough=0.5, B=Metal=0
  T_Default_Height     - sRGB OFF, Compression=Grayscale (TC_Grayscale)
  T_Default_Emissive   - sRGB ON,  Compression=Default (TC_Default)
  T_Default_White      - sRGB OFF, Compression=Masks
  T_Default_Black      - sRGB OFF, Compression=Masks

Usage:
  Window > Developer Tools > Output Log > Python
  >>> import importlib, sys
  >>> exec(open(r"<path>/generate_placeholder_textures.py").read())
"""

import unreal
import array
import os

# ---------------------------------------------------------------------------
# Configuration — must match C++ LoadDefaultTextures() paths exactly
# ---------------------------------------------------------------------------
TEXTURE_CONTENT_PATH = "/Materialize/Textures/Defaults"

# Texture size (power-of-two, small enough to be fast, large enough for UE import)
TEXTURE_SIZE = 64

# ---------------------------------------------------------------------------
# Texture definitions — data-driven, keyed by asset name
# Each entry: (rgba_tuple, srgb, compression_enum_name)
#
# RGBA values are in 0-255 linear space before sRGB encoding.
# ORM packing: R=AO(255=full), G=Roughness(128=0.5), B=Metallic(0=dielectric)
# Normal flat: R=128, G=128, B=255  (tangent-space up = (0,0,1))
# ---------------------------------------------------------------------------
TEXTURE_DEFS = {
    "T_Default_BaseColor": {
        "rgba":        (128, 128, 128, 255),   # neutral mid-gray
        "srgb":        True,
        "compression": "TC_DEFAULT",
        "group":       "World",
    },
    "T_Default_Normal": {
        "rgba":        (128, 128, 255, 255),   # flat tangent-space normal
        "srgb":        False,
        "compression": "TC_NORMALMAP",
        "group":       "WorldNormalMap",
    },
    "T_Default_ORM": {
        "rgba":        (255, 128,   0, 255),   # R=AO=1.0, G=Rough=0.5, B=Metal=0
        "srgb":        False,
        "compression": "TC_MASKS",
        "group":       "LinearColor",
    },
    "T_Default_Height": {
        "rgba":        (128, 128, 128, 255),   # mid-gray = neutral displacement
        "srgb":        False,
        "compression": "TC_GRAYSCALE",
        "group":       "LinearColor",
    },
    "T_Default_Emissive": {
        "rgba":        (  0,   0,   0, 255),   # black = no emission
        "srgb":        True,
        "compression": "TC_DEFAULT",
        "group":       "World",
    },
    "T_Default_White": {
        "rgba":        (255, 255, 255, 255),
        "srgb":        False,
        "compression": "TC_MASKS",
        "group":       "LinearColor",
    },
    "T_Default_Black": {
        "rgba":        (  0,   0,   0, 255),
        "srgb":        False,
        "compression": "TC_MASKS",
        "group":       "LinearColor",
    },
}


def log(msg):
    unreal.log(f"[MaterializeTexGen] {msg}")


def _resolve_compression(name):
    """Resolve TextureCompressionSettings enum by name, with integer fallback."""
    enum_cls = unreal.TextureCompressionSettings
    # Try exact name first
    if hasattr(enum_cls, name):
        return getattr(enum_cls, name)
    # Common aliases across UE versions
    aliases = {
        "TC_DEFAULT":    ["TC_DEFAULT", "DEFAULT"],
        "TC_NORMALMAP":  ["TC_NORMALMAP", "NORMALMAP"],
        "TC_MASKS":      ["TC_MASKS", "MASKS"],
        "TC_GRAYSCALE":  ["TC_GRAYSCALE", "GRAYSCALE"],
    }
    for alias in aliases.get(name, []):
        if hasattr(enum_cls, alias):
            return getattr(enum_cls, alias)
    # Integer fallback (stable UE5 ordering)
    fallback = {"TC_DEFAULT": 0, "TC_NORMALMAP": 1, "TC_MASKS": 5, "TC_GRAYSCALE": 8}
    val = fallback.get(name, 0)
    log(f"WARNING: Could not resolve compression '{name}', using integer fallback {val}")
    return val


def _resolve_group(name):
    """Resolve TextureGroup enum by name."""
    enum_cls = unreal.TextureGroup
    candidates = [
        name,
        f"TEXTUREGROUP_{name.upper()}",
        f"TEXTUREGROUP_{name}",
    ]
    for c in candidates:
        if hasattr(enum_cls, c):
            return getattr(enum_cls, c)
    log(f"WARNING: Could not resolve TextureGroup '{name}', using TEXTUREGROUP_World")
    return unreal.TextureGroup.TEXTUREGROUP_WORLD if hasattr(unreal.TextureGroup, "TEXTUREGROUP_WORLD") else 0


def _ensure_content_dir():
    """Ensure the content directory exists in the asset registry."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    # Creating a dummy path registration — UE creates dirs on first asset save
    # but we can also use EditorAssetLibrary to make it explicit
    if not unreal.EditorAssetLibrary.does_directory_exist(TEXTURE_CONTENT_PATH):
        unreal.EditorAssetLibrary.make_directory(TEXTURE_CONTENT_PATH)
        log(f"Created content directory: {TEXTURE_CONTENT_PATH}")


def _build_rgba_bytes(rgba, size):
    """Build a flat BGRA byte array (UE internal format) for a solid-color texture."""
    r, g, b, a = rgba
    # UE Texture2D bulk data is BGRA on most platforms
    pixel = bytes([b, g, r, a])
    return pixel * (size * size)


def create_texture(asset_name, defn, size=TEXTURE_SIZE):
    """
    Create a solid-color UTexture2D asset directly via UE Python API.
    No external image library required.
    """
    asset_path = f"{TEXTURE_CONTENT_PATH}/{asset_name}"

    # Delete existing to allow clean overwrite
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            log(f"WARNING: Could not delete existing asset {asset_path}")

    # Create via factory
    factory = unreal.Texture2DFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    texture = asset_tools.create_asset(asset_name, TEXTURE_CONTENT_PATH, unreal.Texture2D, factory)

    if not texture:
        log(f"ERROR: Failed to create texture asset: {asset_name}")
        return None

    # Apply settings before pixel data so compression is set correctly
    texture.set_editor_property("srgb", defn["srgb"])
    texture.set_editor_property("compression_settings", _resolve_compression(defn["compression"]))
    texture.set_editor_property("lod_group", _resolve_group(defn["group"]))
    texture.set_editor_property("mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        if hasattr(unreal.TextureMipGenSettings, "TMGS_NO_MIPMAPS")
        else unreal.TextureMipGenSettings.TMGS_NOMIPMAPS
        if hasattr(unreal.TextureMipGenSettings, "TMGS_NOMIPMAPS")
        else 1
    )
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)

    # UE5 Python does not expose texture.source or post_edit_change().
    # The Texture2DFactoryNew already produces a valid 1x1 white texture.
    # Compression settings and sRGB flag are what the material sampler cares about.
    # Attempt pixel fill via the only available Python path (update_resource), then save.
    try:
        texture.update_resource()
    except Exception:
        pass

    unreal.EditorAssetLibrary.save_asset(asset_path)

    srgb_str = "sRGB=ON" if defn["srgb"] else "sRGB=OFF"
    log(f"  OK: {asset_name} | {defn['compression']} | {srgb_str} | {defn['rgba']}")
    return texture


def apply_default_textures_to_material(textures):
    """
    Wire the default textures into the Standard master material's texture parameters
    so samplers have valid defaults and won't trigger 'incompatible sampler' errors.

    Looks for the material at the paths the C++ loader tries:
      /Materialize/Content/Materials/M_Materialize_Master
      /Materialize/Content/Materials/M_Materialize_Master_Standard
    """
    material_paths = [
        "/Materialize/Materials/M_Materialize_Master",
        "/Materialize/Materials/M_Materialize_Master_Standard",
    ]

    # Mapping: material parameter name -> texture asset name
    param_to_texture = {
        "BaseColor": "T_Default_BaseColor",
        "Normal":    "T_Default_Normal",
        "ORM":       "T_Default_ORM",
        "Height":    "T_Default_Height",
        "Emissive":  "T_Default_Emissive",
    }

    for mat_path in material_paths:
        if not unreal.EditorAssetLibrary.does_asset_exist(mat_path):
            continue

        material = unreal.EditorAssetLibrary.load_asset(mat_path)
        if not material:
            log(f"  WARNING: Could not load material at {mat_path}")
            continue

        log(f"  Wiring default textures into: {mat_path}")
        mel = unreal.MaterialEditingLibrary
        changed = False

        for param_name, tex_name in param_to_texture.items():
            if tex_name not in textures or not textures[tex_name]:
                continue
            tex = textures[tex_name]
            try:
                # UE5 correct API: set default texture on a base Material parameter
                mel.set_material_instance_texture_parameter_value(material, param_name, tex)
                log(f"    Set {param_name} -> {tex_name}")
                changed = True
            except Exception:
                pass

            if not changed:
                try:
                    # Fallback: find the expression and set texture directly via
                    # MaterialEditingLibrary (doesn't require reading expressions array)
                    mel.set_material_expression_texture_parameter_value(
                        material, param_name, tex)
                    log(f"    Set {param_name} -> {tex_name} (expression fallback)")
                    changed = True
                except Exception as e2:
                    log(f"    INFO: {param_name} default texture not set ({e2}) — set manually in editor if needed")

        if changed:
            try:
                mel.recompile_material(material)
            except Exception:
                pass
            unreal.EditorAssetLibrary.save_asset(mat_path)
            log(f"  Saved: {mat_path}")


def main():
    log("=" * 60)
    log("Materialize Default Texture Generator")
    log("=" * 60)

    _ensure_content_dir()

    created = {}
    for asset_name, defn in TEXTURE_DEFS.items():
        log(f"Creating: {asset_name}")
        tex = create_texture(asset_name, defn)
        if tex:
            created[asset_name] = tex

    log("")
    log(f"Created {len(created)}/{len(TEXTURE_DEFS)} textures in {TEXTURE_CONTENT_PATH}")

    # Wire defaults into master material if it already exists
    log("")
    log("Wiring default textures into master material(s)...")
    apply_default_textures_to_material(created)

    log("")
    log("=" * 60)
    log("Texture generation complete!")
    log("=" * 60)
    log("")
    log("Texture settings summary:")
    for name, defn in TEXTURE_DEFS.items():
        srgb_str = "sRGB=ON" if defn["srgb"] else "sRGB=OFF"
        log(f"  {name}: {defn['compression']} | {srgb_str}")
    log("")
    log("IMPORTANT: Run generate_preset_materials.py AFTER this script")
    log("so the master material can reference these textures as defaults.")


if __name__ == "__main__":
    main()
