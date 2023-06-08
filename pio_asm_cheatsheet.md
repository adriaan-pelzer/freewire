JMP [!X|X—|!Y|Y—|X!=Y|PIN|!OSRE] label

	!reg: if reg is 0
	reg--: if reg is not 0 and decr
	reg0!=reg1: reg0 not equal reg1
	PIN: if pin number EXECCTRL_JMP_PIN is high
	!OSRE: if OSR not empty

WAIT 1|0 GPIO|PIN|IRQ index

IN PINS|X|Y|NULL|ISR|OSR bit_count

OUT PINS|X|Y|NULL|PINDIRS|PC|ISR|EXEC bit_count

PUSH [iffull] [noblock]

PULL [ifempty] [noblock]

MOV PINS|X|Y|EXEC|PC|ISR|OSR [!|~|::]PINS|X|Y|NULL|STATUS|ISR|OSR

IRQ [SET|NOWAIT|WAIT|CLEAR] irq_num [REL]

SET PINS|X|Y|PINDIRS value
