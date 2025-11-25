"(x-1)^3"|.\a.exe|%{if($_-match"((\^|\+|\-)[^\[\]]*){2}"){$s = $_-replace"\^","**"; $res = py -c "x=3;print(($s))"; "$_  =  $res"}else{$_}}
