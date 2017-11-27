/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "FixedString.h"
#include "Import.h"


enum StatCode
{
	STAT_INCLUDE = -999,
	STAT_REPEAT,
	STAT_ENDR,
	STAT_MACRO,
	STAT_ENDM,
	STAT_EXITM,
	STAT_IF_TRUE,
	STAT_IF_FALSE,
	STAT_IF_UNDETERMINED,
	STAT_ELSE,
	STAT_ENDIF,
	STAT_ASM,
	STAT_SKIP,
	STAT_USER_DEF_ERR,			// b��d u�ytkownika (.ERROR)
	STAT_FIN,
	OK = 0,
	ERR_DAT,					// nieooczekiwane wyst�pienie danych (tu tylko komentarz)
	ERR_UNEXP_DAT,				// unexpected data in a first line position
	ERR_UNEXP_REG,				// register not allowed at the start of the line
	ERR_OUT_OF_MEM,
	ERR_FILE_READ,
	ERR_NUM_LONG,				// oczekiwana liczba max $FFFF
	ERR_NUM_NOT_BYTE,			// oczekiwana liczba max $FF
	ERR_NUM_NOT_LONG,			// oczekiwana liczba max $FFFFffff
	ERR_NUM_NEGATIVE,			// oczekiwana warto�� nieujemna
	ERR_NUM_NOT_POSITIVE,		// expected positive value
	ERR_INSTR_OR_NULL_EXPECTED,	// oczekiwana instrukcja, komentarz lub CR
	ERR_COMMA_OR_BRACKET_EXPECTED,	// oczekiwany przecinek lub nawias ')'
	ERR_BRACKET_R_EXPECTED,		// oczekiwany nawias ')'
	ERR_BRACKET_L_EXPECTED,		// oczekiwany nawias '('
	ERR_DIV_BY_ZERO,			// dzielenie przez zero w wyra�eniu
	ERR_EXPR_BRACKET_R_EXPECTED,// brak nawiasu ']' zamykaj�cego wyra�enie
	ERR_CONST_EXPECTED,			// oczekiwana sta�a (liczba lub ident)
	ERR_LABEL_REDEF,			// etykieta ju� zdefiniowana
	ERR_UNDEF_EXPR,				// nieokre�lona warto�� wyra�enia
	ERR_PC_WRAPED,				// "przewini�cie si�" licznika rozkaz�w
	ERR_UNDEF_LABEL,			// niezdefiniowana etykieta
	ERR_PHASE,					// b��d fazy - niezgodne warto�ci etykiety mi�dzy przebiegami
	ERR_REL_OUT_OF_RNG,			// przekroczenie zakresu dla adresowania wzgl�dnego
	ERR_MODE_NOT_ALLOWED,		// niedozwolony tryb adresowania
	ERR_STR_EXPECTED,			// oczekiwany �a�cuch znak�w
	ERR_SPURIOUS_ENDIF,			// wyst�pienie .ENDIF bez odpowiadaj�cego mu .IF
	ERR_SPURIOUS_ELSE,			// wyst�pienie .ELSE bez odpowiadaj�cego mu .IF
	ERR_ENDIF_REQUIRED,			// brak dyrektywy .ENDIF
	ERR_LOCAL_LABEL_NOT_ALLOWED,// wymagane jest wyst�pienie etykiety globalnej
	ERR_LABEL_EXPECTED,			// wymagana etykieta
	ERR_USER_ABORT,				// u�ytkownik przerwa� asemblacj�
	ERR_UNDEF_ORIGIN,			// brak dyrektywy .ORG
	ERR_MACRONAME_REQUIRED,		// brak etykiety nazywaj�cej makrodefinicj�
	ERR_PARAM_ID_REDEF,			// nazwa parametru ju� zdefiniowana
	ERR_NESTED_MACRO,			// definicja makra w makrodefinicji jest zabroniona
	ERR_ENDM_REQUIRED,			// brak dyrektywy .ENDM
	ERR_UNKNOWN_INSTR,			// nierozpoznana nazwa makra/instrukcji/dyrektywy
	ERR_PARAM_REQUIRED,			// brak wymaganej ilo�ci parametr�w wywo�ania makra
	ERR_SPURIOUS_ENDM,			// wyst�pienie .ENDM bez odpowiadaj�cego mu .MACRO
	ERR_SPURIOUS_EXITM,			// wyst�pienie .EXIT poza makrodefinicj� jest niedozwolone
	ERR_STR_NOT_ALLOWED,		// wyra�enie znakowe niedozwolone
	ERR_NOT_STR_PARAM,			// parametr wo�any z '$' nie posiada warto�ci typu tekstowego
	ERR_EMPTY_PARAM,			// wo�any parametr nie istnieje (za du�y nr przy odwo�aniu: %num)
	ERR_UNDEF_PARAM_NUMBER,		// numer parametru w wywo�aniu "%number" jest niezdefiniowany
	ERR_BAD_MACRONAME,			// nazwa makra nie mo�e zaczyna� si� od znaku '.'
	ERR_PARAM_NUMBER_EXPECTED,	// oczekiwany numer parametru makra
	ERR_LABEL_NOT_ALLOWED,		// niedozwolone wyst�pienie etykiety (przed dyrektyw�)
	ERR_BAD_REPT_NUM,			// b��dna ilo�� powt�rze� (dozwolone od 0 do 0xFFFF)
	ERR_SPURIOUS_ENDR,			// wyst�pienie .ENDR bez odpowiadaj�cego mu .REPEAT
	ERR_INCLUDE_NOT_ALLOWED,	// dyrektywa .INCLUDE nie mo�e wyst�powa� w makrach i powt�rkach
	ERR_STRING_TOO_LONG,		// za d�ugi �a�cuch (w .STR)
	//ERR_NOT_BIT_NUM,			// oczekiwana liczba od 0 do 7 (numer bitu)
	ERR_OPT_NAME_REQUIRED,		// brak nazwy opcji
	ERR_OPT_NAME_UNKNOWN,		// nierozpoznana nazwa opcji
	ERR_LINE_TO_LONG,			// za d�ugi wiersz �r�d�owy
	ERR_PARAM_DEF_REQUIRED,		// wymagana nazwa parametru makra
	ERR_CONST_LABEL_REDEF,		// Constant label (predefined) cannot be redefined
	ERR_NO_RANGE,				// expected valid range: first value has to be less than or equal to snd value
	ERR_NUM_ZERO,				// expected nonzero value
	ERR_NUM_NOT_WORD,			// expected word value (signed, -0x8000..0x7fff)
	ERR_NUM_NOT_IN_RANGE,		// number not in an expected range of values
	ERR_NUM_NOT_LEGAL,			// this particular number value is not legal (like 0 in AddQ)
	ERR_EXPECTED_NUMBER,
	ERR_NUM_NOT_FIT_BYTE,		// 
	ERR_NUM_NOT_FIT_WORD,		// 
	ERR_MISSING_COLON_MARK,		// missing ':' separating registers in REMU
	ERR_UNEXPECTED_INSTR_SIZE,	// instruction size attribute (.B, .L) is not expected (for unsized instruction)
	ERR_INVALID_INSTR_SIZE,		// instruction size attribute (.B, .L) is not valid for a given instruction
	ERR_MISSING_INSTR_SIZE,		// required instruction size attribute (.B, .L) is missing
	ERR_INVALID_REG_SIZE,		// indexing register can only be long, word is illegal
	ERR_INVALID_SRC_OPERAND,	// requested source effective address is not supported by given instruction
	ERR_INVALID_DEST_OPERAND,	// requested destination effective address is not supported by given instruction
	ERR_MISSING_COMMA_SEPARATOR,	// expected comma (',') before second operand
	ERR_SIZE_ATTR_NOT_RECOGNIZED,	// instruction size attribute is not recognized
	ERR_INVALID_ADDRESSING_MODE,
	ERR_ADDRESS_REG_EXPECTED,
	ERR_DATA_REG_EXPECTED,		//
	ERR_ADDR_REG_OR_PC_EXPECTED,
	ERR_INDEX_REG_EXPECTED,
	ERR_COMMA_OR_CLOSING_PARENT_EXPECTED,	// here either ',' or ')'
	ERR_MODE_NOT_RECOGNIZED,	// addressing mode not recognized
	ERR_EXPECTED_WORD_OR_LONG_ATTR,	// abs addressing mode supports only .w and .l size attributes
	ERR_REGISTER_LIST_EXPECTED,	// expected register list (for REG)
	ERR_REGISTER_EXPECTED,		// expected register to close range of registers in a list (MOVEM)
	ERR_MALFORMED_REG_LIST,		// list of registers is not in a valid form
	ERR_DUPLICATE_REG_LIST,		// duplicated register in a register list
	ERR_INVALID_RANGE_REG_LIST,	// invalid range of regiters in a register list (like D0-D3-D5)
	ERR_RELATIVE_OFFSET_OUT_OF_BYTE_RANGE,
	ERR_RELATIVE_OFFSET_OUT_OF_WORD_RANGE,
	ERR_INVALID_RELATIVE_OFFSET,
	ERR_AMBIGUOUS_INSTR,		// ambiguous instruction, specify instruction size to disambiguate
	ERR_CANNOT_CALC_SIZE,		// cannot calculate instruction size
	ERR_INSTR_SIZE_MISMATCH,	// internal error: generated size doesn't match calculated size
	ERR_INCLUDE_NOT_FOUND,		// included file not found

	ERR_INTERNAL,
	ERR_LAST					// sentry value; not an error
};
