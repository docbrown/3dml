;/*****************************************************************************
; *
; * Copyright (c) 1996-1999 ADVANCED MICRO DEVICES, INC. All Rights reserved.
; *
; * This software is unpublished and contains the trade secrets and
; * confidential proprietary information of AMD. Unless otherwise
; * provided in the Software Agreement associated herewith, it is
; * licensed in confidence "AS IS" and is not to be reproduced in
; * whole or part by any means except for backup. Use, duplication,
; * or disclosure by the Government is subject to the restrictions
; * in paragraph(b)(3)(B)of the Rights in Technical Data and
; * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
; * Software owned by Advanced Micro Devices Inc., One AMD Place
; * P.O. Box 3453, Sunnyvale, CA 94088-3453.
; *
; *****************************************************************************
; *
; * AMD3D.INC
; *
; * AMD 3D instruction macros used to support AMD 3D instruction set extentions.
; *
; * You do not need to use this header if you have MASM 6.13 or later.
; *
; *****************************************************************************

opc_Pfcmpge   = 090H
opc_Pfcmpgt   = 0A0H
opc_Pfcmpeq   = 0B0H
opc_Pfmin     = 094H
opc_Pfmax     = 0A4H
opc_Pfmul     = 0B4H
opc_Pfrcp     = 096H
opc_Pfrcpit1  = 0A6H
opc_Pfrcpit2  = 0B6H
opc_Pfrsqrt   = 097H
opc_Pfrsqit1  = 0A7H
opc_Pmulhr    = 0B7H
opc_Pfsub     = 09AH
opc_Pfsubr    = 0AAH
opc_Pi2fd     = 00DH
opc_Pf2id     = 01DH
opc_Pfacc     = 0AEH
opc_Pfadd     = 09EH
opc_Pavgusb   = 0BFH

opc_Movd_ld   = 06EH
opc_Movd_st   = 07EH
opc_Movq_ld   = 06FH
opc_Movq_st   = 07FH
opc_Paddd     = 0FEH

nummm0	equ	0
nummm1	equ	1
nummm2	equ	2
nummm3	equ	3
nummm4	equ	4
nummm5	equ	5
nummm6	equ	6
nummm7	equ	7

say	macro	x
%	echo	x
	endm

amd3dop  macro   op:req, dst:req, src:req

if ((OPATTR(dst)) AND 00010000y) ; register mode
?SpecCase=0
if (OPATTR(src)) AND 00000100y ; immediate mode
	?SpecCase=1
endif
ifidn	<dst>,<mm0>
	?SpecCase=1
endif
ifidn	<dst>,<mM0>
	?SpecCase=1
endif
ifidn	<dst>,<Mm0>
	?SpecCase=1
endif
ifidn	<dst>,<MM0>
	?SpecCase=1
endif
if	?SpecCase
MMX_OPCODE = $
	cmpxchg   dword ptr src, dst
MMX_NEXT = $
	org	MMX_OPCODE+1
	DB	0Fh
	org	MMX_NEXT
	db	op
else	;~?SpecCase
	DB	0Fh
MMX_OPCODE = $
	mov dst, src
MMX_NEXT = $
	org	MMX_OPCODE
	db	0Fh
	org	MMX_NEXT
	db	op
endif	;~?SpecCase
else
  .err <*** Error: only register allowed as destination>
endif
	endm


.486P

IFDEF APP_16BIT
R0	TEXTEQU	<AX>
R1	TEXTEQU	<CX>
R2	TEXTEQU	<DX>
R3	TEXTEQU	<BX>
R4	TEXTEQU	<SP>
R5	TEXTEQU	<BP>
R6	TEXTEQU	<SI>
R7	TEXTEQU	<DI>

r0	TEXTEQU	<AX>
r1	TEXTEQU	<CX>
r2	TEXTEQU	<DX>
r3	TEXTEQU	<BX>
r4	TEXTEQU <SP>
r5	TEXTEQU	<BP>
r6	TEXTEQU	<SI>
r7	TEXTEQU	<DI>

ELSE
R0	TEXTEQU	<EAX>
R1	TEXTEQU	<ECX>
R2	TEXTEQU	<EDX>
R3	TEXTEQU	<EBX>
R4	TEXTEQU	<ESP>
R5	TEXTEQU	<EBP>
R6	TEXTEQU	<ESI>
R7	TEXTEQU	<EDI>

r0	TEXTEQU	<EAX>
r1	TEXTEQU	<ECX>
r2	TEXTEQU	<EDX>
r3	TEXTEQU	<EBX>
r4	TEXTEQU	<ESP>
r5	TEXTEQU	<EBP>
r6	TEXTEQU	<ESI>
r7	TEXTEQU	<EDI>


ENDIF

; ALIAS R# to MM# registers
IFDEF APP_16BIT
MM0	TEXTEQU	<AX>
MM1	TEXTEQU	<CX>
MM2	TEXTEQU	<DX>
MM3	TEXTEQU	<BX>
MM4	TEXTEQU	<SP>
MM5	TEXTEQU	<BP>
MM6	TEXTEQU	<SI>
MM7	TEXTEQU	<DI>

mm0	TEXTEQU	<AX>
mm1	TEXTEQU	<CX>
mm2	TEXTEQU	<DX>
mm3	TEXTEQU	<BX>
mm4	TEXTEQU	<SP>
mm5	TEXTEQU	<BP>
mm6	TEXTEQU	<SI>
mm7	TEXTEQU	<DI>

Mm0	TEXTEQU	<AX>
Mm1	TEXTEQU	<CX>
Mm2	TEXTEQU	<DX>
Mm3	TEXTEQU	<BX>
Mm4	TEXTEQU	<SP>
Mm5	TEXTEQU	<BP>
Mm6	TEXTEQU	<SI>
Mm7	TEXTEQU	<DI>

mM0	TEXTEQU	<AX>
mM1	TEXTEQU	<CX>
mM2	TEXTEQU	<DX>
mM3	TEXTEQU	<BX>
mM4	TEXTEQU	<SP>
mM5	TEXTEQU	<BP>
mM6	TEXTEQU	<SI>
mM7	TEXTEQU	<DI>

ELSE
MM0	TEXTEQU	<EAX>
MM1	TEXTEQU	<ECX>
MM2	TEXTEQU	<EDX>
MM3	TEXTEQU	<EBX>
MM4	TEXTEQU	<ESP>
MM5	TEXTEQU	<EBP>
MM6	TEXTEQU	<ESI>
MM7	TEXTEQU	<EDI>

mm0	TEXTEQU	<EAX>
mm1	TEXTEQU	<ECX>
mm2	TEXTEQU	<EDX>
mm3	TEXTEQU	<EBX>
mm4	TEXTEQU	<ESP>
mm5	TEXTEQU	<EBP>
mm6	TEXTEQU	<ESI>
mm7	TEXTEQU	<EDI>

Mm0	TEXTEQU	<EAX>
Mm1	TEXTEQU	<ECX>
Mm2	TEXTEQU	<EDX>
Mm3	TEXTEQU	<EBX>
Mm4	TEXTEQU	<ESP>
Mm5	TEXTEQU	<EBP>
Mm6	TEXTEQU	<ESI>
Mm7	TEXTEQU	<EDI>

mM0	TEXTEQU	<EAX>
mM1	TEXTEQU	<ECX>
mM2	TEXTEQU	<EDX>
mM3	TEXTEQU	<EBX>
mM4	TEXTEQU	<ESP>
mM5	TEXTEQU	<EBP>
mM6	TEXTEQU	<ESI>
mM7	TEXTEQU	<EDI>

ENDIF

?isReg	macro par
?Reg = 0
ifidn	  <par>,<eax>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Eax>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<EAX>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<ebx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Ebx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<EBX>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<ecx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Ecx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<ECX>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<edx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Edx>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<EDX>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<esi>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Esi>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<ESI>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<edi>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Edi>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<EDI>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<ebp>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<Ebp>
    ?Reg=1
    exitm
endif
ifidn	  <par>,<EBP>
    ?Reg=1
    exitm
endif
    endm

?isMMX	macro par
?MMXReg = 0
ifidn	  <par>,<mm0>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM0>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm0>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM0>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm1>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM1>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm1>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM1>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm2>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM2>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm2>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM2>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm3>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM3>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm3>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM3>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm4>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM4>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm4>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM4>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm5>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM5>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm5>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM5>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm6>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM6>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm6>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM6>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mm7>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<mM7>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<Mm7>
    ?MMXReg=1
    exitm
endif
ifidn	  <par>,<MM7>
    ?MMXReg=1
    exitm
endif
    endm

;-------------------------------------------------------------------

pavgusb macro    dst:req, src:req
        amd3dop         opc_Pavgusb, dst, src
        endm

pmulhr  macro    dst:req, src:req
        amd3dop         opc_Pmulhr, dst, src
        endm

pf2id   macro    dst:req, src:req
        amd3dop         opc_Pf2id, dst, src
        endm

pfacc   macro    dst:req, src:req
        amd3dop         opc_Pfacc, dst, src
        endm

pfadd   macro    dst:req, src:req
        amd3dop         opc_Pfadd, dst, src
        endm

pfcmpeq macro    dst:req, src:req
        amd3dop         opc_Pfcmpeq, dst, src
        endm

pfcmpge macro    dst:req, src:req
        amd3dop         opc_Pfcmpge, dst, src
        endm

pfcmpgt macro    dst:req, src:req
        amd3dop         opc_Pfcmpgt, dst, src
        endm

pfmax   macro    dst:req, src:req
        amd3dop         opc_Pfmax, dst, src
        endm

pfmin   macro    dst:req, src:req
        amd3dop         opc_Pfmin, dst, src
        endm

pfmul   macro    dst:req, src:req
        amd3dop         opc_Pfmul, dst, src
        endm

pfrcp   macro    dst:req, src:req
        amd3dop         opc_Pfrcp, dst, src
        endm

pfrcpit1 macro    dst:req, src:req
         amd3dop         opc_Pfrcpit1, dst, src
         endm

pfrcpit2 macro    dst:req, src:req
         amd3dop         opc_Pfrcpit2, dst, src
         endm

pfrsqit1 macro    dst:req, src:req
         amd3dop         opc_Pfrsqit1, dst, src
         endm

pfrsqrt macro    dst:req, src:req
        amd3dop         opc_Pfrsqrt, dst, src
        endm

pfsub   macro    dst:req, src:req
        amd3dop         opc_Pfsub, dst, src
        endm

pfsubr  macro    dst:req, src:req
        amd3dop         opc_Pfsubr, dst, src
        endm

pi2fd   macro    dst:req, src:req
        amd3dop         opc_Pi2fd, dst, src
        endm

;Needs special handling for the case when one parameter is a MMX register
;and another is a regular 32 bit register because they're not distinguished
;other then by name at macro level. So it's not clear by using just a register
;code whether it is a load or store type.

movd	macro	dst:req, src:req
?R2RDestMMX=0
if ((OPATTR(dst)) AND 00010000y) ; register mode
  if ((OPATTR(src)) AND 00010000y) ; register mode
      ?isMMX dst
      if  ?MMXReg
	?R2RDestMMX=1
      endif
MMX_OPCODE = $
      if ?R2RDestMMX
	cmpxchg	src, dst
      else
	cmpxchg	dst, src
      endif
MMX_NEXT = $
      org	MMX_OPCODE+1
      if ?R2RDestMMX
	 DB	opc_Movd_ld
      else
	 DB	opc_Movd_st
      endif
      org	MMX_NEXT
      exitm
  endif
endif
?SpecCase=0
ifidn	<dst>,<mm0>
	?SpecCase=1
endif
ifidn	<dst>,<mM0>
	?SpecCase=1
endif
ifidn	<dst>,<Mm0>
	?SpecCase=1
endif
ifidn	<dst>,<MM0>
	?SpecCase=1
endif
if	?SpecCase
MMX_OPCODE = $
	cmpxchg   dword ptr src, dst
MMX_NEXT = $
	org	MMX_OPCODE+1
	db	opc_Movd_ld

else	;;?SpecCase=0
	ifidn	<src>,<mm0>
	    ?SpecCase=1
	endif
	ifidn	<src>,<mM0>
	    ?SpecCase=1
	endif
	ifidn	<src>,<Mm0>
	    ?SpecCase=1
	endif
	ifidn	<src>,<MM0>
	    ?SpecCase=1
	endif
	if	?SpecCase
	    DB	0Fh
MMX_OPCODE = $
	    cmp dst, src
MMX_NEXT = $
	    org	MMX_OPCODE
	    DB	opc_Movd_st
	    org MMX_NEXT
	    exitm
	endif
	DB	0Fh
MMX_OPCODE = $
	mov dst, src
MMX_NEXT = $
	org	MMX_OPCODE
	if (OPATTR(dst)) AND 00010000y ; register
		DB	opc_Movd_ld
	else
		DB	opc_Movd_st
	endif

endif	;;?SpecCase=0|1

	org	MMX_NEXT
	endm

movq   macro   dst:req, src:req
?SpecCase=0
if (OPATTR(src)) AND 00000100y ; immediate mode
	?SpecCase=1
endif
ifidn	<dst>,<mm0>
	?SpecCase=1
endif
ifidn	<dst>,<mM0>
	?SpecCase=1
endif
ifidn	<dst>,<Mm0>
	?SpecCase=1
endif
ifidn	<dst>,<MM0>
	?SpecCase=1
endif
if	?SpecCase
MMX_OPCODE = $
	cmpxchg   dword ptr src, dst
MMX_NEXT = $
	org	MMX_OPCODE+1
	db	opc_Movq_ld

else	;;?SpecCase=0

	DB	0Fh
MMX_OPCODE = $
	mov dst, src
MMX_NEXT = $
	org	MMX_OPCODE
	if (OPATTR(dst)) AND 00010000y ; register
		DB	opc_Movq_ld
	else
		DB	opc_Movq_st
	endif

endif	;;?SpecCase=0|1

	org	MMX_NEXT
	endm

femms	macro
	db 0Fh, 0Eh
        endm

prefetch  macro   dst:req
	DB	0Fh
MMX_OPCODE = $
	inc dst
MMX_NEXT = $
	org	MMX_OPCODE
	if (OPATTR(dst)) AND 00010100y ; register
.err <*** Error: Instruction requires memory address>
	else
	db	0Dh
	endif
	org	MMX_NEXT
	endm

prefetchw macro  dst:req
	DB	0Fh
MMX_OPCODE = $
	inc dst
MMX_NEXT = $
	org	MMX_OPCODE
	if (OPATTR(dst)) AND 00010100y ; register
.err <*** Error: Instruction requires memory address>
	else
	db	0Dh
	endif
	org	MMX_NEXT
	endm

paddd	 macro	 dst:req, src:req
  if ((OPATTR(dst)) AND 00010000y) ; register mode
	DB	0Fh
MMX_OPCODE = $
	mov dst, src
MMX_NEXT = $
	org	MMX_OPCODE
	db	opc_Paddd
	org	MMX_NEXT
  else
  .err <*** Error: only register allowed as destination>
  endif
	endm
