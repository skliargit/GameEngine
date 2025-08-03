@echo off

echo ^>^>^> Building the engine library
pushd engine
if errorlevel 1 (
	popd
    echo Error: Failed to navigate to testapp folder
    exit /b 1
)
call build.bat
if errorlevel 1 (
	popd
    echo Error while running build testapp
    exit /b 1
)
popd

echo ^>^>^> Building the testapp
pushd testapp
if errorlevel 1 (
	popd
    echo Error: Failed to navigate to testapp folder
    exit /b 1
)
call build.bat
if errorlevel 1 (
	popd
    echo Error while running build testapp
    exit /b 1
)
popd

echo ^>^>^> Completed
