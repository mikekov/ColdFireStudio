/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "Asm.h"
#include "Ident.h"
#include "DebugInfo.h"
#include "OutputPointer.h"
#include "BinaryProgram.h"
#include "Isa.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_list.hpp>

class Instruction;


namespace masm {

// hash table with all possible instructions
typedef std::unordered_multimap<std::string, const Instruction*> InstrMap;
// range of instructions matching a mnemonic (i.e. for AND it'll be AND & ANDI)
typedef std::pair<InstrMap::iterator, InstrMap::iterator> InstrRange;
// max number of matching instructions (like CMP, CMPI, CMPA)
const size_t MAX_INSTR_RANGE= 14;

// list of matching instructions during assembly of the single line (from 0 to MAX_INSTR_RANGE)
class PossibleInstructions
{
public:
	PossibleInstructions()
	{
		count_ = 0;
	}

	PossibleInstructions(InstrRange range)
	{
		count_ = 0;
		for (size_t i= 0; i < MAX_INSTR_RANGE; ++i)
			if (range.first != range.second)
			{
				instructions_[i] = range.first->second;
				++range.first;
				++count_;
			}
			else
				instructions_[i] = 0;

		ASSERT(range.first == range.second);	// if this fails, there's not enough space in fixed size array
	}

	size_t count() const	{ return count_; }
	bool empty() const		{ return count_ == 0; }

	const Instruction* operator [] (size_t index)	const
	{ ASSERT(index < count_); return instructions_[index]; }

	// eliminate instruction from the list
	void clear(size_t index)
	{ ASSERT(index < count_); instructions_[index] = 0; }

	// compact list after removal of some of the instructions
	void compact()
	{
		count_ = std::remove(instructions_, instructions_ + count_, static_cast<Instruction*>(0)) - instructions_;
	}

private:
	size_t count_;
	const Instruction* instructions_[MAX_INSTR_RANGE];
};


// output from the instruction parser: recognized instruction(s), source and destination operands (if any)
struct AsmInstruction
{
	AsmInstruction() : requested_size(IS_NONE)
	{}
	PossibleInstructions list;
	InstructionSize requested_size;
	EffectiveAddress ea_src;
	EffectiveAddress ea_dst;
	EffectiveAddress ea_snd_src;
	EffectiveAddress ea_snd_dst;
};


class Lexeme
{
public:
	//struct Code
	//{
	//	OpCode code;
	//	CodeAdr adr;
	//};

	enum InstrArg	// rodzaj argumentów dyrektywy
	{
		A_BYTE,
		A_NUM,
		A_LIST,
		A_STR,
		A_2NUM
	};

	struct Instr
	{
		InstrType type;
		InstrArg arg;
	};

	enum NumType	// type of number
	{
		N_DEC,
		N_HEX,	// hex number ($1234)
		N_BIN,	// binary (@0101010101)
		N_CHR,	// single character ('x')
		N_CHR2	// two chars ('ab')
	};

	struct Num
	{
		NumType type;
		int32 value;
	};

	enum Error
	{
		ERR_NUM_HEX,
		ERR_NUM_DEC,
		ERR_NUM_BIN,
		ERR_NUM_CHR,
		ERR_NUM_BIG,	// exceeded int range
		ERR_BAD_CHR,
		ERR_STR_UNLIM	// missing string delimiter ('"')
	};

	//...........................................................................

	enum LeksType
	{
		L_UNKNOWN,			// unrecognized character

		L_NUM,				// number (dec, hex, bin, or char)
		L_STR,				// ci¹g znaków w apostrofach lub cudzys³owach
		L_IDENT,			// identyfikator
		L_IDENT_N,			// numerowany identyfikator (zakoñczony przez '#' i numer)
		L_SPACE,			// odstêp
		L_OPER,				// operator
		L_PARENTHESIS_L,	// lewy nawias '('
		L_PARENTHESIS_R,	// prawy nawias ')'
		L_EXPR_BRACKET_L,	// lewy nawias dla wyra¿eñ '['
		L_EXPR_BRACKET_R,	// prawy nawias dla wyra¿eñ ']'
		L_COMMENT,			// znak komentarza ';'
		L_LABEL,			// znak etykiety ':'
		L_COMMA,			// znak przecinka ','
		L_STR_ARG,			// znak dolara '$', koñczy parametr typu tekstowego
		L_MULTI,			// znak wielokropka '...'
		L_INV_COMMAS,		// znak cudzys³owu
		L_HASH,				// znak '#'
		L_EQUAL,			// znak przypisania '='
		L_PROC_INSTR,		// instrukcja procesora
		L_ASM_INSTR,		// dyrektywa asemblera
		L_INSTR_SIZE,		// instraction size attribute (.B, .L, etc.)
		L_EOL,				// koniec wiersza
		L_DATA_REG,			// data register Dn
		L_ADDR_REG,			// address register An or SP
		L_REGISTER,			// some register (PC, USP, CCR, SR, etc.)

		L_FIN,				// koniec danych
		L_ERROR				// b³¹d
	};

private:
	union
	{
		OperType op;		// binary or unary operator
		InstrType instr;	// assembler directive
		Num num;			// sta³a liczbowa lub znakowa
		Error err;
		InstructionSize size;
		// possible instructions (could be several)
		Instruction* instructions[MAX_INSTR_RANGE];
		CpuRegister reg;	// register
	} union_;
	PossibleInstructions instr_;
	FixedString str_;		// id or some string
	LeksType type_;

public:
	Lexeme(const Lexeme& lex);

	~Lexeme() {}

	Lexeme(LeksType type) : type_(type)
	{}
	Lexeme(OperType oper) : type_(L_OPER)
	{ union_.op = oper; }

	Lexeme(InstrRange range) : type_(L_PROC_INSTR), instr_(range)
	{}
	Lexeme(InstrType it) : type_(L_ASM_INSTR)
	{ union_.instr = it; }
	Lexeme(InstructionSize size) : type_(L_INSTR_SIZE)
	{ union_.size = size; }
	Lexeme(Error err) : type_(L_ERROR)
	{ union_.err = err; }
	Lexeme(NumType type, int32 val) : type_(L_NUM)
	{ union_.num.type= type;  union_.num.value= val; }
	Lexeme(FixedString str) : type_(L_STR), str_(str)
	{}
	Lexeme(FixedString str, int dummy) : type_(L_IDENT), str_(str)
	{}
	Lexeme(FixedString str, long dummy) : type_(L_IDENT_N), str_(str)
	{}
	Lexeme(CpuRegister reg);

	//	Lexeme& operator = (const Lexeme& lex);

	LeksType Type() const { return type_; }

	operator LeksType () const { return type_; }

	FixedString GetIdent() const
	{
		ASSERT(type_ == L_IDENT);
		return str_;
	}

	OperType GetOper()
	{
		ASSERT(type_ == L_OPER);
		return union_.op;
	}

	PossibleInstructions GetInstructions()
	{
		ASSERT(type_ == L_PROC_INSTR);
		return instr_;
	}

	InstrType GetInstr()
	{
		ASSERT(type_ == L_ASM_INSTR);
		return union_.instr;
	}

	InstructionSize GetInstrSize()
	{
		ASSERT(type_ == L_INSTR_SIZE);
		return union_.size;
	}

	int GetValue()
	{
		ASSERT(type_ == L_NUM);
		return union_.num.value;
	}

	FixedString GetString() const
	{
		ASSERT(type_ == L_STR || type_ == L_IDENT || type_ == L_IDENT_N);
		return str_;
	}

	CpuRegister GetRegister() const;

	void Format(int32 val)	// znormalizowanie postaci etykiety numerycznej
	{
		ASSERT(type_ == L_IDENT_N);
		std::string num(' ', 9);
		//		num.Format("#%08X",(int)val);
		throw 1;
		//		*str += num;		// do³¹czenie numeru
	}
};

//-----------------------------------------------------------------------------

// tablica asocjacyjna identyfikatorów
class IdentTable
{
	typedef std::unordered_map<FixedString, Ident> Map;
	Map map_;
public:
	IdentTable();
	~IdentTable();

	bool insert(FixedString str, Ident& ident);
	bool replace(FixedString str, const Ident& ident);

	bool lookup(FixedString str, Ident& ident) const;

	void clr_table();

	size_t size() const;
};

//=============================================================================

class InputBase	// klasa bazowa dla klas odczytu danych Ÿród³owych
{
protected:
	int line_;
	Path file_name_;
	bool opened_;

public:
	InputBase(const Path* str= nullptr)
	{ line_ = 0; opened_ = false; if (str) file_name_ = *str; }

	virtual ~InputBase()
	{ ASSERT(opened_ == false); }

	virtual void open()
	{ opened_ = true; }

	virtual void close()
	{ opened_ = false; }

	virtual void seek_to_begin()
	{ line_ = 0; }

	virtual char* read_line(char* str, uint32 max_len) = 0;

	virtual int get_line_no() const
	{ return line_ - 1; }			// numeracja wierszy od 0

	virtual const Path& get_file_name() const
	{ return InputBase::file_name_; }
};

//-----------------------------------------------------------------------------

class InputFile : public InputBase
{
public:
	InputFile(const Path& str) : InputBase(&str)
	{}

	~InputFile()
	{}

	virtual void open();
	virtual void close();
	virtual void seek_to_begin();
	virtual char* read_line(char* str, uint32 max_len);

private:
	std::fstream file_;
};

//-----------------------------------------------------------------------------

class Input
{
	InputBase* tail_;
	FileUID fuid;
	typedef boost::ptr_list<InputBase> Stack;
	Stack stack_;
	int calc_index() const;

public:
	void open_file(const Path& fname);
	void close_file();

	Input(const Path& fname) : fuid(0)
	{ open_file(fname); }
	Input() : fuid(0), tail_(nullptr)
	{}
	~Input();

	char* read_line(char* str, uint32 max_len)
	{ return tail_->read_line(str, max_len); }

	void seek_to_begin()
	{ tail_->seek_to_begin(); }

	int get_line_no() const
	{ return tail_->get_line_no(); }

	int get_count()
	{ return static_cast<int>(stack_.size()); }

	const Path& get_file_name() const
	{ return tail_->get_file_name(); }

	FileUID get_file_UID()
	{ return fuid; }

	void set_file_UID(FileUID fuid)
	{ this->fuid = fuid; }

	bool is_present()
	{ return tail_ != nullptr; }
};

//-----------------------------------------------------------------------------

class ConditionalAsm	// asemblacja warunkowa (automat ze stosem)
{
public:
	enum State
	{ BEFORE_ELSE, AFTER_ELSE };

private:
	std::vector<uint8> stack_;

	State get_state()
	{ ASSERT(!stack_.empty()); return stack_.back() & 1 ? BEFORE_ELSE : AFTER_ELSE; }
	bool get_assemble()
	{ ASSERT(!stack_.empty());  return stack_.back() & 2 ? true : false; }
	bool get_prev_assemble()
	{ ASSERT(stack_.size() > 1);  return stack_.at(stack_.size() - 2) & 2 ? true : false; }
	void set_state(State state, bool assemble)
	{
		stack_.back() = uint8((state == BEFORE_ELSE ? 1 : 0) + (assemble ? 2 : 0));
	}
	void add_state(State state, bool assemble)
	{ stack_.push_back(0); set_state(state, assemble); }
public:
	ConditionalAsm()
	{ stack_.reserve(16); }
	Stat instr_if_found(Stat condition);
	Stat instr_else_found();
	Stat instr_endif_found();
	bool in_cond() const	{ return !stack_.empty(); }
	int get_level() const	{ return static_cast<int>(stack_.size()); }
	void set_level(int level)
	{ if (static_cast<size_t>(level) < stack_.size()) stack_.resize(level); else { if (level != 0 && !stack_.empty()) ASSERT(false); } }
};


//-----------------------------------------------------------------------------

// Source of assembly text (it could be a string, .MACRO, or .REPEAT)
class Source
{
	FileUID m_fuid;
	int level_;

public:
	Source() : m_fuid(0)
	{
		level_ = -1;
	}
	virtual ~Source()
	{}

	virtual void Start(ConditionalAsm* cond)		// rozpoczêcie odczytu wierszy
	{
		level_ = cond ? cond->get_level() : -1;
	}
	virtual void Fin(ConditionalAsm* cond)			// zakoñczenie odczytu wierszy
	{
		if (cond && level_ >= 0)
			cond->set_level(level_);
	}
	virtual const char* GetCurrLine(std::string& str) = 0;	// odczyt bie¿¹cego wiersza
	virtual int GetLineNo() const = 0;		// odczyt numeru wiersza
	virtual FileUID GetFileUID()			// odczyt ID pliku
	{ return m_fuid; }
	void SetFileUID(FileUID fuid)			// ustawienie ID pliku
	{ m_fuid = fuid; }
	virtual const Path& GetFileName() const = 0;	// nazwa aktualnego pliku

protected:
	static const Path empty_;
};

//.............................................................................

// elementy wymagane do zapamiêtywania i odtwarzania wierszy Ÿród³owych programu
class Recorder
{
	struct Src
	{
		Src(const std::string& text, int line) : text(text), line_number(line)
		{}
		std::string text;
		int line_number;
	};
	boost::ptr_vector<Src> storage_;
	int line_;
public:

	Recorder(size_t reserve= 10)
	{
		storage_.reserve(reserve);
	}
	virtual ~Recorder()
	{}

	void AddLine(const std::string& line, int num)		// zapamiêtanie kolejnego wiersza
	{
		storage_.push_back(new Src(line, num));
	}

	const std::string& GetLine(int line_no)		// odczyt wiersza 'line_no'
	{ return storage_.at(line_no).text; }

	int GetLineNo(int line_no) const		// odczyt numeru wiersza w pliku Ÿród³owym
	{ return storage_.at(line_no).line_number; }

	int GetSize()			// odczyt iloœci wierszy w tablicy
	{
		return static_cast<int>(storage_.size());
	}
};

//-----------------------------------------------------------------------------

class CFAsm;

class MacroDef : public Source, public Recorder
{
public:
	FixedString macro_name_;		// nazwa makra
	bool first_code_line_;		// flaga odczytu pierwszego wiersza makra zawieraj¹cego instr. 6502

	MacroDef(const Path& source) : param_names_(), m_nParams(0), line_number_(0), first_line_number_(-1),
		first_line_fuid_(FileUID(-1)), first_code_line_(true), path_(source), size_(IS_NONE)
	{}
	~MacroDef()
	{}

	int AddParam(FixedString param)	// dopisanie nazwy kolejnego parametru
	{
		if (param == MULTIPARAM)
		{
			m_nParams = -(m_nParams + 1);
			return 1;		// koniec listy parametrów
		}
		if (!param_names_.insert(param, Ident(Ident::I_VALUE, m_nParams)))
			return -1;							// powtórzona nazwa parametru!
		m_nParams++;
		return 0;
	}

	//int GetParamsFormat()			// iloœæ przekazywanych parametrów
	//{ return m_nParams; }

	virtual const char* GetCurrLine(std::string& str);	// odczyt aktualnego wiersza makra

	virtual int GetLineNo() const
	{ return Recorder::GetLineNo(line_number_ - 1); }

	int GetFirstLineNo()
	{ return first_line_number_; }

	FileUID GetFirstLineFileUID()
	{ return first_line_fuid_; }

	void Start(ConditionalAsm* cond, int line, FileUID file)	// przygotowanie do odczytu
	{ Source::Start(cond); line_number_ = 0; first_line_number_ = line; first_line_fuid_ = file; }

	virtual void Fin()
	{}					// zakoñczenie rozwijania bie¿¹cego makra

	// wczytanie argumentów wywo³ania
	void ParseArguments(Lexeme& lex, CFAsm& asmb);

	Expr ParamLookup(Lexeme& lex, FixedString param_name, CFAsm* asmb);
	Expr ParamLookup(Lexeme& lex, int param_number, CFAsm* asmb);
	void AnyParamLookup(Lexeme& lex, CFAsm& asmb);
	InstructionSize SizeAttribute() const;

	virtual const Path& GetFileName() const	// nazwa aktualnego pliku
	{ return path_; }

private:
	MacroDef& operator = (const MacroDef& src)
	{
		ASSERT(false);	// nie wolno przypisywaæ obiektów typu MacroDef
		return *this;
	}

	IdentTable param_names_;	// table of macro parameter names
	std::vector<Expr> params_;	// actual values of parameters during macro expansion
	int m_nParams;				// iloœæ parametrów wymagana
	int line_number_;			// nr aktualnego wiersza (przy odczycie)
	int first_line_number_;		// numer wiersza, z którego wywo³ywane jest makro
	FileUID first_line_fuid_;	// ID pliku, z którego wywo³ywane jest makro
	Path path_;					//
	InstructionSize size_;		// size specified during macro call (if any)
};

typedef boost::ptr_vector<MacroDef> MacroDefs;

//-----------------------------------------------------------------------------

class RepeatDef : public Source, public Recorder
{
	int line_number_;		// nr aktualnego wiersza (przy odczycie)
	int repeat_;			// iloœæ powtórzeñ wierszy
public:

	RepeatDef(int nRept= 0) : line_number_(0), repeat_(nRept)
	{}
	~RepeatDef()
	{}

	virtual const char* GetCurrLine(std::string& str);	// odczyt aktualnego wiersza

	virtual int GetLineNo() const
	{ return Recorder::GetLineNo(line_number_ - 1); }

	virtual void Start(ConditionalAsm* cond)
	{ Source::Start(cond); line_number_ = GetSize(); }		// licznik wierszy na koniec
	virtual void Fin(ConditionalAsm* cond)			// zakoñczenie powtórki wierszy
	{ Source::Fin(cond); delete this; }

	RepeatDef & operator = (const RepeatDef& src)
	{
		ASSERT(false);	// nie wolno przypisywaæ obiektów typu RepeatDef
		return *this;
	}

	virtual const Path& GetFileName() const	// nazwa aktualnego pliku
	{ return empty_; }
};


//typedef boost::ptr_vector<RepeatDef> CRepeatDefs;

//-----------------------------------------------------------------------------

class SourceText : public Source
{
	Input input;
public:
	SourceText(const std::wstring& file_in_name) : input(file_in_name)
	{}
	SourceText()
	{}

	virtual void Start(ConditionalAsm* cond)			// start reading lines
	{ Source::Start(nullptr); input.seek_to_begin(); }

	virtual const char* GetCurrLine(std::string& str)	// read current line
	{
		std::vector<char> buf(4000, 0);
		const char* ret= input.read_line(&buf.front(), static_cast<uint32>(buf.size()));
		str = ret;
		return str.c_str();
	}

	virtual int GetLineNo() const	// odczyt numeru wiersza
	{ return input.get_line_no(); }

	FileUID GetFileUID()			// odczyt ID pliku
	{ return input.get_file_UID(); }

	void SetFileUID(DebugInfo* debugInfo)
	{
		if (debugInfo)
			input.set_file_UID(debugInfo->GetFileUID(input.get_file_name()));
	}

	void Include(const Path& fname, DebugInfo* debugInfo= nullptr)	// include new file
	{
		input.open_file(fname);
		if (debugInfo)
			input.set_file_UID(debugInfo->GetFileUID(fname));
	}

	bool TextFin()					// current source file finished
	{
		if (input.get_count() > 1)	// nested reading (.include) ?
		{
			input.close_file();
			return true;
		}
		else
			return false;			// the end of source files
	}

	virtual const Path& GetFileName() const	// current file name
	{ return input.get_file_name(); }
};


//-----------------------------------------------------------------------------


class SourceStack	// stack of asm source objects
{
	std::deque<Source*> stack_;
	int top_index() const { return static_cast<int>(stack_.size()) - 1; }
public:
	SourceStack()
	{}

	~SourceStack()
	{
		for (int i= top_index(); i >= 0; --i)
			stack_[i]->Fin(nullptr);
	}

	void Push(Source* src)		// Dodanie elementu na wierzcho³ku stosu
	{ stack_.push_back(src); }

	Source* Peek()			// Sprawdzenie elementu na szczycie stosu
	{ return stack_.empty() ? 0 : stack_.back(); }

	Source* Pop()			// Zdjêcie elementu ze stosu
	{ Source* s= stack_.back(); stack_.pop_back(); return s; }

	MacroDef* FindMacro()			// odszukanie ostatniego makra
	{
		for (int i= top_index(); i >= 0; --i)
			if (MacroDef * src= dynamic_cast<MacroDef*>(stack_[i]))
				return src;
		return 0;
	}

	RepeatDef* FindRepeat()			// odszukanie ostatniego powtórzenia
	{
		for (int i= top_index(); i >= 0; --i)
			if (RepeatDef * src= dynamic_cast<RepeatDef*>(stack_[i]))
				return src;
		return 0;
	}
};


//-----------------------------------------------------------------------------
class MarkArea;

class CFAsm
{
	friend class MacroDef;

	std::string current_line_;		// current line buffer
	const char* ptr_;				// do œledzenia aktualnego wiersza
	const char* err_start_;
	const char* ident_start_;		// po³o¿enie identyfikatora w wierszu
	const char* ident_fin_;			// po³o¿enie koñca identyfikatora w wierszu

	bool check_line_;				// flaga: true - analiza jednego wiersza, false - programu
	int local_area_;				// nr obszaru etykiet lokalnych
	int macro_local_area;			// nr obszaru etykiet lokalnych makrodefinicji
	int pass_;						// numer przejœcia (1 lub 2)
	Path include_fname_;
	FixedString user_error_text_;	// tekst b³êdu u¿ytkownika (dyrektywy .ERROR)
	const char* instr_start_;		// do zapamiêtania pocz¹tku
	const char* instr_fin_;			// i koñca instrukcji w wierszu
	MacroDefs macros_;				// makrodefinicje

	Source* text_;					// current source text
	SourceText entire_text_;		// first (starting) source text
	RepeatDef* repeat_def_;

	// parsing
	Lexeme get_dec_num();			// interpretacja liczby dziesiêtnej
	Lexeme get_hex_num();			// interpretacja liczby szesnastkowej
	Lexeme get_bin_num();			// interpretacja liczby dwójkowej
	Lexeme get_char_num();			// interpretacja sta³ej znakowej

	FixedString get_ident();		// wyodrêbnienie napisu
	Lexeme get_string(char lim);	// wyodrêbnienie ³añcucha znaków
	Lexeme eat_space();				// ominiêcie odstêpu
	InstrRange proc_instr(FixedString str);
	CpuRegister proc_register(FixedString str);
	bool asm_instr(FixedString str, InstrType& it);
	Lexeme next_lexeme(bool nospace= true);	// pobranie kolejnego symbolu
	bool next_line();				// wczytanie kolejnego wiersza

	MarkArea* mark_area_;			// do zaznaczania u¿ytych obszarów pamiêci z 'out'
	IdentTable local_ident_;		// tablica identyfikatorów lokalnych
	IdentTable global_ident_;		// tablica identyfikatorów globalnych
	IdentTable macro_names_;		// tablica nazw makrodefinicji
	IdentTable macro_ident_;		// tablica identyfikatorów w makrorozwiniêciach
	DebugInfo* debug_;				// informacja uruchomieniowa dla symulatora
	ISA isa_;
	InstrMap instructions_;
	size_t max_mnemonic_length_;
	size_t min_mnemonic_length_;
	FixedString err_ident_;			// name of the label that caused error

	bool abort_asm_;				// flag to abort assembly process
	bool is_aborted()
	{ return abort_asm_ ? abort_asm_ = false, true : false; }

	bool add_ident(FixedString ident, Ident& inf);
	Stat def_ident(FixedString ident, Ident& inf);
	Stat chk_ident(FixedString ident, Ident& inf);
	Stat chk_ident_def(FixedString ident, Ident& inf);
	Stat def_macro_name(FixedString ident, Ident& inf);
	Stat chk_macro_name(FixedString ident);
	Expr find_ident(Lexeme& lex, FixedString id);

	FixedString format_local_label(FixedString ident, int area);
	// parse CPU instruction
	Stat proc_instr_syntax(Lexeme& lex, AsmInstruction& asm_instr);

	Stat check_argument_sizes(AsmInstruction& asm_instr) const;

	// parse directive
	Stat asm_instr_syntax_and_generate(Lexeme& lex, InstrType it, FixedString* label, cf::BinaryProgram& prg);

	Stat asm_decl_const(Lexeme& lex, cf::BinaryProgram& prg, bool str_length_byte, InstructionSize size);

	MacroDef* get_new_macro_entry(const Path& source_file)
	{ macros_.push_back(new MacroDef(source_file)); return &macros_.back(); }
	MacroDef* get_last_macro_entry()
	{ ASSERT(!macros_.empty());  return &macros_.back(); }
	int get_last_macro_entry_index()
	{ ASSERT(!macros_.empty());  return static_cast<int>(macros_.size() - 1); }

	enum class PredefConst { NONE, ORIGIN, IO, VERSION };
	PredefConst find_const(FixedString str);
	Expr predef_const(FixedString str);
	Expr predef_function(Lexeme& lex);
	Expr constant_value(Lexeme& lex, bool nospace);
	Expr factor(Lexeme& lex, bool nospace= true);
	Expr mul_expr(Lexeme& lex);
	Expr shift_expr(Lexeme& lex);
	Expr add_expr(Lexeme& lex);
	Expr bit_expr(Lexeme& lex);
	Expr cmp_expr(Lexeme& lex);
	Expr bool_expr_and(Lexeme& lex);
	Expr bool_expr_or(Lexeme& lex);
	Expr expression(Lexeme& lex, bool str= false);	// parse expression
	bool is_expression(const Lexeme& lex);
	Stat assemble_line();			// assemble single line
	Stat assemble();
	const char* get_next_line();	// read next line to assemble
	const char* play_macro();		// read next line from macro
	const char* play_repeat();		// read next line from repeat
	SourceStack source_;			// stack of assembly sources
	void asm_start();				// start assembly
	void asm_fin();					// finish assembly
	void asm_start_pass();			// start assembly pass
	void asm_fin_pass();			// enf assembly pass
	Stat chk_instr_code(const AsmInstruction& asm_instr, uint32& length);
	uint32 generate_code(const AsmInstruction& asm_instr);
	Stat look_for_endif();			// szukanie .ENDIF lub .ELSE
	void generate_debug(uint32 addr, int line_no, FileUID file_UID);
	Stat generate_debug(InstrType it, int line_no, FileUID file_UID);
	void generate_debug();
	Stat look_for_endm();
	Stat record_macro();
	MacroDef* in_macro_;			// macro being registered or null
	MacroDef* expanding_macro_;		// macro being expanded or null
	RepeatDef* repeating_;			// current repeat (.REPEAT)
	Stat record_rept(RepeatDef* repeat_def);
	Stat look_for_repeat();			// find .ENDR or .REPEAT
	int rept_init_;					// to init repeat count
	int rept_nested_;				// netsed .REPEATs (while recording)
	cf::BinaryProgram program_;		// assembled program is placed here
	ProgramOrigin origin_;
	ConditionalAsm conditional_asm_;
	bool case_sensitive_;			// true -> case sensitive labels

	static int __cdecl asm_str_key_cmp(const void* elem1, const void* elem2);
	struct ASM_STR_KEY
	{
		const char* str;
		masm::InstrType it;
	};

	class Listing	//TODO
	{
		std::string str_;		// bie¿¹cy wiersz listingu
		int line_;		// bie¿¹cy wiersz

		void Open(const char* fname)
		{}	//m_File.Open(fname,CFile::modeCreate|CFile::modeWrite|CFile::typeText); }
		void Close()
		{}	//m_File.Close(); }
	public:
		Listing()
		{ line_ = -1; }
		Listing(const char* fname);
		~Listing()
		{ if (line_ != -1) Close(); }

		void Remove();
		void NextLine();
		void AddCodeBytes(uint16 addr, int code1= -1, int code2= -1, int code3= -1);
		void AddValue(uint16 val);
		void AddBytes(uint16 addr, uint16 mask, const uint8 mem[], int len);
		void AddSourceLine(const char* line);

		bool IsOpen()
		{ return line_ != -1; }
	} listing_;

	void init(ISA isa);
	void init_members();

	bool parse_reg_list(Lexeme& lex, EffectiveAddress& ea);
	Stat parse_operand(Lexeme& lex, AddressingMode modes, const PossibleInstructions& instr, EffectiveAddress& ea);
	Stat parse_indirect_modes(Lexeme& lex, EffectiveAddress& ea);
	Stat parse_indexing_register(Lexeme& lex, EffectiveAddress& ea);
	Stat parse_operand(Lexeme& lex, AddressingMode modes, const PossibleInstructions& instr, EffectiveAddress& ea, bool first_part);
	RegisterWord parse_register_size(Lexeme& lex);

public:

	CFAsm(const std::wstring& file_in_name, bool case_sensitive, DebugInfo* debug= nullptr, MarkArea* area= nullptr, ISA isa= ISA::A, const char* listing_file= nullptr);

	CFAsm(ISA isa, bool case_sensitive);

	~CFAsm()
	{
		if (text_)
			text_->Fin(nullptr);
		if (repeat_def_)
			repeat_def_->Fin(nullptr);
	}

	// sprawdzenie sk³adni w wierszu 'str'
	// w 'instr_idx_start' zwracane po³o¿enie instrukcji w wierszu lub 0
	// w 'instr_idx_fin' zwracane po³o¿enie koñca instrukcji w wierszu lub 0
	Stat CheckLine(const char* str, int& instr_idx_start, int& instr_idx_fin);

	void Abort()
	{ abort_asm_ = true; }

	std::string GetErrMsg(Stat stat);		// opis b³êdu

	Stat Assemble()						// asemblacja
	{ return assemble(); }

	const std::string& CurrentLine() const
	{ return current_line_; }

	size_t CurrentLinePosition() const
	{ return ptr_ ? ptr_ - current_line_.c_str() : 0; }

	const cf::BinaryProgram& GetProgram() const;	// assembled program

	int GetLineNo() const
	{
		if (text_)
			return text_->GetLineNo();
		return 0;
	}

	Path GetFileName() const
	{
		if (text_)
			return text_->GetFileName();
		return Path();
	}
};

}	// namespace
