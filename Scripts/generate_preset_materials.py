"""
Materialize Preset Material Generator
Runs entirely inside Unreal Engine's Python environment.

Depends on:
  1. generate_placeholder_textures.py  (textures must exist first)
  2. generate_material_functions.py    (MF_ assets must exist first)

Asset paths produced — MUST match FMaterializeMaterialLoader::LoadMasterMaterial() exactly:
  Standard : /Materialize/Materials/M_Materialize_Master
  Metal    : /Materialize/Materials/Presets/M_Materialize_Master_Metal
  Glossy   : /Materialize/Materials/Presets/M_Materialize_Master_Glossy
  Toon     : /Materialize/Materials/Presets/M_Materialize_Master_Toon

Texture parameter names — MUST match SetTextureParameterValue() in SMaterializeEditor.cpp:
  BaseColor, Normal, ORM, Height, Emissive
  Roughness, Metallic, AO  (individual-channel fallback path)

Scalar parameter names — MUST match SetScalarParameterValue():
  Tiling, AO_Power, Roughness_Mult, Roughness_Offset, Metallic_Mult, Emissive_Power

Vector parameter names — MUST match SetVectorParameterValue():
  Tint

Sampler types — MUST match MaterializeTransientGenerator.cpp:
  BaseColor  -> SAMPLERTYPE_Color        (sRGB)
  Normal     -> SAMPLERTYPE_Normal
  ORM        -> SAMPLERTYPE_LinearColor  (linear, no sRGB)
  Height     -> SAMPLERTYPE_LinearColor
  Emissive   -> SAMPLERTYPE_Color        (sRGB)
  Roughness/Metallic/AO -> SAMPLERTYPE_LinearColor

Usage (UE Output Log > Python):
  exec(open(r"<plugin>/Scripts/generate_preset_materials.py").read())
"""

import unreal

# ---------------------------------------------------------------------------
# Paths — must match C++ loader exactly
# ---------------------------------------------------------------------------
MATERIALS_PATH      = "/Materialize/Materials"
PRESETS_PATH        = f"{MATERIALS_PATH}/Presets"
FUNCTIONS_PATH      = f"{MATERIALS_PATH}/Functions"
TEXTURES_DEFAULTS   = "/Materialize/Textures/Defaults"

STANDARD_NAME       = "M_Materialize_Master"
STANDARD_PATH       = f"{MATERIALS_PATH}/{STANDARD_NAME}"

OVERWRITE           = True

# ---------------------------------------------------------------------------
# Default texture asset paths — match C++ LoadDefaultTextures() keys exactly
# ---------------------------------------------------------------------------
DEFAULT_TEXTURES = {
    "BaseColor": f"{TEXTURES_DEFAULTS}/T_Default_BaseColor",
    "Normal":    f"{TEXTURES_DEFAULTS}/T_Default_Normal",
    "ORM":       f"{TEXTURES_DEFAULTS}/T_Default_ORM",
    "Height":    f"{TEXTURES_DEFAULTS}/T_Default_Height",
    "Emissive":  f"{TEXTURES_DEFAULTS}/T_Default_Emissive",
    "White":     f"{TEXTURES_DEFAULTS}/T_Default_White",
    "Black":     f"{TEXTURES_DEFAULTS}/T_Default_Black",
}

# ---------------------------------------------------------------------------
# Preset definitions — data-driven, keyed by preset name
#
# "asset"     : output asset name (goes into PRESETS_PATH)
# "functions" : list of (MF_asset_name, connect_to_mp)
#               connect_to_mp: unreal.MaterialProperty enum name, or None
# "scalars"   : list of (param_name, default_value)  -- preset-specific extras
# "vectors"   : list of (param_name, r, g, b)        -- preset-specific extras
# ---------------------------------------------------------------------------
PRESET_DEFS = {
    "Metal": {
        "asset": "M_Materialize_Master_Metal",
        "functions": [
            ("MF_MetalAnisotropicSpecular", None),
            ("MF_MetalFresnelRim",          None),
        ],
        "scalars": [
            ("Anisotropy",          0.5),
            ("AnisotropyDirection", 0.0),
            ("SpecularIntensity",   1.0),
            ("RimIntensity",        0.3),
        ],
        "vectors": [
            ("SpecularTint", 1.0, 1.0, 1.0),
            ("RimColor",     1.0, 1.0, 1.0),
        ],
    },
    "Glossy": {
        "asset": "M_Materialize_Master_Glossy",
        "functions": [
            ("MF_GlossyClearCoat",  None),
            ("MF_GlossyDualLobe",   None),
            ("MF_GlossySubsurface", "MP_EMISSIVE_COLOR"),
        ],
        "scalars": [
            ("ClearCoat",          1.0),
            ("ClearCoatRoughness", 0.1),
            ("CoatIOR",            1.5),
            ("SubsurfaceStrength", 0.5),
            ("Thickness",          0.5),
        ],
        "vectors": [
            ("SubsurfaceColor", 1.0, 0.5, 0.3),
        ],
    },
    "Toon": {
        "asset": "M_Materialize_Master_Toon",
        "functions": [
            ("MF_ToonCelShading", "MP_BASE_COLOR"),
            ("MF_ToonSpecular",   "MP_EMISSIVE_COLOR"),
            ("MF_ToonRimLight",   "MP_EMISSIVE_COLOR"),
        ],
        "scalars": [
            ("BandCount",      3.0),
            ("BandSmoothness", 0.05),
            ("SpecularSize",   0.5),
            ("SpecularSteps",  2.0),
            ("RimIntensity",   1.0),
            ("RimPower",       3.0),
        ],
        "vectors": [
            ("ShadowColor",    0.3, 0.3, 0.5),
            ("HighlightColor", 1.0, 1.0, 1.0),
            ("MidtoneColor",   0.7, 0.7, 0.8),
        ],
    },
}


# ---------------------------------------------------------------------------
# Enum resolvers
# ---------------------------------------------------------------------------

def _resolve_sampler(name):
    """Resolve MaterialSamplerType enum across UE5 minor versions."""
    enum_cls = unreal.MaterialSamplerType
    aliases = {
        "Color":       ["SAMPLERTYPE_COLOR",       "COLOR"],
        "Normal":      ["SAMPLERTYPE_NORMAL",       "NORMAL"],
        "LinearColor": ["SAMPLERTYPE_LINEAR_COLOR", "LINEAR_COLOR", "LINEARCOLOR"],
        "Masks":       ["SAMPLERTYPE_MASKS",        "MASKS"],
        "Grayscale":   ["SAMPLERTYPE_GRAYSCALE",    "GRAYSCALE"],
    }
    for cand in aliases.get(name, [name]):
        if hasattr(enum_cls, cand):
            return getattr(enum_cls, cand)
    fallback = {"Color": 1, "Normal": 4, "LinearColor": 7, "Masks": 5, "Grayscale": 8}
    return fallback.get(name, 1)


def _resolve_mp(name):
    """Resolve MaterialProperty enum by string name."""
    if name is None:
        return None
    enum_cls = unreal.MaterialProperty
    if hasattr(enum_cls, name):
        return getattr(enum_cls, name)
    # Try without prefix
    short = name.replace("MP_", "")
    for attr in dir(enum_cls):
        if attr.upper().endswith(short):
            return getattr(enum_cls, attr)
    log(f"WARNING: Could not resolve MaterialProperty '{name}'")
    return None


def _get_mp():
    """
    Return a dict of material property enum values, verified by scanning
    the actual enum so we are never relying on assumed attribute names.
    Keys are short names; values are the enum instances.
    """
    enum_cls = unreal.MaterialProperty
    result = {}
    # Map short token -> list of candidate attribute substrings (most specific first)
    wanted = {
        "BASE_COLOR":        ["MP_BASE_COLOR",        "BASE_COLOR"],
        "METALLIC":          ["MP_METALLIC",           "METALLIC"],
        "ROUGHNESS":         ["MP_ROUGHNESS",          "ROUGHNESS"],
        "EMISSIVE_COLOR":    ["MP_EMISSIVE_COLOR",     "EMISSIVE_COLOR", "EMISSIVE"],
        "NORMAL":            ["MP_NORMAL",             "NORMAL"],
        "AMBIENT_OCCLUSION": ["MP_AMBIENT_OCCLUSION",  "AMBIENT_OCCLUSION", "AMBIENT"],
    }
    all_attrs = {a.upper(): a for a in dir(enum_cls) if not a.startswith("_")}
    for key, candidates in wanted.items():
        for cand in candidates:
            if cand.upper() in all_attrs:
                result[key] = getattr(enum_cls, all_attrs[cand.upper()])
                break
        if key not in result:
            log(f"WARNING: Could not find MaterialProperty for '{key}'")
    return result


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def log(msg):
    unreal.log(f"[MatPresetGen] {msg}")


def _ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        log(f"Created directory: {path}")


def _delete_if_exists(path):
    if OVERWRITE and unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            log(f"WARNING: Could not delete {path}")


def _load_default_tex(key):
    path = DEFAULT_TEXTURES.get(key)
    if not path:
        return None
    tex = unreal.EditorAssetLibrary.load_asset(path)
    if not tex:
        log(f"WARNING: Default texture not found: {path}")
        log(f"         Run generate_placeholder_textures.py first!")
    return tex


def _load_function(func_name):
    path = f"{FUNCTIONS_PATH}/{func_name}"
    func = unreal.EditorAssetLibrary.load_asset(path)
    if not func:
        log(f"WARNING: Material function not found: {path}")
        log(f"         Run generate_material_functions.py first!")
    return func


def _create_mat(name, folder):
    _delete_if_exists(f"{folder}/{name}")
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not mat:
        log(f"ERROR: Failed to create {folder}/{name}")
    return mat


def _create_expr(mat, cls, x, y):
    try:
        return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    except Exception as e:
        log(f"ERROR: create_material_expression({cls.__name__}): {e}")
        return None


def _connect(src, src_pin, dst, dst_pin):
    """
    Connect src->dst. Tries src_pin first, then falls back to "" if it fails.
    Logs success/failure so we can diagnose pin name issues.
    """
    src_pins = [src_pin] if src_pin != "" else [""]
    if src_pin != "":
        src_pins.append("")  # fallback to default
    for sp in src_pins:
        try:
            unreal.MaterialEditingLibrary.connect_material_expressions(
                src, sp, dst, dst_pin)
            if sp != src_pin:
                log(f"  INFO: connect used fallback src_pin='' (requested '{src_pin}') -> dst_pin='{dst_pin}'")
            return
        except Exception as e:
            last_err = e
    log(f"WARNING: connect '{src_pin}'->'{dst_pin}' failed: {last_err}")


def _connect_prop(expr, output_pin, mp):
    if mp is None or expr is None:
        return
    last_err = None
    # Try output_pin first, then "" fallback
    for pin in ([output_pin] if output_pin else []) + [""]:
        try:
            unreal.MaterialEditingLibrary.connect_material_property(expr, pin, mp)
            log(f"  OK: connect_material_property pin='{pin}' mp={mp}")
            return
        except Exception as e:
            last_err = e
    log(f"WARNING: connect_material_property failed (mp={mp}): {last_err}")


# Mapping from our short key -> material object property name (confirmed from UE5 source)
_MAT_INPUT_PROP = {
    "BASE_COLOR":        "base_color",
    "METALLIC":          "metallic",
    "ROUGHNESS":         "roughness",
    "EMISSIVE_COLOR":    "emissive_color",
    "NORMAL":            "normal",
    "AMBIENT_OCCLUSION": "ambient_occlusion",
}


def _connect_prop_direct(mat, expr, slot_key):
    """
    Connect expr to a material output slot.
    Tries direct struct assignment first, then falls back to connect_material_property.
    """
    if not expr:
        return
    prop_name = _MAT_INPUT_PROP.get(slot_key)
    # Try direct struct assignment
    if prop_name:
        try:
            inp = mat.get_editor_property(prop_name)
            inp.set_editor_property("expression", expr)
            mat.set_editor_property(prop_name, inp)
            log(f"  OK: direct wire '{slot_key}' -> {prop_name}")
            return
        except Exception as e:
            log(f"  INFO: direct wire failed for '{slot_key}': {e}")
    # Fallback: connect_material_property with confirmed enum
    mp_map = _get_mp()
    mp = mp_map.get(slot_key)
    if mp is None:
        log(f"  WARNING: no MaterialProperty enum for '{slot_key}'")
        return
    # Output pin: Normal uses RGB, others use default ("")
    out_pin = "RGB" if slot_key == "NORMAL" else ""
    for pin in ([out_pin] if out_pin else []) + [""]:
        try:
            unreal.MaterialEditingLibrary.connect_material_property(expr, pin, mp)
            log(f"  OK: connect_material_property '{slot_key}' pin='{pin}' mp={mp}")
            return
        except Exception as e:
            last_err = e
    log(f"  WARNING: all wiring attempts failed for '{slot_key}': {last_err}")


def _tex_param(mat, name, sampler_key, default_key, x, y):
    expr = _create_expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    if not expr:
        return None
    expr.set_editor_property("parameter_name", name)
    expr.set_editor_property("sampler_type", _resolve_sampler(sampler_key))
    tex = _load_default_tex(default_key)
    if tex:
        expr.set_editor_property("texture", tex)
    return expr


def _scalar_param(mat, name, val, x, y):
    expr = _create_expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    if expr:
        expr.set_editor_property("parameter_name", name)
        expr.set_editor_property("default_value", float(val))
    return expr


def _vector_param(mat, name, r, g, b, a, x, y):
    expr = _create_expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    if expr:
        expr.set_editor_property("parameter_name", name)
        expr.set_editor_property("default_value", unreal.LinearColor(r, g, b, a))
    return expr


def _multiply(mat, x, y):
    return _create_expr(mat, unreal.MaterialExpressionMultiply, x, y)


def _add(mat, x, y):
    return _create_expr(mat, unreal.MaterialExpressionAdd, x, y)


def _clamp(mat, x, y):
    expr = _create_expr(mat, unreal.MaterialExpressionClamp, x, y)
    if expr:
        expr.set_editor_property("min_default", 0.0)
        expr.set_editor_property("max_default", 1.0)
    return expr


CLAMP_INPUT_PIN = ""  # connect_material_expressions uses "" for first/default input
MASK_INPUT_PIN  = ""  # connect_material_expressions uses "" for first/default input


def _mask(mat, r, g, b, a, x, y):
    expr = _create_expr(mat, unreal.MaterialExpressionComponentMask, x, y)
    if expr:
        expr.set_editor_property("r", r)
        expr.set_editor_property("g", g)
        expr.set_editor_property("b", b)
        expr.set_editor_property("a", a)
    return expr


# ---------------------------------------------------------------------------
# Standard Master Material
# ---------------------------------------------------------------------------

def build_standard_material():
    """
    Full PBR master material.
    All parameter names match SMaterializeEditor.cpp SetXxxParameterValue calls exactly.

    Graph layout (x increases left-to-right toward material outputs at x=0):
      Textures    x=-1600
      Scalars     x=-1200
      Masks/Math  x=-900 to -600
      Outputs     x=0
    """
    log(f"Building Standard: {STANDARD_PATH}")

    mat = _create_mat(STANDARD_NAME, MATERIALS_PATH)
    if not mat:
        return None

    mat.set_editor_property("blend_mode",    unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("two_sided",     False)

    MP = _get_mp()
    log(f"  MaterialProperty map: {MP}")

    # ------------------------------------------------------------------
    # Texture parameters
    # Sampler types match MaterializeTransientGenerator.cpp exactly
    # ------------------------------------------------------------------
    tex_base   = _tex_param(mat, "BaseColor", "Color",       "BaseColor", -1600, -700)
    tex_norm   = _tex_param(mat, "Normal",    "Normal",      "Normal",    -1600, -550)
    tex_orm    = _tex_param(mat, "ORM",       "LinearColor", "ORM",       -1600, -400)
    tex_height = _tex_param(mat, "Height",    "LinearColor", "Height",    -1600, -250)
    tex_emit   = _tex_param(mat, "Emissive",  "Color",       "Emissive",  -1600, -100)
    # Individual channel fallbacks — used by SetTextureParameterValue fallback path
    tex_rough  = _tex_param(mat, "Roughness", "LinearColor", "ORM",       -1600,  100)
    tex_metal  = _tex_param(mat, "Metallic",  "LinearColor", "ORM",       -1600,  250)
    tex_ao     = _tex_param(mat, "AO",        "LinearColor", "ORM",       -1600,  400)

    # ------------------------------------------------------------------
    # Scalar parameters — names MUST match SetScalarParameterValue calls
    # ------------------------------------------------------------------
    p_tiling        = _scalar_param(mat, "Tiling",           1.0,  -1200, -700)
    p_ao_power      = _scalar_param(mat, "AO_Power",         1.0,  -1200, -600)
    p_rough_mult    = _scalar_param(mat, "Roughness_Mult",   1.0,  -1200, -500)
    p_rough_offset  = _scalar_param(mat, "Roughness_Offset", 0.0,  -1200, -400)
    p_metal_mult    = _scalar_param(mat, "Metallic_Mult",    1.0,  -1200, -300)
    p_emit_power    = _scalar_param(mat, "Emissive_Power",   1.0,  -1200, -200)

    # ------------------------------------------------------------------
    # Vector parameters — name MUST match SetVectorParameterValue call
    # ------------------------------------------------------------------
    p_tint = _vector_param(mat, "Tint", 1.0, 1.0, 1.0, 1.0, -1200, -100)

    # ------------------------------------------------------------------
    # ORM component masks
    # ORM packing: R=AO, G=Roughness, B=Metallic
    # Use "RGB" as source pin (confirmed valid); ComponentMask dst uses "" (default first input)
    # ------------------------------------------------------------------
    mask_ao    = _mask(mat, True,  False, False, False, -900, -400)
    mask_rough = _mask(mat, False, True,  False, False, -900, -300)
    mask_metal = _mask(mat, False, False, True,  False, -900, -200)
    _connect(tex_orm, "", mask_ao,    MASK_INPUT_PIN)
    _connect(tex_orm, "", mask_rough, MASK_INPUT_PIN)
    _connect(tex_orm, "", mask_metal, MASK_INPUT_PIN)

    # ------------------------------------------------------------------
    # BaseColor * Tint
    # ------------------------------------------------------------------
    mul_base = _multiply(mat, -700, -700)
    _connect(tex_base, "RGB", mul_base, "A")
    _connect(p_tint,   "",    mul_base, "B")

    # ------------------------------------------------------------------
    # Roughness: clamp(ORM.G * Roughness_Mult + Roughness_Offset, 0, 1)
    # ------------------------------------------------------------------
    mul_rough = _multiply(mat, -700, -500)
    _connect(mask_rough,    "", mul_rough, "A")
    _connect(p_rough_mult,  "", mul_rough, "B")

    add_rough = _add(mat, -550, -500)
    _connect(mul_rough,     "", add_rough, "A")
    _connect(p_rough_offset,"", add_rough, "B")

    clamp_rough = _clamp(mat, -400, -500)
    _connect(add_rough, "", clamp_rough, CLAMP_INPUT_PIN)

    # ------------------------------------------------------------------
    # Metallic: ORM.B * Metallic_Mult
    # ------------------------------------------------------------------
    mul_metal = _multiply(mat, -700, -300)
    _connect(mask_metal,   "", mul_metal, "A")
    _connect(p_metal_mult, "", mul_metal, "B")

    # ------------------------------------------------------------------
    # AO: ORM.R ^ AO_Power  (approximated as multiply for material graph)
    # ------------------------------------------------------------------
    mul_ao = _multiply(mat, -700, -100)
    _connect(mask_ao,   "", mul_ao, "A")
    _connect(p_ao_power,"", mul_ao, "B")

    # ------------------------------------------------------------------
    # Emissive: Emissive * Emissive_Power
    # ------------------------------------------------------------------
    mul_emit = _multiply(mat, -700, 100)
    _connect(tex_emit,    "RGB", mul_emit, "A")
    _connect(p_emit_power,"",    mul_emit, "B")

    # ------------------------------------------------------------------
    # Connect to material outputs — use direct property assignment to
    # avoid connect_material_property enum mapping bugs
    # ------------------------------------------------------------------
    _connect_prop_direct(mat, mul_base,   "BASE_COLOR")
    _connect_prop_direct(mat, tex_norm,   "NORMAL")
    _connect_prop_direct(mat, clamp_rough,"ROUGHNESS")
    _connect_prop_direct(mat, mul_metal,  "METALLIC")
    _connect_prop_direct(mat, mul_ao,     "AMBIENT_OCCLUSION")
    _connect_prop_direct(mat, mul_emit,   "EMISSIVE_COLOR")

    unreal.EditorAssetLibrary.save_asset(STANDARD_PATH)
    log(f"OK: {STANDARD_PATH}")
    return mat


# ---------------------------------------------------------------------------
# Preset Master Materials
# ---------------------------------------------------------------------------

def build_preset_material(preset_key):
    """
    Build a preset material by duplicating Standard and adding preset-specific
    function call nodes and parameters on top.
    """
    defn = PRESET_DEFS[preset_key]
    asset_name = defn["asset"]
    asset_path = f"{PRESETS_PATH}/{asset_name}"

    log(f"Building preset '{preset_key}': {asset_path}")

    # Verify Standard exists before duplicating
    if not unreal.EditorAssetLibrary.does_asset_exist(STANDARD_PATH):
        log(f"ERROR: Standard material not found at {STANDARD_PATH}")
        log(f"       build_standard_material() must succeed first")
        return None

    _delete_if_exists(asset_path)

    # Duplicate Standard as the base — inherits all PBR params and connections
    # duplicate_asset(src_path, dest_path) where dest_path is the full asset path
    mat = unreal.EditorAssetLibrary.duplicate_asset(STANDARD_PATH, asset_path)
    if not mat:
        log(f"ERROR: Failed to duplicate Standard to {asset_path}")
        return None

    # Reload the duplicated asset so we can add expressions to it
    mat = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mat:
        log(f"ERROR: Failed to load duplicated asset {asset_path}")
        return None

    MP = _get_mp()

    # ------------------------------------------------------------------
    # Add preset-specific scalar parameters
    # ------------------------------------------------------------------
    y_scalar = 600
    for pname, pval in defn["scalars"]:
        sp = _create_expr(mat, unreal.MaterialExpressionScalarParameter, -1200, y_scalar)
        if sp:
            sp.set_editor_property("parameter_name", pname)
            sp.set_editor_property("default_value", float(pval))
        y_scalar += 100

    # ------------------------------------------------------------------
    # Add preset-specific vector parameters
    # ------------------------------------------------------------------
    y_vector = y_scalar + 50
    for vdef in defn["vectors"]:
        vname, r, g, b = vdef[0], vdef[1], vdef[2], vdef[3]
        vp = _create_expr(mat, unreal.MaterialExpressionVectorParameter, -1200, y_vector)
        if vp:
            vp.set_editor_property("parameter_name", vname)
            vp.set_editor_property("default_value", unreal.LinearColor(r, g, b, 1.0))
        y_vector += 100

    # ------------------------------------------------------------------
    # Add material function call nodes
    # Each function is placed to the right of the parameter columns and
    # optionally connected to a material output property.
    # ------------------------------------------------------------------
    y_func = -200
    for func_name, mp_name in defn["functions"]:
        func_asset = _load_function(func_name)
        if not func_asset:
            log(f"  SKIP: {func_name} (not found)")
            y_func += 200
            continue

        func_call = _create_expr(mat, unreal.MaterialExpressionMaterialFunctionCall, -300, y_func)
        if func_call:
            try:
                func_call.set_editor_property("material_function", func_asset)
            except Exception as e:
                log(f"  WARNING: Could not set material_function on call node: {e}")

            # Connect to material output if specified
            mp = _resolve_mp(mp_name)
            if mp is not None:
                _connect_prop(func_call, "", mp)

        y_func += 200

    unreal.EditorAssetLibrary.save_asset(asset_path)
    log(f"OK: {asset_path}")
    return mat


# ---------------------------------------------------------------------------
# Post-build: wire default textures into all master materials
# ---------------------------------------------------------------------------

def wire_default_textures_into_material(mat, mat_path):
    """
    Walk all TextureSampleParameter2D expressions in the material and assign
    the correct default texture based on parameter name.
    This ensures sampler types never mismatch at runtime.
    """
    param_to_default = {
        "BaseColor": "BaseColor",
        "Normal":    "Normal",
        "ORM":       "ORM",
        "Height":    "Height",
        "Emissive":  "Emissive",
        "Roughness": "ORM",
        "Metallic":  "ORM",
        "AO":        "ORM",
    }

    changed = False
    try:
        exprs = unreal.MaterialEditingLibrary.get_material_expressions(mat)
        for expr in exprs:
            if not isinstance(expr, unreal.MaterialExpressionTextureSampleParameter2D):
                continue
            pname = str(expr.get_editor_property("parameter_name"))
            default_key = param_to_default.get(pname)
            if not default_key:
                continue
            tex = _load_default_tex(default_key)
            if tex:
                expr.set_editor_property("texture", tex)
                changed = True
    except Exception as e:
        log(f"  WARNING: wire_default_textures failed for {mat_path}: {e}")

    if changed:
        unreal.EditorAssetLibrary.save_asset(mat_path)
        log(f"  Default textures wired: {mat_path}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    log("=" * 60)
    log("Materialize Preset Material Generator")
    log("=" * 60)

    # Ensure directories exist
    _ensure_dir(MATERIALS_PATH)
    _ensure_dir(PRESETS_PATH)

    results = {}

    # 1. Standard master material
    log("")
    log("--- Standard ---")
    std_mat = build_standard_material()
    results["Standard"] = "OK" if std_mat else "FAILED"

    if std_mat:
        wire_default_textures_into_material(std_mat, STANDARD_PATH)

    # 2. Preset materials (Metal, Glossy, Toon)
    for preset_key in PRESET_DEFS:
        log("")
        log(f"--- {preset_key} ---")
        preset_mat = build_preset_material(preset_key)
        results[preset_key] = "OK" if preset_mat else "FAILED"

        if preset_mat:
            asset_path = f"{PRESETS_PATH}/{PRESET_DEFS[preset_key]['asset']}"
            wire_default_textures_into_material(preset_mat, asset_path)

    # Summary
    log("")
    log("=" * 60)
    log("Results:")
    for name, status in results.items():
        log(f"  {status}: {name}")

    failed = [n for n, s in results.items() if s == "FAILED"]
    if failed:
        log(f"\nWARNING: {len(failed)} material(s) failed.")
        log("Check that generate_placeholder_textures.py and")
        log("generate_material_functions.py ran successfully first.")
    else:
        log(f"\nAll {len(results)} materials created successfully.")

    log("")
    log("Asset paths registered in C++ FMaterializeMaterialLoader:")
    log(f"  Standard : {STANDARD_PATH}")
    for pk, pd in PRESET_DEFS.items():
        log(f"  {pk:<8} : {PRESETS_PATH}/{pd['asset']}")
    log("=" * 60)


if __name__ == "__main__":
    main()
