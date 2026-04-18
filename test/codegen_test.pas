{ 代码生成测试程序 }
program codegen_test(input, output);

const
    MAX = 100;
    MIN = 0;

var
    x, y, sum: integer;
    average: real;
    flag: boolean;

{ 计算两个数的和 }
function add(a, b: integer): integer;
begin
    add := a + b
end;

{ 计算阶乘 }
function factorial(n: integer): integer;
var
    i, result: integer;
begin
    result := 1;
    for i := 1 to n do
        result := result * i;
    factorial := result
end;

begin
    { 测试赋值 }
    x := 10;
    y := 20;
    
    { 测试函数调用 }
    sum := add(x, y);
    
    { 测试if语句 }
    if sum > 25 then
        flag := true
    else
        flag := false;
    
    { 测试for循环 }
    sum := 0;
    for x := 1 to 10 do
        sum := sum + x;
    
    { 测试write }
    write(sum)
end.
