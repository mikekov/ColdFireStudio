/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Assembler for ColdFire series of microcontrollers

#include "pch.h"
#include "resource.h"
#include "Asm.h"
#include "CFAsm.h"
#include "typeinfo.h"
#include "MarkArea.h"
#include "InstructionRepository.h"
#include <iostream>
#include <strstream>
#include <boost/algorithm/string/case_conv.hpp>
#include "Exceptions.h"


namespace masm {

enum { MASM_VERSION= 0x0001 };
uint32 g_io_addr= 0; // not used

//-----------------------------------------------------------------------------

Lexeme::Lexeme(const Lexeme& lex) : type_(lex.type_)
{
	union_ = lex.union_;
	instr_ = lex.instr_;
	str_ = lex.str_;
}

Lexeme::Lexeme(CpuRegister reg)
{
	if (reg >= R_D0 && reg <= R_D7)
		type_ = L_DATA_REG;
	else if (reg >= R_A0 && reg <= R_A7 || reg == R_SP)
		type_ = L_ADDR_REG;
	else
		type_ = L_REGISTER;
	union_.reg = reg;
}

CpuRegister Lexeme::GetRegister() const
{
	ASSERT(type_ == L_DATA_REG || type_ == L_ADDR_REG || type_ == L_REGISTER);
	return union_.reg;
}

//-----------------------------------------------------------------------------

IdentTable::IdentTable()
{
	map_.max_load_factor(0.7f);
	map_.reserve(20);
}


IdentTable::~IdentTable()
{}


bool IdentTable::lookup(FixedString str, Ident& ident) const
{
	auto it= map_.find(str);
	if (it == map_.end())
		return false;
	ident = it->second;
	return true;
}


void IdentTable::clr_table()
{
	map_.clear();
}


size_t IdentTable::size() const
{
	return map_.size();
}


bool IdentTable::insert(FixedString str, Ident& ident)
{
	Ident& val= map_[str];
	if (val.info == Ident::I_INIT)		// true -> nowy element, false -> ju¿ by³
	{
		val = ident;	// insert new identifier
		return true;
	}
	else
	{
		ident = val;	// return existing identifier
		return false;
	}
}


bool IdentTable::replace(FixedString str, const Ident& ident)
{
	Ident& val= map_[str];
	if (val.info == Ident::I_INIT)		// true -> nowy element, false -> ju¿ by³
	{
		val = ident;	// wpisanie nowego elementu
	}
	else
	{
		if ((val.variable || val.info == Ident::I_UNDEF) && ident.variable)
		{
			val = ident;	// zast¹pienie nowym elementem starej wartoœci zmiennej
			return true;
		}
		else if (val.variable || ident.variable)	// tutaj spr. albo, albo
		{
			return false;	// niedozwolone przedefiniowanie (zmiana typu ze sta³ej na zmienn¹ lub odwrotnie)
		}
		else if (val.info != Ident::I_UNDEF)	// stary element ju¿ zdefiniowany?
		{
			val = ident;	// zast¹pienie nowym elementem starego
			return false;	// zg³oszenie redefinicji
		}
		val = ident;	// zast¹pienie nowym elementem starego, niezdefiniowanego
	}
	return true;		// OK
}

//=============================================================================

char* InputFile::read_line(char* str, uint32 max_len)
{
	*str = 0;

	if (file_.eof())
		return str;

	file_.getline(str, max_len - 1);

	if (file_.good())
	{
		++line_;
		size_t len= strlen(str);
		str[len] = '\n';
		str[len + 1] = '\0';
	}

	return str;
}


void InputFile::open()
{
	auto path= get_file_name();
	try
	{
		file_.exceptions(std::ios::badbit);
		file_.open(path.c_str());
		opened_ = true;
	}
	catch (std::exception&)
	{
		throw std::exception(("Cannot open file " + path.string() /* + ": " + ex.what()*/).c_str());
	}
}


void InputFile::close()
{
	file_.close();
	opened_ = false;
}


void InputFile::seek_to_begin()
{
	if (file_.eof())
		file_.clear();
	file_.seekg(0);
	line_ = 0;
}

//-----------------------------------------------------------------------------

void Input::open_file(const Path& fname)
{
	std::unique_ptr<InputFile> file(::new InputFile(fname));
	file->open();
	stack_.push_back(file.release());
	fuid = (FileUID)calc_index();	// AddTail(tail) );
	tail_ = &stack_.back();
}


int Input::calc_index() const
{
	return static_cast<int>(stack_.size());
}


void Input::close_file()
{
	tail_->close();
	stack_.pop_back();
	tail_ = stack_.empty() ? nullptr : &stack_.back();
	if (tail_)
		fuid = (FileUID)calc_index();
	else
		fuid = 0;
}


Input::~Input()
{
	try
	{
		Stack::iterator end= stack_.end();
		for (Stack::iterator it= stack_.begin(); it != end; ++it)
			it->close();
	}
	catch (...)
	{
		ASSERT(false);
	}
}

//=============================================================================

void CFAsm::init_members()
{
	abort_asm_ = false;
	check_line_ = false;
	in_macro_ = nullptr;
	expanding_macro_ = nullptr;
	repeating_ = nullptr;
	rept_nested_ = 0;
	rept_init_ = 0;
	repeat_def_ = nullptr;
	case_sensitive_ = true;
}


void CFAsm::init(ISA isa)
{
	isa_ = isa;
	program_.SetIsa(isa);
	init_members();

	max_mnemonic_length_ = 0;
	min_mnemonic_length_ = 999999;

	auto instructions= GetInstructions().GetInstructions(isa);
	instructions_.reserve(instructions.size() * 3 / 2);
	for (auto i : instructions)
	{
		std::string mnemonic(i->Mnemonic());
		instructions_.insert(std::make_pair(mnemonic, i));

		if (max_mnemonic_length_ < mnemonic.length())
			max_mnemonic_length_ = mnemonic.length();
		if (min_mnemonic_length_ > mnemonic.length())
			min_mnemonic_length_ = mnemonic.length();

		if (i->AlternativeMnemonic())
		{
			std::string mnemonic(i->AlternativeMnemonic());
			instructions_.insert(std::make_pair(std::string(i->AlternativeMnemonic()), i));

			if (max_mnemonic_length_ < mnemonic.length())
				max_mnemonic_length_ = mnemonic.length();
			if (min_mnemonic_length_ > mnemonic.length())
				min_mnemonic_length_ = mnemonic.length();
		}
	}
}

//=============================================================================

#ifdef USE_UNICODE
	#define _istxdigit iswxdigit
	#define _istspace iswspace
	#define _istdigit iswdigit
	#define _istalpha isalpha
	#define _tcschr wcschr
	#define _totupper towupper
	#define _tcsicmp _wcsicmp
#else
	#define _istxdigit isxdigit
	#define _istspace isspace
	#define _istdigit isdigit
	#define _istalpha iswalpha
	#define _tcschr strchr
	#define _totupper toupper
	#define _tcsicmp _stricmp
#endif

int StrICmp(FixedString str, const char* text)
{
	return _tcsicmp(str.c_str(), text);
}

int StrICmp(const std::string& str, const char* text)
{
	return _tcsicmp(str.c_str(), text);
}

int StrICmp(const char* str1, const char* str2)
{
	return _tcsicmp(str1, str2);
}


InstructionSize size_attribute(char size_attr)
{
	if (size_attr == 'b' || size_attr == 'B')
		return IS_BYTE;
	else if (size_attr == 'w' || size_attr == 'W')
		return IS_WORD;
	else if (size_attr == 'l' || size_attr == 'L')
		return IS_LONG;
	else if (size_attr == 's' || size_attr == 'S')
		return IS_SHORT;

	return IS_NONE;
}


InstructionSize size_attribute(FixedString str)
{
	if (str.length() == 2 && str[0] == '.')
		return size_attribute(str[1]);

	return IS_NONE;
}


Lexeme CFAsm::next_lexeme(bool nospace)		// pobranie kolejnego symbolu
{
	if (!ptr_)
		return Lexeme(Lexeme::L_FIN);

	char c= *ptr_++;

	switch (c)
	{
	case '\n':
	case '\r':
		return Lexeme(Lexeme::L_EOL);

	case '\0':
		ptr_--;
		return Lexeme(Lexeme::L_FIN);

	case '$':
		if (!_istxdigit(*ptr_))		// znak '$' na koñcu parametru makra?
			return Lexeme(Lexeme::L_STR_ARG);
		break;
	case ';':
		return Lexeme(Lexeme::L_COMMENT);
	case ':':
		return Lexeme(Lexeme::L_LABEL);
	case '=':
		if (*ptr_ == '=')	// operator '==' równe?
		{
			ptr_++;
			return Lexeme(O_EQ);
		}
		return Lexeme(Lexeme::L_EQUAL);
	case '\'':
		return get_char_num();
	case '"':
		return get_string('"');
	case ',':
		return Lexeme(Lexeme::L_COMMA);

	case '(':
		return Lexeme(Lexeme::L_PARENTHESIS_L);
	case ')':
		return Lexeme(Lexeme::L_PARENTHESIS_R);
	case '[':
		return Lexeme(Lexeme::L_EXPR_BRACKET_L);
	case ']':
		return Lexeme(Lexeme::L_EXPR_BRACKET_R);

	case '>':
		if (*ptr_ == '>')		// operator '>>' przesuniêcia?
		{
			ptr_++;
			return Lexeme(O_SHR);
		}
		else if (*ptr_ == '=')	// operator '>=' wiêksze równe?
		{
			ptr_++;
			return Lexeme(O_GTE);
		}
		return Lexeme(O_GT);
	case '<':
		if (*ptr_ == '<')		// operator '<<' przesuniêcia?
		{
			ptr_++;
			return Lexeme(O_SHL);
		}
		else if (*ptr_ == '=')	// operator '<=' mniejsze równe?
		{
			ptr_++;
			return Lexeme(O_LTE);
		}
		return Lexeme(O_LT);
	case '&':
		if (*ptr_ == '&')	// operator '&&' ?
		{
			ptr_++;
			return Lexeme(O_AND);
		}
		return Lexeme(O_B_AND);
	case '|':
		if (*ptr_ == '|')	// operator '||' ?
		{
			ptr_++;
			return Lexeme(O_OR);
		}
		return Lexeme(O_B_OR);
	case '^':
		return Lexeme(O_B_XOR);
	case '+':
		return Lexeme(O_PLUS);
	case '-':
		return Lexeme(O_MINUS);
	case '*':
		if (*ptr_ == '=')	// operator '*=' .ORG?
		{
			ptr_++;
			return Lexeme(I_ORG);
		}
		return Lexeme(O_MUL);
	case '/':
		return Lexeme(O_DIV);
	case '%':
		return Lexeme(O_MOD);
	case '~':
		return Lexeme(O_B_NOT);
	case '!':
		if (*ptr_ == '=')	// operator '!=' ro¿ne?
		{
			ptr_++;
			return Lexeme(O_NE);
		}
		return Lexeme(O_NOT);

	case '#':
		return Lexeme(Lexeme::L_HASH);
	case '.':
		if (*ptr_ == '=')	// operator '.=' przypisania?
		{
			ptr_++;
			return Lexeme(I_SET);
		}
		else if (ptr_[0] == '.' && ptr_[1] == '.')	// wielokropek '...' ?
		{
			ptr_ += 2;
			return Lexeme(Lexeme::L_MULTI);
		}
		break;
	};

	if (_istspace(c))
	{
		if (!nospace)		// zwróciæ leksem L_SPACE?
			return eat_space();
		eat_space();
		return next_lexeme();
	}
	else if (_istdigit(c))	// cyfra dziesiêtna?
	{
		ptr_--;
		return get_dec_num();
	}
	else if (c == '$')		// liczba hex?
		return get_hex_num();
	else if (c == '@')		// liczba bin?
		return get_bin_num();
	else if (_istalpha(c) || c == '_' || c == '.' || c == '?')
	{
		ptr_--;

		FixedString str= get_ident();
		if (str.empty())
			return Lexeme(Lexeme::ERR_BAD_CHR);

		if (c == '.')		// it could be instruction size attribute, or assembly directive
		{
			InstructionSize size= size_attribute(str);
			if (size != IS_NONE)
				return Lexeme(size);

			InstrType it;
			if (asm_instr(str, it))
				return Lexeme(it);
		}
		else if (c != '.' && c != '_' && c != '?')	// to mo¿e byæ instrukcja
		{
			InstrType it;
			if (asm_instr(str, it))
				return Lexeme(it);

			CpuRegister reg= proc_register(str);
			if (reg != R_NONE)
				return Lexeme(reg);

			InstrRange range= proc_instr(str);
			if (range.first != range.second)
				return Lexeme(range);
		}
		if (*ptr_ == '#')			// znak '#' na koñcu etykiety?
		{
			ptr_++;
			return Lexeme(str, 1L);		// identyfikator numerowany (po '#' oczekiwana liczba)
		}
		return Lexeme(str, 1);	// L_IDENT
	}

	return Lexeme(Lexeme::L_UNKNOWN);	// niesklasyfikowany znak - b³¹d
}


Lexeme CFAsm::get_hex_num()		// interpretacja liczby szesnastkowej
{
	uint32 val= 0;
	const char* tmp= ptr_;

	if (!_istxdigit(*ptr_))
	{
		err_start_ = tmp;
		return Lexeme(Lexeme::ERR_NUM_HEX);	// oczekiwana cyfra liczby szesnastkowej
	}

	do
	{
		if (val & 0xF0000000)
		{
			err_start_ = tmp;
			return Lexeme(Lexeme::ERR_NUM_BIG);	// przekroczenie zakresu liczb 32-bitowych
		}

		char c= *ptr_++;
		val <<= 4;
		if (c >= 'a')
			val += c - 'a' + 10;
		else if (c >= 'A')
			val += c - 'A' + 10;
		else
			val += c - '0';
	} while (_istxdigit(*ptr_));

	return Lexeme(Lexeme::N_HEX, int32(val));
}


Lexeme CFAsm::get_dec_num()		// interpretacja liczby dziesiêtnej
{
	uint32 val= 0;
	const char* tmp= ptr_;

	if (!_istdigit(*ptr_))
	{
		err_start_ = tmp;
		return Lexeme(Lexeme::ERR_NUM_DEC);	// oczekiwana cyfra
	}

	do
	{
		if (val > ~0u / 10)
		{
			err_start_ = tmp;
			return Lexeme(Lexeme::ERR_NUM_BIG);	// przekroczenie zakresu liczb 32-bitowych
		}

		val *= 10;
		val += *ptr_++ - '0';

	} while (_istdigit(*ptr_));

	return Lexeme(Lexeme::N_DEC, int32(val));
}


Lexeme CFAsm::get_bin_num()		// interpretacja liczby dwójkowej
{
	uint32 val= 0;
	const char* tmp= ptr_;

	if (*ptr_ != '0' && *ptr_ != '1')
	{
		err_start_ = tmp;
		return Lexeme(Lexeme::ERR_NUM_HEX);	// oczekiwana cyfra liczby szesnastkowej
	}

	do
	{
		if (val & 0x80000000u)
		{
			err_start_ = tmp;
			return Lexeme(Lexeme::ERR_NUM_BIG);	// przekroczenie zakresu liczb 32-bitowych
		}

		val <<= 1;
		if (*ptr_++ == '1')
			val++;

	} while (*ptr_ == '0' || *ptr_ == '1');

	return Lexeme(Lexeme::N_BIN, int32(val));
}


Lexeme CFAsm::get_char_num()		// interpretacja sta³ej znakowej
{
	char c1= *ptr_++;	// pierwszy znak w apostrofie

	if (*ptr_ != '\'')
	{
		char c2 = *ptr_++;
		if (*ptr_ != '\'')
		{
			err_start_ = ptr_ - 2;
			return Lexeme(Lexeme::ERR_NUM_CHR);
		}
		ptr_++;		// ominiêcie zamykaj¹cego apostrofu
		return Lexeme(Lexeme::N_CHR2, ((c2 & 0xFF) << 8) + (c1 & 0xFF));
	}
	else
	{
		ptr_++;		// ominiêcie zamykaj¹cego apostrofu
		return Lexeme(Lexeme::N_CHR, c1 & 0xFF);
	}
}


FixedString CFAsm::get_ident()	// wyodrêbnienie napisu
{
	const char* start= ptr_;
	char c= *ptr_++;			// pierwszy znak

	if (!(_istalpha(c) || c == '_' || c == '.' || c == '?'))
	{
		err_start_ = start;
		return nullptr;
	}

	while (__iscsym(*ptr_))		// litera, cyfra lub '_'
		ptr_++;

	FixedString str(start, ptr_ - start);
	ident_start_ = start;		// zapamiêtanie po³o¿enia identyfikatora w wierszu
	ident_fin_ = ptr_;
	return str;
}


Lexeme CFAsm::get_string(char lim)		// wyodrêbnienie ³añcucha znaków
{
	const char* fin= _tcschr(ptr_, lim);

	if (fin == nullptr)
	{
		err_start_ = ptr_;
		return Lexeme(Lexeme::ERR_STR_UNLIM);
	}

	FixedString str(ptr_, fin - ptr_);

	ptr_ = fin + 1;

	return Lexeme(str);
}


Lexeme CFAsm::eat_space()			// ominiêcie odstêpu
{
	ptr_--;
	while (_istspace(*++ptr_) && *ptr_ != '\n' && *ptr_ != '\r')
		;		// white characters (but not CR)
	return Lexeme(Lexeme::L_SPACE);
}


template<class T> struct NameValue
{
	NameValue(const char* name, T t) : name(name), value(t)
	{}

	const char* name;
	T value;

	bool operator < (const NameValue<T>& a) const
	{
		return StrICmp(name, a.name) < 0;
	}
};

template<class T> NameValue<T> Pair(const char* name, T t)
{
	return NameValue<T>(name, t);
}

template<class T> const NameValue<T>* BinarySearch(const NameValue<T>* from, const NameValue<T>* to, const NameValue<T>& el)
{
	auto range= std::equal_range(from, to, el);
	if (range.first == range.second)
		return 0;
	return range.first;
}


CpuRegister CFAsm::proc_register(FixedString str)
{
	if (str.length() == 2)
	{
		if ((str[0] == 'd' || str[0] == 'D') && str[1] >= '0' && str[1] <= '7')
			return static_cast<CpuRegister>(R_D0 + str[1] - '0');

		if ((str[0] == 'a' || str[0] == 'A') && str[1] >= '0' && str[1] <= '7')
			return static_cast<CpuRegister>(R_A0 + str[1] - '0');
	}

	static const NameValue<CpuRegister> reg[]=
	{
		// registers in alphabetical order
		Pair("ACC0", R_ACC0),
		Pair("ACC1", R_ACC1),
		Pair("ACC2", R_ACC2),
		Pair("ACC3", R_ACC3),
		Pair("ACR0", R_ACR0),
		Pair("ACR1", R_ACR1),
		Pair("ACR2", R_ACR2),
		Pair("ACR3", R_ACR3),
		Pair("ASID", R_ASID),
		Pair("CACR", R_CACR),
		Pair("CCR", R_CCR),
		Pair("MBAR", R_MBAR),
		Pair("MMUBAR", R_MMUBAR),
		Pair("PC", R_PC),
		Pair("RAMBAR0", R_RAMBAR0),
		Pair("RAMBAR1", R_RAMBAR1),
		Pair("ROMBAR0", R_ROMBAR0),
		Pair("ROMBAR1", R_ROMBAR1),
		Pair("SP", R_SP),
		Pair("SR", R_SR),
		Pair("USP", R_USP),
		Pair("VBR", R_VBR),
	};

	const NameValue<CpuRegister>* r= BinarySearch(reg, reg + array_count(reg), Pair(str.c_str(), R_NONE));

	return r ? r->value : R_NONE;
}


InstrRange CFAsm::proc_instr(FixedString str)
{
	const size_t length= str.length();

	if (length < min_mnemonic_length_ || length > max_mnemonic_length_)
		return InstrRange(instructions_.end(), instructions_.end());

	// prepare uppercase version of identifier
	std::string copy= str.c_str();
	for (size_t i= 0; i < length; ++i)
	{
		char c= copy[i];
		if (c >= 'a' && c <= 'z')		// this is all that's needed for mnemonics (no Unicode chars)
			copy[i] = c - ('a' - 'A');
	}

	InstrRange range= instructions_.equal_range(copy);

	return range;
}


int __cdecl CFAsm::asm_str_key_cmp(const void* elem1, const void* elem2)
{
	return _tcsicmp(((CFAsm::ASM_STR_KEY*)elem1)->str, ((CFAsm::ASM_STR_KEY*)elem2)->str);
}


bool CFAsm::asm_instr(FixedString str, InstrType& it)
{	// check if 'str' identifies assembly directive
	static const ASM_STR_KEY instr[]=
	{	// assembler directives in alphabetical order!
		"ALIGN",	I_ALIGN,
		"DC",		I_DC,		// declare bytes
		"DCB",		I_DCB,		// declare block
		"DS",		I_DS,		// reserve space (define space)
		"ELSE",		I_ELSE,
		//"ELSEIF",		I_ELSEIF,
		"END",		I_END,		// zakoñczenie programu (pliku)
		"ENDIF",	I_ENDIF,	// koniec .IF
		"ENDM",		I_ENDM,		// koniec .MACRO
		"ENDR",		I_ENDR,		// koniec .REPEAT
		"ERROR",	I_ERROR,	// zg³oszenie b³êdu
		"EXITM",	I_EXITM,	// zakoñczenie rozwijania makra
		"IF",		I_IF,		// asemblacja warunkowa
		"INCBIN",	I_INCLUDE_BIN,	// insert binary file
		"INCLUDE",	I_INCLUDE,	// w³¹czenie pliku do asemblacji
		//"IO_WND",	I_IO_WND,	// I/O terminal window size
		"MACRO",	I_MACRO,	// makrodefinicja
		"OPT",		I_OPT,		// opcje asemblera
		"ORG",		I_ORG,		// origin
		"REPEAT",	I_REPEAT,	// powtórka
		//"ROM_AREA", I_ROM_AREA,	// protected memory area
		"SECTION",	I_SECTION,
		"SET",		I_SET,		// przypisanie wartoœci
	};
	ASM_STR_KEY find;
	find.str = str.c_str();

	void* ret= bsearch(&find, instr, sizeof instr / sizeof(ASM_STR_KEY), sizeof(ASM_STR_KEY), asm_str_key_cmp);

	if (ret)
	{
		it = ((ASM_STR_KEY*)ret)->it;
		return true;
	}

	return false;
}


int RegNumber(CpuRegister reg)
{
	if (reg >= R_D0 && reg <= R_D7)
		return static_cast<int>(reg - R_D0);
	if (reg >= R_A0 && reg <= R_A7)
		return 8 + static_cast<int>(reg - R_A0);
	if (reg == R_SP)
		return 15;	// same as A7
	return -1;
}


void ConstructRegList(CpuRegister reg_from, CpuRegister reg_to, uint16& register_mask)
{
	ASSERT(reg_from >= R_D0 && reg_from <= R_SP);
	ASSERT(reg_to >= R_D0 && reg_to <= R_SP);

	if (reg_from > reg_to || reg_from < R_A0 && reg_to >= R_A0)
		throw ERR_MALFORMED_REG_LIST;

	int from= RegNumber(reg_from);
	int to= RegNumber(reg_to);

	if (from < 0 || to < 0)
		throw LogicError("invalid register in " __FUNCTION__);

	uint16 mask= 1 << from;

	for (int i= from; i <= to; ++i)
	{
		if (mask & register_mask)
			throw ERR_DUPLICATE_REG_LIST;
		register_mask |= mask;
		mask <<= 1;
	}
}


// check if list of registers have been specified (for MOVEM)
bool CFAsm::parse_reg_list(Lexeme& lex, EffectiveAddress& ea)
{
	// expected register on entry
	ASSERT(ea.mode_ == AM_Dx || ea.mode_ == AM_Ax);

	enum State { Range, NextReg, Register, Finish } state= Register;

	CpuRegister reg= ea.first_reg_;
	ea.register_mask = 0;
	bool has_reg_list= false;

	for (;;)
	{
		switch (state)
		{
		case Range:
			if (reg == R_NONE)
				throw ERR_INVALID_RANGE_REG_LIST;
			// expected closing register for a range (Rx-Ry)
			if (lex == Lexeme::L_DATA_REG || lex == Lexeme::L_ADDR_REG)
				ConstructRegList(reg, lex.GetRegister(), ea.register_mask);
			else
				throw ERR_REGISTER_EXPECTED;

			reg = R_NONE;
			lex = next_lexeme();
			state = Register;
			break;

		case NextReg:
			if (reg != R_NONE)
				ConstructRegList(reg, reg, ea.register_mask);

			// expected register in a list following '/'
			if (lex == Lexeme::L_DATA_REG || lex == Lexeme::L_ADDR_REG)
				reg = lex.GetRegister();
			else
				throw ERR_REGISTER_EXPECTED;

			lex = next_lexeme();
			state = Register;
			break;

		case Register:
			state = Finish;
			if (lex == Lexeme::L_OPER)
			{
				OperType op= lex.GetOper();
				if (op == O_MINUS)
					state = Range;
				else if (op == O_DIV)
					state = NextReg;
			}

			if (state != Finish)
			{
				has_reg_list = true;
				lex = next_lexeme();
			}

			break;

		case Finish:
			if (reg != R_NONE)
				ConstructRegList(reg, reg, ea.register_mask);

			if (has_reg_list)
				ea.mode_ = AM_REG_LIST;
			else
				ea.mode_ |= AM_REG_LIST;	// we have one register; it can be consiered a single element list too

			return has_reg_list;
		}
	}

//	return has_reg_list;
}


Stat CFAsm::parse_indexing_register(Lexeme& lex, EffectiveAddress& ea)
{
	// index register expected here
	lex = next_lexeme();
	if (lex != Lexeme::L_ADDR_REG && lex != Lexeme::L_DATA_REG)
		return ERR_INDEX_REG_EXPECTED;
	ea.second_reg_ = lex.GetRegister();
	lex = next_lexeme();
	if (lex == Lexeme::L_OPER && lex.GetOper() == O_MUL)	// scale?
	{
		lex = next_lexeme();
		ea.scale_ = expression(lex);
	}

	return OK;
}


// helper function to parse closing sequence of indirect addressing modes
Stat CFAsm::parse_indirect_modes(Lexeme& lex, EffectiveAddress& ea)
{
	bool pc_relative= false;
	bool indexed= false;
	lex = next_lexeme();
	// address register or PC expected here
	if (lex == Lexeme::L_ADDR_REG)
		ea.first_reg_ = lex.GetRegister();
	else if (lex == Lexeme::L_REGISTER && lex.GetRegister() == R_PC)
		ea.first_reg_ = R_PC, pc_relative = true;
	else
		return ERR_ADDR_REG_OR_PC_EXPECTED;
	lex = next_lexeme();
	if (lex == Lexeme::L_COMMA)	// indexed mode?
	{
		Stat stat= parse_indexing_register(lex, ea);
		if (stat)
			return stat;
		indexed = true;
	}
	if (lex != Lexeme::L_PARENTHESIS_R)
		return ERR_BRACKET_R_EXPECTED;
	lex = next_lexeme();
	if (indexed)
		ea.mode_ = pc_relative ? AM_DISP_PC_Ix : AM_DISP_Ax_Ix;
	else
		ea.mode_ = pc_relative ? AM_DISP_PC : AM_DISP_Ax;

	if (pc_relative)
	{
		// adjust offset from it's absolute form to be PC-relative
		//
		if (ea.val_.inf != Expr::EX_UNDEF)
		{
			//ctx.
		}
	}

	return OK;
}

namespace {

CpuRegister MaskToRegister(uint16 mask)
{
	CpuRegister reg= CpuRegister::R_NONE;

	for (int i= 0; i < 16; ++i)
		if (mask & (1 << i))
			return static_cast<CpuRegister>(CpuRegister::R_D0 + i);

	return reg;
}

}

// parse input operand; accept only addressing 'modes'; modify 'ea' on exit to indicate recognized addressing mode(s) and parameters
//
Stat CFAsm::parse_operand(Lexeme& lex, AddressingMode modes, const PossibleInstructions& instr, EffectiveAddress& ea)
{
	return parse_operand(lex, modes, instr, ea, true);
}

RegisterWord CFAsm::parse_register_size(Lexeme& lex)
{
	if (lex.Type() == Lexeme::L_IDENT)
	{
		auto& id= lex.GetIdent();
		if (id == ".u" || id == ".U")
		{
			lex = next_lexeme();
			return RegisterWord::UPPER;
		}
	}
	else if (lex.Type() == Lexeme::L_INSTR_SIZE)
	{
		if (lex.GetInstrSize() == IS_LONG)	// .L stands for long most of the time, but in MAC it's 'Lower'
		{
			lex = next_lexeme();
			return RegisterWord::LOWER;
		}
	}

	return RegisterWord::NONE;
}

Stat CFAsm::parse_operand(Lexeme& lex, AddressingMode modes, const PossibleInstructions& instr, EffectiveAddress& ea, bool first_part)
{
	// recognize/accept the following:
	// Dx
	// Ax
	// (Ax)
	// (Ax)+
	// -(Ax)
	// (expr, Ax)
	// (expr, Ax, Xi*scale)
	// (expr).w, expr.w
	// (expr).l, expr.l
	// expr
	// #expr
	// (expr, PC)
	// (expr, PC, Xi*scale)
	// list of registers for MOVEM (dx/dy-dz)
	//   where expr is a constant expression that can be calculated during assembly time

	ea.mode_ = AM_NONE;

	if (lex.Type() == Lexeme::L_SPACE)
		lex = next_lexeme();

	switch (lex.Type())
	{
	case Lexeme::L_DATA_REG:
		ea.mode_ = AM_Dx;
		ea.first_reg_ = lex.GetRegister();
		lex = next_lexeme();
		if (modes & AM_REG_LIST)
		{
			parse_reg_list(lex, ea);
		}
		else if (modes == AM_Dx_Dy)
		{
			if (lex.Type() != Lexeme::L_LABEL)	// ':'?
				return ERR_MISSING_COLON_MARK;

			lex = next_lexeme();
			if (lex.Type() != Lexeme::L_DATA_REG)
				return ERR_DATA_REG_EXPECTED;

			ea.mode_ = AM_Dx_Dy;
			ea.second_reg_ = lex.GetRegister();
			lex = next_lexeme();
		}
		else if (modes == AM_Rx_Ry)
		{
			// parse upper/lower register word
			auto part= parse_register_size(lex);
			if (first_part)
				ea.first_reg_word_ = part;
			else
				ea.second_reg_word_ = part;
		}
		break;

	case Lexeme::L_ADDR_REG:
		ea.mode_ = AM_Ax;
		ea.first_reg_ = lex.GetRegister();
		lex = next_lexeme();
		if (modes & AM_REG_LIST)
			parse_reg_list(lex, ea);
		else if (modes == AM_Rx_Ry)
		{
			// parse upper/lower register word
			auto part= parse_register_size(lex);
			if (first_part)
				ea.first_reg_word_ = part;
			else
				ea.second_reg_word_ = part;
		}
		break;

	case Lexeme::L_REGISTER:
		ea.mode_ = AM_SPEC_REG;
		ea.first_reg_ = lex.GetRegister();
		lex = next_lexeme();
		break;

	case Lexeme::L_HASH:			// '#' immediate mode
		ea.mode_ = AM_IMMEDIATE;
		lex = next_lexeme();
		ea.val_ = expression(lex);
		break;

	case Lexeme::L_PARENTHESIS_L:	// opening '('
		lex = next_lexeme();
		if (is_expression(lex))
		{
			ea.val_ = expression(lex);

			if (lex == Lexeme::L_PARENTHESIS_R)	// closing ')'
			{
				ea.mode_ = AM_ABS_L;	// defaults to .L if not explicitly stated
				lex = next_lexeme();
				if (lex == Lexeme::L_INSTR_SIZE)
				{
					InstructionSize size= lex.GetInstrSize();
					// check/modify expression to match requested size
					ea.val_;
					//todo

					// modify
					if (size == IS_WORD)
						ea.mode_ = AM_ABS_W;
					else if (size == IS_LONG)
						ea.mode_ = AM_ABS_L;
					else
						return ERR_EXPECTED_WORD_OR_LONG_ATTR;

					lex = next_lexeme();
				}
			}
			else if (lex == Lexeme::L_COMMA)	// (expr, register...
			{
				Stat ret= parse_indirect_modes(lex, ea);
				if (ret)
					return ret;
			}
			else
				return ERR_COMMA_OR_CLOSING_PARENT_EXPECTED;
		}
		else
		{
			// here expected address register
			if (lex != Lexeme::L_ADDR_REG)
				return ERR_ADDRESS_REG_EXPECTED;
			ea.first_reg_ = lex.GetRegister();
			lex = next_lexeme();
			if (ea.mode_ == AM_NONE && lex == Lexeme::L_COMMA)
			{
				// this could be (An, Dx) without offset
				Stat stat= parse_indexing_register(lex, ea);
				if (stat)
					return stat;
				if (lex == Lexeme::L_INSTR_SIZE)
				{
					if (lex.GetInstrSize() == IS_LONG)	// OK, just ignore it
						lex = next_lexeme();
					else
						return ERR_INVALID_REG_SIZE;
				}

				if (lex != Lexeme::L_PARENTHESIS_R)
					return ERR_BRACKET_R_EXPECTED;

				lex = next_lexeme();
				ea.val_ = Expr(0);	// no offset given
				ea.mode_ = AM_DISP_Ax_Ix;
			}
			else
			{
				if (lex != Lexeme::L_PARENTHESIS_R)
					return ERR_BRACKET_R_EXPECTED;
				lex = next_lexeme();
				if (lex == Lexeme::L_OPER && lex.GetOper() == O_PLUS)	// postincrement mode?
				{
					ea.mode_ = AM_Ax_INC;
					lex = next_lexeme();
				}
				else
					ea.mode_ = AM_INDIRECT_Ax;
			}
		}
		break;

	case Lexeme::L_EOL:
		// no data follows, it could only be implied addressing mode
		ea.mode_ = AM_IMPLIED;
		break;

	case Lexeme::L_OPER:
/*		if (lex.GetOper() == O_B_AND && (modes & AM_MAC_MASK))
		{
			ea.mode_ |= AM_MAC_MASK;
			modes &= ~AM_MAC_MASK;
			lex = next_lexeme();
			break;
		}
		else*/ if (lex.GetOper() == O_MINUS)
		{
			const char* temp= ptr_;
			Lexeme temp_lex= lex;
			lex = next_lexeme();
			if (lex == Lexeme::L_PARENTHESIS_L)
			{
				// -(An) mode expected
				lex = next_lexeme();
				if (lex != Lexeme::L_ADDR_REG)
					return ERR_ADDRESS_REG_EXPECTED;
				ea.first_reg_ = lex.GetRegister();
				lex = next_lexeme();
				if (lex != Lexeme::L_PARENTHESIS_R)
					return ERR_BRACKET_R_EXPECTED;
				lex = next_lexeme();
				ea.mode_ = AM_DEC_Ax;
				break;
			}

			ptr_ = temp;	// 'rewind' last next_lexeme() call
			lex = temp_lex;
		}
		// no break, continue:
	default:
		//
		if (lex == Lexeme::L_IDENT) //todo: numbered params: lex == Lexeme::L_OPER && lex.GetOper() == O_MOD)
		{
			auto expr= find_ident(lex, lex.GetString());

			if (expr.inf == Expr::EX_REGISTER || expr.inf == Expr::EX_REG_LIST)
			{
				// identifier contains register or register list; parse this operand
				auto reg= MaskToRegister(expr.RegMask());
				lex = Lexeme(reg);
				return parse_operand(lex, modes, instr, ea);
			}
		}
/*
		if ((modes & AM_REG_LIST) && lex.Type() == Lexeme::L_IDENT)
		{
			auto expr= find_ident(lex, lex.GetString());

			if (expr.inf == Expr::EX_REG_LIST)
			{
				ea.mode_ = AM_REG_LIST;	//TODO: distinguish single reg from multiple?
				ea.register_mask = uint16(expr.RegMask());
				break;
			}
		}
*/
		if (is_expression(lex))
		{
			// number/expression may form absolute addressing mode, or be placed in front
			// of (An), or (PC) for instance

			ea.val_ = expression(lex);

			if (ea.val_.inf == Expr::EX_STRING || (pass_ == 2 && !ea.val_.IsDefined()))
				return ERR_CONST_EXPECTED;

			if (ea.mode_ == AM_NONE && lex == Lexeme::L_PARENTHESIS_L)	// indirect modes?
			{
				// expr(An), expr(PC), expr(An, Dn)

				Stat ret= parse_indirect_modes(lex, ea);
				if (ret)
					return ret;
			}
			else
			{
				ea.mode_ = /*AM_ABS_W |*/ AM_ABS_L;	// defaults to long

				// todo: accept instr. size?
				if (lex == Lexeme::L_INSTR_SIZE)
				{
					switch (lex.GetInstrSize())
					{
					case IS_WORD:
						ea.mode_ = AM_ABS_W;
						lex = next_lexeme();
						break;
					case IS_LONG:
						ea.mode_ = AM_ABS_L;
						lex = next_lexeme();
						break;
					default:
						break;
					}
				}
			}
		}
		else
			return ERR_MODE_NOT_RECOGNIZED;

		break;
	}

	if ((ea.mode_ & modes) == 0)
	{
		if ((modes == AM_RELATIVE ||	// relative mode looks like absolute
			 modes == (AM_RELATIVE | AM_IMPLIED)) &&
			(ea.mode_ == AM_ABS_W || ea.mode_ == AM_ABS_L))
		{
			ea.mode_ = AM_RELATIVE;
		}
		else if (modes == AM_Rx_Ry && (ea.mode_ == AM_Dx || ea.mode_ == AM_Ax))
		{
			if (first_part)
			{
				auto reg_x= ea.first_reg_;

				// parse next source register
				if (lex == Lexeme::L_COMMA)
				{
					lex = next_lexeme();
					auto stat= parse_operand(lex, modes, instr, ea, false);
					if (stat)
						return stat;
					if (ea.mode_ == AM_Dx || ea.mode_ == AM_Ax)
					{
						ea.mode_ = AM_Rx_Ry;
						ea.second_reg_ = ea.first_reg_;
						ea.first_reg_ = reg_x;

						// parse optional scale factor field (<<, >>)

						if (lex == Lexeme::L_OPER)
						{
							auto op= lex.GetOper();
							if (op == O_SHL || op == O_SHR)
							{
								lex = next_lexeme();
								ea.shift_ = op == O_SHL ? ShiftFactor::LEFT : ShiftFactor::RIGHT;
							}
						}
					}
					else
						return ERR_INVALID_ADDRESSING_MODE;
				}
				else
					return ERR_INVALID_ADDRESSING_MODE;	// expected second register
			}
			return OK;
		}
		else
			return ERR_INVALID_ADDRESSING_MODE;
	}

	ea.mode_ &= modes;

	return OK;
}


InstructionSize ListOfSizes(const PossibleInstructions& instr)
{
	InstructionSize sizes= IS_NONE;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
		sizes |= instr[i]->SupportedSizes();

	if (sizes == IS_NONE)
		throw LogicError("internal error - list of instruction sizes expected " __FUNCTION__);

	return sizes;
}

enum Operand { SourceOp, DestinationOp, SecondSourceOp, SecondDestinationOp };

AddressingMode ListOfModes(const PossibleInstructions& instr, Operand op)
{
	AddressingMode modes= AM_NONE;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
		if (op == SourceOp)
			modes |= instr[i]->SourceEAModes();
		else if (op == DestinationOp)
			modes |= instr[i]->DestinationEAModes();
		else if (op == SecondSourceOp)
			modes |= instr[i]->SecondSourceEAModes();
		else if (op == SecondDestinationOp)
			modes |= instr[i]->SecondDestinationEAModes();

	return modes;
}

AddressingMode ListOfAllModes(const PossibleInstructions& instr)
{
	// ignore second source/destination modes here

	AddressingMode src= ListOfModes(instr, SourceOp);
	AddressingMode dst= ListOfModes(instr, DestinationOp);
	AddressingMode ea= src | dst;
	if (ea == AM_NONE)
		throw LogicError("internal error - list of addressing modes expected " __FUNCTION__);
	return ea;
}


// scan list of possible instructions and leave only those that match 'size' attribute
//
bool LimitInstructions(PossibleInstructions& instr, InstructionSize size)
{
	bool changes= false;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
		if ((size & instr[i]->SupportedSizes()) == 0)
		{
			instr.clear(i); // eliminate this instruction
			changes = true;
		}

	if (changes)
		instr.compact();

	return changes;
}


bool LimitInstructions(PossibleInstructions& instr, const EffectiveAddress& ea, Operand op)
{
	bool changes= false;
	const AddressingMode sel_mode= ea.mode_;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
	{
		AddressingMode modes= AM_NONE;
		switch (op)
		{
		case SourceOp:
			modes = instr[i]->SourceEAModes();
			break;
		case DestinationOp:
			modes = instr[i]->DestinationEAModes();
			break;
		case SecondSourceOp:
			modes = instr[i]->SecondSourceEAModes();
			break;
		case SecondDestinationOp:
			modes = instr[i]->SecondDestinationEAModes();
			break;
		default:
			assert(false);
			break;
		}

		if (sel_mode & modes)
		{
			// included till now; check special registers
			if ((sel_mode & AM_SPEC_REG) && (modes & AM_SPEC_REG))
			{
				if (instr[i]->SupportedSpecialRegisters().count(ea.first_reg_) == 0)
					modes &= ~AM_SPEC_REG;
			}
		}

		if ((sel_mode & modes) == 0)
		{
			instr.clear(i); // eliminate this instruction
			changes = true;
		}
	}

	if (changes)
		instr.compact();

	return changes;
}


bool RemoveInstrWithoutDefaultSize(PossibleInstructions& instr)
{
	bool changes= false;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
	{
		InstructionSize s= instr[i]->SupportedSizes();
		if (s != IS_NONE && s != IS_UNSIZED && instr[i]->DefaultSize() == IS_NONE)
		{
			instr.clear(i); // eliminate this instruction
			changes = true;
		}
	}

	if (changes)
		instr.compact();

	return changes;
}


bool RemoveInstrWithSecondSource(PossibleInstructions& instr)
{
	bool changes= false;

	const size_t count= instr.count();
	for (size_t i= 0; i < count; ++i)
	{
		if (instr[i]->SecondSourceEAModes() != AM_NONE)
		{
			instr.clear(i); // eliminate this instruction
			changes = true;
		}
	}

	if (changes)
		instr.compact();

	return changes;
}


// interpretacja argumentów rozkazu procesora
Stat CFAsm::proc_instr_syntax(Lexeme& lex, AsmInstruction& asm_instr)
{
	// recognize instruction syntax: Mnemonic [.size] [src] [, dest]

	enum	// automaton's states
	{
		START,
		SIZE_ATTR_EXPECTED,
		TEST_SIZE_ATTR,			// test explicit size attribute
		MNEMONIC,
		SRC_OPERAND_EXPECTED,
		OPERAND_SEPARATOR_EXPECTED,
		SECOND_SRC_OPERAND_EXPECTED,
		DEST_OPERAND_SEPARATOR_EXPECTED,
		DEST_OPERAND_EXPECTED,
		SECOND_OPERAND_SEPARATOR_EXPECTED,
		SECOND_DEST_OPERAND_EXPECTED,
		END_OF_LINE_EXPECTED,
	} state= START;

	InstructionSize sizes= ListOfSizes(asm_instr.list);
	InstructionSize explicit_size= IS_NONE;

	for (;;)
	{
		switch (state)
		{
		case START:
			switch (lex.Type())
			{
			case Lexeme::L_INSTR_SIZE:				// instruction size attribute (.B, .W, etc.)
				explicit_size = lex.GetInstrSize();
				state = TEST_SIZE_ATTR;
				lex = next_lexeme();
				break;

			case Lexeme::L_IDENT:				// identifier following instruction mnemonic (without separating space)
				if (lex.GetIdent() == ".")
				{
					state = SIZE_ATTR_EXPECTED;
					lex = next_lexeme();
				}
				else if (lex.GetIdent()[0] == '.')
					return ERR_SIZE_ATTR_NOT_RECOGNIZED;	// bogus size attrib (or else it would have been L_INSTR_SIZE)
				else
					state = MNEMONIC;
				break;

			case Lexeme::L_SPACE:
				state = MNEMONIC;
				break;

			default:
				state = MNEMONIC;
				break;
			}
			break;

		case SIZE_ATTR_EXPECTED:
			explicit_size = IS_NONE;
			if (lex == Lexeme::L_IDENT)
			{
				FixedString str= lex.GetIdent();
				if (str.length() == 1)
					explicit_size = size_attribute(str[0]);
			}
			else if (expanding_macro_ && lex == Lexeme::L_OPER && lex.GetOper() == O_MOD)
			{
				lex = next_lexeme();
				if (lex == Lexeme::L_IDENT && StrICmp(lex.GetIdent().c_str(), "size") == 0)
				{
					explicit_size = expanding_macro_->SizeAttribute();
					if (explicit_size == IS_NONE)
					{
						lex = next_lexeme();
						state = MNEMONIC;
						break;	// this is OK, size was not specified during macro call, go on
					}
				}
			}

			if (explicit_size != IS_NONE)
			{
				state = TEST_SIZE_ATTR;
				lex = next_lexeme();
			}
			else
				return ERR_SIZE_ATTR_NOT_RECOGNIZED;	// input sequence not recognized

			break;

		case TEST_SIZE_ATTR:
			ASSERT(explicit_size != IS_NONE && explicit_size != IS_UNSIZED);
			if (sizes == IS_UNSIZED)
				return ERR_UNEXPECTED_INSTR_SIZE;	// this instruction is unsized, it's an error to specify size

			if ((sizes & explicit_size) == 0)
				return ERR_INVALID_INSTR_SIZE;		// this instruction doesn't support requested size

			if (LimitInstructions(asm_instr.list, explicit_size))	// limit possible instructions here
				if (asm_instr.list.empty()) // this shouldn't happen, b/c of above test...
					return ERR_INVALID_INSTR_SIZE;	// requested size is not supported

			asm_instr.requested_size = explicit_size;
			state = MNEMONIC;
			break;

		case MNEMONIC:								// we already have a valid mnemonic
			if (explicit_size == IS_NONE)
			{
				// size not provided, check instruction
				// if instruction doesn't have default size attribute (and it's not unsized)
				// required explicit size attribute needs to be enforced here:
				RemoveInstrWithoutDefaultSize(asm_instr.list);

				if (asm_instr.list.empty())
					return ERR_MISSING_INSTR_SIZE;
			}
			{
				AddressingMode ea= ListOfAllModes(asm_instr.list);	// list of all possible addressing modes, src & dest
				if (ea == AM_IMPLIED)
					state = END_OF_LINE_EXPECTED;		// implied state - nothing else can follow
				else
					state = SRC_OPERAND_EXPECTED;
			}
			break;

		case SRC_OPERAND_EXPECTED:
			{
				AddressingMode modes= ListOfModes(asm_instr.list, SourceOp);
				if (modes == AM_NONE)
				{
					state = DEST_OPERAND_EXPECTED;
					break;
				}

				ASSERT((modes & AM_IMPLIED) == 0 || (modes & ~AM_IMPLIED) != 0);

				Stat ret= parse_operand(lex, modes, asm_instr.list, asm_instr.ea_src);
				if (ret != OK)
					return ret;

				if (LimitInstructions(asm_instr.list, asm_instr.ea_src, SourceOp))
					if (asm_instr.list.empty())
						return ERR_INVALID_SRC_OPERAND;	// requested source effective address is not supported

				// is there any secondary source possible?
				auto src_modes= ListOfModes(asm_instr.list, SecondSourceOp);
				// now check destination addressing modes
				auto dst_modes= ListOfModes(asm_instr.list, DestinationOp);

				if (src_modes != AM_NONE || dst_modes != AM_NONE)
					state = OPERAND_SEPARATOR_EXPECTED;
				else
					state = END_OF_LINE_EXPECTED;
			}
			break;

		case OPERAND_SEPARATOR_EXPECTED:
			if (lex == Lexeme::L_COMMA)
			{
				state = SECOND_SRC_OPERAND_EXPECTED;
				lex = next_lexeme();
			}
			else
			{
				RemoveInstrWithSecondSource(asm_instr.list);

				if (asm_instr.list.empty())
					return ERR_MISSING_COMMA_SEPARATOR;
				else
					state = DEST_OPERAND_EXPECTED;
			}
			break;

		case SECOND_SRC_OPERAND_EXPECTED:
			{
				// is there any secondary source possible?
				auto modes= ListOfModes(asm_instr.list, SecondSourceOp);
				if (modes == AM_NONE)
				{
					state = DEST_OPERAND_EXPECTED;
					break;
				}

				// second source argument
				Stat ret= parse_operand(lex, modes, asm_instr.list, asm_instr.ea_snd_src);
				if (ret != OK)
					return ret;

				// MAC mask register (&)
				if (lex.Type() == Lexeme::L_OPER && lex.GetOper() == O_B_AND && (modes & AM_MAC_MASK))
				{
					asm_instr.ea_snd_src.mode_ |= AM_MAC_MASK;
					lex = next_lexeme();
				}

				if (LimitInstructions(asm_instr.list, asm_instr.ea_snd_src, SecondSourceOp))
					if (asm_instr.list.empty())
						return ERR_INVALID_SRC_OPERAND;	// requested source effective address is not supported

				// now check destination addressing modes
				auto dst_modes= ListOfModes(asm_instr.list, DestinationOp);

				if (dst_modes != AM_NONE)
					state = DEST_OPERAND_SEPARATOR_EXPECTED;
				else
					state = END_OF_LINE_EXPECTED;
			}
			break;

		case DEST_OPERAND_SEPARATOR_EXPECTED:
			if (lex == Lexeme::L_COMMA)
			{
				state = DEST_OPERAND_EXPECTED;
				lex = next_lexeme();
			}
			else
				return ERR_MISSING_COMMA_SEPARATOR;
			break;

		case DEST_OPERAND_EXPECTED:
			{
				AddressingMode modes= ListOfModes(asm_instr.list, DestinationOp);
				if (modes == AM_NONE)
				{
					state = END_OF_LINE_EXPECTED;
					break;
				}

				Stat ret= parse_operand(lex, modes, asm_instr.list, asm_instr.ea_dst);
				if (ret != OK)
					return ret;

				if (LimitInstructions(asm_instr.list, asm_instr.ea_dst, DestinationOp))
					if (asm_instr.list.empty())
						return ERR_INVALID_DEST_OPERAND;	// requested destination effective address is not supported

				// now check second destination addressing modes
				auto dst_modes= ListOfModes(asm_instr.list, SecondDestinationOp);

				if (dst_modes != AM_NONE)
					state = SECOND_OPERAND_SEPARATOR_EXPECTED;
				else
					state = END_OF_LINE_EXPECTED;
			}
			break;

		case SECOND_OPERAND_SEPARATOR_EXPECTED:
			if (lex == Lexeme::L_COMMA)
			{
				state = SECOND_DEST_OPERAND_EXPECTED;
				lex = next_lexeme();
			}
			else
				return ERR_MISSING_COMMA_SEPARATOR;
			break;

		case SECOND_DEST_OPERAND_EXPECTED:
			{
				AddressingMode modes= ListOfModes(asm_instr.list, SecondDestinationOp);
				if (modes == AM_NONE)
				{
					state = END_OF_LINE_EXPECTED;
					break;
				}

				Stat ret= parse_operand(lex, modes, asm_instr.list, asm_instr.ea_snd_dst);
				if (ret != OK)
					return ret;

				if (LimitInstructions(asm_instr.list, asm_instr.ea_snd_dst, SecondDestinationOp))
					if (asm_instr.list.empty())
						return ERR_INVALID_DEST_OPERAND;	// requested destination effective address is not supported

				state = END_OF_LINE_EXPECTED;
			}
			break;

		case END_OF_LINE_EXPECTED:
			if (lex == Lexeme::L_SPACE)
				lex = next_lexeme();
			if (lex != Lexeme::L_EOL && lex != Lexeme::L_COMMENT)
				return ERR_DAT;

			if (asm_instr.list.count() > 1)
			{
				//todo: should this be allowed?

				return ERR_AMBIGUOUS_INSTR;
			}

			return OK;
		}
	}
}

//-----------------------------------------------------------------------------

static uint32 SizeOfData(InstructionSize size)
{
	if (size & IS_DOUBLE)
		return 8;
	else if (size & (IS_FLOAT | IS_LONG))
		return 4;
	else if (size & IS_WORD)
		return 2;
	else if (size & IS_BYTE)
		return 1;
	return 0;
}


Stat ExpectedSignedNumber(const Expr& expr, InstructionSize size)
{
	if (!expr.IsNumber())
	{
		if (expr.inf == Expr::EX_UNDEF)
			return ERR_UNDEF_EXPR;

		return ERR_EXPECTED_NUMBER;
	}

	int32 val= expr.Value();

	switch (size)
	{
	case IS_BYTE:
	case IS_SHORT:
		if (val > 0xff || val < -0x80)
			return ERR_NUM_NOT_BYTE;
		break;

	case IS_WORD:
		if (val > 0xffff || val < -0x8000)
			return ERR_NUM_NOT_WORD;
		break;

	case IS_LONG:
		//todo
		break;

	case IS_FLOAT:
	case IS_DOUBLE:
		return ERR_EXPECTED_NUMBER;

	default:
		break;
	}

	return OK;
}

//-----------------------------------------------------------------------------


Stat CFAsm::asm_decl_const(Lexeme& lex, cf::BinaryProgram& prg, bool str_length_byte, InstructionSize size)
{
	switch (size)
	{
	case IS_BYTE:
	case IS_WORD:
	case IS_LONG:
		break;	// currently only those types are supported

	default:
		return ERR_INVALID_INSTR_SIZE;
	}

	Stat ret= OK;

	auto counter_org= origin_.Origin();	// miejsce na bajt d³ugoœci danych (tylko .STR)
	int counter= 0;					// d³ugoœæ danych (inf. dla .STR)
	if (str_length_byte)			// jeœli .STR to zarezerwowanie bajtu
		origin_.Advance(1);

	for (;;)
	{
		auto expr= expression(lex, size == IS_BYTE);	// oczekiwane wyra¿enie

		if (expr.inf == Expr::EX_STRING)		// tekst?
		{
			ASSERT(size == IS_BYTE);	// tekst tylko w .DB i .STR
			auto str= expr.string;
			auto org= origin_.Origin();

			auto len= static_cast<uint32>(str.length());
			counter += len;
			origin_.Advance(len);
			if (pass_ == 2)
			{
				for (uint32 i= 0; i < len; i++)
					prg.PutByte(org + i, uint8(str[i]));
			}
		}
		else if (pass_ == 1)
		{
			if (expr.inf != Expr::EX_UNDEF)
			{
				ret = ExpectedSignedNumber(expr, size);
				if (ret)
					return ret;		// number too big to fit, or not a number
			}
			origin_.Advance(SizeOfData(size));
			counter++;
		}
		else
		{
			if (expr.inf == Expr::EX_UNDEF)
				return ERR_UNDEF_EXPR;
			ret = ExpectedSignedNumber(expr, size);
			if (ret)
				return ERR_NUM_NOT_BYTE;	// za du¿a liczba

			auto org= origin_.Origin();

			origin_.Advance(SizeOfData(size));

			if (size & IS_BYTE)
				prg.PutByte(org, expr.value);
			else if (size & IS_WORD)
				prg.PutWord(org, expr.value);
			else if (size & IS_LONG)
				prg.PutLongWord(org, expr.value);
			else
				throw LogicError("CFAsm::asm_decl_const unexpected size " __FUNCTION__);
		}

		if (lex.Type() != Lexeme::L_COMMA)	// po przecinku (jeœli jest) kolejne dane
		{
			if (str_length_byte)	// string with length byte output?
			{
				if (counter >= 256)
					return ERR_STRING_TOO_LONG;
				if (pass_ == 2)
					prg.PutByte(counter_org, uint8(counter));
			}
			//else if (def == -2)		// .ASCIS ?
			//{
			//	if (pass_ == 2 && out)
			//		(*out)[origin_ - 1] ^= uint8(0x80);
			//}
			return OK;			// nie ma przecinka - koniec danych
		}
		lex = next_lexeme();
	}

//	if (pass_ == 2 && listing_.IsOpen())
//		listing_.AddBytes(uint16(counter_org), mem_mask, out->Mem(), prg.Origin() - counter_org);

	return ret;
}

//-----------------------------------------------------------------------------

// interpretacja dyrektywy
Stat CFAsm::asm_instr_syntax_and_generate(Lexeme& lex, InstrType it, FixedString* label, cf::BinaryProgram& prg)
{
	Stat ret;
	int def= -2;

	switch (it)
	{
	case I_ORG:		// origin
		{
			Expr expr= expression(lex);	// oczekiwane s³owo

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				return ERR_UNDEF_EXPR;

			if (origin_.Missing())			// is this first time ORG is used?
			{
				origin_.Set(expr.value);
				program_.SetProgramStart(expr.value);
				//if (mark_area_ && pass_ == 2)
				//	mark_area_->SetStart(origin_);
			}
			else
			{
				//if (mark_area_ && pass_ == 2 && origin_ != -1)
				//	mark_area_->SetEnd(uint16(origin_ - 1));
				origin_.Set(expr.value);
				//if (mark_area_ && pass_ == 2)
				//	mark_area_->SetStart(origin_);
			}
			if (pass_ == 2 && listing_.IsOpen())
				listing_.AddCodeBytes(origin_);
			break;
		}

	case I_DS:		// define space
		if (lex.Type() != Lexeme::L_INSTR_SIZE)
			return ERR_MISSING_INSTR_SIZE;

		{
			InstructionSize size= lex.GetInstrSize();
			auto step= SizeOfData(size);
			if (step < 1 || step > 4)
				return ERR_INVALID_INSTR_SIZE;

			lex = next_lexeme();

			Expr expr= expression(lex);	// number of bytes/word/lwords

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				return ERR_UNDEF_EXPR;
			if (expr.value < 0)
				return ERR_NUM_NEGATIVE;		// oczekiwana wartoœæ nieujemna
			origin_.Advance(expr.value * step);	// reserved space
			if (pass_ == 2 && listing_.IsOpen())
				listing_.AddCodeBytes(origin_);
		}
		break;

	//case I_RS:		// reserve space
	//	{
	//		Expr expr= expression(lex);	// oczekiwane s³owo

	//		if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
	//			return ERR_UNDEF_EXPR;
	//		if (expr.value < 0)
	//			return ERR_NUM_NEGATIVE;		// oczekiwana wartoœæ nieujemna
	//		if (expr.inf == Expr::EX_LONG)		// za du¿a wartoœæ
	//			return ERR_NUM_LONG;
	//		if (origin_.Missing())
	//			return ERR_UNDEF_ORIGIN;
	//		origin_.Advance(expr.value);	// zarezerwowana przestrzeñ
	//		if (pass_ == 2 && listing_.IsOpen())
	//			listing_.AddCodeBytes(origin_);
	//		break;
	//	}

	case I_ALIGN:
		{
			int align= 2;	// default alignment
			if (lex.Type() != Lexeme::L_COMMENT && lex.Type() != Lexeme::L_EOL)
			{
				Expr expr= expression(lex);	// expected alignment value

				if (expr.inf == Expr::EX_UNDEF)	// must have a value even in a first pass
					return ERR_UNDEF_EXPR;
				align = expr.value;
			}
			if (align < 1)
				return ERR_NUM_NOT_POSITIVE;
			auto pc= origin_;
			auto diff= pc % align;
			if (diff)
				origin_.Advance(align - diff);	// align origin
		}
		break;

	case I_END:		// zakoñczenie
		{
			if (!is_expression(lex))			// nie ma wyra¿enia?
				return STAT_FIN;
			Expr expr= expression(lex);	// oczekiwane s³owo

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				return pass_ == 1 ? OK : ERR_UNDEF_EXPR;

			program_.SetProgramStart(expr.value);
			return STAT_FIN;
		}

	case I_ERROR:	// zg³oszenie b³êdu
		if (label)		// jest etykieta przed .ERROR ?
			return ERR_LABEL_NOT_ALLOWED;

		if (lex.Type() == Lexeme::L_STR)
		{
			Expr expr= expression(lex, true);		// oczekiwany tekst

			if (expr.inf != Expr::EX_STRING)
				return ERR_STR_EXPECTED;
			user_error_text_ = expr.string;
		}
		else
			user_error_text_.clear();

		return STAT_USER_DEF_ERR;		// b³¹d u¿ytkownika

	case I_INCLUDE:		// include assembly source file
		if (label)		// jest etykieta przed .INCLUDE ?
			return ERR_LABEL_NOT_ALLOWED;

		if (lex.Type() == Lexeme::L_STR)
		{
			Expr expr= expression(lex, true);		// oczekiwany tekst

			if (expr.inf != Expr::EX_STRING)
				return ERR_STR_EXPECTED;

			Path path(expr.string.c_str());
			if (path.is_relative() || !path.has_root_directory())
				path = boost::filesystem::absolute(path);

			boost::system::error_code ec;
			path = boost::filesystem::canonical(path, ec);

			if (ec || !boost::filesystem::exists(path))
				return ERR_INCLUDE_NOT_FOUND;

			path.make_preferred();
			include_fname_ = path;
		}
		else
			return ERR_STR_EXPECTED;		// oczekiwany ³añcuch znaków
		return STAT_INCLUDE;

	case I_IF:
		{
			if (label)		// jest etykieta przed .IF ?
				return ERR_LABEL_NOT_ALLOWED;
			Expr expr= expression(lex);	// oczekiwane wyra¿enie

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				return check_line_ ? OK : STAT_IF_UNDETERMINED;
			return expr.value ? STAT_IF_TRUE : STAT_IF_FALSE;
		}

	case I_ELSE:
		if (label)		// jest etykieta przed .ELSE ?
			return ERR_LABEL_NOT_ALLOWED;
		return STAT_ELSE;

	case I_ENDIF:
		if (label)		// jest etykieta przed .ENDIF ?
			return ERR_LABEL_NOT_ALLOWED;
		return STAT_ENDIF;

	case I_MACRO:			// makrodefinicja
		{
			if (!label)		// nie ma etykiety przed .MACRO ?
				return ERR_MACRONAME_REQUIRED;
			if ((*label)[0] == LOCAL_LABEL_CHAR)		// etykiety lokalne niedozwolone
				return ERR_BAD_MACRONAME;

			MacroDef* macro= nullptr;
			if (pass_ == 1)
			{
				Path path= text_->GetFileName();
				macro = get_new_macro_entry(path);	// miejsce na now¹ makrodefinicjê
				ret = def_macro_name(*label, Ident(Ident::I_MACRONAME, get_last_macro_entry_index()));
				if (ret)
					return ret;
				if (!check_line_)
					macro->SetFileUID(text_->GetFileUID());
			}
			else if (pass_ == 2)
			{
				ret = chk_macro_name(*label);
				if (ret)
					return ret;
				return STAT_MACRO;		// makro zosta³o ju¿ zarejestrowane
			}

			ASSERT(macro);
			macro->macro_name_ = *label;	// zapamiêtanie nazwy makra w opisie makrodefinicji

			for (bool required= false;; )
			{
				if (lex.Type() == Lexeme::L_IDENT)	// nazwa parametru?
				{
					if (macro->AddParam(lex.GetString()) < 0)
						return ERR_PARAM_ID_REDEF;		// powtórzona nazwa parametru
				}
				else if (lex.Type() == Lexeme::L_MULTI)	// wielokropek?
				{
					macro->AddParam(MULTIPARAM);
					lex = next_lexeme();
					break;
				}
				else
				{
					if (required)		// po przecinku wymagany paramter makra
						return ERR_PARAM_DEF_REQUIRED;
					break;
				}
				lex = next_lexeme();
				if (lex.Type() == Lexeme::L_COMMA)	// przecinek?
				{
					lex = next_lexeme();
					required = true;
				}
				else
					break;
			}
			in_macro_ = macro;		// aktualnie nagrywane makro
			return STAT_MACRO;
		}

	case I_ENDM:			// koniec makrodefinicji
		if (label)
			return ERR_LABEL_NOT_ALLOWED;
		return ERR_SPURIOUS_ENDM;		// .ENDM bez wczeœniejszego .MACRO

	case I_EXITM:			// opuszczenie makrodefinicji
		if (label)
			return ERR_LABEL_NOT_ALLOWED;
		return expanding_macro_ ? STAT_EXITM : ERR_SPURIOUS_EXITM;


	case I_SET:				// przypisanie wartoœci zmiennej
		{
			if (!label)		// nie ma etykiety przed .SET ?
				return ERR_LABEL_EXPECTED;
			Expr expr= expression(lex);	// oczekiwane wyra¿enie

			Ident::IdentInfo info= expr.inf == Expr::EX_UNDEF ? Ident::I_UNDEF : Ident::I_VALUE;
			if (pass_ == 1)
				ret = def_ident(*label, Ident(info, expr.value, true));
			else
				ret = chk_ident_def(*label, Ident(info, expr.value, true));
			if (ret)
				return ret;
			if (pass_ == 2 && listing_.IsOpen())
				listing_.AddValue(uint16(expr.value));
			break;
		}

	//case I_REG:				// define register list
	//	if (!label)		// label needed
	//		return ERR_LABEL_EXPECTED;

	//	if (lex.Type() == Lexeme::L_SPACE)
	//		lex = next_lexeme();

	//	{
	//		EffectiveAddress ea;

	//		if (lex.Type() == Lexeme::L_DATA_REG)
	//			ea.mode_ = AM_Dx;
	//		else if (lex.Type() == Lexeme::L_ADDR_REG)
	//			ea.mode_ = AM_Ax;
	//		else
	//			return ERR_REGISTER_LIST_EXPECTED;

	//		ea.first_reg_ = lex.GetRegister();
	//		lex = next_lexeme();
	//		Stat stat= parse_reg_list(lex, ea);
	//		if (stat)
	//			return stat;

	//		// store label value
	//		ret = pass_ == 1 ? def_ident(*label, Ident(Ident::I_VALUE, ea.register_mask)) :
	//			chk_ident_def(*label, Ident(Ident::I_VALUE, ea.register_mask));
	//	}
	//	break;

	case I_REPEAT:			// powtórka wierszy
		{
			Expr expr= expression(lex);	// oczekiwane wyra¿enie

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				return ERR_UNDEF_EXPR;
			if (expr.value < 0 || expr.value > 0xFFFF)
				return ERR_BAD_REPT_NUM;
			rept_init_ = expr.value;
			return STAT_REPEAT;
		}

	case I_ENDR:			// koniec powtórki
		return ERR_SPURIOUS_ENDR;		// .ENDR bez wczeœniejszego .REPEAT


	case I_OPT:				// opcje asemblera
		{
			if (label)
				return ERR_LABEL_NOT_ALLOWED;
			static const char* opts[]= { "CaseSensitive", "CaseInsensitive" };
			for (;;)
			{
				if (lex.Type() == Lexeme::L_IDENT)	// nazwa opcji?
				{
					if (StrICmp(lex.GetString().c_str(), opts[0]) == 0)
						case_sensitive_ = true;
					else if (StrICmp(lex.GetString().c_str(), opts[1]) == 0)
						case_sensitive_ = false;
					else
						return ERR_OPT_NAME_UNKNOWN;				// nierozpoznana nazwa opcji
				}
				else
					return ERR_OPT_NAME_REQUIRED;				// oczekiwana nazwa opcji
				lex = next_lexeme();
				if (lex.Type() == Lexeme::L_COMMA)			// przecinek?
					lex = next_lexeme();
				else
					break;
			}
			break;
		}

	case I_DC:	// declare constant
		{
			// first we need size for dc: .b, .w, or .l
			if (lex.Type() != Lexeme::L_INSTR_SIZE)
				return ERR_MISSING_INSTR_SIZE;

			InstructionSize size= lex.GetInstrSize();

			lex = next_lexeme();

			// next list of values
			ret = asm_decl_const(lex, prg, false, size);
			if (ret)
				return ret;
		}
		break;

	case I_DCB:		// declare block: dcb.b/w/l count, fill - repeat 'fill' 'count' times
		{
			// first we need size for dc: .b, .w, or .l
			if (lex.Type() != Lexeme::L_INSTR_SIZE)
				return ERR_MISSING_INSTR_SIZE;

			InstructionSize size= lex.GetInstrSize();
			auto step= SizeOfData(size);
			if (step < 1 || step > 4)
				return ERR_INVALID_INSTR_SIZE;

			lex = next_lexeme();

			Expr expr= expression(lex);		// expected count

			if (expr.inf == Expr::EX_UNDEF)		// has to be defined in first pass
				return ERR_UNDEF_EXPR;
			if (expr.value < 0)
				return ERR_NUM_NEGATIVE;		// no negative numbers here

			uint32 org= origin_;

			// check overflow
			uint64 total_size= expr.value;
			total_size *= step;
			if (total_size > uint32(~0))
				return ERR_PC_WRAPED;
			origin_.Advance(expr.value * step);

			if (lex.Type() != Lexeme::L_COMMA)	// after count init value expected
				return OK;

			lex = next_lexeme();
			Expr init= expression(lex);		// init value

			if (init.inf == Expr::EX_UNDEF)		// undefined yet?
				return pass_ == 1 ? OK : ERR_UNDEF_EXPR;
			else
			{
				ret = ExpectedSignedNumber(expr, size);
				if (ret)
					return ret;
			}

			if (pass_ == 2)
			{
				uint32 len= origin_ - org;
				for (uint32 i= 0; i < len; i++)
					if (size & IS_BYTE)
						prg.PutByte(org + i, uint8(init.value));
					else if (size & IS_WORD)
						prg.PutWord(org + i * step, uint16(init.value));
					else if (size & IS_LONG)
						prg.PutLongWord(org + i * step, init.value);

				//if (len && listing_.IsOpen())
			//		listing_.AddBytes(uint16(org - len), mem_mask, out->Mem(), len);
			}
			break;
		}

	default:
		ASSERT(false);
	}

	return OK;
}

//-----------------------------------------------------------------------------

const Path Source::empty_;

// check if exactly one bit is set in register mask
//bool SingleRegMask(uint16 register_mask)
//{
//	if (register_mask)
//		return false;
//
//	int count= 0;
//	for (int i= 0; i < 16; ++i)
//		if (register_mask & (1 << i))
//			if (++count > 1)
//				return false;
//
//	return true;
//}


// wczytanie argumentów wywo³ania makra
void MacroDef::ParseArguments(Lexeme& lex, CFAsm& asmb)
{
	bool get_param= true;
	bool first_param= true;
	Stat ret;
	int count= 0;
	EffectiveAddress ea;

	int required= m_nParams >= 0 ? m_nParams : -m_nParams - 1;	// iloœæ wymaganych arg.

	params_.clear();
	size_ = IS_NONE;

	if (m_nParams == 0)		// makro bezparametrowe?
		return;

	for (;;)
		if (get_param)		// ew. kolejny argument
			switch (lex.Type())
			{
			case Lexeme::L_STR:		// ci¹g znaków?
				params_.push_back(Expr(lex.GetString()));
				count++;
				get_param = false;			// parametr ju¿ zinterpretowany
				first_param = false;		// pierwszy parametr ju¿ wczytany
				lex = asmb.next_lexeme();
				break;

			case Lexeme::L_DATA_REG:
			case Lexeme::L_ADDR_REG:
				{
					ea.mode_ = lex.Type() == Lexeme::L_DATA_REG ? AM_Dx : AM_Ax;
					ea.first_reg_ = lex.GetRegister();
					lex = asmb.next_lexeme();
					bool list= asmb.parse_reg_list(lex, ea);
					params_.push_back(Expr(!list ? Expr::EX_REGISTER : Expr::EX_REG_LIST, ea.register_mask));
					count++;
					get_param = false;		// parametr ju¿ zinterpretowany
				}
				break;

			case Lexeme::L_REGISTER:
				ea.first_reg_ = lex.GetRegister();
				//todo

				throw ERR_PARAM_REQUIRED;

				break;

			case Lexeme::L_INSTR_SIZE:	// macro can be invoked with instruction size
				size_ = lex.GetInstrSize();
				lex = asmb.next_lexeme();
				break;

			default:
				if (asmb.is_expression(lex))	// wyra¿enie?
				{
					Expr expr= asmb.expression(lex);

					params_.push_back(expr);
					count++;
					get_param = false;		// parametr ju¿ zinterpretowany
				}
				else
				{
					if (count < required)
						throw ERR_PARAM_REQUIRED;	// za ma³o parametrów wywo³ania makra
					if (!first_param)
						throw ERR_PARAM_REQUIRED;	// po przecinku trzeba podaæ kolejny parametr
					//m_nParamCount = count;
					return;
				}
			}
		else			// za argumentem przecinek, œrednik lub koniec
		{
			if (count==required && m_nParams>0)
			{
				return;		// wszystkie wymagane parametry ju¿ wczytane
			}
			switch (lex.Type())
			{
			case Lexeme::L_COMMA:		// przecinek
				get_param = true;		// nastêpny parametr
				lex = asmb.next_lexeme();
				break;
			default:
				if (count < required)
					throw ERR_PARAM_REQUIRED;	// za ma³o parametrów wywo³ania makra
				return;
			}
		}
	throw ERR_PARAM_REQUIRED;
}


// odszukanie parametru 'param_name' aktualnego makra
Expr MacroDef::ParamLookup(Lexeme& lex, FixedString param_name, CFAsm* asmb)
{
	Ident ident;
	if (!param_names_.lookup(param_name, ident))	// odszukanie parametru o danej nazwie
		return Expr();

	if (asmb)
		lex = asmb->next_lexeme(false);

	return ParamLookup(lex, ident.val.Value(), asmb);
}


// odszukanie wartoœci parametru numer 'param_number' aktualnego makra
Expr MacroDef::ParamLookup(Lexeme& lex, int param_number, CFAsm* asmb)
{
	bool special= param_number == -1;	// zmienna %0 ? (nie parametr)
	if (lex.Type() == Lexeme::L_STR_ARG)	// odwo³anie do wartoœci znakowej parametru?
	{
		Expr expr;
		if (!special && (static_cast<size_t>(param_number) >= params_.size() ))
			throw ERR_EMPTY_PARAM;

		if (special)			// odwo³anie do %0$ -> co oznacza nazwê makra
			expr = Expr(macro_name_);
		else
		{
			assert(params_.size() > static_cast<size_t>(param_number));
			if (params_[param_number].inf != Expr::EX_STRING)	// spr. czy zmienna ma wartoœæ tekstow¹
				throw ERR_NOT_STR_PARAM;
			expr = Expr(params_[param_number].string);
		}
		if (asmb)
			lex = asmb->next_lexeme();
		return expr;
	}
	else if (lex.Type() == Lexeme::L_SPACE)
	{
		if (asmb)
			lex = asmb->next_lexeme();
	}

	if (special)	// odwo³anie do %0 -> iloœæ aktualnych parametrów w wywo³aniu makra
	{
		return Expr(static_cast<int32>(params_.size()));
	}
	else		// odwo³anie do aktualnego parametru
	{
		if (static_cast<size_t>(param_number) >= params_.size())
			throw ERR_EMPTY_PARAM;		// parametru o takim numerze nie ma

		return params_[param_number];
	}
}


// spr. sk³adni odwo³ania do parametru makra (tryb sprawdzania wiersza)
void MacroDef::AnyParamLookup(Lexeme& lex, CFAsm& asmb)
{
	if (lex.Type() == Lexeme::L_STR_ARG)	// odwo³anie do wartoœci znakowej parametru?
		lex = asmb.next_lexeme();
	else if (lex.Type() == Lexeme::L_SPACE)
		lex = asmb.next_lexeme();
}


const char* MacroDef::GetCurrLine(std::string& str)	// odczyt aktualnego wiersza makra
{
	ASSERT(line_number_ >= 0);
	if (line_number_ < GetSize())
	{
		str = GetLine(line_number_++);
		return str.c_str();
	}
	else				// koniec wierszy?
	{
		ASSERT(line_number_ == GetSize());
		return nullptr;
	}
}


InstructionSize MacroDef::SizeAttribute() const
{
	return size_;
}


//-----------------------------------------------------------------------------

CFAsm::CFAsm(const std::wstring& file_in_name, bool case_sensitive, DebugInfo* debug, MarkArea* area, ISA isa, const char* listing_file)
	  : entire_text_(file_in_name), debug_(debug), mark_area_(area), listing_(listing_file)
{
	text_ = nullptr;
	repeat_def_ = nullptr;
	init(isa);
	case_sensitive_ = case_sensitive;
}


CFAsm::CFAsm(ISA isa, bool case_sensitive)
{
	isa_ = isa;
	init_members();
	check_line_ = true;
	text_ = nullptr;
	repeat_def_ = nullptr;
	init(isa);
	case_sensitive_ = case_sensitive;
}


Stat CFAsm::CheckLine(const char* str, int& instr_idx_start, int& instr_idx_fin)
{
	Stat ret;
	ptr_ = str;

	instr_idx_start = instr_idx_fin = 0;

	try
	{
		pass_ = 1;
		local_area_ = 0;
		macro_local_area = 0;
		ret = assemble_line();
	}
	catch (std::exception&)
	{
		//TODO
		ASSERT(false);
		ret = ERR_OUT_OF_MEM;
	}

	if (instr_start_)
	{
		instr_idx_start = static_cast<int>(instr_start_ - str);
		instr_idx_fin = static_cast<int>(instr_fin_ - str);
	}

	if (ret < OK)
		ret = OK;
//  if (ret == STAT_FIN || ret == STAT_USER_DEF_ERR || ret == STAT_INCLUDE)
//    ret = OK;
	switch (ret)	// nie wszystke b³êdy s¹ b³êdami w trybie analizowania jednego wiersza
	{
	case ERR_UNDEF_EXPR:
	case ERR_UNKNOWN_INSTR:
	case ERR_SPURIOUS_ENDM:
	case ERR_SPURIOUS_EXITM:
	case ERR_SPURIOUS_ENDR:
		ret = OK;
		break;
	}
	ASSERT(ret >= OK);

	return ret;
}


void Output(const AsmInstruction& asm_instr)
{
	if (asm_instr.list.count() != 1)
		std::cerr << "error: instr. count = " << asm_instr.list.count();
	else
	{
		std::cerr << asm_instr.list[0]->Mnemonic();

		std::cerr << ' ' << std::hex << asm_instr.ea_src.mode_ << ' ' << asm_instr.ea_dst.mode_ << '\n';
	}
}


Stat CFAsm::look_for_endif()		// szukanie .IF, .ENDIF lub .ELSE
{
	Lexeme lex= next_lexeme(false);	// kolejny leksem, byæ mo¿e pusty (L_SPACE)
	bool labelled= false;

	switch (lex.Type())
	{
	case Lexeme::L_IDENT:	// etykieta
	case Lexeme::L_IDENT_N:	// etykieta numerowana
		lex = next_lexeme();
		if (lex.Type() == Lexeme::L_IDENT_N)
			factor(lex);

		labelled = true;
		switch (lex.Type())
		{
		case Lexeme::L_LABEL:	// znak ':'
			lex = next_lexeme();
			if (lex.Type() != Lexeme::L_ASM_INSTR)
				return OK;
			break;
		case Lexeme::L_ASM_INSTR:
			break;
		default:
			return OK;
		}
		//      lex = next_lexeme();
		break;

	case Lexeme::L_SPACE:	// odstêp
		lex = next_lexeme();
		if (lex.Type() != Lexeme::L_ASM_INSTR)		// nie dyrektywa asemblera?
			return OK;
		break;

	case Lexeme::L_COMMENT:	// komentarz
	case Lexeme::L_EOL:		// koniec wiersza
		return OK;

	case Lexeme::L_FIN:	// koniec tekstu
		return STAT_FIN;
		break;

	default:
		return ERR_UNEXP_DAT;
	}

	ASSERT(lex.Type() == Lexeme::L_ASM_INSTR);		// dyrektywa asemblera

	switch (lex.GetInstr())
	{
	case I_IF:
		return labelled ? ERR_LABEL_NOT_ALLOWED : STAT_IF_UNDETERMINED;
	case I_ELSE:
		return labelled ? ERR_LABEL_NOT_ALLOWED : STAT_ELSE;
	case I_ENDIF:
		return labelled ? ERR_LABEL_NOT_ALLOWED : STAT_ENDIF;
	default:
		return OK;
	}
}


Stat CFAsm::assemble_line()	// interpretacja wiersza
{
	enum			// stany automatu
	{
		START,
		AFTER_LABEL,
		INSTR,
		EXPR,
		COMMENT,
		FINISH
	} state= START;
	FixedString label;	// pomocnicza zmienna do zapamiêtania identyfikatora
	bool labelled= false;	// flaga wyst¹pienia etykiety
	Stat ret, ret_stat= OK;
	instr_start_ = nullptr;
	instr_fin_ = nullptr;
	ident_start_ = nullptr;
	ident_fin_ = nullptr;

	Lexeme lex= next_lexeme(false);	// kolejny leksem, byæ mo¿e pusty (L_SPACE)

	for (;;)
	{
		switch (state)
		{
		case START:			// pocz¹tek wiersza
			switch (lex.Type())
			{
			case Lexeme::L_IDENT:	// etykieta
				label = lex.GetString();	// zapamiêtanie identyfikatora
				state = AFTER_LABEL;
				lex = next_lexeme();
				break;
			case Lexeme::L_IDENT_N:	// etykieta numerowana
				{
					Lexeme ident(lex);
					lex = next_lexeme();
					auto expr= factor(lex);
					if (!check_line_)
					{
						if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
							return ERR_UNDEF_EXPR;
						ident.Format(expr.value);	// znormalizowanie postaci etykiety
						label = ident.GetString();	// zapamiêtanie identyfikatora
					}
					else
						label = "x";
					state = AFTER_LABEL;
					break;
				}
			case Lexeme::L_SPACE:	// po odstêpie ju¿ nie mo¿e byæ etykiety
				state = INSTR;
				lex = next_lexeme();
				break;
			case Lexeme::L_COMMENT:	// komentarz
				state = COMMENT;
				break;
			case Lexeme::L_EOL:		// koniec wiersza
			case Lexeme::L_FIN:		// koniec tekstu
				state = FINISH;
				break;
			case Lexeme::L_REGISTER:
				return ERR_UNEXP_REG;
			default:
				return ERR_UNEXP_DAT;	// nierozpoznany napis
			}
			break;


		case AFTER_LABEL:			// wyst¹pi³a etykieta
			switch (lex.Type())
			{
			case Lexeme::L_SPACE:
				ASSERT(false);
				break;
			case Lexeme::L_LABEL:	// znak ':'
				state = INSTR;
				lex = next_lexeme();
				break;
			case Lexeme::L_EQUAL:	// znak '='
				state = EXPR;
				lex = next_lexeme();
				break;
			default:
				state = INSTR;
				break;
			}
			labelled = true;
			break;


		case INSTR:			// oczekiwana instrukcja, komentarz lub nic
			if (labelled &&		// przed instr. by³a etykieta
				!(lex.Type() == Lexeme::L_ASM_INSTR &&	// i za etykiet¹
				(lex.GetInstr() == I_MACRO ||			// nie wystêpuje dyrektywa .MACRO
				 lex.GetInstr() == I_SET)))				// ani dyrektywa .SET?
				 //lex.GetInstr() == I_REG)))
			{
				if (origin_.Missing())
					return ERR_UNDEF_ORIGIN;
				ret = pass_ == 1 ? def_ident(label, Ident(Ident::I_ADDRESS, origin_)) :
					chk_ident_def(label, Ident(Ident::I_ADDRESS, origin_));
				if (ret)
					return ret;
			}

			switch (lex.Type())
			{
			case Lexeme::L_SPACE:
				ASSERT(false);
				break;
			case Lexeme::L_ASM_INSTR:	// dyrektywa asemblera
				{
					InstrType it= lex.GetInstr();
					instr_start_ = ident_start_;	// po³o¿enie instrukcji w wierszu
					instr_fin_ = ident_fin_;

					lex = next_lexeme();
					ret_stat = asm_instr_syntax_and_generate(lex, it, labelled ? &label : nullptr, program_);
					if (ret_stat > OK)		// b³¹d? (ERR_xxx)
						return ret_stat;
					if (pass_ == 2 && debug_ &&
						ret_stat != STAT_MACRO && ret_stat != STAT_EXITM && it != I_SET &&
						ret_stat != STAT_REPEAT && ret_stat != STAT_ENDR)
					{
						ret = generate_debug(it, text_->GetLineNo(), text_->GetFileUID());
						if (ret)
							return ret;
					}
					if (pass_ == 2 && ret_stat == STAT_MACRO)
						return ret_stat;		// w drugim przejœciu omijamy .MACRO
					state = COMMENT;
					break;
				}

			case Lexeme::L_PROC_INSTR:	// rozkaz procesora
				{
					instr_start_ = ident_start_;	// po³o¿enie instrukcji w wierszu
					instr_fin_ = ident_fin_;
					AsmInstruction asm_instr;
					asm_instr.list = lex.GetInstructions();
					Expr expr, expr_bit, expr_zpg;
					lex = next_lexeme(false);
					ret = proc_instr_syntax(lex, asm_instr);
					if (ret)		// syntax error
						return ret;

					ret = check_argument_sizes(asm_instr);
					if (ret)		// semantic error if second pass, syntax error otherwise
						return ret;

					uint32 instr_size= 0;
					ret = chk_instr_code(asm_instr, instr_size);
					if (ret)
						return ret;		// b³¹d trybu adresowania lub zakresu

					//if (asm_instr.list.count() > 1)
					//	return ERR_AMBIGUOUS_INST;

					uint32 len= 0;			// d³ugoœæ rozkazu z argumentem
					if (pass_ == 2 && debug_)
					{
						if (origin_.Missing())
							return ERR_UNDEF_ORIGIN;
						MacroDef* macro= dynamic_cast<MacroDef*>(text_);
						if (macro && macro->first_code_line_)
						{	// debug_ info dla pierwszego wiersza makra zawieraj¹cego instr.
							macro->first_code_line_ = false;
							generate_debug(origin_, macro->GetFirstLineNo(), macro->GetFirstLineFileUID());
						}
						else
							generate_debug(origin_, text_->GetLineNo(), text_->GetFileUID());

						len = generate_code(asm_instr);
					}
					else if (pass_ == 2)
					{
						len = generate_code(asm_instr);
					}

					if (pass_ == 2 && len != instr_size)
					{
						// internal error, precalculated instruction size doesn't match generated size
						return ERR_INSTR_SIZE_MISMATCH;
					}

					origin_.Advance(instr_size);
					state = COMMENT;		// dozwolony ju¿ tylko komentarz
					break;
				}

			case Lexeme::L_IDENT:	// etykieta - tu tylko jako nazwa makra
			case Lexeme::L_IDENT_N:	// etykieta numerowana - tu tylko jako nazwa makra
				{
					if (lex.Type() == Lexeme::L_IDENT)
					{
						label = lex.GetString();	// zapamiêtanie identyfikatora
						lex = next_lexeme();
					}
					else
					{
						Lexeme ident(lex);
						lex = next_lexeme();
						auto expr= factor(lex);
						if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
							return ERR_UNDEF_EXPR;
						ident.Format(expr.value);		// znormalizowanie postaci etykiety
						label = ident.GetString();	// zapamiêtanie identyfikatora
					}
					Ident name;
					if (!macro_names_.lookup(label, name))		// jeœli etykiety nie ma w tablicy
						return ERR_UNKNOWN_INSTR;
					ASSERT((int)macros_.size() > name.val.Value() && name.val.Value() >= 0);
					MacroDef* macro= &macros_[name.val.Value()];
					macro->ParseArguments(lex, *this);		// wczytanie argumentów makra
					macro->Start(&conditional_asm_, text_->GetLineNo(), text_->GetFileUID());
					source_.Push(text_);		// remember current text source on a stack
					text_ = expanding_macro_ = macro;
					macro_local_area++;		// nowy obszar lokalny etykiet makra
					state = COMMENT;		// dozwolony ju¿ tylko komentarz
					break;
				}

			case Lexeme::L_EOL:
			case Lexeme::L_FIN:
				state = FINISH;
				break;
			case Lexeme::L_COMMENT:	// komentarz
				state = COMMENT;
				break;
			default:
				return ERR_INSTR_OR_NULL_EXPECTED;
			}
			break;


		case EXPR:			// expected expression - value for a preceeding label
			switch (lex.Type())
			{
			case Lexeme::L_SPACE:
				ASSERT(false);
				break;

			case Lexeme::L_DATA_REG:
			case Lexeme::L_ADDR_REG:
				{
					EffectiveAddress ea;

					if (lex.Type() == Lexeme::L_DATA_REG)
						ea.mode_ = AM_Dx;
					else if (lex.Type() == Lexeme::L_ADDR_REG)
						ea.mode_ = AM_Ax;

					ea.first_reg_ = lex.GetRegister();
					lex = next_lexeme();
					bool list= parse_reg_list(lex, ea);

					// store label value
					Ident ident(Ident::I_VALUE, ea.register_mask);
					ident.val.inf = !list ? Expr::EX_REGISTER : Expr::EX_REG_LIST;
					ret = pass_ == 1 ? def_ident(label, ident) : chk_ident_def(label, ident);
				}
				if (ret)
					return ret;
				state = COMMENT;
				break;

			default:
				auto expr= expression(lex);	// zinterpretowanie wyra¿enia

				// is it predefined label?
				auto constant= find_const(label);
				if (constant != PredefConst::NONE)
				{
					// assignment to io_area is fine
					if (constant == PredefConst::IO)	// io_area label?
					{
						if (expr.inf == Expr::EX_WORD || expr.inf == Expr::EX_BYTE)
						{
							if (!check_line_)
								g_io_addr = expr.value;
						}
						else if (expr.inf == Expr::EX_LONG)
							return ERR_NUM_LONG;
						else if (expr.inf == Expr::EX_UNDEF)
							;	// not yet defined; this is fine
						else
							return ERR_CONST_EXPECTED;
					}
					else
					{
						err_ident_ = label;
						return ERR_CONST_LABEL_REDEF;
					}
				}
				else if (pass_ == 1)			// pierwsze przejœcie?
				{
					if (expr.inf != Expr::EX_UNDEF)
						ret = def_ident(label, Ident(Ident::I_VALUE, expr.value));
					else
						ret = def_ident(label, Ident(Ident::I_UNDEF, 0));
				}
				else			// drugie przejœcie
				{
					if (expr.inf != Expr::EX_UNDEF)
						ret = chk_ident_def(label, Ident(Ident::I_VALUE, expr.value));
					else
						return ERR_UNDEF_EXPR;
					if (listing_.IsOpen())
						listing_.AddValue(expr.value);
				}
				if (ret)
					return ret;
				state = COMMENT;
				break;
			}
			break;


		case COMMENT:			// jeszcze tylko komentarz lub koniec wiersza
			switch (lex.Type())
			{
			case Lexeme::L_SPACE:
				ASSERT(false);
				//	    break;
			case Lexeme::L_COMMENT:
				return ret_stat;
			case Lexeme::L_EOL:
			case Lexeme::L_FIN:
				state = FINISH;
				break;
			default:
				return ERR_DAT;
			}
			break;


		case FINISH:
			switch (lex.Type())
			{
			case Lexeme::L_EOL:
				return ret_stat;
			case Lexeme::L_FIN:
				return ret_stat ? ret_stat : STAT_FIN;
			default:
				return ERR_DAT;
			}
			break;
		}
	}
}


Stat ExpectedNumberInRange(const Expr& expr, int32 min, uint32 max)
{
	if (!expr.IsNumber())
	{
		if (expr.inf == Expr::EX_UNDEF)
			return ERR_UNDEF_EXPR;

		return ERR_EXPECTED_NUMBER;
	}

	int32 val= expr.Value();

	bool b1= val < min;
	bool b2= uint32(val) > max;

	if (val < min || (val > 0 && uint32(val) > max))
	{
		std::ostringstream ost;
		ost << "Valid range: " << min << " to " << max << '.';
		return Stat(ERR_NUM_NOT_IN_RANGE, ost.str().c_str());
	}

	return OK;
}


Stat ExpectedNumberInRange(const Expr& expr, InstrSize size)
{
	if (!expr.IsNumber())
	{
		if (expr.inf == Expr::EX_UNDEF)
			return ERR_UNDEF_EXPR;

		return ERR_EXPECTED_NUMBER;
	}

	switch (size)
	{
	case S_BYTE:
		if (expr.inf != Expr::EX_BYTE)
		{
			std::ostringstream ost;
			ost << "Value: " << expr.Value() << '.';
			return Stat(ERR_NUM_NOT_FIT_BYTE, ost.str().c_str());
		}
		break;

	case S_WORD:
		if (expr.inf > Expr::EX_WORD)
		{
			std::ostringstream ost;
			ost << "Value: " << expr.Value() << '.';
			return Stat(ERR_NUM_NOT_FIT_WORD, ost.str().c_str());
		}
		break;

		// when floats are supported this may be needed?
	//case S_LONG:
	//	if (expr.inf > Expr::EX_LONG)
	//		return ERR_NUM_NOT_FIT_LONG;
	//	break;
	}

	return OK;
}


Stat AbsToRelative(Expr& val, uint32 origin, InstructionSize size)
{
	uint32 pc= origin + 2;

	// first of all number is expected
	Stat stat= ExpectedSignedNumber(val, IS_LONG);
	if (stat != OK)
		return OK;

	// now adjust it to relative form
	int32 addr= val.Value();
	val = addr - pc;
	val.inf = Expr::EX_LONG;

	stat = ExpectedSignedNumber(val, size);
	if (stat == ERR_NUM_NOT_BYTE)
		return ERR_RELATIVE_OFFSET_OUT_OF_BYTE_RANGE;
	else if (stat == ERR_NUM_NOT_WORD)
		return ERR_RELATIVE_OFFSET_OUT_OF_WORD_RANGE;

	return stat;
}


// size instruction's attribute will occupy (in bytes)
/*
Stat calculate_sizes(InstructionSize requested_size, const EffectiveAddress& ea, uint32& size)
{
	size = 0;

	switch (ea.mode_)
	{
	case AM_RELATIVE:
		switch (requested_size)
		{
		case IS_BYTE:
		case IS_SHORT:
			break;	// offset inside instruction
		case IS_WORD:
			size += 2;
			break;
		case IS_LONG:
			size += 4;
			break;
		default:
			return ERR_CANNOT_CALC_SIZE;
		}
		break;

		// expected word offset/value (signed)
	case AM_DISP_Ax:
	case AM_ABS_W:
		size += 2;
		break;

	case AM_DISP_PC:
		size += 2;
		break;

		// expected byte offset (signed)
	case AM_DISP_Ax_Ix:
		size += 2;
		break;

	case AM_DISP_PC_Ix:
		size += 2;
		break;

		// expected long value
	case AM_ABS_L:
		size += 4;
		break;

		// expected number at most 'requested_size' in range
	case AM_IMMEDIATE:

		// some instructions have argument "built-in", like AddQ, MoveQ

		//todo

		switch (requested_size)
		{
		case IS_BYTE:
			size += 2;	// byte in a word
			break;

		case IS_WORD:
			size += 2;
			break;

		case IS_LONG:
			size += 4;
			break;

		default:
			return ERR_CANNOT_CALC_SIZE;
		}
		break;
	}

	// todo:
	// per instruction range (addq, btst, ect.)

	return OK;
} */


Stat check_sizes(InstructionSize requested_size, EffectiveAddress& ea, uint32 origin, const Instruction* instr)
{
	switch (ea.mode_)
	{
	case AM_RELATIVE:
		{
			auto stat= AbsToRelative(ea.val_, origin, requested_size);
			if (stat != OK)
				return stat;
			if (ea.val_.IsNumber() && !instr->IsRelativeOffsetValid(ea.val_.Value(), requested_size))
				return ERR_INVALID_RELATIVE_OFFSET;
		}
		return OK;

		// expected word offset/value (signed)
	case AM_DISP_Ax:
	case AM_ABS_W:
		return ExpectedSignedNumber(ea.val_, IS_WORD);

	case AM_DISP_PC:
		return AbsToRelative(ea.val_, origin, IS_WORD);

		// expected byte offset (signed)
	case AM_DISP_Ax_Ix:
		return ExpectedSignedNumber(ea.val_, IS_BYTE);

	case AM_DISP_PC_Ix:
		return AbsToRelative(ea.val_, origin, IS_BYTE);

		// expected long value
	case AM_ABS_L:
		return ExpectedSignedNumber(ea.val_, IS_LONG);

		// expected number at most 'requested_size' in range
	case AM_IMMEDIATE:
		{
			Stat stat= ExpectedSignedNumber(ea.val_, requested_size);
			if (stat != OK)
				return stat;

			if (instr)
			{
				IDataRange range= instr->ImmediateDataRange();
				if (range.is_valid)
				{
					stat = ExpectedNumberInRange(ea.val_, range.min, range.max);
					if (stat != OK)
						return stat;

					if (range.exclude && ea.val_.Value() == range.exclude_val)
					{
						std::ostringstream ost;
						ost << "Illegal value: " << ea.val_.Value() << '.';
						return Stat(ERR_NUM_NOT_LEGAL, ost.str().c_str());
					}
				}

				if (instr->ImmediateDataSize() != S_NA)
				{
					stat = ExpectedNumberInRange(ea.val_, instr->ImmediateDataSize());
					if (stat != OK)
						return stat;
				}
			}

			// coerce value into requested size to emit right size of data
			// note: this is required currently, but it's a dubious idea; size of BTST.L #n, d0 instruction has nothing to do with size of n
			switch (requested_size)
			{
			case IS_BYTE:
				ea.val_.inf = Expr::EX_BYTE;
				break;
			case IS_WORD:
				ea.val_.inf = Expr::EX_WORD;
				break;
			case IS_LONG:
				ea.val_.inf = Expr::EX_LONG;
				break;
			default:
				// some instructions are unsized and use immediate mode (like TRAP)
				// so do nothing
				break;
			}
			return stat;
		}
	}

	// todo:
	// per instruction range (addq, btst, ect.)

	return OK;
}


Stat CFAsm::check_argument_sizes(AsmInstruction& asm_instr) const
{
	// in second pass all expressions have to be resolved; check their values
	// to make sure they fit in the range supported by instruction/addressing mode

	ASSERT(asm_instr.list.count() == 1);
	if (asm_instr.list.count() != 1)
		throw LogicError("Internal error in check_argument_sizes - expected one instruction ");

	uint32 origin= origin_;

	if (asm_instr.requested_size == IS_NONE && asm_instr.ea_src.mode_ == AM_IMMEDIATE)
		asm_instr.requested_size = asm_instr.list[0]->DefaultSize();

	Stat stat= check_sizes(asm_instr.requested_size, asm_instr.ea_src, origin, asm_instr.list[0]);
	if (stat != OK && (pass_ != 1 || stat != ERR_UNDEF_EXPR))
		return stat;

	stat = check_sizes(asm_instr.requested_size, asm_instr.ea_dst, origin, 0);
	if (stat != OK && (pass_ != 1 || stat != ERR_UNDEF_EXPR))
		return stat;

	return OK;
}

//-----------------------------------------------------------------------------


bool CFAsm::is_expression(const Lexeme& lex)	// pocz¹tek wyra¿enia?
{
	switch (lex.Type())
	{
	case Lexeme::L_NUM:				// liczba (dec, hex, bin, lub znak)
	case Lexeme::L_IDENT:			// identyfikator
	case Lexeme::L_IDENT_N:			// identyfikator numerowany
	case Lexeme::L_OPER:			// operator
	case Lexeme::L_EXPR_BRACKET_L:	// lewy nawias dla wyra¿eñ '['
	case Lexeme::L_EXPR_BRACKET_R:	// prawy nawias dla wyra¿eñ ']'
		return true;				// to pocz¹tek wyra¿enia

	default:
		return false;	// to nie pocz¹tek wyra¿enia
	}
}


CFAsm::PredefConst CFAsm::find_const(FixedString str)
{
	// predefined constants
	static const char cnst1[]= "ORG";		// origin
	static const char cnst2[]= "IO_AREA";	// I/O start
	static const char cnst3[]= "__VERSION";	// MASM version number

	if (StrICmp(str.c_str(), cnst1) == 0)
		return PredefConst::ORIGIN;
	if (StrICmp(str.c_str(), cnst2) == 0)
		return PredefConst::IO;
	if (StrICmp(str.c_str(), cnst3) == 0)
		return PredefConst::VERSION;

	return PredefConst::NONE;
}


Expr CFAsm::predef_const(FixedString str)
{
	auto constant= find_const(str);

	switch (constant)
	{
	case PredefConst::ORIGIN:
		if (origin_.Missing())
			throw ERR_UNDEF_ORIGIN;

		return Expr(origin_.Origin());	// wartoœæ licznika rozkazów

	case PredefConst::IO:
		return Expr(g_io_addr);		// io simulator area

	case PredefConst::VERSION:
		return Expr(MASM_VERSION);

	case PredefConst::NONE:
		return Expr::Undefined();
	}

	throw LogicError("missing case in " __FUNCTION__);
}


Expr CFAsm::predef_function(Lexeme& lex)
{
	// predefined functions
	static const char def[]= "DEF";			// function DEF(label) - is label defined?
	static const char ref[]= "REF";			// function REF(lable) - has label been referenced (used)?
	static const char strl[]= "STRLEN";		// function STRLEN(str) - string length
	static const char pdef[]= "PASSDEF";	// function PASSDEF(label) - has label been defined in current assembly pass?
	static const char bitcnt[]= "BITCOUNT";	// function BITCOUNT(value) - number of bits set in a given value

	FixedString str= lex.GetString();

	int hit= 0;
	if (StrICmp(str.c_str(), def) == 0)
		hit = 1;
	else if (StrICmp(str.c_str(), ref) == 0)
		hit = 2;
	else if (StrICmp(str.c_str(), pdef) == 0)
		hit = 3;
	else if (StrICmp(str.c_str(), strl) == 0)
		hit = -1;
	else if (StrICmp(str.c_str(), bitcnt) == 0)
		hit = -2;

	if (hit > 0)
	{
		lex = next_lexeme(false);
		if (lex.Type() != Lexeme::L_PARENTHESIS_L)
			throw ERR_BRACKET_L_EXPECTED;		// wymagany nawias '(' (bez odstêpu)
		lex = next_lexeme();
		FixedString label;
		if (lex.Type() == Lexeme::L_IDENT)
		{
			label = lex.GetString();
			lex = next_lexeme();
		}
		else if (lex.Type() == Lexeme::L_IDENT_N)
		{
			Lexeme ident(lex);
			lex = next_lexeme();
			auto expr= factor(lex);

			if (expr.inf == Expr::EX_UNDEF)	// nieokreœlona wartoœæ
				throw ERR_UNDEF_EXPR;
			ident.Format(expr.value);		// znormalizowanie postaci etykiety
			label = ident.GetString();		// zapamiêtanie identyfikatora
		}
		else
			throw ERR_LABEL_EXPECTED;		// wymagana etykieta - argument .DEF lub .REF

		if (lex.Type() != Lexeme::L_PARENTHESIS_R)
			throw ERR_BRACKET_R_EXPECTED;		// wymagany nawias ')'

		if (label[0] == LOCAL_LABEL_CHAR)		// etykiety lokalne niedozwolone
			throw ERR_LOCAL_LABEL_NOT_ALLOWED;
		Ident ident;
		int32 value= 0;

		if (hit == 1)			// .DEF?
		{
			if (global_ident_.lookup(label, ident) && ident.info != Ident::I_UNDEF)
			{
				ASSERT(ident.info != Ident::I_INIT);
				value = 1;			// 1 - etykieta zdefiniowana
			}
			else
				value = 0;		// 0 - etykieta niezdefiniowana
		}
		else if (hit == 2)			// .REF?
		{
			value = global_ident_.lookup(label, ident) ? 1 : 0;	// 1 - jeœli etykieta jest w tablicy
		}
		else				// .PASSDEF?
		{
			if (global_ident_.lookup(label, ident) && ident.info != Ident::I_UNDEF)
			{
				ASSERT(ident.info != Ident::I_INIT);
				if (pass_ == 1)
					value = 1;		// 1 - etykieta zdefiniowana
				else		// drugie przejœcie asemblacji
					value = ident.checked ? 1 : 0;		// 1 - def. etykiety znaleziona w 2. przejœciu
			}
			else		// etykieta jeszcze nie zdefiniowana
				value = 0;		// 0 - etykieta niezdefiniowana
		}

		lex = next_lexeme();

		return Expr(value);
	}
	else if (hit == -1 || hit == -2)		// function STRLEN or BITCOUNT?
	{
		lex = next_lexeme(false);
		if (lex.Type() != Lexeme::L_PARENTHESIS_L)
			throw ERR_BRACKET_L_EXPECTED;		// expected opening bracket '(' (without space)
		lex = next_lexeme();
		Expr expr= expression(lex, true);

		if (hit == -2)		// function BITCOUNT?
		{
			if (!(expr.IsNumber() || expr.inf == Expr::EX_REG_LIST))
			{
				if (expr.inf == Expr::EX_UNDEF)
					throw ERR_UNDEF_EXPR;
				else
					throw ERR_EXPECTED_NUMBER;	// number expected
			}
		}
		else
		{
			if (expr.inf != Expr::EX_STRING)
				throw ERR_STR_EXPECTED;		// string expected as an argument to STRLEN
		}
		if (lex.Type() != Lexeme::L_PARENTHESIS_R)
			throw ERR_BRACKET_R_EXPECTED;	// expected paren ')'
		lex = next_lexeme();

		if (hit == -1)
			return Expr(static_cast<int32>(expr.string.length()));
		else
		{
			// count set bits
			uint32 mask= 1;
			int bit_count= 0;

			for (int i= 0; i < 32; ++i, mask <<= 1)
				if (expr.value & mask)
					++bit_count;

			return Expr(bit_count);
		}
	}

	return Expr::Undefined();
}


// constant value - label, parameter, number, predefined constant or function
Expr CFAsm::constant_value(Lexeme& lex, bool nospace)
{
	Expr expr(0);

	switch (lex.Type())
	{
	case Lexeme::L_NUM:		// number (dec, hex, bin, or char)
		expr.value = lex.GetValue();
		break;

	case Lexeme::L_STR:		// string of chars in quotes
		expr = Expr(lex.GetString());
		break;

	case Lexeme::L_IDENT:	// identifier
	{
		expr = predef_const(lex.GetString());
		if (expr.IsDefined())
		{
			lex = next_lexeme();
			return expr;
		}

		expr = predef_function(lex);
		if (expr.IsDefined())
			return expr;

		if (expanding_macro_)
		{
			// check macro parameter names, if expanding macro
			auto expr= expanding_macro_->ParamLookup(lex, lex.GetString(), this);
			if (expr.IsValid())
				return expr;
		}

		Ident id(Ident::I_UNDEF);	// undefined identifier
		if (!add_ident(lex.GetString(), id) && id.info != Ident::I_UNDEF)	// already defined?
			expr = id.val;			// read label's value
		else
		{
			expr.inf = Expr::EX_UNDEF;	// no value yet
			if (pass_ == 2)
			{
				err_ident_ = lex.GetString();
				throw ERR_UNDEF_LABEL;	// label still undefined in a second pass
			}
		}

		if (check_line_)			// tryb sprawdzania jednego wiersza?
		{
			lex = next_lexeme(false);
			if (lex.Type() == Lexeme::L_STR_ARG)	// skip dollar sign '$' at the end of label name
				lex = next_lexeme();
			if (lex.Type() == Lexeme::L_SPACE)
				lex = next_lexeme();
			return expr;
		}
		break;
	}

	case Lexeme::L_IDENT_N:		// numbered identifier
	{
		Lexeme ident(lex);
		lex = next_lexeme();
		auto num= factor(lex);

		if (num.inf == Expr::EX_UNDEF)	// undefined value?
			throw ERR_UNDEF_EXPR;
		ident.Format(num.value);	// znormalizowanie postaci etykiety

		Ident id(Ident::I_UNDEF);	// niezdefiniowany identyfikator
		if (!add_ident(ident.GetString(), id) && id.info != Ident::I_UNDEF)	// ju¿ zdefiniowany?
			return Expr(id.val);		// odczytana wartoœæ etykiety
		else
		{
			if (pass_ == 2)
			{
				err_ident_ = ident.GetString();
				throw ERR_UNDEF_LABEL;		// niezdefiniowana etykieta w drugim przebiegu
			}
			return Expr::Undefined();
		}
		break;
	}

	case Lexeme::L_OPER:		// operator
		if (expanding_macro_ && lex.GetOper() == O_MOD)	// '%' (referencing macro parameter)?
		{
			lex = next_lexeme(false);
			if (lex.Type() == Lexeme::L_SPACE)		// no space allowed
				throw ERR_PARAM_NUMBER_EXPECTED;

			auto num= factor(lex, false);			// expected macro parameter number

			if (num.inf == Expr::EX_UNDEF)
				throw ERR_UNDEF_PARAM_NUMBER;		// param number has to be defined

			return expanding_macro_->ParamLookup(lex, num.value - 1, this);
		}
		else if (check_line_ && lex.GetOper() == O_MOD)	// tryb sprawdzania wiersza?
		{
			lex = next_lexeme(false);
			if (lex.Type() == Lexeme::L_SPACE)		// odstêp niedozwolony
				throw ERR_PARAM_NUMBER_EXPECTED;
			expr = factor(lex, false);		// oczekiwany numer parametru makra

			expanding_macro_->AnyParamLookup(lex, *this);

			return expr;
		}
		else if (lex.GetOper() == O_MUL)	// '*' ?
		{
			if (origin_.Missing())
				throw ERR_UNDEF_ORIGIN;
			expr.value = origin_;			// current origin
			break;
		}
	// no break here
	default:
		throw ERR_CONST_EXPECTED;
	}

	lex = next_lexeme(nospace);
	return expr;
}


Expr CFAsm::factor(Lexeme& lex, bool nospace)	// [~|!|-|>|<] constant | '['expression']'
{
	OperType oper;

	if (lex.Type() == Lexeme::L_OPER)
	{
		oper = lex.GetOper();
		switch (oper)
		{
		case O_B_NOT:	// negacja bitowa '~'
		case O_NOT:		// negacja logiczna '!'
		case O_MINUS:	// minus unarny '-'
		case O_GT:		// górny bajt s³owa '>'
		case O_LT:		// dolny bajt s³owa '<'
		{
			lex = next_lexeme();	// kolejny niepusty leksem
			auto expr= factor(lex, nospace);

			if (expr.inf == Expr::EX_STRING)
				throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

			if (expr.inf != Expr::EX_UNDEF)
				switch (oper)
				{
				case O_B_NOT:	// negacja bitowa '~'
					expr.value = ~expr.value;
					break;
				case O_NOT:		// negacja logiczna '!'
					expr.value = !expr.value;
					break;
				case O_MINUS:	// minus unarny '-'
					expr.value = -expr.value;
					break;

				case O_GT:		// hi word '>'
					expr.value = (uint32(expr.value) >> 16) & 0xffff;
					break;
				case O_LT:		// low word '<'
					expr.value = expr.value & 0xffff;
					break;
				}
			return expr;
		}
		default:
			break;
		}
	}

	if (lex.Type() == Lexeme::L_EXPR_BRACKET_L)
	{
		lex = next_lexeme();		// kolejny niepusty leksem
		auto expr= expression(lex, true);

		if (lex.Type() != Lexeme::L_EXPR_BRACKET_R)
			throw ERR_EXPR_BRACKET_R_EXPECTED;

		lex = next_lexeme(nospace);	// kolejny leksem
		return expr;
	}
	else
	{
		return constant_value(lex, nospace);
	}
}


Expr CFAsm::mul_expr(Lexeme& lex)	// factor [*|/|% factor]
{
	auto expr= factor(lex);		// calculate first factor

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// not an operator?
			return expr;

		OperType oper= lex.GetOper();
		if (oper != O_MUL && oper != O_DIV && oper != O_MOD)
			return expr;

		if (expr.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// string is not allowed

		lex = next_lexeme();				// skip '*', '/' or '%' operatora

		Expr expr2= factor(lex);			// next factor

		if (expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// string is not allowed

		if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
			switch (oper)
			{
			case O_MUL:
				expr.value *= expr2.value;
				break;
			case O_DIV:
				if (expr2.value == 0)
					throw ERR_DIV_BY_ZERO;
				expr.value /= expr2.value;
				break;
			case O_MOD:
				if (expr2.value == 0)
					throw ERR_DIV_BY_ZERO;
				expr.value %= expr2.value;
				break;
			}
		else
			expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::shift_expr(Lexeme& lex)	// czynnik1 [<<|>> czynnik1]
{
	auto expr= mul_expr(lex);		// obliczenie sk³adnika

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;

		bool left;
		switch (lex.GetOper())
		{
		case O_SHL:			// przesuniêcie w lewo?
			left = true;
			break;
		case O_SHR:			// przesuniêcie w prawo?
			left = false;
			break;
		default:
			return expr;
		}
		if (expr.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony
		lex = next_lexeme();		// ominiêcie operatora '>>' lub '<<'
		Expr expr2= mul_expr(lex);			// obliczenie kolejnego sk³adnika

		if (expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
		{
			if (left)
				expr.value <<= expr2.value;
			else
				expr.value >>= expr2.value;
		}
		else
			expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::add_expr(Lexeme& lex)	// sk³adnik [+|- sk³adnik]
{
	auto expr= shift_expr(lex);	// obliczenie sk³adnika

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;

		bool add;
		switch (lex.GetOper())
		{
		case O_MINUS:			// odejmowanie?
			add = false;
			if (expr.inf == Expr::EX_STRING)
				throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony
			break;
		case O_PLUS:			// dodawanie?
			add = true;
			break;
		default:
			return expr;
		}
		lex = next_lexeme();		// ominiêcie operatora '+' lub '-'
		Expr expr2= shift_expr(lex);	// obliczenie kolejnego sk³adnika

		if (!add && expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if ((expr.inf == Expr::EX_STRING) ^ (expr2.inf == Expr::EX_STRING))	// albo, albo
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
		{
			if (add)
			{
				if (expr.inf == Expr::EX_STRING && expr2.inf == Expr::EX_STRING)
					expr.string += expr2.string;
				else
				{
					ASSERT(expr.inf != Expr::EX_STRING && expr2.inf != Expr::EX_STRING);
					expr.value += expr2.value;
				}
			}
			else
				expr.value -= expr2.value;
		}
		else
			expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::bit_expr(Lexeme& lex)	// wyr_proste [ & | '|' | ^ wyr_proste ]
{
	auto expr= add_expr(lex);		// obliczenie wyra¿enia prostego

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;
		OperType oper= lex.GetOper();
		if (oper != O_B_AND && oper != O_B_OR && oper != O_B_XOR)
			return expr;
		if (expr.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		lex = next_lexeme();		// ominiêcie operatora '&', '|' lub '^'

		Expr expr2= add_expr(lex);			// kolejne wyra¿enie proste

		if (expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
			switch (oper)
			{
			case O_B_AND:
				expr.value &= expr2.value;
				break;
			case O_B_OR:
				expr.value |= expr2.value;
				break;
			case O_B_XOR:
				expr.value ^= expr2.value;
				break;
			}
		else
			expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::cmp_expr(Lexeme& lex)	// wyr [>|<|>=|<=|==|!= wyr]
{
	auto expr= bit_expr(lex);		// obliczenie sk³adnika

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;

		OperType oper= lex.GetOper();
		if (oper != O_GT && oper != O_GTE && oper != O_LT && oper != O_LTE && oper != O_EQ && oper != O_NE)
			return expr;

		lex = next_lexeme();			// ominiêcie operatora logicznego
		Expr expr2= bit_expr(lex);		// obliczenie kolejnego sk³adnika

		if ((expr.inf == Expr::EX_STRING) ^ (expr2.inf == Expr::EX_STRING))	// albo, albo
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
			if (expr.inf == Expr::EX_STRING && expr2.inf == Expr::EX_STRING)
			{
				switch (oper)
				{
				case O_GT:
					expr.value = expr.string > expr2.string;
					break;
				case O_LT:
					expr.value = expr.string < expr2.string;
					break;
				case O_GTE:
					expr.value = expr.string >= expr2.string;
					break;
				case O_LTE:
					expr.value = expr.string <= expr2.string;
					break;
				case O_EQ:
					expr.value = expr.string == expr2.string;
					break;
				case O_NE:
					expr.value = expr.string != expr2.string;
					break;
				}
				expr.inf = Expr::EX_BYTE;
			}
			else
			{
				ASSERT(expr.inf != Expr::EX_STRING && expr2.inf != Expr::EX_STRING);
				switch (oper)
				{
				case O_GT:
					expr.value = expr.value > expr2.value;
					break;
				case O_LT:
					expr.value = expr.value < expr2.value;
					break;
				case O_GTE:
					expr.value = expr.value >= expr2.value;
					break;
				case O_LTE:
					expr.value = expr.value <= expr2.value;
					break;
				case O_EQ:
					expr.value = expr.value == expr2.value;
					break;
				case O_NE:
					expr.value = expr.value != expr2.value;
					break;
				}
			}
		else
			expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::bool_expr_and(Lexeme& lex)	// wyr [&& wyr]
{
	bool skip= false;

	auto expr= cmp_expr(lex);		// obliczenie sk³adnika

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;
		if (lex.GetOper() != O_AND)
			return expr;
		if (expr.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (expr.inf != Expr::EX_UNDEF && expr.value == 0)
			skip = true;		// ju¿ false - nie potrzeba dalej liczyæ

		lex = next_lexeme();			// ominiêcie operatora '&&'
		Expr expr2= cmp_expr(lex);			// obliczenie kolejnego sk³adnika

		if (expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (!skip)
			if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
				expr.value = expr2.value ? 1 : 0;
			else
				expr.inf = Expr::EX_UNDEF;
	}
}


Expr CFAsm::bool_expr_or(Lexeme& lex)	// wyr [|| wyr]
{
	bool skip= false;

	auto expr= bool_expr_and(lex);		// obliczenie sk³adnika

	for (;;)
	{
		if (lex.Type() != Lexeme::L_OPER)	// nie operator?
			return expr;
		if (lex.GetOper() != O_OR)
			return expr;

		if (expr.inf != Expr::EX_UNDEF && expr.value != 0)
			skip = true;		// ju¿ true - nie potrzeba dalej liczyæ

		lex = next_lexeme();			// ominiêcie operatora '||'
		Expr expr2= bool_expr_and(lex);		// obliczenie kolejnego sk³adnika

		if (expr2.inf == Expr::EX_STRING)
			throw ERR_STR_NOT_ALLOWED;		// tekst niedozwolony

		if (!skip)
			if (expr.IsDefined() && expr2.IsDefined())	// calculate value?
				expr.value = expr2.value ? 1 : 0;
			else
				expr.inf = Expr::EX_UNDEF;
	}
}

// interpretacja wyra¿enia
Expr CFAsm::expression(Lexeme& lex, bool str)
{
	auto expr= bool_expr_or(lex);

	if (expr.inf == Expr::EX_STRING)
	{
		if (!str)		// wyra¿enie znakowe dozwolone?
			throw ERR_STR_NOT_ALLOWED;
	}
	else if (expr.inf == Expr::EX_REGISTER || expr.inf == Expr::EX_REG_LIST)
	{
		// don't change it
	}
	else if (expr.inf != Expr::EX_UNDEF)
	{
		int32 value= int32(expr.value);
		if (value > -0x100 && value < 0x100)
			expr.inf = Expr::EX_BYTE;
		else if (value > -0x10000 && value < 0x10000)
			expr.inf = Expr::EX_WORD;
		else
			expr.inf = Expr::EX_LONG;
	}
	return expr;
}

//-----------------------------------------------------------------------------

Stat CFAsm::look_for_endm()	// look for .ENDM or .MACRO
{
	Lexeme lex= next_lexeme(false);	// kolejny leksem, byæ mo¿e pusty (L_SPACE)
	bool labelled= false;

	switch (lex.Type())
	{
	case Lexeme::L_IDENT:	// etykieta
	case Lexeme::L_IDENT_N:	// etykieta numerowana
		lex = next_lexeme();
		if (lex.Type() == Lexeme::L_IDENT_N)
			factor(lex);

		labelled = true;
		switch (lex.Type())
		{
		case Lexeme::L_LABEL:	// znak ':'
			lex = next_lexeme();
			if (lex.Type() != Lexeme::L_ASM_INSTR)
				return OK;
			break;
		case Lexeme::L_ASM_INSTR:
			break;
		default:
			return OK;
		}
		break;

	case Lexeme::L_SPACE:	// odstêp
		lex = next_lexeme();
		if (lex.Type() != Lexeme::L_ASM_INSTR)			// nie dyrektywa asemblera?
			return OK;
		break;

	case Lexeme::L_COMMENT:	// komentarz
	case Lexeme::L_EOL:		// koniec wiersza
		return OK;

	case Lexeme::L_FIN:	// koniec tekstu
		return STAT_FIN;

	default:
		return ERR_UNEXP_DAT;
	}

	ASSERT(lex.Type() == Lexeme::L_ASM_INSTR);		// dyrektywa asemblera

	switch (lex.GetInstr())
	{
	case I_MACRO:
		return ERR_NESTED_MACRO;	// nested macro definition is not allowed
	case I_ENDM:
		return labelled ? ERR_LABEL_NOT_ALLOWED : STAT_ENDM;	// koniec makra
	default:
		return OK;
	}
}


Stat CFAsm::record_macro()	// wczytanie kolejnego wiersza makrodefinicji
{
	MacroDef* macro= get_last_macro_entry();

	Stat ret= look_for_endm();
	if (ret > 0)
		return ret;

	if (ret != STAT_ENDM)		// wiersza z .ENDM ju¿ nie potrzeba zapamiêtywaæ
		macro->AddLine(current_line_, text_->GetLineNo());

	return ret;
}

//-----------------------------------------------------------------------------

static FixedString CaseChange(FixedString id, bool case_sensitive)
{
	if (case_sensitive)
		return id;

	return boost::algorithm::to_lower_copy(id);
}


FixedString CFAsm::format_local_label(FixedString ident, int area)
{
	std::ostringstream ost;
	ost << area << ident.c_str();
	return FixedString(ost.str());
}


// spr. czy dana etykieta jest ju¿ zdefiniowana
bool CFAsm::add_ident(FixedString id, Ident& inf)
{
	auto ident= CaseChange(id, case_sensitive_);

	if (ident[0] == LOCAL_LABEL_CHAR)	// etykieta lokalna?
	{
		if (expanding_macro_)		// etykieta lokalna w makrorozszerzeniu?
			return macro_ident_.insert(format_local_label(ident, macro_local_area), inf);
		else
			return local_ident_.insert(format_local_label(ident, local_area_), inf);
	}
	else		// etykieta globalna
		return global_ident_.insert(ident, inf);
}


// wprowadzenie definicji etykiety (1. przebieg asemblacji)
Stat CFAsm::def_ident(FixedString id, Ident& inf)
{
	ASSERT(pass_ == 1);

	if (find_const(id) != PredefConst::NONE)
		return err_ident_ = id, ERR_CONST_LABEL_REDEF;

	auto ident= CaseChange(id, case_sensitive_);

	if (ident[0] == LOCAL_LABEL_CHAR)	// etykieta lokalna?
	{
		if (expanding_macro_)
		{
			if (!macro_ident_.replace(format_local_label(ident, macro_local_area), inf))
				return err_ident_= ident, ERR_LABEL_REDEF;		// ju¿ zdefiniowana
		}
		else if (!local_ident_.replace(format_local_label(ident, local_area_), inf))
			return err_ident_= ident, ERR_LABEL_REDEF;				// ju¿ zdefiniowana
	}
	else		// etykieta globalna
	{
		if (!global_ident_.replace(ident, inf))
			return err_ident_= ident, ERR_LABEL_REDEF;				// ju¿ zdefiniowana
		//    if (inf.info == Ident::I_ADDRESS)// etykieta z adresem odgradza etykiety lokalne
		local_area_++;			// nowy obszar lokalny
	}
	return OK;
}


//bool CFAsm::is_parameter(Lexeme& lex) const
//{
//	if (lex == Lexeme::L_IDENT || expanding_macro_ && lex == Lexeme::L_OPER && lex.GetOper() == O_MOD)
//		return true;
//
//	return false;
//}


Expr CFAsm::find_ident(Lexeme& lex, FixedString id)
{
	Ident i;

	auto ident= CaseChange(id, case_sensitive_);

	if (expanding_macro_)	// check macro param names first
	{
		auto expr= expanding_macro_->ParamLookup(lex, ident, nullptr);

		if (expr.IsValid())
			return expr;
	}

	if (id[0] == LOCAL_LABEL_CHAR)	// etykieta lokalna?
	{
		if (expanding_macro_)
		{
			if (macro_ident_.lookup(ident, i))
				return i.val;
		}
		else
		{
			if (local_ident_.lookup(format_local_label(ident, local_area_), i))
				return i.val;
		}
	}
	else
	{
		// global label
		if (global_ident_.lookup(ident, i))
			return i.val;
	}

	return Expr::Undefined();
}

// sprawdzenie czy etykieta jest zdefiniowana (2. przebieg asemblacji)
Stat CFAsm::chk_ident(FixedString id, Ident& inf)
{
	ASSERT(pass_ == 2);
	auto ident= CaseChange(id, case_sensitive_);

	Ident info;
	bool exist= false;

	if (ident[0] == LOCAL_LABEL_CHAR)	// etykieta lokalna?
	{
		if (expanding_macro_)		// etykieta lokalna w makrorozszerzeniu?
			exist = macro_ident_.lookup(format_local_label(ident, macro_local_area), info);
		else
			exist = local_ident_.lookup(format_local_label(ident, local_area_), info);
	}
	else		// etykieta globalna
	{
		exist = global_ident_.lookup(ident, info);
		local_area_++;	// nowy obszar lokalny
	}

	if (exist)	// sprawdzana etykieta znaleziona w tablicy
	{
		if (info.info == Ident::I_UNDEF)
			return err_ident_= ident, ERR_UNDEF_LABEL;				// etykieta bez definicji
		ASSERT(info.variable && inf.variable || !info.variable && !inf.variable);
		if (info.val != inf.val && !info.variable)
			return err_ident_= ident, ERR_PHASE;		// niezgodne wartoœci miêdzy przebiegami - b³¹d fazy
	}
	else		// sprawdzanej etykiety nie ma w tablicy
		return err_ident_= ident, ERR_UNDEF_LABEL;

	inf = info;
	return OK;
}


// sprawdzenie definicji etykiety (2. przebieg asemblacji)
Stat CFAsm::chk_ident_def(FixedString id, Ident& inf)
{
	auto ident= CaseChange(id, case_sensitive_);

	Expr val= inf.val;		// zapamiêtanie wartoœci
	Stat ret= chk_ident(ident, inf);
	if (ret != OK && ret != ERR_UNDEF_LABEL)
		return ret;
	if (inf.variable)		// etykieta zmiennej?
	{
		inf.val = val;		// nowa wartoœæ zmiennej
		bool ret;
		if (ident[0] == LOCAL_LABEL_CHAR)	// etykieta lokalna?
		{
			if (expanding_macro_)		// etykieta lokalna w makrorozszerzeniu?
				ret = macro_ident_.replace(format_local_label(ident, macro_local_area), inf);
			else
				ret = local_ident_.replace(format_local_label(ident, local_area_), inf);
		}
		else
			ret = global_ident_.replace(ident, inf);
		ASSERT(ret);
	}
	else if (ident[0] != LOCAL_LABEL_CHAR)	// etykieta globalna sta³ej?
	{
		ASSERT(!inf.checked);
		inf.checked = true;		// potwierdzenie definicji w drugim przejœciu asemblacji
		bool ret= global_ident_.replace(ident, inf);
//		ASSERT(!ret && inf.info == I_ADDRESS || ret);		// etykieta musi byæ redefiniowana
	}
	return OK;
}


Stat CFAsm::def_macro_name(FixedString id, Ident& inf)
{
	ASSERT(pass_ == 1);
	auto ident= CaseChange(id, case_sensitive_);

	if (!macro_names_.replace(ident, inf))
		return err_ident_ = ident, ERR_LABEL_REDEF;			// nazwa ju¿ zdefiniowana

	return OK;
}


Stat CFAsm::chk_macro_name(FixedString id)
{
	ASSERT(pass_ == 2);
	auto ident= CaseChange(id, case_sensitive_);

	Ident info;

	if (macro_names_.lookup(ident, info))	// sprawdzana etykieta znaleziona w tablicy
	{
		ASSERT(info.info == Ident::I_MACRONAME);
		return OK;
		//    if (info.val != inf.val)
		//      return err_ident_=ident, ERR_PHASE;// niezgodne wartoœci miêdzy przebiegami - b³¹d fazy
	}
	else		// sprawdzanej etykiety nie ma w tablicy
		return err_ident_= ident, ERR_UNDEF_LABEL;

	return OK;
}

//-----------------------------------------------------------------------------

const char* RepeatDef::GetCurrLine(std::string& str)	// odczyt aktualnego wiersza do powtórki
{
	ASSERT(line_number_ >= 0);
	if (line_number_ == GetSize())	// koniec wierszy?
	{
		if (repeat_ == 0)		// koniec powtórzeñ?
			return nullptr;
		if (GetSize() == 0)		// puste powtórzenie (bez wierszy)?
			return nullptr;
		repeat_--;		// odliczanie powtórzeñ
		//    m_nRepeatLocalArea++;	// nowy obszar etykiet
		ASSERT(repeat_ >= 0);
		line_number_ = 0;
	}
	ASSERT(line_number_ < GetSize());
	str = GetLine(line_number_++);
	return str.c_str();
}


Stat CFAsm::record_rept(RepeatDef* rept)	// wczytanie kolejnego wiersza do powtórki
{
	Stat ret= look_for_repeat();
	if (ret > 0)
		return ret;

	if (ret == STAT_REPEAT)	// zagnie¿d¿one .REPEAT
	{
		ret = OK;
		rept_nested_++;
	}
	else if (ret == STAT_ENDR)
		if (rept_nested_ == 0)		// koniec .REPEAT?
			return ret;
		else
		{
			rept_nested_--;		// koniec zagnie¿d¿onego .REPEAT
			ret = OK;
		}

	rept->AddLine(current_line_, text_->GetLineNo());

	return ret;
}


Stat CFAsm::look_for_repeat()	// szukanie .ENDR lub .REPEAT
{
	Lexeme lex= next_lexeme(false);	// kolejny leksem, byæ mo¿e pusty (L_SPACE)

	switch (lex.Type())
	{
	case Lexeme::L_IDENT:		// etykieta
	case Lexeme::L_IDENT_N:	// etykieta numerowana
		lex = next_lexeme();
		if (lex.Type() == Lexeme::L_IDENT_N)
			factor(lex);

		switch (lex.Type())
		{
		case Lexeme::L_LABEL:	// znak ':'
			lex = next_lexeme();
			if (lex.Type() != Lexeme::L_ASM_INSTR)
				return OK;
			break;
		case Lexeme::L_ASM_INSTR:
			break;
		default:
			return OK;
		}
		//      lex = next_lexeme();
		break;
	case Lexeme::L_SPACE:	// odstêp
		lex = next_lexeme();
		if (lex.Type() != Lexeme::L_ASM_INSTR)			// nie dyrektywa asemblera?
			return OK;
		break;
	case Lexeme::L_COMMENT:	// komentarz
	case Lexeme::L_EOL:		// koniec wiersza
		return OK;
	case Lexeme::L_FIN:	// koniec tekstu
		return STAT_FIN;
	default:
		return ERR_UNEXP_DAT;
	}

	ASSERT(lex.Type() == Lexeme::L_ASM_INSTR);		// dyrektywa asemblera

	switch (lex.GetInstr())
	{
	case I_REPEAT:
		return STAT_REPEAT;		// zagnie¿d¿one .REPEAT
	case I_ENDR:
		return STAT_ENDR;		// koniec .REPEAT
	default:
		return OK;
	}
}

//-----------------------------------------------------------------------------

void CFAsm::asm_start()
{
	user_error_text_.clear();
	if (mark_area_)
		mark_area_->Clear();
	if (debug_)
	{
		debug_->ResetFileMap();
		entire_text_.SetFileUID(debug_);	// wygenerowanie FUID dla tekstu Ÿród³owego
	}
}

void CFAsm::asm_fin()
{
	if (debug_)
		generate_debug();
	if (mark_area_ && mark_area_->IsStartSet())
		mark_area_->SetEnd(origin_ - 1);
}

void CFAsm::asm_start_pass()
{
	source_.Push(&entire_text_);
	text_ = source_.Peek();
	local_area_ = 0;
	macro_local_area = 0;
	text_->Start(nullptr);
	origin_.Reset();
	if (pass_ == 2 && debug_ != 0)
		debug_->Empty();
}

void CFAsm::asm_fin_pass()
{
	text_->Fin(nullptr);
}

Stat CFAsm::assemble()	// translacja programu
{
	Stat ret;
	bool skip= false;
	bool skip_macro= false;

	try
	{
		asm_start();

		for (pass_= 1; pass_ <= 2; pass_++)	// dwa przejœcia asemblacji
		{
			asm_start_pass();

			for (bool fin= false; !fin; )
			{
				while (!(ptr_ = text_->GetCurrLine(current_line_)))		// funkcja nie zwraca ju¿ wierszy?
				{
					if (source_.Peek())		// jest jeszcze jakaœ funkcja odczytu wierszy?
					{
						text_->Fin(&conditional_asm_);
						expanding_macro_ = source_.FindMacro();
						repeating_ = source_.FindRepeat();
						text_ = source_.Pop();
					}
					else
						break;		// nie ma, zwracamy ptr_==nullptr na oznaczenie koñca tekstu programu
				}
				if (current_line_.length() > 1024)			// spr. max d³ugoœæ wiersza
					return ERR_LINE_TO_LONG;

				if (is_aborted())
					return ERR_USER_ABORT;

				if (skip)		// asemblacja warunkowa (po .IF) ?
					ret = look_for_endif();		// omijanie instrukcji a¿ do .ENDIF lub .ELSE
				else if (in_macro_)		// zapamiêtywanie makra (po .MACRO) ?
					ret = record_macro();		// zapamiêtanie wiersza makra
				else if (skip_macro)		// omijanie makra (w 2. przejœciu po .MACRO) ?
					ret = look_for_endm();		// omijanie wierszy a¿ do .ENDM
				else if (repeat_def_)		// zapamiêtywanie wiersza powtórzeñ?
					ret = record_rept(repeat_def_);		// zapamiêtanie wiersza do powtórzeñ
				else
				{
					ret = assemble_line();	// asemblacja wiersza
					if (pass_ == 2 && listing_.IsOpen())
					{
						listing_.AddSourceLine(current_line_.c_str());
						listing_.NextLine();
					}
				}

				switch (ret)
				{
				case STAT_INCLUDE:
					{
						SourceText* src= dynamic_cast<SourceText*>(text_);
						if (src == nullptr)
							return ERR_INCLUDE_NOT_ALLOWED;		// INCLUDE w makrze/powtórce niedozwolone
						src->Include(include_fname_, debug_);
						break;
					}

				case STAT_IF_TRUE:
				case STAT_IF_FALSE:
				case STAT_IF_UNDETERMINED:
					ret = conditional_asm_.instr_if_found(ret);
					if (ret > OK)
						return ret;		// b³¹d
					skip = ret == STAT_SKIP;	// omijanie instrukcji a¿ do .ELSE lub .ENDIF?
					break;
				case STAT_ELSE:
					ret = conditional_asm_.instr_else_found();
					if (ret > OK)
						return ret;		// b³¹d
					skip = ret == STAT_SKIP;	// omijanie instrukcji a¿ do .ELSE lub .ENDIF?
					break;
				case STAT_ENDIF:
					ret = conditional_asm_.instr_endif_found();
					if (ret > OK)
						return ret;		// b³¹d
					skip = ret == STAT_SKIP;	// omijanie instrukcji a¿ do .ELSE lub .ENDIF?
					break;

				case STAT_MACRO:		// makrodefinicja
					if (pass_ == 2)		// drugie przejœcie?
						skip_macro = true;		// omijanie makrodefinicji (ju¿ zarejestrowanej)
					break;

				case STAT_ENDM:		// koniec makrodefinicji
					if (pass_ == 1)		// pierwsze przejœcie?
					{
						ASSERT(in_macro_);
						in_macro_ = nullptr;		// rejestracja makra zakoñczona
					}
					else
					{
						ASSERT(skip_macro);
						skip_macro = false;	// omijanie definicji makra zakoñczone
					}
					break;

				case STAT_EXITM:
					ASSERT(expanding_macro_);
					while (expanding_macro_ != text_)	// szukanie opuszczanego makra
					{
						text_->Fin(&conditional_asm_);
						text_ = source_.Pop();
					}
					text_->Fin(&conditional_asm_);		// zakoñczenie rozwijania makra
					expanding_macro_ = source_.FindMacro();
					repeating_ = source_.FindRepeat();
					text_ = source_.Pop();	// poprzednie Ÿród³o wierszy
					break;

				case STAT_REPEAT:		// zarejestrowanie wierszy po REPEAT
					repeat_def_ = new RepeatDef(rept_init_);
					if (pass_ == 2)
						repeat_def_->SetFileUID(text_->GetFileUID());
					break;

				case STAT_ENDR:		// end of recording, now play back of 'repeat'
					source_.Push(text_);		// push current text source onto the stack
					text_ = repeat_def_;	// use recorder repeat
					repeat_def_ = nullptr;
					text_->Start(&conditional_asm_);
					break;

				case STAT_FIN:		// koniec pliku
					if (dynamic_cast<SourceText*>(text_) == nullptr)
						return ERR_ENDIF_REQUIRED;	// TODO: cause may be different

					if (!static_cast<SourceText*>(text_)->TextFin())	// koniec zagnie¿d¿onego odczytu (.include) ?
					{
						if (conditional_asm_.in_cond())				// w œrodku dyrektywy IF ?
							return ERR_ENDIF_REQUIRED;
						if (in_macro_)
							return ERR_ENDM_REQUIRED;
						fin = true;		// koniec przejœcia asemblacji
						ret = OK;
					}
					break;

				case OK:
					break;

				default:
					if (listing_.IsOpen())		// usuniêcie listingu ze wzglêdu na b³êdy
						listing_.Remove();
					return ret;			// b³¹d asemblacji
				}
			}

			asm_fin_pass();
		}

		asm_fin();
	}
	catch (StatCode stat)
	{
		return stat;
	}
	catch (Stat stat)
	{
		return stat;
	}

	return ret;
}

//-----------------------------------------------------------------------------

// calculate size of complete instruction
Stat CFAsm::chk_instr_code(const AsmInstruction& asm_instr, uint32& length)
{
	length = 0;

	if (asm_instr.list.count() != 1)		//todo: should this be allowed?
		return ERR_AMBIGUOUS_INSTR;

	if (!asm_instr.list[0]->CalcSize(asm_instr.requested_size, asm_instr.ea_src, asm_instr.ea_dst, length))
		return ERR_CANNOT_CALC_SIZE;

	return OK;
}


// generate code
uint32 CFAsm::generate_code(const AsmInstruction& asm_instr)
{
	// at this point instruction has to be resolved, that is determined which one of several possible choices
	// we are dealing with; otherwise it is ambiguous, code cannot be generated

	if (asm_instr.list.count() != 1)
		throw LogicError("unresolved instruction " __FUNCTION__);

	uint32 o= origin_;
	OutputPointer out(origin_, program_);
	asm_instr.list[0]->Encode(asm_instr.requested_size, asm_instr.ea_src, asm_instr.ea_snd_src, asm_instr.ea_dst, asm_instr.ea_snd_dst, out);
	uint32 size= out.Origin() - o;

	return size;
}


//-----------------------------------------------------------------------------

Stat ConditionalAsm::instr_if_found(Stat condition)
{
	ASSERT(condition == STAT_IF_TRUE || condition == STAT_IF_FALSE || condition == STAT_IF_UNDETERMINED);

	bool assemble= stack_.empty() || get_assemble();

	if (assemble && condition == STAT_IF_UNDETERMINED)
		return ERR_UNDEF_EXPR;		// oczekiwane zdefiniowane wyra¿enie

	if (assemble && condition == STAT_IF_TRUE)
	{
		add_state(BEFORE_ELSE, true);
		return STAT_ASM;		// kolejne wiersze nale¿y poddaæ asemblacji
	}
	else
	{
		add_state(BEFORE_ELSE, false);
		return STAT_SKIP;		// kolejne wiersze nale¿y omin¹æ, a¿ do .ELSE lub .ENDIF
	}
}


Stat ConditionalAsm::instr_else_found()
{
	if (stack_.empty() || get_state() != BEFORE_ELSE)
		return ERR_SPURIOUS_ELSE;
	// zmiana stanu automatu
	if (get_assemble())		// przed .ELSE wiersze asemblowane?
		set_state(AFTER_ELSE, false);		// wiêc po .ELSE ju¿ nie
	else
	{		// wiersze przed .ELSE nie by³y asemblowane
		if (stack_.size() > 1 && get_prev_assemble() || stack_.size() == 1)	// nadrzêdne if/endif asemblowane lub
			set_state(AFTER_ELSE, true);			// nie ma nadrzêdnego if/endif
		else
			set_state(AFTER_ELSE, false);
	}
	return get_assemble() ? STAT_ASM : STAT_SKIP;
}


Stat ConditionalAsm::instr_endif_found()
{
	if (stack_.empty())
		return ERR_SPURIOUS_ENDIF;
	//level--;		// zmiana stanu przez usuniêcie wierzcho³ka stosu
	stack_.pop_back();
	if (!stack_.empty())
		return get_assemble() ? STAT_ASM : STAT_SKIP;
	return STAT_ASM;
}

//-----------------------------------------------------------------------------
/*
int CFAsm::get_line_no()		// numer wiersza (dla debug_ info)
{
	if (repeating)
	return repeating->GetLineNo();
	return expanding_macro_ ? expanding_macro_->GetLineNo() : input.get_line_no();
}

masm::FileUID CFAsm::get_file_UID()	// id pliku (dla debug_ info)
{
	if (repeating)
	return repeating->GetFileUID();
	return expanding_macro_ ? expanding_macro_->GetFileUID() : input.get_file_UID();
}
*/

void CFAsm::generate_debug(uint32 addr, int line_no, FileUID file_uid)
{
	ASSERT(debug_ != nullptr);
	debug_->AddLine(DebugLine(line_no, file_uid, addr, typeid(text_) == typeid(MacroDef) ? DBG_CODE | DBG_MACRO : DBG_CODE));
}


Stat CFAsm::generate_debug(InstrType it, int line_no, FileUID file_UID)
{
	ASSERT(debug_ != nullptr);

	switch (it)
	{
	case I_DC:
	case I_DC_B:
	case I_DC_W:
	case I_DC_L:
	case I_DCB:		// declare block
	case I_DS:
	{
		if (origin_.Missing())
			return ERR_UNDEF_ORIGIN;
		DebugLine dl(line_no, file_UID, origin_, DBG_DATA);
		debug_->AddLine(dl);
		break;
	}

	case I_ORG:		// origin
	case I_END:		// zakoñczenie
	case I_ERROR:	// zg³oszenie b³êdu
	case I_INCLUDE:	// w³¹czenie pliku
	case I_IF:
	case I_ELSE:
	case I_ENDIF:
	case I_OPT:
	case I_ALIGN:
		break;

	default:
		ASSERT(false);
	}

	return OK;
}


void CFAsm::generate_debug()
{
	Ident info;
	std::string ident;
	debug_->SetIdentArrSize(static_cast<int>(global_ident_.size() + local_ident_.size()));
/*todo
	POSITION pos= global_ident_.GetStartPosition();
	int index= 0;
	while (pos)
	{
		global_ident_.GetNextAssoc(pos,ident,info);
		debug_->SetIdent(index++,ident,info);
	}
	pos = local_ident_.GetStartPosition();
	while (pos)
	{
		local_ident_.GetNextAssoc(pos,ident,info);
		debug_->SetIdent(index++,ident,info);
	}
*/
}

//=============================================================================

static const char* GetErrorMsg(Stat stat)
{
	switch (stat)
	{
//	case ERR_DAT:						return "Unexpected data after instruction/label--comment or <CR> expected";
	case ERR_DAT:						return "Unexpected data encountered. Only end of line or comment expected.";
	case ERR_UNEXP_DAT:					return "Unexpected data encountered. Space or label expected.";
	case ERR_UNEXP_REG:					return "Register names cannot be used as labels.";
	case ERR_OUT_OF_MEM:				return "Out of memory.";
	case ERR_FILE_READ:					return "Error reading file.";
	case ERR_NUM_LONG:					return "Number exceeds 16-bit integer range.";
	case ERR_NUM_NOT_BYTE:				return "Value doesn't fit in single byte.";
	case ERR_NUM_NEGATIVE:				return "Illegal negative value.";
	case ERR_NUM_NOT_POSITIVE:			return "Expected positive number.";
	case ERR_INSTR_OR_NULL_EXPECTED:	return "Illegal data--instruction, comment or <CR> expected.";
	case ERR_COMMA_OR_BRACKET_EXPECTED:	return "Expected comma ',' or round bracket ')'.";
	case ERR_BRACKET_L_EXPECTED:		return "Expected opening parenthesis '('.";
	case ERR_DIV_BY_ZERO:				return "Divide by zero in expression.";
	case ERR_EXPR_BRACKET_R_EXPECTED:	return "Missing square bracket ']' closing expression.";
	case ERR_CONST_EXPECTED:			return "Missing constant value (number, label, function or '*').";
	case ERR_LABEL_REDEF:				return "Redefinition - label already defined.";
	case ERR_UNDEF_EXPR:				return "Undefined expression value.";
	case ERR_PC_WRAPED:					return "Wrapped program counter.";
	case ERR_UNDEF_LABEL:				return "Undefined label.";
	case ERR_PHASE:						return "Phase error--inconsistent label value between passes.";
	case ERR_REL_OUT_OF_RNG:			return "Relative address out of range.";
	case ERR_MODE_NOT_ALLOWED:			return "Addressing mode not allowed.";
	case ERR_STR_EXPECTED:				return "Missing string.";
	case ERR_SPURIOUS_ENDIF:			return "ENDIF directive without matching IF.";
	case ERR_SPURIOUS_ELSE:				return "ELSE directive without matching IF.";
	case ERR_ENDIF_REQUIRED:			return "Missing ENDIF directive.";
	case ERR_LOCAL_LABEL_NOT_ALLOWED:	return "Local label not allowed - global label expected.";
	case ERR_LABEL_EXPECTED:			return "Missing label.";
	case ERR_USER_ABORT:				return "Assembly stopped.";
	case ERR_UNDEF_ORIGIN:				return "Missing .ORG directive--undetermined program begining address.";
	case ERR_MACRONAME_REQUIRED:		return "Missing macro name label.";
	case ERR_PARAM_ID_REDEF:			return "Repeated macro parameter name.";
	case ERR_NESTED_MACRO:				return "Nested macrodefiniton not allowed.";
	case ERR_ENDM_REQUIRED:				return "Missing ENDM directive ending macrodefinition.";
	case ERR_UNKNOWN_INSTR:				return "Unrecognized instruction/directive/macro name.";
	case ERR_PARAM_REQUIRED:			return "Not enough parameters in macro call.";
	case ERR_SPURIOUS_ENDM:				return "ENDM directive without matching MACRO.";
	case ERR_SPURIOUS_EXITM:			return "EXITM directive outside macrodefinition not allowed.";
	case ERR_STR_NOT_ALLOWED:			return "Text expression not allowed.";
	case ERR_NOT_STR_PARAM:				return "Parameter referenced with dollar char '$' has no text value.";
	case ERR_EMPTY_PARAM:				return "Referenced parameter not exist--param number out of range.";
	case ERR_UNDEF_PARAM_NUMBER:		return "Undetermined expression value in macro parameter number.";
	case ERR_BAD_MACRONAME:				return "Local label not allowed for macrodefiniiton name.";
	case ERR_PARAM_NUMBER_EXPECTED:		return "After '%' char macro parameter number is expected.";
	case ERR_LABEL_NOT_ALLOWED:			return "Label not allowed for directive.";
	case ERR_BAD_REPT_NUM:				return "Wrong repeat number - expected value in range from 0 to 65535.";
	case ERR_SPURIOUS_ENDR:				return "ENDR directive without matching REPEAT.";
	case ERR_INCLUDE_NOT_ALLOWED:		return "INCLUDE directive not allowed in macro/repeat.";
	case ERR_STRING_TOO_LONG:			return "String to long--maximal length: 255 characters.";
	//case ERR_NOT_BIT_NUM:				return "Wrong bit number--expected value from 0 to 7.";
	case ERR_OPT_NAME_REQUIRED:			return "Missing required option name.";
	case ERR_OPT_NAME_UNKNOWN:			return "Unrecognized option name.";
	case ERR_LINE_TO_LONG:				return "Input line exeeds 1024 characters limit.";
	case ERR_PARAM_DEF_REQUIRED:		return "Missing macro parameter name.";

	case ERR_BRACKET_R_EXPECTED:		return "Missing expceted closing parenthesis ')'.";
	case ERR_NUM_ZERO:					return "Expected nonzero value.";
	case ERR_NUM_NOT_WORD:				return "Expected word value (-0x8000..0x7fff).";
	case ERR_NUM_NOT_IN_RANGE:			return "Number out of supported range of values.";
	case ERR_NUM_NOT_LEGAL:				return "This particular number value is not legal.";
	case ERR_EXPECTED_NUMBER:			return "Number expected.";
	case ERR_NUM_NOT_FIT_BYTE:			return "Number does not fit in a BYTE.";
	case ERR_NUM_NOT_FIT_WORD:			return "Number does not fit in a WORD.";

	case ERR_MISSING_INSTR_SIZE:		return "Missing instruction size (like .B, .W, .L)";
	case ERR_INVALID_REG_SIZE:			return "Invalid indexing register size. Indexing register size can only be LONG";
	case ERR_UNEXPECTED_INSTR_SIZE:		return "Unexpected instruction size attribute.";	// instruction size attribute (.B, .L) is not expected (for unsized instruction)
	case ERR_INVALID_INSTR_SIZE:		return "Invalid instruction size attribute for this instruction.";		// instruction size attribute (.B, .L) is not valid for a given instruction
	case ERR_INVALID_SRC_OPERAND:		return "Invalid source operand, addressing mode not supported.";	// requested source effective address is not supported by given instruction
	case ERR_INVALID_DEST_OPERAND:		return "Invalid destination operand, addressing mode not supported.";	// requested destination effective address is not supported by given instruction
	case ERR_MISSING_COMMA_SEPARATOR:	return "Missing comma before second operand.";	// expected comma (',') before second operand
	case ERR_SIZE_ATTR_NOT_RECOGNIZED:	return "Instruction size attribute not recognized.";	// instruction size attribute is not recognized
	case ERR_INVALID_ADDRESSING_MODE:	return "Invalid addressing mode.";
	case ERR_ADDRESS_REG_EXPECTED:		return "Address register expected.";
	case ERR_ADDR_REG_OR_PC_EXPECTED:	return "Address register of PC register expected.";
	case ERR_INDEX_REG_EXPECTED:		return "Index register expected.";
	case ERR_COMMA_OR_CLOSING_PARENT_EXPECTED: return "Comma or closing parenthesis expected.";	// here either ',' or ')'
	case ERR_MODE_NOT_RECOGNIZED:		return "Addressing mode not recognized.";	// addressing mode not recognized
	case ERR_EXPECTED_WORD_OR_LONG_ATTR: return "Expected word or long size attribute (.w or .l).";	// abs addressing mode supports only .w and .l size attributes
	case ERR_REGISTER_EXPECTED:			return "Missing register.";		// expected register to close range of registers in a list (MOVEM)
	case ERR_MALFORMED_REG_LIST:		return "Malformed register list.";		// list of registers is not in a valid form
	case ERR_DUPLICATE_REG_LIST:		return "Duplicated register in a register list.";		// duplicated register in a register list
	case ERR_INVALID_RANGE_REG_LIST:	return "Invalid register list range.";	// invalid range of regiters in a register list (like D0-D3-D5)
	case ERR_RELATIVE_OFFSET_OUT_OF_BYTE_RANGE: return "Relative offset exceeds byte range.";
	case ERR_RELATIVE_OFFSET_OUT_OF_WORD_RANGE: return "Relative offset exceeds word range.";
	case ERR_INVALID_RELATIVE_OFFSET:	return "Invalid relative offset.";
	case ERR_AMBIGUOUS_INSTR:			return "Ambiguous instruction, provide size attribute to choose one.";		// ambiguous instruction, specify instruction size to disambiguate

	case ERR_CANNOT_CALC_SIZE:			return "Cannot calculate instruction size.";
	case ERR_INSTR_SIZE_MISMATCH:		return "Internal error: generated size doesn't match calculated size.";

	case ERR_INTERNAL:					return "Internal error.";

	default:	return nullptr;
	}
}


std::string CFAsm::GetErrMsg(Stat stat)
{
	if ((stat < OK || stat >= ERR_LAST) && stat != STAT_USER_DEF_ERR)
	{
		ASSERT(false);		// b³êdna wartoœæ 'stat'
		return std::string("???");
	}

	const char* err= GetErrorMsg(stat);

	switch (stat)
	{
	case ERR_CONST_LABEL_REDEF:
	case ERR_LABEL_REDEF:
	case ERR_PHASE:
		{
			assert(err);
			std::ostringstream ost;
			ost << err << " Label: " << err_ident_.c_str();
			return ost.str();
		}

	case STAT_USER_DEF_ERR:
		{
			std::ostringstream ost;
			if (user_error_text_.empty())
				ost << "User error.";
			else
				ost << "User error: " << user_error_text_.c_str();
			return ost.str();
		}
	}

	if (stat.message() && *stat.message())
	{
		std::ostringstream ost;
		if (err)
			ost << err << ' ';
		ost << stat.message();
		return ost.str();
	}

	if (err == nullptr)
	{
		std::ostringstream ost;
		ost << "Error: " << stat << ".";
		return ost.str();
	}
	else
		return err;

/*todo
//  if (!text->IsPresent())	// asemblacja wiersza?
	if (check_line_)
	{
		ASSERT(stat>0);
		if (form.LoadString(IDS_ASM_FORM3) && txt.LoadString(IDS_ASM_ERR_MSG_FIRST+stat))
			msg.Format(form,(int)stat,(LPCTSTR)txt);
		return msg;
	}

	switch (stat)
	{
	case OK:
		msg.LoadString(IDS_ASM_ERR_MSG_FIRST);
		break;
	case ERR_OUT_OF_MEM:
		if (form.LoadString(IDS_ASM_FORM3) && txt.LoadString(IDS_ASM_ERR_MSG_FIRST+stat))
			msg.Format(form,(int)stat,(LPCTSTR)txt);
		break;
	case ERR_FILE_READ:
		if (form.LoadString(IDS_ASM_FORM2) && txt.LoadString(IDS_ASM_ERR_MSG_FIRST+stat))
			msg.Format(form,(int)stat,(LPCTSTR)txt,(LPCTSTR)text->GetFileName());
		break;
	case ERR_UNDEF_LABEL:	// niezdefiniowana etykieta
	case ERR_PHASE:
	case ERR_LABEL_REDEF:	// etykieta ju¿ zdefiniowana
		if (form.LoadString(IDS_ASM_FORM4) && txt.LoadString(IDS_ASM_ERR_MSG_FIRST+stat))
			msg.Format(form,(int)stat,(LPCTSTR)txt,(LPCTSTR)err_ident_,text->GetLineNo()+1,(LPCTSTR)text->GetFileName());
		break;
	case STAT_USER_DEF_ERR:
		if (!user_error_text.IsEmpty())
		{
			if (form.LoadString(IDS_ASM_FORM5))
				msg.Format(form,(LPCTSTR)user_error_text,text->GetLineNo()+1,(LPCTSTR)text->GetFileName());
		}
		else
		{
			if (form.LoadString(IDS_ASM_FORM6))
				msg.Format(form,text->GetLineNo()+1,(LPCTSTR)text->GetFileName());
		}
		break;
	default:
		if (form.LoadString(IDS_ASM_FORM1) && txt.LoadString(IDS_ASM_ERR_MSG_FIRST+stat))
			msg.Format(form,(int)stat,(LPCTSTR)txt,text->GetLineNo()+1,(LPCTSTR)text->GetFileName());
		break;
	}
*/
//	return msg;
}

//=============================================================================

void CFAsm::Listing::Remove()
{
	ASSERT(line_ != -1);	// plik musi byæ otwarty
/*	try
	{
//todo		m_File.Remove(m_File.GetFilePath());
	}
	catch (CFileException *)
	{
	}*/
}


void CFAsm::Listing::NextLine()
{
/*todo
	ASSERT(line_ != -1);	// plik musi byæ otwarty
	if (line_ != 0)
		m_File.WriteString(m_Str);
	line_++;
	m_Str.Format(_T("%04d    "),line_);
	*/
}


void CFAsm::Listing::AddCodeBytes(uint16 addr, int code1	/*= -1*/, int code2	/*= -1*/, int code3	/*= -1*/)
{
/*todo
	ASSERT(line_ != -1);	// plik musi byæ otwarty
	char buf[32];
	if (code3 != -1)
		wsprintf(buf,_T("%04X  %02X %02X %02X     "),(int)addr,(int)code1,code2,code3);
	else if (code2 != -1)
		wsprintf(buf,_T("%04X  %02X %02X        "),(int)addr,(int)code1,code2);
	else if (code1 != -1)
		wsprintf(buf,_T("%04X  %02X           "),(int)addr,(int)code1);
	else
		wsprintf(buf,_T("%04X               "),(int)addr);
	m_Str += buf;
*/
}


void CFAsm::Listing::AddValue(uint16 val)
{
/*todo
	ASSERT(line_ != -1);	// plik musi byæ otwarty
	char buf[32];
	wsprintf(buf,_T("  %04X             "),val);
	m_Str += buf;
	*/
}


void CFAsm::Listing::AddBytes(uint16 addr, uint16 mask, const uint8 mem[], int len)
{
/*todo
	ASSERT(line_ != -1);	// plik musi byæ otwarty
	ASSERT(len > 0);
	char buf[32];
	for (int i=0; i<len; i+=4)
	{
		switch ((len-i) % 4)
		{
		case 1:
			wsprintf(buf,_T("%04X  %02X           "),int(addr),int(mem[addr & mask]));
			break;
		case 2:
			wsprintf(buf,_T("%04X  %02X %02X        "),int(addr),int(mem[addr & mask]),
				int(mem[addr+1 & mask]));
			break;
		case 3:
			wsprintf(buf,_T("%04X  %02X %02X %02X     "),int(addr),int(mem[addr & mask]),
				int(mem[addr+1 & mask]),int(mem[addr+2 & mask]));
			break;
		case 0:
			wsprintf(buf,_T("%04X  %02X %02X %02X %02X  "),int(addr),int(mem[addr & mask]),
				int(mem[addr+1 & mask]),int(mem[addr+2 & mask]),int(mem[addr+3 & mask]));
			break;
		}
		m_Str += buf;
		addr = addr+4 & mask;
		NextLine();
	}
*/
}


void CFAsm::Listing::AddSourceLine(const char* line)
{
	ASSERT(line_ != -1);	// plik musi byæ otwarty
	str_ += line;
}


CFAsm::Listing::Listing(const char* fname)
{
	if (fname && *fname)
	{
		Open(fname);
		line_ = 0;
	}
	else
		line_ = -1;
}


const cf::BinaryProgram& CFAsm::GetProgram() const	// assembled program
{
	return program_;
}


}	// namespace
