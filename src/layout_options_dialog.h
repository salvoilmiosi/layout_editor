#ifndef __LAYOUT_OPTIONS_DIALOG_H__
#define __LAYOUT_OPTIONS_DIALOG_H__

#include <wx/dialog.h>
#include <wx/choice.h>
#include <wx/checkbox.h>

#include "layout.h"

using namespace bls;

class LayoutOptionsDialog : public wxDialog {
public:
    LayoutOptionsDialog(wxWindow *parent, layout_box_list *m_layout);

    void OnOK(wxCommandEvent &evt);

private:
    layout_box_list *m_layout;

    wxCheckBox *m_find_layout_box;

    wxChoice *m_language_box;

    std::list<std::string> m_language_ids;

    DECLARE_EVENT_TABLE()
};

#endif