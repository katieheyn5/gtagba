.global subtract

subtract:
    mov r1, #1
    cmp r0, #0
    bgt sub
    b done
sub:
    sub r0, r0, r1
    b done 
done:
    mov pc, lr
