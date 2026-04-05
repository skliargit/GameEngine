import os
import sys
import textwrap
import subprocess as proc
from pathlib import Path

# Настройки путей.
SRC_DIR = "src/"
OBJ_DIR = "../bin/objs/engine/"
BIN_DIR = "../bin/"

# Настройки целевого файла.
TARGET = "engine"

def parse_arguments(index: int, count: int) -> list:
    """
    Получает аргументы командной строки
    -----------------------------------------------------
    index - элемент с которого начать считывать аргументы
    count - количество считываемых аргументов
    """
    args = sys.argv[index:index+count+1]
    return args + [None] * (count - len(args))

def compile_source_files(common_flags, object_flags, linker_flags, define_flags, include_flags, output_file):
    """
    Общая функция для компиляции исходных файлов
    ----------------------------------------------------------------------------
    common_flags  - общие флаги для компиляции файлов и сборки целевого файла
    object_flags  - флаги компиляции для исходных файлов
    linker_flags  - флаги сборки для целевого файла
    define_flags  - флаги объявлений имен для исходных файлов
    include_flags - флаги с директориями заголовочных файлов для исходных файлов
    output_file   - путь для сохранения целевого файла после сборки
    """
    # Получение списка исходных и объектных файлов.
    src_files = [str(path).replace("\\","/") for path in Path(SRC_DIR).rglob("*.c")]
    obj_files = ""
    # Флаги состояния процесса компиляции и сборки.
    exists_target_file  = os.path.exists(output_file)
    rebuild_target_file = False
    compile_error_flag  = False

    # Процесс компиляции каждого исходного файла.
    for src_file in src_files:
        # Получение пути объектного файла.
        obj_file = src_file.replace(SRC_DIR, OBJ_DIR).replace(".c",".o")
        # Создание списка объектных файлов для создание цели.
        obj_files += f" {obj_file}"
        # Пропустить компиляцию, если файл существует или метка времени объектного файла выше чем у исходного.
        if os.path.exists(obj_file) and os.path.getmtime(obj_file) >= os.path.getmtime(src_file):
            continue
        # Создание директории для объектного файла.
        os.makedirs(os.path.dirname(obj_file), exist_ok=True)
        # Требование пересборки целевого файла.
        rebuild_target_file = True
        # Компиляция исходного файла.
        compile_cmd = f"clang {common_flags} {object_flags} {define_flags} {include_flags} -c {src_file} -o {obj_file}"

        if proc.run(compile_cmd, shell=True).returncode == 0:
            print(f" + Compile {src_file}")
        else:
            compile_error_flag = True

    # Проверка наличия ошибок в процессе компиляции файлов.
    if compile_error_flag:
        sys.exit(1)

    # Процесс сборки целевого файла.
    if rebuild_target_file or not exists_target_file:
        # Сборка целевого файла.
        build_cmd = f"clang {common_flags} {linker_flags} {obj_files} -o {output_file}"

        if not proc.run(build_cmd, shell=True).returncode == 0:
            sys.exit(1)

        # Вывод результата сборки.
        if exists_target_file:
            print(" = Has been updated")
        else:
            print(" = Assembled")
    else:
        print(" = No changes found")

def linux_generate_source_files():

    protocol_files = [
        "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml",
        "/usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml",
        "/usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml"
    ]

    header_files = [
        "src/platform/linux/wayland_protocols/xdg_shell.h",
        "src/platform/linux/wayland_protocols/relative_pointer.h",
        "src/platform/linux/wayland_protocols/pointer_constraints.h"
    ]

    source_files = [
        "src/platform/linux/wayland_protocols/xdg_shell.c",
        "src/platform/linux/wayland_protocols/relative_pointer.c",
        "src/platform/linux/wayland_protocols/pointer_constraints.c"
    ]

    for protocol_file, header_file, source_file in zip(protocol_files, header_files, source_files):
        # Проверка существования файлов.
        if not os.path.exists(protocol_file):
            print(f" !WARN! Wayland protocol file not found at '{protocol_file}'")
            continue

        # Генерация заголовочного файла при необходимости.
        if not os.path.exists(header_file) or os.path.getmtime(header_file) < os.path.getmtime(protocol_file):
            # Создание директории для файла.
            os.makedirs(os.path.dirname(header_file), exist_ok=True)

            # Генерация файла.
            if proc.run(f"wayland-scanner client-header {protocol_file} {header_file}", shell=True).returncode != 0:
                print(f" !ERROR! Failed to generate header file at '{header_file}'")
                sys.exit(1)
            else:
                print(f" + Generate {header_file}")

        # Генерация исходного файла при необходимости.
        if not os.path.exists(source_file) or os.path.getmtime(source_file) < os.path.getmtime(protocol_file):
            # Создание директории для файла.
            os.makedirs(os.path.dirname(source_file), exist_ok=True)

            # Генерация временного файла.
            if proc.run(f"wayland-scanner private-code {protocol_file} {source_file}.tmp", shell=True).returncode != 0:
                print(f" !ERROR! Failed to generate source file at '{source_file}.tmp'")
                sys.exit(1)

            # Чтение временного файла.
            with open(f"{source_file}.tmp", "r", encoding="utf-8") as file:
                source_code = textwrap.indent(file.read(), '    ')

            # Запись файла.
            with open(source_file, "w", encoding="utf-8") as file:
                file.write("// Auto-generated by BuildSystem\n")
                file.write("// Do not edit manually\n")
                file.write("\n#include <core/defines.h>\n")
                file.write("\n#ifdef PLATFORM_LINUX_FLAG\n")
                file.write(source_code)
                file.write("\n#endif\n")

            # Удаление временного файла.
            os.remove(f"{source_file}.tmp")

            print(f" + Generate {source_file}")

def linux_compile_source_files():
    compile_source_files(
        common_flags  = "-fPIC",
        object_flags  = "-fvisibility=hidden -g -Wall -Wextra -Werror -Wvla -Wreturn-type",
        linker_flags  = "-shared $(pkg-config --libs xcb xcb-xinput wayland-client wayland-cursor xkbcommon xkbcommon-x11 vulkan)",
        define_flags  = "-DMAKE_LIB_FLAG -DDEBUG_FLAG -DDEBUG_PLATFORM_FLAG",
        include_flags = f"-I{SRC_DIR}",
        output_file   = f"{BIN_DIR}lib{TARGET}.so"
    )

def windows_compile_source_files():
    compile_source_files(
        common_flags  = "-fdeclspec",
        object_flags  = "-g -Wall -Wextra -Werror -Wvla -Wreturn-type",
        linker_flags  = "-shared -luser32 -lgdi32 -lwinmm -lvulkan-1",
        define_flags  = "-DMAKE_LIB_FLAG -DDEBUG_FLAG -DDEBUG_PLATFORM_FLAG",
        include_flags = f"-I{SRC_DIR}",
        output_file   = f"{BIN_DIR}{TARGET}.dll"
    )

# Точка выполнения скрипта.
def main():
    """Точка начала выполенния скрипта"""
    [build_type, system] = parse_arguments(1,2)

    if system == "linux":
        linux_generate_source_files()
        linux_compile_source_files()
    elif system == "windows":
        windows_compile_source_files()
    else:
        print(f"Error: Unknown system named '{system}'")
        sys.exit(1)

if __name__ == "__main__":
    main()
