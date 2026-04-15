{ Simple Pascal-S Program }
program test;
var
    x, y: integer;
    z: real;
begin
    x := 10;
    y := 20;
    z := 3.14;
    if x < y then
        writeln(x + y)
    else
        writeln(x - y)
end.
