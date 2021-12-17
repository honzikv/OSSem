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

# Todo:
- Spravne ukoncovani vsech procesu - deadlock pri shutdown 
- Signaly pro proces - ctrl + c pro ukonceni apod
- Testovani pipe - implementovane ale tezko rict jestli fungujou
- Propojit FAT12 se syscally
- Implementace jednotlivych programu
