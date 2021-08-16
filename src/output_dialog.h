#ifndef __OUTPUT_DIALOG_H__
#define __OUTPUT_DIALOG_H__

#include <wx/dialog.h>
#include <wx/combobox.h>
#include <wx/thread.h>

#include "editor.h"
#include "reader.h"
#include "text_dialog.h"

#include "variable_table_model.h"

using namespace bls;

class output_dialog;

class reader_thread : public wxThread {
public:
    reader_thread(output_dialog *parent, reader &m_reader, const layout_box_list &layout);
    ~reader_thread();

protected:
    virtual ExitCode Entry();

private:
    reader &m_reader;
    output_dialog *parent;

    layout_box_list m_layout;
};

class output_dialog : public wxDialog {
public:
    output_dialog(frame_editor *parent);
    void compileAndRead();

private:
    frame_editor *parent;

    wxToolBar *m_toolbar;

    wxDataViewCtrl *m_display;
    wxObjectDataPtr<VariableTableModel> m_model;

    reader_thread *m_thread = nullptr;
    reader m_reader;
    
    TextDialog *error_dialog;

    void OnClickUpdate(wxCommandEvent &evt);

    void OnReadCompleted(wxCommandEvent &evt);
    void OnLayoutError(wxCommandEvent &evt);

    DECLARE_EVENT_TABLE()

    friend class reader_thread;
};

#endif