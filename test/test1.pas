{ 这是一个简单的Pascal-S程序测试用例 }

program example(input, output);

var
    x, y: integer;
    result: real;
    ch: char;
    flag: boolean;

{ 计算最大公约数 }
function gcd(a, b: integer): integer;
begin
    if b = 0 then
        gcd := a
    else
        gcd := gcd(b, a mod b)
end;

begin
    x := 10;
    y := 25;
    result := 3.14;
    ch := 'A';
    flag := true;
    
    read(x, y);
    write(gcd(x, y))
end.
