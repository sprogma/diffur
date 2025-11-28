param(
        [parameter(Mandatory=$true,
                   ValueFromPipeline=$true,
                   HelpMessage="Enter expression to calculate derivative from")] 
        [ValidateScript({ 
            $_ | .\a.exe t 1>$null 2>variable:res
            $b = $?
            $res = ((($res | oss)-join"`n")-split"\n(?=Variant)")
            # $res = $res[1..$res.Length] | sort {[void]($_-match"SCORE (\-?\+?\d+)"); [double]$Matches[1]}
            # $res = $res-join"`n"
            if (!$b)
            {
                throw "The expression $_ is not a valid or understanble. Maybe`n$res"
            }
            $true
        })]
        [string] $Expression
)
$Expression | .\a.exe | % {
    if ($_ -match "no changes") { }
    else
    {
        if ($_-notmatch":|<|>|\." -and $_.Trim())
        {
            Write-Host "$($PSStyle.Reverse)$_$($PSStyle.Reset)"
            Write-Progress -Activity "optimizing" -Status "$_"
            # sleep 0.1
        }
        else
        {
            Write-Host "$($PSStyle.Foreground.BrightBlack)$_$($PSStyle.Reset)"
        }
    }
}
