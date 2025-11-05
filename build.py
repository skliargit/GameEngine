#!/usr/bin/env python3

import os
import sys
import platform
import shutil as sh
import subprocess as proc
from pathlib import Path

# Определение платформы.
SYSTEM = platform.system().lower()
ARCH   = platform.machine().lower()

# Настройки путей.
BUILD_DIR = "bin/"

# Общие зависимости для всех операционных систем.
REQUIRED_TOOLS = ["clang", "glslc"]
REQUIRED_LIBS  = []

# Зависимости для Linux.
if SYSTEM == "linux":
    REQUIRED_TOOLS += ["pkg-config", "wayland-scanner"]
    REQUIRED_LIBS  += ["wayland-client", "xcb", "xkbcommon", "vulkan"]

# Зависимости для Windows.
elif SYSTEM == "windows":
    REQUIRED_TOOLS += []
    REQUIRED_LIBS  += ["user32", "gdi32", "winmm", "vulkan-1"]

def check_dependencies():
    """Проверяет наличие требуемых зависимостей"""
    missing_tools = []
    missing_libs  = []

    for name in REQUIRED_TOOLS:
        if sh.which(name) is None:
            missing_tools.append(name)

    for name in REQUIRED_LIBS:
        if SYSTEM == "linux" and proc.run(["pkg-config", "--exists", name], capture_output=True).returncode != 0:
            missing_libs.append(name)

        elif SYSTEM == "windows":
            print(" ! Library check on Windows will be done during compilation.")
            break

    if len(missing_tools) != 0:
        print(f"Error: Missing tools: {', '.join(missing_tools)}")

    if len(missing_libs) != 0:
        print(f"Error: Missing libraries: {', '.join(missing_libs)}")

    if len(missing_tools) != 0 or len(missing_libs) != 0:
        print("Error: Please install the missing tools and libraries before building. Aborted.")
        sys.exit(1)

def run_script(path: str, build_type: str):
    """Выполняет указанный скрипт"""
    if not os.path.exists(path):
        print(f"Error: No script '{path}' found. Aborted.")
        sys.exit(1)

    if proc.run([sys.executable, Path(path).name, build_type, SYSTEM], cwd=Path(path).parent).returncode != 0:
        print(f"Error: Failed to proccess script '{path}'. Aborted.")
        sys.exit(1)

def build(build_type: str = "debug"):
    """Выполняет сборку всего проекта"""

    # TODO: Если собран debug, то при сборке release выполнить очистку.
    #       Создать файл с именем debug.lock/release.lock

    print("Checking dependencies...")
    check_dependencies()

    print("Creating build directory...")
    os.makedirs(BUILD_DIR, exist_ok=True)

    print("Building engine library...")
    run_script("engine/build.py", build_type)

    print("Building testapp...")
    run_script("testapp/build.py", build_type)

    print("Building shaders...")
    run_script("assets/build.py", build_type)

    print("Building completed successfully!")

def clean():
    """Выполняет очистку всего проекта"""
    print("Cleaning build directory...")

    if os.path.exists(BUILD_DIR):
        sh.rmtree(BUILD_DIR)

def help():
    """
    Usage:
        python build.py [command]

    Commands:
        help      - Show this help message
        debug     - Build in debug mode
        release   - Build in release mode
        clean     - Clean build artifacts
    """
    print(help.__doc__)
    print(f"Current OS: {SYSTEM.capitalize()} ({ARCH})")

def parse_arguments(index: int, count: int) -> list:
    """
    Получает аргументы командной строки.
    index - элемент с которого начать считывать аргументы
    count - количество считываемых аргументов
    """
    args = sys.argv[index:index+count+1]
    return args + [None] * (count - len(args))

def main():
    """Точка начала выполенния скрипта"""
    [command] = parse_arguments(1,1)

    if command is None or command == "help":
        help()

    elif command in ["debug", "release"]:
        build(command)

    elif command == "clean":
        clean()

    else:
        print(f"Error: Unknown command '{command}'")
        help()
        sys.exit(1)

if __name__ == "__main__":
    main()
