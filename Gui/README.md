## How to run

# Compile
```bash
g++ xml_editor.cpp functions.cpp utils.cpp -o xml_editor.exe
```

# For Windows
- To verify
```bash
.\xml_editor.exe verify -i input_file.xml -o output_file.xml
```
- To format
```bash
.\xml_editor.exe format -i input_file.xml -o output_file.xml
```
- Convert to JSON
```bash
.\xml_editor.exe json -i input_file.xml -o output_file.json
```

- To minify
```bash
.\xml_editor.exe mini -i input_file.xml -o output_file.xml
```

- Compress
```bash
.\xml_editor.exe compress -i input_file.xml -o output_file.comp
```

- Decompress
```bash
.\xml_editor.exe decompress -i input_file.comp -o output_file.xml
```

# For Linux/macOS/Unix
- To verify
```bash
./xml_editor.exe verify -i input_file.xml -o output_file.xml
```
- To format
```bash
./xml_editor.exe format -i input_file.xml -o output_file.xml
```
- Convert to JSON
```bash
./xml_editor.exe json -i input_file.xml -o output_file.json
```

- To minify
```bash
./xml_editor.exe mini -i input_file.xml -o output_file.xml
```

- Compress
```bash
./xml_editor.exe compress -i input_file.xml -o output_file.comp
```

- Decompress
```bash
./xml_editor.exe decompress -i input_file.comp -o output_file.xml
```