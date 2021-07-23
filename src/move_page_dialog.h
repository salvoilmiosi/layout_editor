#ifndef __MOVE_PAGE_DIALOG_H__
#define __MOVE_PAGE_DIALOG_H__

#include <wx/dialog.h>

#include "layout.h"
#include "editor.h"
#include "page_ctl.h"

class MovePageDialog : public wxDialog {
public:
    MovePageDialog(frame_editor *app, bls::layout_box *box);

private:
    frame_editor *m_app;
    bls::layout_box *m_box;
    PageCtrl *m_page;
    int origpage;

    void OnPageSelect(wxCommandEvent &evt);
    void OnOK(wxCommandEvent &evt);
    void OnCancel(wxCommandEvent &evt);

    DECLARE_EVENT_TABLE()
};

#endif