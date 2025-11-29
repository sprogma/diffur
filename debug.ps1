param(
    [string]$s,
    [double]$x
)
$s = $s-replace"\^","**"
$res = py -c "x=$x;print(($s))";
"$_  =  $res"
