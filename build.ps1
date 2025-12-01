param()

pushd $PSScriptRoot

# 

if ($IsWindows)
{
    g++ -Wno-write-strings -fpermissive (ls *.c, *.cpp) -I"D:\C\SDL2\x86_64-w64-mingw32\include" SDL2.dll SDL2_ttf.dll -o a.exe  -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -g # -fsanitize=address
}
else
{
    g++ -Wno-write-strings -fpermissive (ls *.c, *.cpp) SDL2.dll -o a.out -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -g # -fsanitize=address
}

popd
