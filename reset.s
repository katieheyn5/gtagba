.global reset

reset:
    mov r1, #3
    ldr r3, [r0]
    cmp r3, #0
    beq add
    bne done
add:
    add r3, r3, r1
done:
    str r3, [r0]
    mov r0, r3
    mov pc, lr
