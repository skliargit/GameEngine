#!/bin/sh

COMMON_FLAGS="-fvisibility=hidden"
OBJECT_FLAGS="-g"
LINKER_FLAGS=""
DEFINE_FLAGS="-DDEBUG_FLAG -DIMPORT_FLAG"
INCLUDE_FLAGS=""

SRC_DIR="src/"
OBJ_DIR="../bin/objs/"
OUTPUT_FILE="../bin/testapp"

src_files=$(find "$SRC_DIR" -type f -name "*.c")

obj_files=""
need_rebuild=false
app_exists=false

[ -f "$OUTPUT_FILE" ] && app_exists=true

for src_file in $src_files; do

    relative_path=${src_file#$SRC_DIR}
    obj_path="$OBJ_DIR${relative_path%.c}.o"
    obj_dir=$(dirname "$obj_path")

    mkdir -p "$obj_dir" || exit 1

    if [ ! -f "$obj_path" ] || [ "$src_file" -nt "$obj_path" ]; then
        if ! clang $COMMON_FLAGS $OBJECT_FLAGS $DEFINE_FLAGS $INCLUDE_FLAGS -c "$src_file" -o "$obj_path"; then
            echo "Compilation error: $src_file"
            exit 1
        else
            echo "Compilation completed: $src_file"
        fi
        need_rebuild=true
    fi

    obj_files="$obj_files $obj_path"
done

if [ "$need_rebuild" = true ] || [ "$app_exists" = false ]; then

    if ! clang $COMMON_FLAGS $LINKER_FLAGS $obj_files -o "$OUTPUT_FILE"; then
        echo "Build error."
        exit 1
    fi

    # Вывод результата
    if [ "$app_exists" = false ]; then
        echo "The application is assembled."
    else
        echo "The application has been updated."
    fi
else
    echo "No changes found, no assembly required."
fi
