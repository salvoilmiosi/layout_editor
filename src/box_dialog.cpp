#include "box_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>

#include "editor.h"
#include "utils/utils.h"

template<enums::reflected_enum T>
class RadioGroupValidator : public wxValidator {
protected:
    T m_radio;
    T *m_value;

public:
    RadioGroupValidator(T n, T &m) : m_radio(n), m_value(&m) {}
    
    virtual wxObject *Clone() const override {
        return new RadioGroupValidator(*this);
    }

    virtual bool TransferFromWindow() override {
        if (m_value) {
            wxRadioButton *radio_btn = dynamic_cast<wxRadioButton*>(GetWindow());
            if (radio_btn->GetValue()) {
                *m_value = m_radio;
            }
        }
        return true;
    }

    virtual bool TransferToWindow() override {
        if (m_value) {
            wxRadioButton *radio_btn = dynamic_cast<wxRadioButton*>(GetWindow());
            if (*m_value == m_radio) {
                radio_btn->SetValue(true);
            }
        }
        return true;
    }
};

template<enums::flags_enum T>
class FlagValidator : public wxValidator {
protected:
    T m_flag;
    enums::bitset<T> *m_value;

public:
    FlagValidator(T n, enums::bitset<T> &m) : m_flag(n), m_value(&m) {}

    virtual wxObject *Clone() const override {
        return new FlagValidator(*this);
    }

    virtual bool TransferFromWindow() override {
        if (m_value) {
            wxCheckBox *check_box = dynamic_cast<wxCheckBox*>(GetWindow());
            m_value->unset(m_flag);
            if (check_box->GetValue()) {
                m_value->set(m_flag);
            }
        }
        return true;
    }

    virtual bool TransferToWindow() override {
        if (m_value) {
            wxCheckBox *check_box = dynamic_cast<wxCheckBox*>(GetWindow());
            check_box->SetValue(m_value->check(m_flag));
        }
        return true;
    }
};

template<typename WindowType = wxTextCtrl>
class StringValidator : public wxValidator {
protected:
    std::string *m_value;

public:
    StringValidator(std::string *value) : m_value(value) {}

    virtual wxObject *Clone() const override {
        return new StringValidator(*this);
    }

    virtual bool TransferFromWindow() override {
        if (m_value) {
            WindowType *ctl = dynamic_cast<WindowType*>(GetWindow());
            *m_value = ctl->GetValue().ToStdString();
        }
        return true;
    }

    virtual bool TransferToWindow() override {
        if (m_value) {
            WindowType *ctl = dynamic_cast<WindowType*>(GetWindow());
            ctl->SetValue(wxintl::to_wx(*m_value));
        }
        return true;
    }
};

enum {
    BUTTON_TEST = 1001
};

box_dialog::box_dialog(frame_editor *parent, layout_box &out_box) :
    wxDialog(parent, wxID_ANY, wxintl::translate("EDIT_BOX"), wxDefaultPosition, wxSize(700, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_box(out_box), app(parent)
{
    wxBoxSizer *top_level = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    auto addLabelAndCtrl = [&](const wxString &labelText, int vprop, int hprop, auto* ... ctrls) {
        wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);

        wxStaticText *label = new wxStaticText(this, wxID_ANY, labelText, wxDefaultPosition, wxSize(60, -1), wxALIGN_RIGHT);
        hsizer->Add(label, wxSizerFlags().Center().Border(wxALL, 5));

        (hsizer->Add(ctrls, wxSizerFlags(hprop).Expand().Border(wxLEFT, 5)), ...);

        sizer->Add(hsizer, wxSizerFlags(vprop).Expand().Border(wxALL, 5));
    };
    
    addLabelAndCtrl(wxintl::translate("BOX_NAME"), 0, 1, new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, StringValidator(&m_box.name)));

    auto add_radio_btns = [&]<enums::reflected_enum Enum>(const wxString &label, Enum &value) {
        [&] <Enum ... Es> (enums::enum_sequence<Es...>) {
            auto create_btn = [&, first=true](Enum i) mutable {
                auto ret = new wxRadioButton(this, wxID_ANY, wxintl::enum_label(i),
                    wxDefaultPosition, wxDefaultSize, first ? wxRB_GROUP : 0,
                    RadioGroupValidator(i, value));
                first = false;
                return ret;
            };
            addLabelAndCtrl(label, 0, 0, create_btn(Es) ...);
        }(enums::make_enum_sequence<Enum>());
    };

    auto add_check_boxes = [&] <enums::flags_enum Enum>(const wxString &label, enums::bitset<Enum> &value) {
        [&] <Enum ... Es> (enums::enum_sequence<Es...>) {
            auto create_btn = [&](Enum flag){
                return new wxCheckBox(this, wxID_ANY, wxintl::enum_label(flag), wxDefaultPosition, wxDefaultSize, 0, FlagValidator(flag, value));
            };
            addLabelAndCtrl(label, 0, 0, create_btn(Es) ...);
        }(enums::make_enum_sequence<Enum>());
    };

    add_radio_btns(wxintl::translate("BOX_MODE"), m_box.mode);
    add_check_boxes(wxintl::translate("BOX_FLAGS"), m_box.flags);

    auto make_script_box = [&](std::string &value) {
        wxStyledTextCtrl *text = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        text->StyleSetFont(0, font);
        text->SetEOLMode(wxSTC_EOL_LF);
        text->SetValidator(StringValidator<wxStyledTextCtrl>(&value));
        return text;
    };

    addLabelAndCtrl(wxintl::translate("BOX_GOTO_LABEL"), 0, 1, new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, StringValidator(&m_box.goto_label)));
    addLabelAndCtrl(wxintl::translate("BOX_SPACERS"), 1, 1, make_script_box(m_box.spacers));
    addLabelAndCtrl(wxintl::translate("BOX_SCRIPT"), 3, 1, make_script_box(m_box.script));

    top_level->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

    wxStaticLine *line = new wxStaticLine(this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    top_level->Add(line, wxSizerFlags().Expand().Border(wxALL, 5));

    wxBoxSizer *okCancelSizer = new wxBoxSizer(wxHORIZONTAL);

    okCancelSizer->Add(new wxButton(this, wxID_OK, wxintl::translate("OK")), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, wxID_CANCEL, wxintl::translate("Cancel")), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, wxID_APPLY, wxintl::translate("Apply")), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, BUTTON_TEST, wxintl::translate("Test")), wxSizerFlags().Center().Border(wxALL, 5));

    top_level->Add(okCancelSizer, wxSizerFlags().Center().Border(wxALL, 5));

    SetSizer(top_level);
    Show();

    reader_output = new TextDialog(this, wxintl::translate("TEST_OUTPUT"));
}

std::map<layout_box *, box_dialog *> box_dialog::open_dialogs;

box_dialog *box_dialog::openDialog(frame_editor *parent, layout_box &out_box) {
    auto it = open_dialogs.find(&out_box);
    if (it != open_dialogs.end()) {
        it->second->Raise();
        return it->second;
    } else {
        return open_dialogs[&out_box] = new box_dialog(parent, out_box);
    }
}

bool box_dialog::closeDialog(layout_box &out_box) {
    auto it = open_dialogs.find(&out_box);
    if (it != open_dialogs.end()) {
        it->second->Raise();
        return it->second->Close();
    }
    return true;
}

bool box_dialog::closeAll() {
    bool ret = true;
    auto open_dialogs_copy = open_dialogs;
    for (auto &dialog : open_dialogs_copy | std::views::values) {
        if (!dialog->Close()) ret = false;
    }
    return ret;
}

BEGIN_EVENT_TABLE(box_dialog, wxDialog)
    EVT_BUTTON(wxID_APPLY, box_dialog::OnApply)
    EVT_BUTTON(wxID_OK, box_dialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, box_dialog::OnCancel)
    EVT_BUTTON(BUTTON_TEST, box_dialog::OnTest)
    EVT_CLOSE(box_dialog::OnClose)
END_EVENT_TABLE()

void box_dialog::saveBox() {
    TransferDataFromWindow();
    app->updateLayout();
    app->selectBox(&m_box);
}

void box_dialog::OnApply(wxCommandEvent &evt) {
    saveBox();
}

void box_dialog::OnOK(wxCommandEvent &evt) {
    saveBox();
    Close();
}

void box_dialog::OnCancel(wxCommandEvent &evt) {
    Close();
}

void box_dialog::OnTest(wxCommandEvent &evt) {
    auto box_copy = m_box;
    TransferDataFromWindow();
    m_box.rotate(app->getBoxRotation());
    std::string text = app->getPdfDocument().get_text(m_box);
    reader_output->ShowText(wxintl::to_wx(app->getPdfDocument().get_text(m_box)));
    m_box = box_copy;
}

static bool operator == (const layout_box &a, const layout_box &b) {
    return a.name == b.name
        && a.script == b.script
        && a.spacers == b.spacers
        && a.goto_label == b.goto_label
        && a.mode == b.mode;
}

void box_dialog::OnClose(wxCloseEvent &evt) {
    auto box_copy = m_box;
    TransferDataFromWindow();
    if (box_copy != m_box) {
        wxMessageDialog dialog(this, wxintl::translate("SAVE_CHANGES_DIALOG"), wxintl::translate("PROGRAM_NAME"), wxYES_NO | wxCANCEL | wxICON_WARNING);

        switch (dialog.ShowModal()) {
        case wxID_YES:
            app->updateLayout();
            app->selectBox(&m_box);
            break;
        case wxID_NO:
            m_box = box_copy;
            break;
        case wxID_CANCEL:
            m_box = box_copy;
            evt.Veto();
            return;
        }
    }
    open_dialogs.erase(&m_box);
    Destroy();
}