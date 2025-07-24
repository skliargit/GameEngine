@echo off

if exist bin\ (
    echo === Cleaning up the project ===
    rmdir /s /q bin\
    if not exist bin\ mkdir bin
) else (
    echo Folder bin\ is missing
)
