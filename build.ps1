param()

pushd $PSScriptRoot

if ($IsWindows)
{
    gcc (ls *.c) -o a.exe  -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -g # -fsanitize=address
}
else
{
    gcc (ls *.c) -o a.out -DPARSE_DLL_BUILD -DSEE_VERBOSE_ERRORS -DUSE_LEVINSTEIN -g # -fsanitize=address
}

popd
