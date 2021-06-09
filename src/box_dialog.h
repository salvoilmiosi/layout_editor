#ifndef __BOX_DIALOG_H__
#define __BOX_DIALOG_H__

#include <wx/dialog.h>

#include "text_dialog.h"
#include "layout.h"

class box_dialog : public wxDialog {
public:
    static box_dialog *openDialog(class frame_editor *app, layout_box &box);
    static bool closeDialog(layout_box &box);
    static bool closeAll();

private:
    box_dialog(class frame_editor *parent, layout_box &box);

private:
    void saveBox();

    void OnApply(wxCommandEvent &evt);
    void OnOK(wxCommandEvent &evt);
    void OnCancel(wxCommandEvent &evt);
    void OnTest(wxCommandEvent &evt);
    void OnClose(wxCloseEvent &evt);

private:
    static std::map<layout_box *, box_dialog *> open_dialogs;

    layout_box &m_box;

    class frame_editor *app;
    TextDialog *reader_output;

    DECLARE_EVENT_TABLE()
};


#endif