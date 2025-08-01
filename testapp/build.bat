@echo off
setlocal EnableDelayedExpansion

set COMMON_FLAGS=-fvisibility=hidden
set OBJECT_FLAGS=-g
set LINKER_FLAGS=
set DEFINE_FLAGS=-DDEBUG_FLAG -DIMPORT_FLAG
set INCLUDE_FLAGS=

set SRC_DIR=src\
set OBJ_DIR=..\bin\objs\
set OUTPUT_FILE=..\bin\testapp.exe

set src_files=
pushd %~dp0
for /r %%f in (*.c) do (
    set full_path=%%f
    set relative_path=!full_path:%CD%\=!
    set relative_path=!relative_path:\/=!
    set src_files=!src_files! !relative_path!
)
popd

set obj_files=
set need_rebuild="false"
set app_exists="false"

if exist %OUTPUT_FILE% set app_exists="true"

for %%s in (%src_files%) do (
    set src_file=%%s
    set relative_path=!src_file:%SRC_DIR%=!
    set obj_path=%OBJ_DIR%!relative_path:.c=.o!
    set obj_dir=!obj_path:%%~ns.o=!

    if not exist !obj_dir! mkdir !obj_dir!
    if errorlevel 1 exit /b 1
    
    set should_compile="false"
    if not exist !obj_path! (
        set should_compile="true"
    ) else (
        for /f %%i in ('powershell -command "(Get-Item '!src_file!').LastWriteTimeUtc.Ticks"') do set src_time="%%i"
        for /f %%i in ('powershell -command "(Get-Item '!obj_path!').LastWriteTimeUtc.Ticks"') do set obj_time="%%i"
        if !src_time! gtr !obj_time! set should_compile="true"
    )
    
    if !should_compile!=="true" (
        clang %COMMON_FLAGS% %OBJECT_FLAGS% %DEFINE_FLAGS% %INCLUDE_FLAGS% -c !src_file! -o !obj_path!
        if errorlevel 1 (
            echo Compilation error: !src_file!
            exit /b 1
        ) else (
            echo Compilation completed: !src_file!
        )
        powershell -command "(Get-Item '!obj_path!').LastWriteTimeUtc = (Get-Item '!src_file!').LastWriteTimeUtc"
        set need_rebuild="true"
    )
    
    set obj_files=!obj_files! !obj_path!
)

if !need_rebuild!=="true" (
    set build_app="true"
) else if !app_exists!=="false" (
    set build_app="true"
) else (
    set build_app="false"
)

if !build_app!=="true" (
    clang %COMMON_FLAGS% %LINKER_FLAGS% %obj_files% -o %OUTPUT_FILE%
    if errorlevel 1 (
        echo Build error
        exit /b 1
    )
    
    if !app_exists!=="false" (
        echo The application is assembled.
    ) else (
        echo The application has been updated.
    )
) else (
    echo No changes found, no assembly required.
)

endlocal
