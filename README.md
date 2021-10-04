# OSSem
Zábava na celou noc

# Quickstart
Pro build je potřeba Windows + Visual Studio (Code?)

Ve Visual Studio otevřeme jednotlivé solutions:
 - user.sln, kernel.sln, boot.sln
 - alternativne jde otevrit i root git repozitare a klikat si na jednotlive solutiony

Buildíme popořadě (asi? fungovalo i Kernel -> User -> Boot):

 1. User
 2. Kernel
 3. Boot

# Linky
- Setup pro VSCode: https://code.visualstudio.com/docs/cpp/config-msvc
- Resharper C++: https://www.jetbrains.com/resharper-cpp/

# Todo:
- Parsing vstupu z konzole
- Impl procesu
- FAT12 implementace
- Impl jednotlivych programu - cd, ls, ... - vetsina bude asi pres procesy, ktere se musi vzdycky spustit / kontaktovat pres jadro ?
- ?
