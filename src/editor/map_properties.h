
/**
   @file    map_properties.h
   @date    May 2006
   @version 1.0
   @brief   Dialog pro vlastnosti mapy.
*/

#ifndef _MAP_PROPERTIES_H_
#define _MAP_PROPERTIES_H_

class MapSettingsDialog : public wxPropertySheetDialog
{
	/*wxTextCtrl* m_becherdir;
    wxButton* m_browse;
    wxButton* m_add;
    wxButton* m_remove;
    wxButton* m_addres;
    wxButton* m_removeres;
    wxListBox* m_listfiles;
    wxListCtrl* m_listresources;*/
public:
	MapSettingsDialog(wxWindow* win);

	//DECLARE_EVENT_TABLE()
};


#endif // _MAP_PROPERTIES_H_
