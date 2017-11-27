#pragma once
#include "CFTypes.h"
#include "SettingsClient.h"
class Debugger;


// DisasmDoc document

class DisasmDoc : public CDocument, SettingsClient
{
	DECLARE_DYNCREATE(DisasmDoc)

public:
	DisasmDoc();
	virtual ~DisasmDoc();

	void SetDebugger(Debugger* dbg);
	Debugger* GetDebugger();

	virtual void Serialize(CArchive& ar);   // overridden for document i/o

	std::string GetDeasmLine(cf::uint32& addr, bool code_bytes, bool ascii) const;
	cf::uint32 GetStartAddr() const;
	void SetStartAddr(cf::uint32 addr);

	cf::uint32 FindAddress(cf::uint32 addr, int delta_lines) const;
	cf::uint32 FindAbsAddress(cf::uint32 addr) const;
	cf::uint32 GetPC() const;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual BOOL OnNewDocument();

	DECLARE_MESSAGE_MAP()

private:
	Debugger* debugger_;
	cf::uint32 addr_;
	unsigned int disasm_flags_;

	void ApplySettings(SettingsSection& settings);
};
