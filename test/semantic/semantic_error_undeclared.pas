program error_undeclared(input, output);
var 
    x: integer;
begin
    x := 10;
    y := 20;        { 错误：y 未声明 }
    write(x)
end.