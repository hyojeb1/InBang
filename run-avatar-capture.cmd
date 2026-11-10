@echo off
setlocal

set "PROJECT_PATH=%~dp0InBang\InBang.uproject"
if not exist "%PROJECT_PATH%" set "PROJECT_PATH=C:\Dev\InBang\InBang\InBang.uproject"
if not exist "%PROJECT_PATH%" set "PROJECT_PATH=C:\Dev\00InBang\InBang\InBang.uproject"

if not exist "%PROJECT_PATH%" (
  echo [InBang] InBang.uproject was not found.
  echo Checked this repository and both work PC paths.
  pause
  exit /b 1
)

if defined UNREAL_ENGINE_ROOT set "UNREAL_EDITOR=%UNREAL_ENGINE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not defined UNREAL_EDITOR (
  for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul ^| find "REG_SZ"') do set "UNREAL_EDITOR=%%B\Engine\Binaries\Win64\UnrealEditor.exe"
)
if not exist "%UNREAL_EDITOR%" set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%UNREAL_EDITOR%" (
  echo [InBang] UnrealEditor.exe for UE 5.8 was not found.
  echo Set UNREAL_ENGINE_ROOT to the UE_5.8 installation directory and try again.
  pause
  exit /b 1
)

echo [InBang] Project: %PROJECT_PATH%
echo [InBang] Unreal:  %UNREAL_EDITOR%
echo [InBang] Starting /Game/Lvl_Stage at 854x480 Windowed.

set "CAPTURE_USER_DIR=%LOCALAPPDATA%\InBangAvatarCapture"
start "" "%UNREAL_EDITOR%" "%PROJECT_PATH%" /Game/Lvl_Stage -game -AvatarCapture -UserDir="%CAPTURE_USER_DIR%" -windowed -ResX=854 -ResY=480 -log %*

endlocal
