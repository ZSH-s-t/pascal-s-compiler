program correct(input, output);

var 
    x, y, result: integer;
    flag: boolean;

function gcd(a, b: integer): integer;
begin
    if b = 0 then
        gcd := a
    else
        gcd := gcd(b, a mod b)
end;

begin
    x := 10;
    y := 15;
    flag := (x > 0) and (y > 0);
    
    if flag then
        result := gcd(x, y)
    else
        result := 0;
    
    write(result)
end.