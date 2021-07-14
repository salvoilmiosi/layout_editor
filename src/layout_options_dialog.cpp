#include "layout_options_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(LayoutOptionsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, LayoutOptionsDialog::OnOK)
END_EVENT_TABLE()

LayoutOptionsDialog::LayoutOptionsDialog(wxWindow *parent, layout_box_list *layout) :
    wxDialog(parent, wxID_ANY, "Cambia Opzioni Di Layout", wxDefaultPosition, wxSize(400, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_layout(layout)
{
    m_setlayout_box = new wxCheckBox(this, wxID_ANY, "Carica in Auto Layout");
    m_setlayout_box->SetValue(m_layout->setlayout);

    wxBoxSizer *top_level = new wxBoxSizer(wxVERTICAL);

    top_level->Add(m_setlayout_box, 0, wxEXPAND | wxALL, 5);

    m_language_box = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    auto *language_id_ptr = m_language_ids;
    m_language_box->Append("Lingua di sistema");
    size_t selection = 0;
    for (int lang = 2; lang != wxLANGUAGE_USER_DEFINED; ++lang) {
        if (!wxLocale::IsAvailable(lang)) continue;
        auto lang_str = wxLocale::GetLanguageCanonicalName(lang).ToStdString();
        if (lang_str.empty()) continue;
        *language_id_ptr = lang_str;
        if (lang_str == layout->language) {
            selection = language_id_ptr - m_language_ids + 1;
        }
        m_language_box->Append(wxLocale::GetLanguageName(lang), language_id_ptr++);
    }
    m_language_box->SetSelection(selection);

    top_level->Add(m_language_box, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer *ok_cancel_sizer = new wxBoxSizer(wxHORIZONTAL);

    ok_cancel_sizer->Add(new wxButton(this, wxID_OK, "OK", wxDefaultPosition, wxSize(100, -1)), 0, wxALL, 5);
    ok_cancel_sizer->Add(new wxButton(this, wxID_CANCEL, "Annulla", wxDefaultPosition, wxSize(100, -1)), 0, wxALL, 5);

    top_level->Add(ok_cancel_sizer, 0, wxALIGN_CENTER_HORIZONTAL);

    SetSizerAndFit(top_level);
}

void LayoutOptionsDialog::OnOK(wxCommandEvent &evt) {
    m_layout->setlayout = m_setlayout_box->GetValue();
    auto *data = m_language_box->GetClientData(m_language_box->GetSelection());
    if (data) {
        m_layout->language = *static_cast<std::string *>(data);
    } else {
        m_layout->language.clear();
    }
    evt.Skip();
}