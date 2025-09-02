#!/bin/bash

# Инициализация флагов.
tool_missing=false
lib_missing=false

# Функции.
check_tool()
{
    if ! command -v "$1" &> /dev/null; then
        echo "Error: $1 is not installed"
        tool_missing=true
    fi
}

check_lib()
{
    if ! pkg-config --exists "$1"; then
        echo "Error: $1 library not found"
        lib_missing=true
    fi
}

proj_build()
{
    local dir=$(dirname -- "$1")
    local script=$(basename -- "$1")

    # Переход в директорию.
    if ! pushd "$dir" > /dev/null; then
        echo "Error: Failed to enter directory '$dir'"
        return 1
    fi

    # Выполнение скрипта.
    if ! ./"$script"; then
        echo "Error: Failed to execute '$script' in '$dir'"
        popd > /dev/null
        return 1
    fi

    # Возврат из директории.
    if ! popd > /dev/null; then
        echo "Warning: Failed to return from directory '$dir'"
    fi
}

# Выполнение скрипта.
echo ">>> Checking tools..."
check_tool clang
check_tool glslc
check_tool wayland-scanner

echo ">>> Checking libraries..."
check_lib vulkan
check_lib xcb
check_lib wayland-client
check_lib xkbcommon

[ "$tool_missing" = true ] && exit 1
[ "$lib_missing" = true ]  && exit 1

echo ">>> Generation code..."
proj_build "engine/generate-wayland.sh" || exit 1

echo ">>> Building engine library..."
proj_build "engine/build.sh" || exit 1

echo ">>> Building testapp..."
proj_build "testapp/build.sh" || exit 1

echo ">>> Build completed successfully!"
