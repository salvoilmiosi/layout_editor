#include "layout_options_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/msgdlg.h>

#include "wxintl.h"

BEGIN_EVENT_TABLE(LayoutOptionsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, LayoutOptionsDialog::OnOK)
END_EVENT_TABLE()

LayoutOptionsDialog::LayoutOptionsDialog(wxWindow *parent, layout_box_list *layout) :
    wxDialog(parent, wxID_ANY, wxintl::translate("LAYOUT_OPTIONS_DIALOG_TITLE"), wxDefaultPosition, wxSize(400, 500)),
    m_layout(layout)
{
    m_find_layout_box = new wxCheckBox(this, wxID_ANY, wxintl::translate("FIND_LAYOUT_CHECKBOX"));
    m_find_layout_box->SetValue(m_layout->find_layout_flag);

    wxBoxSizer *top_level = new wxBoxSizer(wxVERTICAL);

    top_level->Add(m_find_layout_box, wxSizerFlags().Expand().Border(wxALL, 5));

    m_language_box = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    m_language_box->Append(wxintl::translate("SYSTEM_LANGUAGE"));
    size_t selection = 0;
    for (int lang = 2; lang != wxLANGUAGE_USER_DEFINED; ++lang) {
        if (!wxLocale::IsAvailable(lang)) continue;
        auto lang_str = wxLocale::GetLanguageCanonicalName(lang).ToStdString();
        if (lang_str.empty()) continue;
        auto *language_id_ptr = &m_language_ids.emplace_back(lang_str);
        if (lang_str == layout->language) {
            selection = m_language_ids.size();
        }
        m_language_box->Append(wxLocale::GetLanguageName(lang), language_id_ptr);
    }
    m_language_box->SetSelection(selection);

    top_level->Add(m_language_box, wxSizerFlags().Expand().Border(wxALL, 5));

    wxBoxSizer *ok_cancel_sizer = new wxBoxSizer(wxHORIZONTAL);

    ok_cancel_sizer->Add(new wxButton(this, wxID_OK, wxintl::translate("OK"), wxDefaultPosition, wxSize(100, -1)), wxSizerFlags().Expand().Border(wxALL, 5));
    ok_cancel_sizer->Add(new wxButton(this, wxID_CANCEL, wxintl::translate("Cancel"), wxDefaultPosition, wxSize(100, -1)), wxSizerFlags().Expand().Border(wxALL, 5));

    top_level->Add(ok_cancel_sizer, wxSizerFlags().Center());

    SetSizerAndFit(top_level);
}

void LayoutOptionsDialog::OnOK(wxCommandEvent &evt) {
    m_layout->find_layout_flag = m_find_layout_box->GetValue();
    auto *data = m_language_box->GetClientData(m_language_box->GetSelection());
    if (data) {
        m_layout->language = *static_cast<std::string *>(data);
    } else {
        m_layout->language.clear();
    }
    evt.Skip();
}