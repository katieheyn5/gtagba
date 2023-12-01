.global subtract

subtract:
    mov r1, #1
    mov r2, r0
    cmp r2, #0
    bgt sub
    b done
sub:
    sub r2, r2, r1
    b done 
done:
    mov r0, r2
    mov pc, lr
