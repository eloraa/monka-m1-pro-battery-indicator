# Hello Win32

A simple win32 application with dark/light mode detection and interactive border colors.


### Development Requirements
- **Visual Studio 2019/2022** (with C++17 support)
- **CMake 3.16+** (for CMake builds)
- **Windows SDK 10.0** or later

## Building the Project

### Option 1: Visual Studio (Recommended)

1. **Open the solution**:
   ```bash
   Test.sln
   ```

2. **Build the project**:
   - Press `Ctrl+Shift+B` or
   - Go to `Build -> Build Solution`

3. **Run the application**:
   - Press `F5` for debug mode or
   - Press `Ctrl+F5` for release mode

### Option 2: CMake Build

1. **Create build directory**:
   ```bash
   mkdir build
   cd build
   ```

2. **Configure with CMake**:
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

3. **Build the project**:
   ```bash
   cmake --build . --config Release
   ```

4. **Install (optional)**:
   ```bash
   cmake --install . --prefix ./install
   ```

### Option 3: Command Line with MSBuild

```bash
msbuild Test.sln /p:Configuration=Release /p:Platform=x64
```
