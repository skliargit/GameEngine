# Написание игрового движка на C

## Содержание
1. [Скачивание проекта](#download)
2. [Сборка на Arch Linux](#archlinux)
3. [Сборка на Windows](#windows)

## <a name="download"></a>Скачивание проекта
Выберите директорию, в которую необходимо сохранить проект, и перейдите в неё. Затем выполните:
```sh
git clone https://github.com/skliargit/GameEngine.git GameEngine
```

## <a name="archlinux"></a>Сборка проекта на Arch Linux
>### ⚠️ Важно! Установка зависимостей:
> **Обязательные:** python clang glslang vulkan-headers vulkan-icd-loader vulkan-tools vulkan-validation-layers<br>
> **Рекомендуемые:** vulkan-utility-libraries vulkan-mesa-layers vulkan-extra-layers vulkan-extra-tools vulkan-swrast

Команда для установки:
```sh
pacman -S python clang vulkan-headers vulkan-icd-loader vulkan-tools vulkan-validation-layers glslang
vulkan-utility-libraries vulkan-mesa-layers vulkan-extra-layers vulkan-extra-tools vulkan-swrast
```
Сборка и запуск:
```sh
cd GameEngine
./build.py clean
./build.py debug
cd bin
./testapp
```

## <a name="windows"></a>Сборка проекта на Windows
>### ⚠️ Важно! Установка зависимостей:
>1. LLVM:
>    + Скачать: [LLVM-21.1.0-rc1-win64](https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.0-rc1/LLVM-21.1.0-rc1-win64.exe "Ссылка на скачивание"), можно и другую версию по [ссылке](https://github.com/llvm/llvm-project/releases "Ссылка на репозиторий github")
>    + При установке необходимо выбрать: добавить путь в переменную PATH
>2. MSBuildTools:
>    + Скачать: [MSBuildTools](https://aka.ms/vs/17/release/vs_BuildTools.exe "Ссылка на скачивание"), можно выбрать и другое решение по [ссылке](https://visualstudio.microsoft.com/ru/downloads/?q=build+tools "Ссылка на сайт microsoft"), например ***community***
>    + При установке во вкладке "Отдельные компоненты" выбрать:
>        +  Пакет SDK для Windows 10 (10.0.20348.0), можно и другую версию
>        +  MSVC версия 142 - VS 2022 C++ x64/x86 Build Tools, можно и другую версию
>3. Vulkan SDK:
>    + Скачать: [VulkanSDK-windows-X64-1.4.321.1](https://sdk.lunarg.com/sdk/download/1.4.321.1/windows/vulkansdk-windows-X64-1.4.321.1.exe "Ссылка на скачивание"), можно и другую версию по [ссылке](https://vulkan.lunarg.com/sdk/home#windows "Ссылка на сайт Vulkan Lunar")
>4. Python:
>    + Скачать: [Python-3.14.0](https://www.python.org/ftp/python/3.14.0/python-3.14.0-amd64.exe "Ссылка на скачивание"), можно и другую версию по [ссылке](https://www.python.org/downloads/ "Ссылка на сайт python.org")
>    + При установке необходимо выбрать: добавить путь в переменную PATH

>### ℹ️ Дополнительно:
> Версии можно выбирать другие, но тогда пути к файлам придется подправить!

Для настройка переменных среды выполните в командной строке:
```cmd
setx INCLUDE "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\;C:\Program Files (x86)\Windows Kits\10\Include\10.0.20348.0\shared\;C:\Program Files (x86)\Windows Kits\10\Include\10.0.20348.0\ucrt\;C:\Program Files (x86)\Windows Kits\10\Include\10.0.20348.0\um\;C:\VulkanSDK\1.4.321.0\Include\\"
&& setx LIB "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\lib\x64\;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.20348.0\ucrt\x64\;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.20348.0\um\x64\;C:\VulkanSDK\1.4.321.0\Lib\\"
```

Для проверки поддержки Vulkan выполните в командной строке:
```cmd
vulkaninfo
```
Если команда выполнилась успешно, в разделе "Device properties and extensions" будут указаны поддерживаемые GPU устройства.

Сборка и запуск:
```cmd
cd GameEngine
build.py clean
build.py debug
cd bin
testapp.exe
```
