#pragma once

class PointerView
{
public:
	virtual void SetPointer(int line, const std::wstring& doc_path, cf::uint32 pc, bool scroll) = 0;

};
