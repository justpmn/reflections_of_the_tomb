@echo off
cd /d "%~dp0"
if not exist "build-desktop\reflections_of_the_tomb.exe" (
  echo Build the game first. See docs\PLAYING.md.
  pause
  exit /b 1
)
"build-desktop\reflections_of_the_tomb.exe"
