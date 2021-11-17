
	.def pushR4ToR11ToPsp
	.def popR4ToR11FromPsp
	.def pushPSP

.thumb


pushR4ToR11ToPsp:
	MRS R0, PSP
	SUB R0, R0, #4
	STR R11, [R0]
	SUB R0, R0, #4
	STR R10, [R0]
	SUB R0, R0, #4
	STR R9, [R0]
	SUB R0, R0, #4
	STR R8, [R0]
	SUB R0, R0, #4
	STR R7, [R0]
	SUB R0, R0, #4
	STR R6, [R0]
	SUB R0, R0, #4
	STR R5, [R0]
	SUB R0, R0, #4
	STR R4, [R0]
	MSR PSP, R0
	BX LR

popR4ToR11FromPsp:
	MRS R0, PSP
	LDR R4, [R0]
	ADD R0, R0, #4
	LDR R5, [R0]
	ADD R0, R0, #4
	LDR R6, [R0]
	ADD R0, R0, #4
	LDR R7, [R0]
	ADD R0, R0, #4
	LDR R8, [R0]
	ADD R0, R0, #4
	LDR R9, [R0]
	ADD R0, R0, #4
	LDR R10, [R0]
	ADD R0, R0, #4
	LDR R11, [R0]
	ADD R0, R0, #4
	MSR PSP, R0
	BX LR

pushPSP:
	MRS R1, PSP
	SUB R1, R1, #4
	STR R0, [R1]
	MSR PSP, R1
	BX LR

.endm
