
#ifndef _BECHER_EDITOR_MAP_H_
#define _BECHER_EDITOR_MAP_H_

#include "../map.h"

class EditorMap : public BecherMap
{
protected:
	wxString m_mapfilepath;
	BecherObject * m_select;
public:
	EditorMap()
	{
		m_select = NULL;
	}
	~EditorMap()
	{
	}
	void CreateNew(uint sizeX, uint sizeY);
	bool LoadMap(const wxString &path);
	bool SaveMap();
	bool SaveMap(const wxString &path);
	void SelectObject(const int x, const int y);
	void SetFilePath(const wxString &path);
	wxString GetFilePath();
	wxString GetTitle();
	void Resize(int top, int bottom, int left, int right);
	void ShowSystemObjects(bool show);
	void ShowObjects(bool show, bool wire);
	void ModelHeightUpdate();
	void DeleteSelect();
};

#endif // _BECHER_EDITOR_MAP_H_

