# bat2exe
Convert batch files into executables to prevent tampering of scripts.

### Implementation
The builder program takes a batch file as input, and extracts an embedded executable called the stub to the disk. The builder then updates the resources of the stub so that there is a resource in the stub containing the contents of the batch file. It then renames the stub so that it has the same name as the batch file (sans the .bat extension).

When the stub is executed, it will extract its embedded batch file to the temp folder, and then run the file. When the batch file is done executing, it will be deleted from the temp folder.

### Limitations
The embedded batch file is stored as a plaintext resource, so it is possible to open the stub in a text editor and see the contents of the script.

### To Do List
- Use XOR or some other algorithm to ensure that embedded batch file is not visible in plaintext.
- Support administrator manifest.
- Support password requirement to launch batch file.
