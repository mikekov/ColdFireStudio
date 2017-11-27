#pragma once

void GetHexEditFont(CFont& font);

void SubclassHexEdit(CWnd* dialog, int ctrl_id, CMFCMaskedEdit& hex_edit, bool long_word);
