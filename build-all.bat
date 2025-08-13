@echo off
SETLOCAL EnableDelayedExpansion

:: Инициализация флагов.
set tool_missing="false"

goto :start

:: Функции.
:check_tool
	where "%~1" >nul 2>&1
	if %errorlevel% neq 0 (
		echo Error: %~1 is not installed
		set tool_missing="true"
	)
goto :eof

:proj_build
	for %%F in ("%~1") do (
		set "dir=%%~dpF"
		set "script=%%~nxF"
	)

	:: Переход в директорию
	pushd "%dir%" || (
		echo Error: Failed to enter directory '%dir%'
		exit /b 1
	)

	:: Выполнение скрипта
	call "%script%" || (
		echo Error: Failed to execute '%script%' in '%dir%'
		popd
		exit /b 1
	)

	:: Возврат из директории
	popd || (
		echo Warning: Failed to return from directory '%dir%'
	)
goto :eof

:start

:: Основной код
echo ^>^>^> Checking tools...
call :check_tool clang
call :check_tool glslc

if %tool_missing%=="true" exit /b 1

echo ^>^>^> Building engine library...
call :proj_build "engine/build.bat" || exit /b 1

echo ^>^>^> Building testapp...
call :proj_build "testapp/build.bat" || exit /b 1

echo ^>^>^> Build completed successfully!
ENDLOCAL
