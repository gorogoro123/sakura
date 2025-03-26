setlocal
set BUILD_DIR=%~dp1
set SOURCE_DIR=%~dp0compiletests

:: find generic tools
if not defined CMD_VSWHERE call %~dp0..\tools\find-tools.bat

if not exist "%CMD_CMAKE%" (
  echo "no cmake found."
  exit /b 1
)

if "%CONFIGURATION%" == "Debug_clang" (
  set CONFIGURATION_TYPE=Debug
) else if "%CONFIGURATION%" == "Release_clang" (
  set CONFIGURATION_TYPE=Release
) else (
  set CONFIGURATION_TYPE=%CONFIGURATION%
)

if not exist "%CMD_NINJA%" (
  set GENERATOR="%CMAKE_G_PARAM%"
  set GENERATOR_OPTS=-A %PLATFORM% "-DCMAKE_CONFIGURATION_TYPES=Debug;Release"
  set "MAKE_PROGRAM=%CMD_MSBUILD%"
) else (
  set GENERATOR=Ninja
  set GENERATOR_OPTS=-DCMAKE_BUILD_TYPE=%CONFIGURATION_TYPE%
  set "MAKE_PROGRAM=%CMD_NINJA%"
)

mkdir %BUILD_DIR% > NUL 2>&1
pushd %BUILD_DIR%

call :run_cmake_configure || endlocal && exit /b 1

endlocal && exit /b 0

:run_cmake_configure
if not exist "%CMD_CL%" (
  echo "no cl.exe found."
  exit /b 1
)

:: replace back-slash to slash in the path.
set CL_COMPILER=%CMD_CL:\=/%

:: run cmake configuration.
"%CMD_CMAKE%" -G %GENERATOR%                        ^
  "-DCMAKE_MAKE_PROGRAM=%MAKE_PROGRAM%"             ^
  "-DCMAKE_C_COMPILER=%CL_COMPILER%"                ^
  "-DCMAKE_CXX_COMPILER=%CL_COMPILER%"              ^
  "-Wno-dev"                                        ^
  %GENERATOR_OPTS%                                  ^
  %SOURCE_DIR%                                      ^
  || endlocal && exit /b 1
goto :EOF

goto :EOF
