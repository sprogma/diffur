param()

pushd $PSScriptRoot

# 

if ($IsWindows)
{
    clang (ls *.c) -o a.exe  -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -g -fsanitize=address
}
else
{
    clang (ls *.c) -o a.out -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -g -fsanitize=address
}

popd
