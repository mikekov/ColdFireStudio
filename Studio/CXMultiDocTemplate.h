/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// zmodyfikowana klasa CMultiDocTemplate - zamieniona funkcja rozpoznaj�ca
// otwierane dokumenty

class CXMultiDocTemplate: public CMultiDocTemplate
{
  bool normal_match_;

public:
  CXMultiDocTemplate(UINT id_resource, CRuntimeClass* doc_class,
    CRuntimeClass* frame_class, CRuntimeClass* view_class, bool normal_match= true) :
      normal_match_(normal_match),
      CMultiDocTemplate(id_resource,doc_class,frame_class,view_class)
  {}
  virtual ~CXMultiDocTemplate()
  {}

  virtual Confidence MatchDocType(LPCTSTR path_name, CDocument*& rpDocMatch)
  {
    if (normal_match_)
      return CMultiDocTemplate::MatchDocType(path_name,rpDocMatch);
    else
      return CDocTemplate::noAttempt;
  }

  virtual BOOL GetDocString(CString& string, enum DocStringIndex i) const;

  static bool registration_ext_;
};
