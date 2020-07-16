PDP8/I simulator

Starting MAINDEC test D01C
> btl d01c
> sr 7777
> pc 144
> go
program will halt
> go
passing all tests is signalized by outputting <BEL> on console



Starting MAINDEC test D02B
> btl d02b
> sr 5000
> pc 200
> go
if no halt occures during about one time switch to second part of test
press ctrl-d
> sr 5700
> go
test will print
ADD OK
and after sometime
ROT
2B
what means second part also pass



Starting focal
> do focal.do
> go