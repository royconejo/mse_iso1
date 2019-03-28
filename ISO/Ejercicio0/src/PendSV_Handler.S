	.syntax unified
	.text
	.global PendSV_Handler

	.extern get_next_context

	.thumb_func

PendSV_Handler:
	push {r4-r11,lr}

	mrs r0,msp 				/* r0 = msp*/
	bl get_next_context 	/* get_next_context(msp) */
	msr msp,r0				/* msp = r0 (valor devuelto por get_next_context) */

	pop {r4-r11,lr}
return:
	bx lr
