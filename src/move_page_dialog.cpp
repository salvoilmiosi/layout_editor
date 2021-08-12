#include "move_page_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/button.h>

enum {
    CTL_PAGE = 10000
};

BEGIN_EVENT_TABLE(MovePageDialog, wxDialog)
    EVT_COMMAND(CTL_PAGE, EVT_PAGE_SELECTED, MovePageDialog::OnPageSelect)
    EVT_BUTTON(wxID_OK, MovePageDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, MovePageDialog::OnCancel)
END_EVENT_TABLE()

MovePageDialog::MovePageDialog(frame_editor *app, bls::layout_box *box)
    : wxDialog(app, wxID_ANY, intl::wxformat("CHANGE_BOX_PAGE")), m_app(app), m_box(box)
{
    origpage = m_box->page;

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(this, wxID_ANY, intl::wxformat("BOX_PAGE_LABEL")), wxSizerFlags().Center().Border(wxALL, 5));

    m_page = new PageCtrl(this, CTL_PAGE, app->getPdfDocument().num_pages());
    m_page->SetValue(app->getSelectedPage());
    sizer->Add(m_page, wxSizerFlags().Center().Border(wxALL, 5));

    sizer->Add(new wxStaticLine(this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), wxSizerFlags().Expand().Border(wxALL, 5));

    wxBoxSizer *okcancel = new wxBoxSizer(wxHORIZONTAL);

    okcancel->Add(new wxButton(this, wxID_OK, intl::wxformat("OK")), wxSizerFlags().Center().Border(wxALL, 5));
    okcancel->Add(new wxButton(this, wxID_CANCEL, intl::wxformat("Cancel")), wxSizerFlags().Center().Border(wxALL, 5));

    sizer->Add(okcancel, wxSizerFlags().Center().Border(wxALL, 5));
    SetSizerAndFit(sizer);
}

void MovePageDialog::OnPageSelect(wxCommandEvent &evt) {
    m_box->page = m_page->GetValue();
    m_app->setSelectedPage(m_box->page);
}

void MovePageDialog::OnOK(wxCommandEvent &evt) {
    m_app->updateLayout(true);
    evt.Skip();
}

void MovePageDialog::OnCancel(wxCommandEvent &evt) {
    m_box->page = origpage;
    m_app->setSelectedPage(origpage);
    evt.Skip();
}