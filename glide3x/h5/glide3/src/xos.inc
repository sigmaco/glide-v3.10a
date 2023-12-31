;
; compulsory header for h5/glide3/xdraw* assembly specializations (NASM)
;
; $Header: /cvsroot/glide/glide3x/h5/glide3/src/Attic/xos.inc,v 1.1.2.2 2003/06/13 07:22:59 dborca Exp $
; $Log: xos.inc,v $
; Revision 1.1.2.2  2003/06/13 07:22:59  dborca
; more fixes to NASM sources
;
; Revision 1.1.2.1  2003/06/07 09:53:25  dborca
; initial checkin for NASM sources
;

;---------------------------------------
; platform defines
;---------------------------------------
%define XOS_DJGPP 1
%define XOS_LINUX 2
%define XOS_WIN32 4

%define STDCALL   0

;---------------------------------------
; pick up the right OS
;---------------------------------------
%ifdef __DJGPP__
%define XOS XOS_DJGPP
%elifdef __linux__
%define XOS XOS_LINUX
%elifdef __WIN32__
%define XOS XOS_WIN32
%define STDCALL 1
%else
%error Unknown OS
%endif

;---------------------------------------
; general purpose macros
;---------------------------------------
%macro	extrn	1-2 0
	%if (XOS == XOS_WIN32) && STDCALL && (%2 > 0)
		%define %1 %1@%2
	%endif
		extern	%1
%endmacro

%macro	globl	1-2 0
	%if (XOS == XOS_WIN32) && STDCALL && (%2 > 0)
		%define %1 %1@%2
	%endif
		global	%1
%endmacro

%macro		proc	1-2 0
	%push	proc
	%if STDCALL && (%2 > 0)
		%define	%$ret	RET %2
	%else
		%define	%$ret	RET
	%endif
	globl	%1, %2
%1:
%endmacro

%macro		endp	0
	%ifnctx	proc
		%error	Mismatched `endp'/`proc'
	%else
		%pop
	%endif
%endmacro

%macro		ret	0
	%ifnctx	proc
		RET
	%else
		%$ret
	%endif
%endmacro

%macro invoke 1-*
	%rep	%0 - 1
	%rotate -1
	push	%1
	%endrep
	%rotate -1
	call	%1
%if (STDCALL == 0) && (%0 > 1)
	add	esp, 4 * (%0 - 1)
%endif
%endmacro

;---------------------------------------
; Windows
;---------------------------------------
%if XOS == XOS_WIN32

%define TEXT  .text   align=32
%define DATA  .data   align=32
%define CONST .rdata  align=32

%macro SET_TLSBASE 1
	mov	%1, dword [fs:18h] ; get thread local storage base pointer
%endmacro

%macro SET_TLSOFFSET 1
	mov	%1, [_GlideRoot+tlsOffset] ; offset of GC into tls
%endmacro

%macro GET_GC 2
	mov	gc, [%1 + %2]
%endmacro

%endif

;---------------------------------------
; DJGPP
;---------------------------------------
%if XOS == XOS_DJGPP

%define TEXT  .text
%define DATA  .data
%define CONST .rodata

extrn threadValueDJGPP

%macro SET_TLSBASE 1
%endmacro

%macro SET_TLSOFFSET 1
%endmacro

%macro GET_GC 2
	mov	gc, [threadValueDJGPP]
%endmacro

%endif

;---------------------------------------
; Linux
;---------------------------------------
%if XOS == XOS_LINUX

%define TEXT  .text   align=32
%define DATA  .data   align=32
%define CONST .rodata align=32

extrn threadValueLinux

%macro SET_TLSBASE 1
%endmacro

%macro SET_TLSOFFSET 1
%endmacro

%macro GET_GC 2
	mov	gc, [threadValueLinux]
%endmacro

%endif
