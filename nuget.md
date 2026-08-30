# NuGet Package Release Guide: MyOddWeb.DirectoryWatcher

This document provides a step-by-step guide for authoring, building, validating, and publishing **`MyOddWeb.DirectoryWatcher`** version **`0.2.0`** to [NuGet.org](https://www.nuget.org/packages/MyOddWeb.DirectoryWatcher/) in compliance with [Microsoft NuGet Package Authoring Best Practices](https://learn.microsoft.com/en-gb/nuget/create-packages/package-authoring-best-practices).

---

## 1. Package Overview

- **Package ID:** `MyOddWeb.DirectoryWatcher`
- **Current Version:** `0.2.0`
- **Supported Target Frameworks:**
  - `.NET Framework 4.6.2` (`net462`)
  - `.NET Standard 2.0` (`netstandard2.0`)
  - `.NET 8.0` (`net8.0`)
- **Native Architecture:** Embedded `Win32` (x86) and `x64` native C++ binaries (`myoddweb.directorywatcher.win.x86.dll` and `myoddweb.directorywatcher.win.x64.dll`).
- **License:** [MIT](https://opensource.org/licenses/MIT) (SPDX expression: `MIT`)
- **Repository:** [https://github.com/FFMG/myoddweb.directorywatcher](https://github.com/FFMG/myoddweb.directorywatcher)

---

## 2. Build Pipeline & Dependency Architecture

> [!IMPORTANT]
> **Native C++ Pre-requisite Build Order:**
> The managed C# assembly (`MyOddWeb.DirectoryWatcher.dll`) embeds `myoddweb.directorywatcher.win.x86.dll` and `myoddweb.directorywatcher.win.x64.dll` at compile time as embedded resources.
>
> Therefore, **the native Win32 and x64 Release builds must be completed before compiling the C# projects**.

```
+--------------------------------------------------------------------------+
| 1. Build C++ Native x86 & x64 (Release)                                  |
|    - src/bin/Release/Win32/myoddweb.directorywatcher.win.x86.dll         |
|    - src/bin/Release/x64/myoddweb.directorywatcher.win.x64.dll           |
+------------------------------------+-------------------------------------+
                                     | (Embedded Resources)
                                     v
+--------------------------------------------------------------------------+
| 2. Build C# Managed Libraries (Release | Any CPU)                        |
|    - MyOddWeb.DirectoryWatcher.Interfaces (net462, netstandard2.0, net8.0)|
|    - MyOddWeb.DirectoryWatcher (net462, netstandard2.0, net8.0)          |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
| 3. Execute Automated Verification Suite                                  |
|    - GoogleTest C++ Native Tests (Win32 / x64)                           |
|    - NUnit C# Managed Tests across TFMs                                  |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
| 4. Pack NuGet Package (.nupkg & .snupkg)                                 |
|    - build.bat /t:Pack                                                   |
|    - Outputs to src/bin/MyOddWeb.DirectoryWatcher.0.2.0.nupkg            |
+------------------------------------+-------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
| 5. Publish to NuGet.org                                                  |
|    - dotnet nuget push / nuget push                                      |
+--------------------------------------------------------------------------+
```

---

## 3. Step-by-Step Release Procedure

### Step 1: Clean and Prepare Working Directory

Ensure that your Git working tree is clean and on the correct release branch:

```powershell
git status
```

If you want a completely fresh build, clean previous output binaries:

```powershell
# Optional: remove previous build outputs
git clean -xfd src\bin samples\bin
```

---

### Step 2: Build Native C++ Binaries in Release Configuration

Open **Visual Studio 2022** or use **MSBuild**:

#### Option A: Using Visual Studio 2022
1. Open `src/myoddweb.directorywatcher.sln`.
2. Set configuration to **Release** and platform to **x86** (or **Win32**). Build `myoddweb.directorywatcher.win.x86`.
3. Set configuration to **Release** and platform to **x64**. Build `myoddweb.directorywatcher.win.x64`.

#### Option B: Using Developer Command Prompt / MSBuild
```powershell
# Build Win32 native DLL
msbuild src\myoddweb.directorywatcher.win\myoddweb.directorywatcher.win.x86.vcxproj /p:Configuration=Release /p:Platform=Win32 /m

# Build x64 native DLL
msbuild src\myoddweb.directorywatcher.win\myoddweb.directorywatcher.win.x64.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

#### Verify Native Binaries:
Ensure the following files exist:
- `src\bin\Release\Win32\myoddweb.directorywatcher.win.x86.dll`
- `src\bin\Release\x64\myoddweb.directorywatcher.win.x64.dll`

---

### Step 3: Build Managed C# Assemblies in Release Configuration

Build the managed solution in **Release | Any CPU**:

#### Option A: Using Visual Studio 2022
1. Set configuration to **Release** and platform to **Any CPU**.
2. Right-click solution and choose **Rebuild Solution**.

#### Option B: Using MSBuild / .NET CLI
```powershell
# Restore NuGet packages
nuget restore src\myoddweb.directorywatcher.sln

# Build entire solution for Release Any CPU
msbuild src\myoddweb.directorywatcher.sln /p:Configuration=Release /p:Platform="Any CPU" /m
```

#### Verify Managed Binaries:
Ensure the following assemblies and XML documentation files exist:
- `src\bin\Release\net462\MyOddWeb.DirectoryWatcher.dll` & `.xml` & `.pdb`
- `src\bin\Release\net462\MyOddWeb.DirectoryWatcher.Interfaces.dll` & `.xml` & `.pdb`
- `src\bin\Release\netstandard2.0\MyOddWeb.DirectoryWatcher.dll` & `.xml` & `.pdb`
- `src\bin\Release\netstandard2.0\MyOddWeb.DirectoryWatcher.Interfaces.dll` & `.xml` & `.pdb`
- `src\bin\Release\net8.0\MyOddWeb.DirectoryWatcher.dll` & `.xml` & `.pdb`
- `src\bin\Release\net8.0\MyOddWeb.DirectoryWatcher.Interfaces.dll` & `.xml` & `.pdb`

---

### Step 4: Run Automated Tests

Run both native C++ tests and managed C# tests before packaging:

#### 1. Native C++ GoogleTest:
```powershell
.\src\bin\Release\Win32\myoddweb.directorywatcher.win.test.exe
```

#### 2. Managed C# NUnit Tests:
```powershell
dotnet test src\myoddweb.directorywatcher.test\myoddweb.directorywatcher.test.csproj --configuration Release --no-build
```

Ensure all tests pass with zero failures.

---

### Step 5: Create the NuGet Package

Run the packaging command via `build.bat`:

```cmd
build.bat /t:Pack
```

Or invoke `nuget.exe` directly:

```powershell
.\tools\nuget\nuget.exe pack src\myoddweb.directorywatcher\myoddweb.directorywatcher.nuspec -OutputDirectory src\bin -Symbols -SymbolPackageFormat snupkg
```

#### Expected Outputs:
- `src\bin\MyOddWeb.DirectoryWatcher.0.2.0.nupkg`
- `src\bin\MyOddWeb.DirectoryWatcher.0.2.0.snupkg` (Symbols package, if supported)

---

### Step 6: Validate Package Structure & Quality

Verify that the `.nupkg` archive contains all required components:

1. **Open the package** using [NuGet Package Explorer](https://github.com/NuGetPackageExplorer/NuGetPackageExplorer) or inspect by renaming `.nupkg` to `.zip`:
   - `docs\README.md` is present.
   - `lib\net462\`, `lib\netstandard2.0\`, and `lib\net8.0\` each contain `.dll`, `.pdb`, and `.xml`.
   - Manifest contains:
     - `License: MIT`
     - `Readme: docs\README.md`
     - `Repository: https://github.com/FFMG/myoddweb.directorywatcher`
     - `ReleaseNotes` describing 0.2.0 improvements.

2. **Local Smoke Test (Consume in a sample project):**
   ```powershell
   # Create a temporary test folder
   mkdir temp_test && cd temp_test
   dotnet new console -f net8.0
   
   # Add local package source and reference version 0.2.0
   dotnet add package MyOddWeb.DirectoryWatcher --version 0.2.0 --source ..\src\bin
   
   # Test build
   dotnet build
   cd .. && Remove-Item -Recurse -Force temp_test
   ```

---

### Step 7: Publish to NuGet.org

> [!CAUTION]
> **Keep your API Key secret!**
> Never commit your NuGet API key to git or paste it into public terminal logs.

#### 1. Set your NuGet API Key in your shell session:
```powershell
# PowerShell
$env:NUGET_API_KEY = "your_actual_nuget_api_key_here"
```

#### 2. Push the package:
```powershell
# Using dotnet nuget CLI (Recommended):
dotnet nuget push src\bin\MyOddWeb.DirectoryWatcher.0.2.0.nupkg --api-key $env:NUGET_API_KEY --source https://api.nuget.org/v3/index.json

# If a symbol package (.snupkg) was generated:
dotnet nuget push src\bin\MyOddWeb.DirectoryWatcher.0.2.0.snupkg --api-key $env:NUGET_API_KEY --source https://api.nuget.org/v3/index.json
```

Or using `build.bat`:
```cmd
build.bat /t:Push %NUGET_API_KEY%
```

---

### Step 8: Post-Publishing Checklist

1. **Verify on NuGet Gallery:**
   Visit [https://www.nuget.org/packages/MyOddWeb.DirectoryWatcher/0.2.0](https://www.nuget.org/packages/MyOddWeb.DirectoryWatcher/0.2.0)
   - Confirm version `0.2.0` is listed and active.
   - Confirm the README renders properly on the package page.
   - Confirm the MIT license badge is displayed.
   - Confirm target frameworks (`.NETFramework 4.6.2`, `.NETStandard 2.0`, `.NET 8.0`) are detected correctly.

2. **Git Tagging & Release:**
   Tag the release in your Git repository:
   ```powershell
   git tag -a v0.2.0 -m "Release v0.2.0"
   git push origin v0.2.0
   ```
   Create a release on GitHub linking to `changelog.md` and the NuGet package.
