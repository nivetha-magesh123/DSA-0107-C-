# Smart Food Waste Redistribution System

A C++17 + Crow + SQLite web application prototype with donor registration, receiver registration, food inventory, greedy priority matching, distribution tracking, and dashboard statistics.

## Requirements (Windows)
- Visual Studio 2022 Build Tools or Visual Studio Community with **Desktop development with C++** workload
- CMake 3.20+
- Git
- vcpkg
- VS Code (recommended editor)

## Install dependencies with vcpkg
Open **Developer PowerShell for VS 2022** (or a terminal where `cl.exe` works):

```powershell
git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
```

From the project folder:

```powershell
C:\dev\vcpkg\vcpkg.exe install
```

This installs the Crow and SQLite dependencies listed in `vcpkg.json`.

## Configure and build
Run from the project folder in Developer PowerShell:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Run
```powershell
cd build
.\Release\smart_food_server.exe
```

If your generator creates the executable directly in `build`, run ` .\smart_food_server.exe` instead. Keep the terminal open. Visit:

- http://localhost:18080/
- http://localhost:18080/api/health

The SQLite file `food_system.db` is created in the current working directory (normally `build`).

## Important prototype notes
- Receiver `distance_km` is entered manually as an estimated donor-to-receiver distance. GPS/maps integration is not included.
- The matching score is a demonstration policy: 50% urgency, 30% location suitability, 20% quantity suitability. Adjust weights with your guide.
- A receiver is eligible only when its full `required_quantity` is no greater than the donation quantity. The prototype allocates that required quantity and marks the donation allocated; it does not split one donation across multiple receivers or maintain remaining receiver demand.
- This is an academic prototype, not a production food-safety or authentication system. Add user authentication, authorization, stronger date validation, food-type/dietary checks, audit logs and secure deployment before real-world use.
