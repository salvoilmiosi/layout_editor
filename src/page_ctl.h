#ifndef __PAGE_CTL_H__
#define __PAGE_CTL_H__

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

wxDECLARE_EVENT(EVT_PAGE_SELECTED, wxCommandEvent);

class PageCtrl : public wxControl {
public:
    PageCtrl(wxWindow *parent, wxWindowID id, int max_pages = 1);
    
    int GetValue();
    void SetValue(int value);

    int GetMaxPages() const;
    void SetMaxPages(int value);

private:
    void selectPage(int amt);

    void OnPageEnter(wxCommandEvent &evt);
    void OnPrevPage(wxCommandEvent &evt);
    void OnNextPage(wxCommandEvent &evt);
    void OnMouseScroll(wxMouseEvent &evt);

    void sendPageSelectedEvent();

    wxTextCtrl *m_page_ctl;
    wxStaticText *m_numpages;

    int m_value;

    DECLARE_EVENT_TABLE()
};

#endif