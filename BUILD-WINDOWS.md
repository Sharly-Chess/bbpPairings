# Building bbpPairings on Windows

This document explains how to build a statically-linked Windows executable for bbpPairings.

## Prerequisites

First, install [chocolatey](https://chocolatey.org/).

Then, in and administrative PowerShell, install the following packages:

```powershell
choco install mingw --version=11.2.0 --force
choco install git
choco install make
```

## Building

```powershell
$env:PATH = "C:\tools\msys64\usr\bin;$env:PATH" 
make clean
make static=yes optimize=no COMP=gcc bbpPairings.exe
```

## Verifying Static Linking

To verify that your executable is statically linked:

```bash
objdump -p bbpPairings.exe | findstr "DLL"
```

