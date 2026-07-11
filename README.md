# OdrCop3
An ODR violations detector using LLVM/Clang's AST parser.  
There appears to be a paucity of ODR violation-detecting tools for MSVC, so I thought I'd write one.  

## How it works
The idea is to do a clean build of your project while logging to a binlog file, which OdrCop3 uses to know what files to parse.  
Do the following in order:
```
1. msbuild YourSolution.slnx /t:YourProject:Rebuild /p:Configuration=Debug /p:Platform=x64 /bl:some.binlog // generates a .binlog file
2. bl2oc.exe <path-to-.binlog> [output-dir] // generates a "compile_commands.json" file
3. OdrCop3 <folder to compile_commands.json> // outputs ODR violations to std out
```  


### What's completed
1. Not much. It's a complete revamp of OdrCop2 in the sense that testing is much more rigorous and less ad hoc.  
2. I've got FunctionDecl, CXXMethodDecl, CXXConstructorDecl, CXXConversionDecl and ParmVarDecl mostly working.  
3. Only the TDD executable even runs.  

### What's left to do
1. Anonymous namespaces, nameless struct/classes/unions, enums, typedefs/using aliases, and UDTs are NOT working yet.  
2. Hook up OdrCop3  
3. Create additional test app to run through STL, Windows.h and other large projects  
