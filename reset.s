.global reset

reset:
    mov r1, #1
    ldr r3, [r0]
top:
    cmp r3, #3
    beq done
    add r3, r3, r1
    b top
done:
    str r3, [r0]
    mov r0, r3
    mov pc, lr
