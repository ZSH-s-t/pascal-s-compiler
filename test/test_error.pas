{ 测试错误处理 }

program error_test;

var
    x: integer;
    y: real;

begin
    { 这个程序包含一些词法错误 }
    
    x := 10;
    y := 3.14;
    
    { 非法字符测试 }
    x := x @ 5;
    
    { 未闭合的注释测试
    x := 20;
    
    { 数字格式错误 }
    y := 3.14.15;
    
    { 正常代码 }
    write(x)
end.
