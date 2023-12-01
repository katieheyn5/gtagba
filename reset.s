.global reset

reset:
    mov r1, #1
    mov r2, r0
top:
    cmp r2, #3
    beq done
    add r2, r2, r1
    b top
done:
    mov r0, r2
    mov pc, lr
