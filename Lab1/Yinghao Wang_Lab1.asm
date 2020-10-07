# author_1: Yinghao Wang
# author_2: Zheng Xu
.data
str1: .asciiz "Conversation"
str2: .asciiz "Immigration"
newline: .asciiz "\n"
.text
main:
    # print str1 string
    la $a0, str1    # get str1 address  into $a0
    li $v0, 4       # sys code = 4
    syscall         # print string
    la $a0, newline    # get newline address
    li $v0, 4       # sys code = 4
    syscall         # print string

    # print str2 string
    la $a0, str2    # get str1 address  into $a0
    li $v0, 4       # sys code = 4
    syscall         # print string
    la $a0, newline    # get newline address
    li $v0, 4       # sys code = 4
    syscall         # print string

    # call firstmatch
    la  $a0, str1   # the first parameter 
    la  $a1, str2   # the second parameter
    jal firstmatch  
   
    # print the found character
    move $a0, $v0  # pass character to $a0
    li $v0, 11     # sys code = 11 
    syscall        # print char

    li $v0, 10     
    syscall        # 10 for exit program

firstmatch:
    #firstmatch (char *s1, char *s2)
    addi $sp, $sp, -12  # allow stack memory for saveing register
    sw   $s0, 0($sp)    # save register $s0
    sw   $s1, 4($sp)    # save register $s1
    sw   $ra, 8($sp)    # save return address register character
    
    move $s0, $a1       # s0 hold s2 value                       s0=str2
    move $s1, $a0       # s1 hold s1 value                      s1=str1

loop1:
    move $a0, $s0       # the first parameter s2                a0=str2
    lb   $a1, ($s1)     # the second parameter *temp(*s1)        a1=str1 1
    jal  strchr         # call strchr
    bne  $v0, $0, temp # if not 0 return *temp 
    addi $s1, $s1, 1    # next s1 address                        temp++
    lb   $t0, ($s1)     # get *temp(*s1)
    bne  $t0, $0, loop1 # if *temp not zero continue loop        judge *temp !=0
    
    li   $v0, 0         # found none of these chars
    j    quit           # jmp to function quit
temp:
    lb   $v0, ($s1)     # move *temp to $v0 for function return  
quit:

    lw   $s0, 0($sp)    # restore register 
    lw   $s1, 4($sp)    # restore register 
    lw   $ra, 8($sp)    # restore register $ra for return
    addi $sp, $sp, 12   # banlance stack pointer
    jr   $ra            # jmp $ra


strchr:
    # char* strchr (register const char *s, int c)
    move $t0, $a0       # $t0 hold s; $a1 hold c                  t0=str2

loop2:
    lb   $t1, ($t0)     # $t1 hold *s                            t1=str2 1 
    beq  $t1, $a1, Next  # if (*s == c), return address s        if str2 1 = str1 1    ->L_c->
    addi $t0, $t0, 1    # next address                         t0 = st2 +1
    lb   $t1, ($t0)     # $t1 load *s                           t1= str2 2
    bne  $t1, $0, loop2 # if *s not zero continue loop            if str2 !=0, continue if not goto another

    li   $v0, 0         # return $v0 
    j    quit2
Next:
    move  $v0, $t0     # return address s        return st2 number xxx
quit2:
    jr    $ra          # jmp $ra
