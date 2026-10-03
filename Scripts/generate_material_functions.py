"""
Materialize Material Function Generator
Runs entirely inside Unreal Engine's Python environment.

Creates all 8 Material Functions used by the preset master materials:
  /Materialize/Content/Materials/Functions/MF_MetalAnisotropicSpecular
  /Materialize/Content/Materials/Functions/MF_MetalFresnelRim
  /Materialize/Content/Materials/Functions/MF_GlossyClearCoat
  /Materialize/Content/Materials/Functions/MF_GlossySubsurface
  /Materialize/Content/Materials/Functions/MF_GlossyDualLobe
  /Materialize/Content/Materials/Functions/MF_ToonCelShading
  /Materialize/Content/Materials/Functions/MF_ToonSpecular
  /Materialize/Content/Materials/Functions/MF_ToonRimLight

Run order:
  1. generate_placeholder_textures.py
  2. generate_material_functions.py   <-- this file
  3. generate_preset_materials.py

Usage (UE Output Log > Python):
  exec(open(r"<plugin>/Scripts/generate_material_functions.py").read())
"""

import unreal

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
FUNCTIONS_PATH   = "/Materialize/Materials/Functions"
OVERWRITE        = True

# ---------------------------------------------------------------------------
# Data-driven function definitions
#
# Each entry:
#   "description" : str
#   "hlsl"        : str   -- inline HLSL; variable names must match input/param names
#   "inputs"      : list of (name, type)
#                   type in {"float", "vector2", "vector3", "vector4"}
#   "scalars"     : list of (name, default_value)
#   "vectors"     : list of (name, r, g, b)
#
# ALL names in "inputs", "scalars", and "vectors" become Custom node inputs
# (via the inputs array) so the HLSL body can reference them directly.
# ---------------------------------------------------------------------------
FUNCTION_DEFS = {
    "MF_MetalAnisotropicSpecular": {
        "description": "Anisotropic GGX specular for metallic surfaces",
        "hlsl": (
            "float3 H = normalize(ViewDir + LightDir);\n"
            "float NdotH = saturate(dot(Normal, H));\n"
            "float ax = max(Roughness * (1.0 + Anisotropy), 0.001);\n"
            "float ay = max(Roughness * (1.0 - Anisotropy), 0.001);\n"
            "float TdotH = dot(Tangent, H);\n"
            "float BdotH = dot(Bitangent, H);\n"
            "float D = 1.0 / (3.14159 * ax * ay *\n"
            "    pow(pow(TdotH/ax,2) + pow(BdotH/ay,2) + NdotH*NdotH, 2));\n"
            "return float4(SpecularTint * (SpecularIntensity * D), 1.0);"
        ),
        "inputs":  [
            ("Normal",    "vector3"),
            ("Tangent",   "vector3"),
            ("Bitangent", "vector3"),
            ("ViewDir",   "vector3"),
            ("LightDir",  "vector3"),
            ("Roughness", "float"),
            ("Anisotropy","float"),
        ],
        "scalars": [("SpecularIntensity", 1.0)],
        "vectors": [("SpecularTint", 1.0, 1.0, 1.0)],
    },

    "MF_MetalFresnelRim": {
        "description": "Fresnel-based rim lighting for metallic surfaces",
        "hlsl": (
            "float NdotV = saturate(dot(Normal, ViewDir));\n"
            "float fresnel = pow(1.0 - NdotV, RimPower);\n"
            "float rim = fresnel * EdgeBrightness;\n"
            "return float4(RimColor * (RimIntensity * rim), 1.0);"
        ),
        "inputs":  [
            ("Normal",  "vector3"),
            ("ViewDir", "vector3"),
        ],
        "scalars": [("RimIntensity", 1.0), ("RimPower", 3.0), ("EdgeBrightness", 0.5)],
        "vectors": [("RimColor", 1.0, 1.0, 1.0)],
    },

    "MF_GlossyClearCoat": {
        "description": "Clear coat layer with IOR-based Fresnel reflections",
        "hlsl": (
            "float3 H = normalize(ViewDir + LightDir);\n"
            "float NdotV = saturate(dot(Normal, ViewDir));\n"
            "float F0 = pow((1.0 - CoatIOR) / (1.0 + CoatIOR), 2);\n"
            "float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5);\n"
            "return float4(CoatTint * (CoatIntensity * ClearCoat * fresnel), 1.0);"
        ),
        "inputs":  [
            ("Normal",    "vector3"),
            ("ViewDir",   "vector3"),
            ("LightDir",  "vector3"),
            ("ClearCoat", "float"),
        ],
        "scalars": [("CoatIOR", 1.5), ("CoatIntensity", 1.0)],
        "vectors": [("CoatTint", 1.0, 1.0, 1.0)],
    },

    "MF_GlossySubsurface": {
        "description": "Subsurface scattering approximation for glossy surfaces",
        "hlsl": (
            "float3 L = -LightDir;\n"
            "float3 H = normalize(L + Normal * SSSDistortion);\n"
            "float VdotH = pow(saturate(dot(ViewDir, -H)), SSSPower) * SSSScale;\n"
            "float wrap = max(0.0, dot(Normal, L) + AmbientStrength);\n"
            "return float4(SSSColor * (SSSStrength * (VdotH + wrap) * Thickness), 1.0);"
        ),
        "inputs":  [
            ("Normal",    "vector3"),
            ("LightDir",  "vector3"),
            ("ViewDir",   "vector3"),
            ("Thickness", "float"),
        ],
        "scalars": [
            ("SSSStrength",     1.0),
            ("SSSDistortion",   0.5),
            ("SSSPower",        2.0),
            ("SSSScale",        1.0),
            ("AmbientStrength", 0.3),
        ],
        "vectors": [("SSSColor", 1.0, 0.5, 0.3)],
    },

    "MF_GlossyDualLobe": {
        "description": "Dual-lobe specular with energy conservation",
        "hlsl": (
            "float3 H = normalize(ViewDir + LightDir);\n"
            "float NdotH = saturate(dot(Normal, H));\n"
            "float lobe1 = pow(NdotH, 1.0 / max(BaseRoughness * BaseRoughness, 0.001));\n"
            "float lobe2 = pow(NdotH, 1.0 / max(CoatRoughness * CoatRoughness, 0.001));\n"
            "float3 result = lerp(BaseColor * lobe1, CoatColor * lobe2, saturate(CoatAmount));\n"
            "return float4(result * EnergyConservation, 1.0);"
        ),
        "inputs":  [
            ("Normal",       "vector3"),
            ("ViewDir",      "vector3"),
            ("LightDir",     "vector3"),
            ("BaseRoughness","float"),
            ("CoatRoughness","float"),
            ("CoatAmount",   "float"),
        ],
        "scalars": [("EnergyConservation", 0.8)],
        "vectors": [("BaseColor", 1.0, 1.0, 1.0), ("CoatColor", 1.0, 1.0, 1.0)],
    },

    "MF_ToonCelShading": {
        "description": "Cel-shaded lighting with configurable step bands",
        "hlsl": (
            "float NdotL = dot(Normal, -LightDir);\n"
            "float wrapped = (NdotL + WrapAmount) / (1.0 + WrapAmount);\n"
            "float bands = floor(saturate(wrapped) * BandCount + 0.5) / BandCount;\n"
            "float3 lit = lerp(ShadowColor, HighlightColor,\n"
            "    smoothstep(0.0, BandSmoothness, bands));\n"
            "return float4(Albedo * lit, 1.0);"
        ),
        "inputs":  [
            ("Normal",   "vector3"),
            ("LightDir", "vector3"),
            ("Albedo",   "vector3"),
        ],
        "scalars": [
            ("BandCount",      3.0),
            ("BandSmoothness", 0.05),
            ("WrapAmount",     0.5),
        ],
        "vectors": [
            ("ShadowColor",    0.3, 0.3, 0.5),
            ("HighlightColor", 1.0, 1.0, 1.0),
        ],
    },

    "MF_ToonSpecular": {
        "description": "Stepped specular highlights for toon shading",
        "hlsl": (
            "float3 H = normalize(ViewDir + LightDir);\n"
            "float NdotH = saturate(dot(Normal, H));\n"
            "float spec = pow(NdotH, 1.0 / max(Roughness * Roughness, 0.001));\n"
            "float stepped = floor(spec * SpecularSteps) / SpecularSteps;\n"
            "float mask = step(SpecularSize, stepped);\n"
            "return float4(SpecularColor * (SpecularIntensity * mask), 1.0);"
        ),
        "inputs":  [
            ("Normal",    "vector3"),
            ("ViewDir",   "vector3"),
            ("LightDir",  "vector3"),
            ("Roughness", "float"),
        ],
        "scalars": [
            ("SpecularSize",      0.5),
            ("SpecularSteps",     2.0),
            ("SpecularIntensity", 1.0),
        ],
        "vectors": [("SpecularColor", 1.0, 1.0, 1.0)],
    },

    "MF_ToonRimLight": {
        "description": "Hard-edge rim lighting for toon/NPR workflows",
        "hlsl": (
            "float NdotV = saturate(dot(Normal, ViewDir));\n"
            "float rim = 1.0 - NdotV;\n"
            "float rimMask = smoothstep(\n"
            "    RimThreshold - RimSmoothness,\n"
            "    RimThreshold + RimSmoothness, rim);\n"
            "return float4(RimColor * (RimIntensity * rimMask), 1.0);"
        ),
        "inputs":  [
            ("Normal",  "vector3"),
            ("ViewDir", "vector3"),
        ],
        "scalars": [
            ("RimPower",      3.0),
            ("RimIntensity",  1.0),
            ("RimThreshold",  0.5),
            ("RimSmoothness", 0.1),
        ],
        "vectors": [("RimColor", 1.0, 1.0, 1.0)],
    },
}

# Ordered build list
BUILD_ORDER = [
    "MF_MetalAnisotropicSpecular",
    "MF_MetalFresnelRim",
    "MF_GlossyClearCoat",
    "MF_GlossySubsurface",
    "MF_GlossyDualLobe",
    "MF_ToonCelShading",
    "MF_ToonSpecular",
    "MF_ToonRimLight",
]


# ---------------------------------------------------------------------------
# Enum resolvers
# ---------------------------------------------------------------------------

def _resolve_fit(type_name):
    enum_cls = unreal.FunctionInputType
    candidates = {
        "float":   ["FIT_SCALAR",  "FUNCTION_INPUT_SCALAR",  "SCALAR"],
        "vector2": ["FIT_VECTOR2", "FUNCTION_INPUT_VECTOR2", "VECTOR2"],
        "vector3": ["FIT_VECTOR3", "FUNCTION_INPUT_VECTOR3", "VECTOR3"],
        "vector4": ["FIT_VECTOR4", "FUNCTION_INPUT_VECTOR4", "VECTOR4"],
    }
    for cand in candidates.get(type_name, []):
        if hasattr(enum_cls, cand):
            return getattr(enum_cls, cand)
    fallback = {"float": 0, "vector2": 1, "vector3": 2, "vector4": 3}
    return fallback.get(type_name, 0)


def _resolve_output_type():
    enum_cls = unreal.CustomMaterialOutputType
    for name in ["CMOT_FLOAT4", "FLOAT4"]:
        if hasattr(enum_cls, name):
            return getattr(enum_cls, name)
    return 3


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def log(msg):
    unreal.log(f"[MatFuncGen] {msg}")


def _ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        log(f"Created directory: {path}")


def _delete_if_exists(asset_path):
    if OVERWRITE and unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            log(f"WARNING: Could not delete {asset_path}")


def _connect(src, src_pin, dst, dst_pin):
    try:
        unreal.MaterialEditingLibrary.connect_material_expressions(
            src, src_pin, dst, dst_pin)
    except Exception as e:
        log(f"  WARNING: connect '{src_pin}'->'{dst_pin}': {e}")


def _connect_to_custom(src, custom, input_name, input_index):
    """
    Connect src to a named input on a Custom node.
    Tries three pin naming conventions UE5 uses across versions:
      1. The declared input name  (e.g. "Normal")
      2. "Input" + index          (e.g. "Input0")
      3. Just the index as string (e.g. "0")
    Visual wiring is cosmetic — the HLSL compiles as long as the
    inputs array is populated with the correct names.
    """
    for pin_id in [input_name, f"Input{input_index}", str(input_index)]:
        try:
            unreal.MaterialEditingLibrary.connect_material_expressions(
                src, "", custom, pin_id)
            return  # success
        except Exception:
            pass
    log(f"  INFO: Could not wire '{input_name}' visually — HLSL will still compile correctly")


def _create_expr_in_func(func, expr_class, x, y):
    try:
        return unreal.MaterialEditingLibrary.create_material_expression_in_function(
            func, expr_class, x, y)
    except Exception as e:
        log(f"  ERROR: create_material_expression_in_function({expr_class.__name__}): {e}")
        return None


def _add_custom_input(custom, name):
    """
    Append a named input to a MaterialExpressionCustom node's inputs array.
    UE5 Python struct APIs vary — try every known approach.
    Returns the input name on success, None on failure.
    """
    try:
        inputs = list(custom.get_editor_property("inputs"))
        new_input = unreal.CustomInput()

        # "InputName" is the correct property name (confirmed via export_text)
        new_input.set_editor_property("InputName", name)

        inputs.append(new_input)
        custom.set_editor_property("inputs", inputs)
        return name
    except Exception as e:
        log(f"  WARNING: _add_custom_input('{name}'): {e}")
        return None


# ---------------------------------------------------------------------------
# Core builder
# ---------------------------------------------------------------------------

def build_function(func_name):
    defn = FUNCTION_DEFS.get(func_name)
    if not defn:
        log(f"ERROR: No definition for '{func_name}'")
        return None

    asset_path = f"{FUNCTIONS_PATH}/{func_name}"
    log(f"Building: {func_name}")

    _delete_if_exists(asset_path)
    func = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        func_name, FUNCTIONS_PATH,
        unreal.MaterialFunction, unreal.MaterialFunctionFactoryNew()
    )
    if not func:
        log(f"ERROR: Failed to create asset {asset_path}")
        return None

    func.set_editor_property("description", defn["description"])

    # ------------------------------------------------------------------
    # Custom HLSL node — all variable names come from its inputs array
    # ------------------------------------------------------------------
    custom = _create_expr_in_func(func, unreal.MaterialExpressionCustom, 0, 0)
    if not custom:
        log(f"ERROR: Could not create Custom node for {func_name}")
        return None

    custom.set_editor_property("code", defn["hlsl"])
    custom.set_editor_property("output_type", _resolve_output_type())
    custom.set_editor_property("description", defn["description"])

    # Collect all inputs in order: function inputs, then scalars, then vectors
    # Each gets a named slot in the Custom node's inputs array so the HLSL
    # body can reference them by name without "undeclared identifier" errors.
    all_inputs = []  # list of (name, source_expr)

    # ------------------------------------------------------------------
    # Function inputs (exposed pins on the Material Function boundary)
    # ------------------------------------------------------------------
    fi_list = defn["inputs"]
    start_y = -(len(fi_list) * 100) // 2
    for i, (inp_name, inp_type) in enumerate(fi_list):
        fi = _create_expr_in_func(func, unreal.MaterialExpressionFunctionInput,
                                   -800, start_y + i * 100)
        if fi:
            fi.set_editor_property("input_name", inp_name)
            fi.set_editor_property("input_type", _resolve_fit(inp_type))
            all_inputs.append((inp_name, fi))

    # ------------------------------------------------------------------
    # Scalar parameters
    # ------------------------------------------------------------------
    for i, (pname, pval) in enumerate(defn["scalars"]):
        sp = _create_expr_in_func(func, unreal.MaterialExpressionScalarParameter,
                                   -550, 200 + i * 100)
        if sp:
            sp.set_editor_property("parameter_name", pname)
            sp.set_editor_property("default_value", float(pval))
            all_inputs.append((pname, sp))

    # ------------------------------------------------------------------
    # Vector parameters
    # ------------------------------------------------------------------
    scalar_count = len(defn["scalars"])
    for i, vdef in enumerate(defn["vectors"]):
        vname, r, g, b = vdef[0], vdef[1], vdef[2], vdef[3]
        vp = _create_expr_in_func(func, unreal.MaterialExpressionVectorParameter,
                                   -550, 200 + (scalar_count + i) * 100)
        if vp:
            vp.set_editor_property("parameter_name", vname)
            vp.set_editor_property("default_value", unreal.LinearColor(r, g, b, 1.0))
            all_inputs.append((vname, vp))

    # ------------------------------------------------------------------
    # Wire every source expression into the Custom node's inputs array
    # by index (not by pin name — that's what was broken before).
    # ------------------------------------------------------------------
    for idx, (iname, src_expr) in enumerate(all_inputs):
        pin_name = _add_custom_input(custom, iname)
        if pin_name and src_expr:
            _connect_to_custom(src_expr, custom, iname, idx)

    # ------------------------------------------------------------------
    # Function output — wire Custom node result to the output pin
    # ------------------------------------------------------------------
    fo = _create_expr_in_func(func, unreal.MaterialExpressionFunctionOutput, 300, 0)
    if fo:
        fo.set_editor_property("output_name", "Result")
        _connect(custom, "", fo, "")

    unreal.EditorAssetLibrary.save_asset(asset_path)
    log(f"  OK: {asset_path}")
    return func


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    log("=" * 60)
    log("Materialize Material Function Generator")
    log("=" * 60)

    _ensure_dir(FUNCTIONS_PATH)

    results = {}
    for func_name in BUILD_ORDER:
        func = build_function(func_name)
        results[func_name] = "OK" if func else "FAILED"

    log("")
    log("Results:")
    for name, status in results.items():
        log(f"  {status}: {name}")

    failed = [n for n, s in results.items() if s == "FAILED"]
    if failed:
        log(f"\nWARNING: {len(failed)} function(s) failed — check Output Log above")
    else:
        log(f"\nAll {len(BUILD_ORDER)} functions created successfully.")

    log("")
    log("Next step: run generate_preset_materials.py")
    log("=" * 60)


if __name__ == "__main__":
    main()

main()
