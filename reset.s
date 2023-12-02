.global reset

/* function to reset the number of lives */
reset:
    /* set the value of r1 to 4 */
    mov r1, #4
    /* load the value of r0 into r2 */
    ldr r2, [r0]
    /* compare r2 to 0 */
    cmp r2, #0
    /* if r2 is less than 0, then go to add: */
    blt add
    /* else go to done: */
    b done
add:
    /* add 4 to the value of r2 */
    add r2, r2, r1
    /* go to done: */
    b done
done:
    /* store the value of r2 back into r0 */
    str r2, [r0]
    mov r0, r2
    mov pc, lr

