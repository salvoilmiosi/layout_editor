#include "page_ctl.h"

#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/artprov.h>
#include <wx/valnum.h>

enum {
    PAGE_SELECT_CTL = 10000,
    PAGE_PREV_BTN,
    PAGE_NEXT_BTN
};

BEGIN_EVENT_TABLE(PageCtrl, wxControl)
    EVT_TEXT_ENTER(PAGE_SELECT_CTL, PageCtrl::OnPageEnter)
    EVT_BUTTON(PAGE_PREV_BTN, PageCtrl::OnPrevPage)
    EVT_BUTTON(PAGE_NEXT_BTN, PageCtrl::OnNextPage)
END_EVENT_TABLE()

wxDEFINE_EVENT(EVT_PAGE_SELECTED, wxCommandEvent);

PageCtrl::PageCtrl(wxWindow *parent, wxWindowID id, int max_pages)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

    sizer->Add(new wxBitmapButton(this, PAGE_PREV_BTN, wxArtProvider::GetBitmap(wxART_MINUS), wxDefaultPosition, wxSize(25, 25)));
    sizer->Add(new wxBitmapButton(this, PAGE_NEXT_BTN, wxArtProvider::GetBitmap(wxART_PLUS), wxDefaultPosition, wxSize(25, 25)));

    m_page_ctl = new wxTextCtrl(this, PAGE_SELECT_CTL, wxEmptyString, wxDefaultPosition, wxSize(30, -1), wxTE_RIGHT | wxTE_PROCESS_ENTER);
    sizer->Add(m_page_ctl, wxSizerFlags(1).Center().Border(wxRIGHT, 5));

    m_numpages = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(20, -1));
    SetMaxPages(max_pages);
    sizer->Add(m_numpages, wxSizerFlags(0).Center());

    SetSizerAndFit(sizer);
}

int PageCtrl::GetValue() {
    TransferDataFromWindow();
    return m_value;
}

void PageCtrl::SetValue(int value) {
    m_value = value;
    TransferDataToWindow();
}

int PageCtrl::GetMaxPages() const {
    return dynamic_cast<wxIntegerValidator<int> *>(m_page_ctl->GetValidator())->GetMax();
}

void PageCtrl::SetMaxPages(int value) {
    m_numpages->SetLabel(wxString::Format("/ %d", value));
    wxIntegerValidator<int> val(&m_value);
    val.SetRange(1, value);
    m_page_ctl->SetValidator(val);
    SetValue(1);
}

void PageCtrl::OnPageEnter(wxCommandEvent &evt) {
    sendPageSelectedEvent();
}

void PageCtrl::OnPrevPage(wxCommandEvent &evt) {
    if (m_value > 1) {
        SetValue(m_value-1);
        sendPageSelectedEvent();
    } else {
        wxBell();
    }
}

void PageCtrl::OnNextPage(wxCommandEvent &evt) {
    if (m_value < GetMaxPages()) {
        SetValue(m_value+1);
        sendPageSelectedEvent();
    } else {
        wxBell();
    }
}

void PageCtrl::sendPageSelectedEvent() {
    wxCommandEvent event(EVT_PAGE_SELECTED, GetId());
    event.SetEventObject(this);
    event.SetInt(GetValue());
    ProcessWindowEvent(event);
}