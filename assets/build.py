import os
import sys
import subprocess as proc
from pathlib import Path

# Настройки путей.
SRC_DIR = "shaders/"
OBJ_DIR = "shaders/"

def parse_arguments(index: int, count: int) -> list:
    """
    Получает аргументы командной строки
    -----------------------------------------------------
    index - элемент с которого начать считывать аргументы
    count - количество считываемых аргументов
    """
    args = sys.argv[index:index+count+1]
    return args + [None] * (count - len(args))

def compile_shader_files(shader_stage):
    """
    Функция компиляции шейдеров
    - вершиных (shader_stage=vert)
    - фрагментных (shader_stage=frag)
    """
    # Получение списка исходных файлов.
    src_files = [str(path).replace("\\","/") for path in Path(SRC_DIR).rglob(f"*.{shader_stage}")]
    # Флаги состояния процесса компиляции.
    rebuild_detected_flag = False
    compile_error_flag    = False

    # Процесс компиляции каждого исходного файла.
    for src_file in src_files:
        # Получение пути объектного файла.
        obj_file = f"{src_file}.spv"

        # Пропустить компиляцию, если файл существует или метка времени объектного файла выше чем у исходного.
        if os.path.exists(obj_file) and os.path.getmtime(obj_file) >= os.path.getmtime(src_file):
            print(f" ! Already exists {obj_file}")
            continue

        # Требование пересборки целевого файла.
        rebuild_detected_flag = True

        # Компиляция исходного файла.
        compile_cmd = f"glslc -fshader-stage={shader_stage} -c {src_file} -o {obj_file}"

        if proc.run(compile_cmd, shell=True).returncode == 0:
            print(f" + Compile {src_file}")
        else:
            compile_error_flag = True

    # Проверка наличия ошибок в процессе компиляции файлов.
    if compile_error_flag:
        sys.exit(1)

    # Вывод результата сборки.
    if rebuild_detected_flag:
        print(" = Has been updated")
    else:
        print(" = No changes found")

# Точка выполнения скрипта.
def main():
    """Точка начала выполенния скрипта"""
    [build_type, system] = parse_arguments(1,2)

    if system == "linux" or system == "windows":
        compile_shader_files("vert")
        compile_shader_files("frag")
    else:
        print(f"Error: Unknown system named '{system}'")
        sys.exit(1)

if __name__ == "__main__":
    main()
