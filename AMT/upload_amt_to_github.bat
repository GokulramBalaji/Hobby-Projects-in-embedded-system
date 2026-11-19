@echo off
setlocal enabledelayedexpansion
title Upload AMT to GitHub

echo ====================================================================
echo        AUTOMATED MANUAL TRANSMISSION (AMT) GITHUB UPLOADER
echo ====================================================================
echo.

:: 1. Verify Git Installation
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git is not installed or not in your system PATH!
    echo Please download and install Git from: https://git-scm.com/downloads
    echo Then run this script again.
    echo ====================================================================
    pause
    exit /b 1
)

:: 2. Resolve Paths
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

if exist "%SCRIPT_DIR%\AMT\AMT_CONTROL_SYSTEM" (
    set "AMT_DIR=%SCRIPT_DIR%\AMT"
    set "PARENT_DIR=%SCRIPT_DIR%"
) else if exist "%SCRIPT_DIR%\AMT_CONTROL_SYSTEM" (
    set "AMT_DIR=%SCRIPT_DIR%"
    for %%I in ("%SCRIPT_DIR%\..") do set "PARENT_DIR=%%~fI"
) else if exist ".\AMT\AMT_CONTROL_SYSTEM" (
    set "AMT_DIR=%CD%\AMT"
    set "PARENT_DIR=%CD%"
) else (
    echo [ERROR] Could not locate the AMT project folder!
    echo Please make sure this .bat file is in the folder containing 'AMT' or inside 'AMT'.
    echo ====================================================================
    pause
    exit /b 1
)

set "REPO_URL=https://github.com/GokulramBalaji/Hobby-Projects-in-embedded-system.git"

:: 3. Check / Configure Git User Identity if missing
git config user.name >nul 2>&1
if errorlevel 1 (
    echo [INFO] Git user identity is not configured on this computer.
    set /p "GIT_USER_NAME=Enter your Name for Git commits (Default: Gokulram Balaji): "
    if "!GIT_USER_NAME!"=="" set "GIT_USER_NAME=Gokulram Balaji"
    git config --global user.name "!GIT_USER_NAME!"
    echo Saved git user.name: !GIT_USER_NAME!
    echo.
)

git config user.email >nul 2>&1
if errorlevel 1 (
    set /p "GIT_USER_EMAIL=Enter your Email address for Git commits: "
    if not "!GIT_USER_EMAIL!"=="" (
        git config --global user.email "!GIT_USER_EMAIL!"
        echo Saved git user.email: !GIT_USER_EMAIL!
    )
    echo.
)

:: 4. Prompt for Upload Structure
echo Target Repository : %REPO_URL%
echo Source AMT Folder : %AMT_DIR%
echo.
echo Choose repository upload structure:
echo.
echo  [1] Upload as 'AMT/' subfolder in repository (RECOMMENDED)
echo      Keeps repository organized: Hobby-Projects-in-embedded-system/AMT/...
echo      Allows adding other hobby projects to this repository later.
echo.
echo  [2] Upload AMT files directly to repository root
echo      Places files directly at: Hobby-Projects-in-embedded-system/...
echo.
set /p "CHOICE=Enter choice [1 or 2] (Default is 1, press Enter): "
if "%CHOICE%"=="" set "CHOICE=1"

if "%CHOICE%"=="1" goto :option1
if "%CHOICE%"=="2" goto :option2

echo [ERROR] Invalid choice selected.
goto :error

:: ======================================================================
:: OPTION 1: Upload as AMT/ folder inside the repository
:: ======================================================================
:option1
set "TARGET_REPO_DIR=%PARENT_DIR%\Hobby-Projects-in-embedded-system"
echo.
echo [*] Selected Option 1: Upload as 'AMT/' subfolder.

if not exist "!TARGET_REPO_DIR!\.git" (
    echo [*] Cloning remote repository into: "!TARGET_REPO_DIR!"...
    git clone "%REPO_URL%" "!TARGET_REPO_DIR!"
    if errorlevel 1 (
        echo.
        echo [ERROR] Failed to clone %REPO_URL%
        goto :git_error
    )
) else (
    echo [*] Local repository found. Pulling latest updates from GitHub...
    cd /d "!TARGET_REPO_DIR!"
    git pull origin main
)

echo [*] Synchronizing AMT files - excluding node_modules, build caches, and logs...
if not exist "!TARGET_REPO_DIR!\AMT" mkdir "!TARGET_REPO_DIR!\AMT"

robocopy "%AMT_DIR%" "!TARGET_REPO_DIR!\AMT" /E /XD node_modules .next .git .vscode /XF *.sqlite *.sqlite-wal *.sqlite-shm *.log >nul
if !ERRORLEVEL! GEQ 8 (
    echo [ERROR] Robocopy encountered an issue while copying files.
    goto :error
)

cd /d "!TARGET_REPO_DIR!"
git branch -M main
git add .

echo.
echo [*] Files staged for commit:
git status -s
echo.

set /p "COMMIT_MSG=Enter commit message [Press Enter for default]: "
if "!COMMIT_MSG!"=="" set "COMMIT_MSG=Add AMT (Automated Manual Transmission) embedded system project"

git commit -m "!COMMIT_MSG!"
if errorlevel 1 (
    echo [INFO] No changes detected or already committed.
)

echo.
echo [*] Pushing commits to GitHub on branch 'main'...
git push -u origin main
if errorlevel 1 goto :git_error

goto :success

:: ======================================================================
:: OPTION 2: Upload AMT contents directly to repository root
:: ======================================================================
:option2
echo.
echo [*] Selected Option 2: Upload AMT directly to repository root.
cd /d "%AMT_DIR%"

if not exist ".git" (
    echo [*] Initializing Git repository in "%AMT_DIR%"...
    git init
    git branch -M main
    git remote add origin "%REPO_URL%" 2>nul
) else (
    git remote set-url origin "%REPO_URL%"
)

echo [*] Syncing remote changes...
git pull origin main --allow-unrelated-histories --no-rebase -s recursive -X ours --no-edit 2>nul

git add .
echo.
echo [*] Files staged for commit:
git status -s
echo.

set /p "COMMIT_MSG=Enter commit message [Press Enter for default]: "
if "!COMMIT_MSG!"=="" set "COMMIT_MSG=Upload AMT (Automated Manual Transmission) project"

git commit -m "!COMMIT_MSG!"
if errorlevel 1 (
    echo [INFO] No changes detected or already committed.
)

echo.
echo [*] Pushing commits to GitHub on branch 'main'...
git push -u origin main
if errorlevel 1 goto :git_error

goto :success

:: ======================================================================
:: Exit handlers
:: ======================================================================
:success
echo.
echo ====================================================================
echo [SUCCESS] AMT project successfully uploaded to GitHub!
echo Repository URL: %REPO_URL%
echo ====================================================================
echo.
pause
exit /b 0

:git_error
echo.
echo ====================================================================
echo [ERROR] Git command failed!
echo.
echo Troubleshooting checklist:
echo 1. Verify you have write/push access to:
echo    %REPO_URL%
echo 2. If prompted by Git Credential Manager, complete the GitHub login.
echo 3. If using a Personal Access Token (PAT), ensure it has 'repo' scope.
echo 4. Check your internet connection.
echo ====================================================================
echo.
pause
exit /b 1

:error
echo.
echo ====================================================================
echo [ERROR] Operation was aborted due to an error.
echo ====================================================================
echo.
pause
exit /b 1
