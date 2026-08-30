@IF /I "%~1" == "" GOTO Error
@IF /I "%~1" == "/t:Push" GOTO Push
@IF /I "%~1" == "/t:Pack" GOTO Pack
@IF /I "%~1" == "/t:Help" GOTO Help

:Pack
@echo Packaging MyOddWeb.DirectoryWatcher version 0.2.0...
.\tools\nuget\Nuget.exe pack ".\src\myoddweb.directorywatcher\myoddweb.directorywatcher.nuspec" -OutputDirectory ".\src\bin" -Symbols -SymbolPackageFormat snupkg
@IF ERRORLEVEL 1 (
  @echo Fallback: packaging without snupkg flag...
  .\tools\nuget\Nuget.exe pack ".\src\myoddweb.directorywatcher\myoddweb.directorywatcher.nuspec" -OutputDirectory ".\src\bin"
)
@echo.
@echo Package created in .\src\bin\
@GOTO End

:Push
@IF "%~2" == "" (
  @echo Usage: build.bat /t:Push ^<NUGET_API_KEY^>
  @echo Or set NUGET_API_KEY environment variable and run:
  @echo   dotnet nuget push .\src\bin\MyOddWeb.DirectoryWatcher.0.2.0.nupkg --api-key %%NUGET_API_KEY%% --source https://api.nuget.org/v3/index.json
  @GOTO End
)
.\tools\nuget\Nuget.exe push ".\src\bin\MyOddWeb.DirectoryWatcher.0.2.0.nupkg" -ApiKey %~2 -Source https://api.nuget.org/v3/index.json
@GOTO End

:Error
:Help
@echo MyOddWeb.DirectoryWatcher Package Utility
@echo.
@echo Commands:
@echo   build.bat /t:Pack                  - Creates the NuGet package in .\src\bin
@echo   build.bat /t:Push ^<API_KEY^>        - Pushes the package to NuGet.org
@echo.
@echo Notes:
@echo   Make sure you have built Release Win32, x64, and Any CPU targets before packing.
@echo   See nuget.md for the complete step-by-step guide.

:End