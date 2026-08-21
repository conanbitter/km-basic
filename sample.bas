const DISPLAY_WIDTH = 50
const PADDING = 40
const TOTAL_WIDTH = DISPLAY_WIDTH + 2 * PADDING


dim cur_line(TOTAL_WIDTH)
dim next_line(TOTAL_WIDTH)


function cell_count(index)
    return cur_line(index - 2) + cur_line(index - 1) _
    + cur_line(index + 1) + cur_line(index + 2)
end function


sub init_line
    for i = 0 to TOTAL_WIDTH - 1
        if rnd# > 0.5 then cur_line(i) = 1 else cur_line(i) = 0
    next i
end sub


sub make_step
    for i = 2 to TOTAL_WIDTH - 3
        count = cell_count(i)
        if cur_line(i) = 1 then
            if count = 2 orelse count = 4 then
                next_line(i) = 1
            else
                next_line(i) = 0
            end if
        else
            if count = 2 orelse count = 3 then
                next_line(i) = 1
            else
                next_line(i) = 0
            end if
        end if
    next i

    for i = 0 to TOTAL_WIDTH - 1
        cur_line(i) = next_line(i)
    next i
end sub


sub display
    for i = PADDING to TOTAL_WIDTH - PADDING - 1
        if cur_line(i) = 1 then print "*"; else print " ";
    next i
    print
end sub


randomize
init_line
display
for i = 1 to 25
    make_step
    display
next i

'test
a = .5
b=3.5e-2
c=3E3
s$="Quotes ""this"" is"