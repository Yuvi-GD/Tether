#!/usr/bin/env python3
"""Assemble the source SDK and generate its portable shader headers."""

import argparse
import platform
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CHITRA = ROOT / "third_party" / "chitra"
SHADERS = "glsl410:hlsl5:metal_macos:metal_ios:metal_sim:glsl300es:wgsl"


def shader_compiler():
    system = platform.system()
    machine = platform.machine().lower()
    if system == "Windows":
        tool = "win32/sokol-shdc.exe"
    elif system == "Darwin":
        tool = ("osx_arm64" if machine in ("arm64", "aarch64") else "osx") + "/sokol-shdc"
    elif system == "Linux":
        tool = ("linux_arm64" if machine in ("arm64", "aarch64") else "linux") + "/sokol-shdc"
    else:
        raise RuntimeError(f"Unsupported shader compiler host: {system} {machine}")
    return CHITRA / "third_party" / "sokol-tools" / "bin" / tool


def copy_tree(source, destination):
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns(".git", "__pycache__"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path, help="new source SDK directory")
    args = parser.parse_args()
    destination = args.destination.resolve()
    destination.mkdir(parents=True, exist_ok=False)

    for directory in ("include", "src", "cmake", "web"):
        copy_tree(ROOT / directory, destination / directory)
    for filename in ("CMakeLists.txt", "LICENSE"):
        shutil.copy2(ROOT / filename, destination / filename)
    shutil.copy2(ROOT / ".github" / "source-sdk-README.md", destination / "README.md")

    yaml = destination / "third_party" / "libyaml"
    for directory in ("include", "src", "cmake"):
        copy_tree(ROOT / "third_party" / "libyaml" / directory, yaml / directory)
    for filename in ("CMakeLists.txt", "yamlConfig.cmake.in", "License"):
        shutil.copy2(ROOT / "third_party" / "libyaml" / filename, yaml / filename)

    chitra = destination / "third_party" / "chitra"
    copy_tree(CHITRA / "include", chitra / "include")
    copy_tree(CHITRA / "src", chitra / "src")
    for filename in ("CMakeLists.txt", "LICENSE"):
        shutil.copy2(CHITRA / filename, chitra / filename)
    for directory in ("sokol", "fontstash", "stb"):
        source = CHITRA / "third_party" / directory
        target = chitra / "third_party" / directory
        target.mkdir(parents=True)
        for file in source.glob("*.h"):
            shutil.copy2(file, target / file.name)
        if (source / "LICENSE").is_file():
            shutil.copy2(source / "LICENSE", target / "LICENSE")

    output = chitra / "src" / "backends" / "shaders" / "gen"
    output.mkdir(parents=True, exist_ok=True)
    compiler = shader_compiler()
    if not compiler.is_file():
        raise FileNotFoundError(f"Shader compiler missing: {compiler}")
    for name in ("chitra_shape.glsl", "chitra_text.glsl"):
        subprocess.run([str(compiler), "-i", str(CHITRA / "src" / "backends" / "shaders" / name),
                        "-o", str(output / (name + ".h")), "-l", SHADERS], check=True)


if __name__ == "__main__":
    main()
