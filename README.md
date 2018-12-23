# bat2exe
Convert batch files into executables to prevent tampering of scripts.

### Implementation
The builder program takes a batch file as input, and extracts an embedded executable called the stub to the disk. The builder then updates the resources of the stub so that there is a resource in the stub containing the contents of the batch file. It then renames the stub so that it has the same name as the batch file (sans the .bat extension).

When the stub is executed, it will extract its embedded batch file to the temp folder, and then run the file. When the batch file is done executing, it will be deleted from the temp folder.

You may notice that there is a file called `msvcrt.lib` and `msvcrt.exp` indicating that I linked against `msvcrt.dll` which is unsupported by Microsoft. The full explanation for this can be found in `stub\main.cpp`, but I will summarize it here. Basically I wanted to create an executable with no dependency on the C runtime to reduce the size of the stub. However, no matter what I did, I could not optimize out calls to `memset(...)`, and the linker was complaining that it could not find the implementation of it since I told the linker not to include the C runtime. After many hours of trial and failure, I gave up and decided I would accept linking up against `msvcrt.dll` so that the linker would shut up. However, since linking against `msvcrt.dll` is unsupported, Microsoft does not supply a .lib file to link against, and I could not use `LoadLibrary(...)` and `GetProcAddress(...)` because the `memset(...)` that kept being called was not explicitly called by me, but rather inserted by the compiler (even after I disabled optimizations). Thus, I had to create my own `msvcrt.lib` which I did by following the instructions at https://stackoverflow.com/a/9946390.

### Limitations
The embedded batch file is stored as a plaintext resource, so it is possible to open the stub in a text editor and see the contents of the script.

### To Do List
- Use XOR or some other algorithm to ensure that embedded batch file is not visible in plaintext.
- Support administrator manifest.
- Support password requirement to launch batch file.
