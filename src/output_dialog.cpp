#include "output_dialog.h"

#include <wx/statline.h>
#include <wx/filename.h>

#include "resources.h"
#include "editor.h"

#include "parser.h"
#include "reader.h"
#include "utils.h"

enum {
    CTL_DEBUG,
    CTL_GLOBALS,
    CTL_OUTPUT_PAGE,
    TOOL_UPDATE,
    TOOL_ABORT,
};

wxDEFINE_EVENT(wxEVT_COMMAND_READ_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_LAYOUT_ERROR, wxThreadEvent);

BEGIN_EVENT_TABLE(output_dialog, wxDialog)
    EVT_MENU(TOOL_UPDATE, output_dialog::OnClickUpdate)
    EVT_CHECKBOX(CTL_DEBUG, output_dialog::OnUpdate)
    EVT_CHECKBOX(CTL_GLOBALS, output_dialog::OnUpdate)
    EVT_COMMAND(CTL_OUTPUT_PAGE, EVT_PAGE_SELECTED, output_dialog::OnUpdate)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_READ_COMPLETE, output_dialog::OnReadCompleted)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_LAYOUT_ERROR, output_dialog::OnLayoutError)
END_EVENT_TABLE()

DECLARE_RESOURCE(tool_reload_png)
DECLARE_RESOURCE(tool_abort_png)

output_dialog::output_dialog(frame_editor *parent) :
    wxDialog(parent, wxID_ANY, "Lettura Dati", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    parent(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    m_toolbar = new wxToolBar(this, wxID_ANY);

    m_toolbar->AddTool(TOOL_UPDATE, "Aggiorna", loadPNG(tool_reload_png));

    m_toolbar->AddStretchableSpace();

    m_show_debug = new wxCheckBox(m_toolbar, CTL_DEBUG, "Debug");
    m_toolbar->AddControl(m_show_debug, "Debug");

    m_show_globals = new wxCheckBox(m_toolbar, CTL_GLOBALS, "Globali");
    m_toolbar->AddControl(m_show_globals, "Globali");

    m_page = new PageCtrl(m_toolbar, CTL_OUTPUT_PAGE);

    m_toolbar->AddControl(m_page, "Pagina");

    m_toolbar->Realize();
    sizer->Add(m_toolbar, wxSizerFlags().Expand());

    m_list_ctrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(320, 450), wxLC_REPORT);

    sizer->Add(m_list_ctrl, wxSizerFlags(1).Expand());

    SetSizerAndFit(sizer);

    error_dialog = new TextDialog(this, "Errore di Layout");
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
        parser my_parser;
        std::filesystem::path path = parent->parent->m_filename;
        if (path.empty()) path = std::filesystem::path(parent->parent->getControlScript().ToStdString());
        my_parser.read_layout(path, m_layout);
        m_reader.add_code(std::move(my_parser).get_bytecode());
        m_reader.start();
        wxQueueEvent(parent, new wxThreadEvent(wxEVT_COMMAND_READ_COMPLETE));

        if (!m_reader.get_notes().empty()) {
            auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
            std::string notes = util::string_join(m_reader.get_notes(), "\n\n");
            evt->SetString(wxString::FromUTF8(notes.data(), notes.size()));
            evt->SetInt(0);
            wxQueueEvent(parent, evt);
        }
        return (wxThread::ExitCode) 0;
    } catch (const layout_runtime_error &error) {
        auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
        evt->SetString(wxString::FromUTF8(error.what()));
        evt->SetInt(error.errcode);
        wxQueueEvent(parent, evt);
    } catch (const std::exception &error) {
        auto *evt = new wxThreadEvent(wxEVT_COMMAND_LAYOUT_ERROR);
        evt->SetString(wxString::FromUTF8(error.what()));
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

        m_reader.clear();
        m_reader.set_document(parent->getPdfDocument());
        m_thread = new reader_thread(this, m_reader, parent->layout);
        if (m_thread->Run() != wxTHREAD_NO_ERROR) {
            delete m_thread;
            m_thread = nullptr;
        }

        m_page->SetMaxPages(1);
        m_list_ctrl->ClearAll();
    }
}

void output_dialog::OnLayoutError(wxCommandEvent &evt) {
    m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_reload_png));

    int errcode = evt.GetInt();
    if (errcode == 0) {
        error_dialog->SetTitle("Note");
    } else if (errcode == -1) {
        error_dialog->SetTitle("Errore Fatale");
    } else {
        error_dialog->SetTitle(wxString::Format("Errore di Layout (Codice %d)", errcode));
    }
    error_dialog->ShowText(evt.GetString());
}

void output_dialog::OnReadCompleted(wxCommandEvent &evt) {
    m_toolbar->SetToolNormalBitmap(TOOL_UPDATE, loadPNG(tool_reload_png));
    m_page->SetMaxPages(m_reader.get_values().size());
    updateItems();
}

void output_dialog::OnUpdate(wxCommandEvent &evt) {
    if (m_thread) return;
    updateItems();
}

void output_dialog::updateItems() {
    m_list_ctrl->ClearAll();

    auto col_name = m_list_ctrl->AppendColumn("Nome", wxLIST_FORMAT_LEFT, 150);
    auto col_value = m_list_ctrl->AppendColumn("Valore", wxLIST_FORMAT_LEFT, 150);

    auto display_table = [&](const variable_map &table) {
        size_t n=0;
        for (const auto &[key, var] : table) {
            if (!m_show_debug->GetValue() && key.front() == '_') {
                continue;
            }
            wxListItem item;
            item.SetId(n);
            m_list_ctrl->InsertItem(item);
            m_list_ctrl->SetItem(n, col_name, key);
            auto view = var.as_view();
            m_list_ctrl->SetItem(n, col_value, wxString::FromUTF8(view.data(), view.size()));
            ++n;
        }
    };

    if (m_show_globals->GetValue()) {
        display_table(m_reader.get_globals());
    } else {
        display_table(*std::next(m_reader.get_values().begin(), m_page->GetValue() - 1));
    }
}