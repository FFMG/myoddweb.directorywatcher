@IF "%~1" == "" GOTO Error
@IF "%~1" == "/t:Push" GOTO Push
@IF "%~1" == "/t:Pack" GOTO Pack

@rem build the x32 version and make sure the new version is in \src\bin\Release\Win32
@rem build the x64 version and make sure the new version is in \src\bin\Release\x64
@rem to pack call "build /t:Pack"


:Pack
@rem remember to update .\src\myoddweb.directorywatcher\myoddweb.directorywatcher.nuspec to correct version number.
@rem Install-Package MyOddWeb.DirectoryWatcher -Version 0.1.9 -Source ..\src\bin\
.\tools\nuget\Nuget.exe pack ".\src\myoddweb.directorywatcher\myoddweb.directorywatcher.nuspec" -OutputDirectory ".\src\bin\."

@GOTO End

:Push
@echo You will need to push it manually or use
@echo .\tools\nuget\Nuget.exe push .\src\bin\Myoddweb.DirectoryWatcher.0.1.9.nupkg <API KEY>

@GOTO End

:Error
@echo Missing parametters, '/t:pack' or '/t:push'

:End