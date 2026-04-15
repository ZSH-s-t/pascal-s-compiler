program test(input, output);

var x, y: integer;
var a: array[1..10] of integer;
var b: boolean;

function gcd(a, b: integer): integer;
begin
    if b = 0 then 
        gcd := a
    else 
        gcd := gcd(b, a mod b)
end;

begin
    x := 10;
    y := x + 20;
    
    { 语义错误示例（注释掉以测试正常情况）}
    // z := 100;        { 错误：未声明的变量 }
    // x := true;       { 错误：类型不匹配 }
    // a := 5;          { 错误：数组需要下标 }
    // if x then        { 错误：条件必须是boolean }
    //     y := 1;
    
    write(gcd(x, y))
end.