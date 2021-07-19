#include "box_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>

#include "editor.h"
#include "utils.h"
template<typename T>
class RadioGroupValidator : public wxValidator {
protected:
    T m_radio;
    T *m_value;

public:
    RadioGroupValidator(int n, T &m) : m_radio(static_cast<T>(n)), m_value(&m) {}
    
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
            ctl->SetValue(wxString::FromUTF8(m_value->data(), m_value->size()));
        }
        return true;
    }
};

enum {
    BUTTON_TEST = 1001
};

template<typename T>
static inline const char *get_label(const T &data) {
    if constexpr (std::is_same_v<T, const char *>) {
        return data;
    } else {
        return std::get<const char *>(data);
    }
}

box_dialog::box_dialog(frame_editor *parent, layout_box &out_box) :
    wxDialog(parent, wxID_ANY, "Modifica Rettangolo", wxDefaultPosition, wxSize(700, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_box(out_box), app(parent)
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
    
    addLabelAndCtrl("Nome:", 0, 1, new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, StringValidator(&m_box.name)));

    auto add_radio_btns = [&](const wxString &label, auto &value) {
        using enum_type = std::decay_t<decltype(value)>;
        bool first = true;
        auto create_btn = [&](size_t i) {
            auto ret = new wxRadioButton(this, wxID_ANY, get_label(enums::get_data(static_cast<enum_type>(i))),
                wxDefaultPosition, wxDefaultSize, first ? wxRB_GROUP : 0,
                RadioGroupValidator(i, value));
            first = false;
            return ret;
        };
        [&] <size_t ... Is> (std::index_sequence<Is...>) {
            addLabelAndCtrl(label, 0, 0, create_btn(Is) ...);
        }(std::make_index_sequence<enums::size<enum_type>()>{});
    };

    auto add_check_boxes = [&] <enums::flags_enum enum_type>(const wxString &label, enums::bitset<enum_type> &value) {
        auto create_btn = [&](size_t i) {
            const auto flag = static_cast<enum_type>(1 << i);
            return new wxCheckBox(this, wxID_ANY, get_label(enums::get_data(flag)), wxDefaultPosition, wxDefaultSize, 0, FlagValidator(flag, value));  
        };
        [&] <size_t ... Is> (std::index_sequence<Is...>) {
            addLabelAndCtrl(label, 0, 0, create_btn(Is) ...);
        }(std::make_index_sequence<enums::size<enum_type>()>{});
    };

    add_radio_btns(L"Modalità:", m_box.mode);
    add_check_boxes("Flag:", m_box.flags);

    auto make_script_box = [&](std::string &value) {
        wxStyledTextCtrl *text = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        text->StyleSetFont(0, font);
        text->SetEOLMode(wxSTC_EOL_LF);
        text->SetValidator(StringValidator<wxStyledTextCtrl>(&value));
        return text;
    };

    addLabelAndCtrl("Label goto:", 0, 1, new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, StringValidator(&m_box.goto_label)));
    addLabelAndCtrl("Spaziatori:", 1, 1, make_script_box(m_box.spacers));
    addLabelAndCtrl("Script:", 3, 1, make_script_box(m_box.script));

    top_level->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

    wxStaticLine *line = new wxStaticLine(this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    top_level->Add(line, wxSizerFlags().Expand().Border(wxALL, 5));

    wxBoxSizer *okCancelSizer = new wxBoxSizer(wxHORIZONTAL);

    okCancelSizer->Add(new wxButton(this, wxID_OK, "OK"), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, wxID_CANCEL, "Annulla"), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, wxID_APPLY, "Applica"), wxSizerFlags().Center().Border(wxALL, 5));
    okCancelSizer->Add(new wxButton(this, BUTTON_TEST, "Test"), wxSizerFlags().Center().Border(wxALL, 5));

    top_level->Add(okCancelSizer, wxSizerFlags().Center().Border(wxALL, 5));

    SetSizer(top_level);
    Show();

    reader_output = new TextDialog(this, "Risultato Lettura");
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
    reader_output->ShowText(wxString::FromUTF8(text.data(), text.size()));
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
        wxMessageDialog dialog(this, "Salvare le modifiche?", "Layout Bolletta", wxYES_NO | wxCANCEL | wxICON_WARNING);

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