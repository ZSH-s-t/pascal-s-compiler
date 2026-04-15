{ Test Complex Pascal-S Program }
program complex_test;

const
    MAX_VALUE = 100;
    PI = 3.14159;

var
    i, j, k: integer;
    arr: array[1..10] of integer;
    flag: boolean;
    ch: char;

procedure print_hello;
begin
    writeln('Hello, World!');
end;

function add(a, b: integer): integer;
begin
    add := a + b
end;

begin
    i := 0;
    j := MAX_VALUE;
    flag := true;
    ch := 'A';
    
    for i := 1 to 10 do
    begin
        arr[i] := i * 2;
    end;
    
    while i < MAX_VALUE do
    begin
        i := i + 1;
    end;
    
    if (i > 10) and (j < 50) then
        writeln('Condition met')
    else if not flag then
        writeln('Flag is false')
    else
        writeln('Default case');
        
    k := add(i, j);
    
    case i of
        1: writeln('One');
        2: writeln('Two');
        3: writeln('Three');
    end;
    
    writeln(i, ' + ', j, ' = ', k)
end.
