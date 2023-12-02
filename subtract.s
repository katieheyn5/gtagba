.global subtract

/* function to decrement the number of lives */
subtract:
    /* set the value of r1 to 1 */
    mov r1, #1
    /* load the value of r0 into r2 */
    ldr r2, [r0]
    /* compare r2 to -1 */
    cmp r2, #-1
    /* if r2 is greater than -1, go to sub: */
    bgt sub
    /* else, go to done: */
    b done
sub:
    /* subtract r1 (1) from the value of r2 */
    sub r2, r2, r1
    /* go to done: */
    b done 
done:
    /* store the value of r2 into r0 */
    str r2, [r0]
    mov r0, r2
    mov pc, lr
