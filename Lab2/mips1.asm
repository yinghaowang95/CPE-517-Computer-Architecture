addi $t2,$0, -1  # t2 = -1
bgtz $t2, L1
j L2
L1:
add  $t2, $0, $t1 # t2+1
L2:
sub  $t2, $t2, $0 # t2-0