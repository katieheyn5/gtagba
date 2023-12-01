.global reset

reset:
    mov r1, #1
top:
    cmp r0, #3
    beq done
    add r0, r0, r1
    b top
done:
    mov pc, lr
