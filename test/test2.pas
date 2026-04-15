{ 测试数组和循环 }

program array_test;

var
    arr: array[1..10] of integer;
    i, sum: integer;

begin
    sum := 0;
    
    for i := 1 to 10 do
    begin
        arr[i] := i * i;
        sum := sum + arr[i]
    end;
    
    write(sum)
end.
