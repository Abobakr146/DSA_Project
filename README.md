# XML Editor & Network Visualizer

A C++ tool to verify, format, minify, compress, and visualize XML data.

## 1. Prerequisites

### For Linux / macOS
> [!IMPORTANT]
> You need a C++ compiler and Graphviz for visualization.

```bash
sudo apt update
sudo apt install build-essential g++
sudo apt install graphviz
### Verify installation:
g++ --version
dot -V
```

### For Windows
> [!IMPORTANT]
> You need a C++ compiler (like MinGW) and Graphviz.

#### step 1: Install Graphviz via Winget: Open PowerShell and run:
```bash
winget install graphviz
```
#### Step 2: Find where Winget hid Graphviz
- We need to find the folder containing dot.exe. It is usually in one of these two places:
    - Standard: `C:\Program Files\Graphviz\bin`
    - User specific: `C:\Users\[YOUR_USERNAME]\AppData\Local\Programs\Graphviz\bin`
- Action: Open your File Explorer and check `C:\Program Files\Graphviz`. Do you see a `bin` folder inside? If yes, copy that address (e.g., `C:\Program Files\Graphviz\bin`).

#### Step 3: Tell Windows where it is (Add to PATH)
- Now we manually add that address to your system settings.
    1. Press the Windows Key and type "env".
    2. Select "Edit the system environment variables".
    3. In the window that pops up, click the Environment Variables button (bottom right).
    4. In the bottom list (System variables), find the variable named Path and select it.
    5. Click Edit.
    6. Click New (on the right side).
    7. Paste the path you found in Step 2 (e.g., C:\Program Files\Graphviz\bin).
    8. Click OK on all three open windows to save.

#### Step 4: Verify
Close your current PowerShell window (this is mandatory) and open a new one.

Run:
```
dot -V
```

## 2.1. Compile & Run, Makefile
### For Windows (CMD)

```bash
make clean all OS=windows
# each command has a unique target 
```

### For Linux / macOS / Unix

```bash
make clean all OS=linux
# each command has a unique target 
```
> [!NOTE]
> You can back to the `Makefile` it's easy to understand

## 2.2. Compile & Run, Manualy
- Run the following command in your terminal to build the project:
```
g++ xml_editor.cpp external/tinyxml2/tinyxml2.cpp -Iexternal/tinyxml2 graph.cpp functions.cpp utils.cpp -o xml_editor
```

### For Windows (PowerShell / CMD)

- Verify XML
```
.\xml_editor.exe verify -i input_file.xml -o output_file.xml
```

- Format (Prettify)
```
.\xml_editor.exe format -i input_file.xml -o output_file.xml
```

- Convert to JSON
```
.\xml_editor.exe json -i input_file.xml -o output_file.json
```

- Minify
```
.\xml_editor.exe mini -i input_file.xml -o output_file.xml
```

- Compress
```
.\xml_editor.exe compress -i input_file.xml -o output_file.comp
```

- Decompress
```
.\xml_editor.exe decompress -i input_file.comp -o output_file.xml
```

- Draw Network (Requires Graphviz)
```
.\xml_editor.exe draw -i input_test_draw.xml -o output_file.jpg
.\xml_editor.exe draw -i input_file.xml -o output_file.jpg
```

### For Linux / macOS / Unix

- Verify XML
```
./xml_editor verify -i input_file.xml -o output_file.xml
```

- Format (Prettify)
```
./xml_editor format -i input_file.xml -o output_file.xml
```

- Convert to JSON
```
./xml_editor json -i input_file.xml -o output_file.json
```

- Minify
```
./xml_editor mini -i input_file.xml -o output_file.xml
```

- Compress
```
./xml_editor compress -i input_file.xml -o output_file.comp
```

- Decompress
```
./xml_editor decompress -i input_file.comp -o output_file.xml
```

- Draw Network (Requires Graphviz)
```
./xml_editor draw -i input_test_draw.xml -o output_file.jpg
./xml_editor draw -i input_file.xml -o output_file.jpg
```
