#include "output_dialog.h"

#include <wx/statline.h>
#include <wx/filename.h>
#include <wx/filefn.h>

#include "resources.h"
#include "editor.h"

#include "parser.h"
#include "reader.h"

#include "utils/utils.h"

enum {
    CTL_DEBUG,
    CTL_OUTPUT_PAGE,
    TOOL_UPDATE,
    TOOL_ABORT,
};

wxDEFINE_EVENT(wxEVT_COMMAND_READ_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_LAYOUT_ERROR, wxThreadEvent);

BEGIN_EVENT_TABLE(output_dialog, wxDialog)
    EVT_MENU(TOOL_UPDATE, output_dialog::OnClickUpdate)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_READ_COMPLETE, output_dialog::OnReadCompleted)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_LAYOUT_ERROR, output_dialog::OnLayoutError)
END_EVENT_TABLE()

DECLARE_RESOURCE(tool_reload_png)
DECLARE_RESOURCE(tool_abort_png)

output_dialog::output_dialog(frame_editor *parent) :
    wxDialog(parent, wxID_ANY, wxintl::translate("READER_DATA_OUTPUT"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    parent(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    m_toolbar = new wxToolBar(this, wxID_ANY);

    m_toolbar->AddTool(TOOL_UPDATE, wxintl::translate("TOOL_UPDATE"), loadPNG(tool_reload_png), wxintl::translate("TOOL_UPDATE"));

    m_toolbar->Realize();
    sizer->Add(m_toolbar, wxSizerFlags().Expand());

    m_display = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(500, 500));
    m_display->AppendTextColumn(wxintl::translate("VARIABLE_NAME"), 0, wxDATAVIEW_CELL_INERT, 150, wxALIGN_LEFT);
    m_display->AppendTextColumn(wxintl::translate("VARIABLE_TYPE"), 1, wxDATAVIEW_CELL_INERT, 100, wxALIGN_LEFT);
    m_display->AppendTextColumn(wxintl::translate("VARIABLE_VALUE"), 2, wxDATAVIEW_CELL_INERT, 150, wxALIGN_LEFT);

    m_model = new VariableTableModel;
    m_display->AssociateModel(m_model.get());

    sizer->Add(m_display, wxSizerFlags(1).Expand());

    SetSizerAndFit(sizer);

    error_dialog = new TextDialog(this, wxintl::translate("LAYOUT_ERROR"));
}

void output_dialog::OnClickUpdate(wxCommandEvent &) {
    if (m_thread) {
        m_reader.abort();
        m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_reload_png));
    } else {
        compileAndRead();
    }
}

reader_thread::reader_thread(output_dialog *parent, reader &m_reader, const layout_box_list &layout)
    : parent(parent), m_reader(m_reader), m_layout(layout) {}

reader_thread::~reader_thread() {
    parent->m_thread = nullptr;
}

wxThread::ExitCode reader_thread::Entry() {
    try {
        if (m_layout.filename.empty()) m_layout.filename = std::filesystem::path(wxGetCwd().ToStdString()) / "tmp.bls";
        m_reader.add_layout(m_layout);
        m_reader.start();
        wxQueueEvent(parent, new wxThreadEvent(wxEVT_COMMAND_READ_COMPLETE));

        if (!m_reader.get_notes().empty()) {
            auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
            evt->SetString(wxintl::to_wx(util::string_join(m_reader.get_notes(), "\n\n")));
            evt->SetInt(0);
            wxQueueEvent(parent, evt);
        }
        return (wxThread::ExitCode) 0;
    } catch (const scripted_error &error) {
        auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
        evt->SetString(wxintl::to_wx(error.what()));
        evt->SetInt(error.errcode);
        wxQueueEvent(parent, evt);
    } catch (const std::exception &error) {
        auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
        evt->SetString(wxintl::to_wx(error.what()));
        evt->SetInt(-1);
        wxQueueEvent(parent, evt);
    } catch (reader_aborted) {
        // ignore output
    }
    return (wxThread::ExitCode) 1;
}

void output_dialog::compileAndRead() {
    if (m_thread || ! parent->getPdfDocument().isopen()) {
        wxBell();
    } else {
        m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_abort_png));
        m_model->ClearTables();

        m_reader.clear();
        m_reader.set_document(parent->getPdfDocument());
        m_thread = new reader_thread(this, m_reader, parent->layout);
        if (m_thread->Run() != wxTHREAD_NO_ERROR) {
            delete m_thread;
            m_thread = nullptr;
        }
    }
}

void output_dialog::OnLayoutError(wxCommandEvent &evt) {
    m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_reload_png));

    int errcode = evt.GetInt();
    if (errcode == 0) {
        error_dialog->SetTitle(wxintl::translate("LAYOUT_NOTES"));
    } else if (errcode == -1) {
        error_dialog->SetTitle(wxintl::translate("LAYOUT_FATAL_ERROR"));
    } else {
        error_dialog->SetTitle(wxintl::translate("LAYOUT_ERROR_CODE", errcode));
    }
    error_dialog->ShowText(evt.GetString());
}

void output_dialog::OnReadCompleted(wxCommandEvent &evt) {
    m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_reload_png));

    int i=1;
    for (const variable_map &table : m_reader.get_values()) {
        m_model->AddTable(wxintl::translate("TABLE_NUMBER", i++), table);
    }
}