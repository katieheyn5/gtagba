.global reset

reset:
    mov r1, #4
    ldr r2, [r0]
    cmp r2, #0
    blt add
    b done
add:
    add r2, r2, r1
    b done
done:
    str r2, [r0]
    mov r0, r2
    mov pc, lr

