.global subtract

subtract:
    mov r1, #1
    /* if lives (r0) = 0, then add 3 */
    cmp r0, #0
    bgt .sub
    /* reset lives to 3 */
    mov r0, #3
    b done
.sub
    sub r0, r0, r1
    b done 
.done
    mov pc, lr
