#include "editor.h"

#include <fstream>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/artprov.h>
#include <wx/splitter.h>

#include "resources.h"
#include "box_editor_panel.h"
#include "pdf_to_image.h"
#include "box_dialog.h"

enum {
    MENU_NEW = 10000, MENU_OPEN, MENU_SAVE, MENU_SAVEAS, MENU_CLOSE,
    MENU_UNDO, MENU_REDO, MENU_CUT, MENU_COPY, MENU_PASTE,
    MENU_LOAD_PDF, MENU_EDITBOX, MENU_DELETE, MENU_READDATA,
    MENU_EDITCONTROL, MENU_OPEN_LAYOUT_OPTIONS,

    MENU_OPEN_RECENT,
    MENU_OPEN_RECENT_END = MENU_OPEN_RECENT + MAX_RECENT_FILES_HISTORY,
    
    MENU_OPEN_PDF_RECENT,
    MENU_OPEN_PDF_RECENT_END = MENU_OPEN_PDF_RECENT + MAX_RECENT_PDFS_HISTORY,

    CTL_ROTATE, CTL_LOAD_PDF, CTL_AUTO_LAYOUT, CTL_PAGE, CTL_SCALE,

    TOOL_MOVEUP, TOOL_MOVEDOWN,
    
    CTL_LIST_BOXES,
};

BEGIN_EVENT_TABLE(frame_editor, wxFrame)
    EVT_MENU (MENU_NEW, frame_editor::OnNewFile)
    EVT_MENU (MENU_OPEN, frame_editor::OnOpenFile)
    EVT_MENU (MENU_SAVE, frame_editor::OnSaveFile)
    EVT_MENU (MENU_SAVEAS, frame_editor::OnSaveFileAs)
    EVT_MENU (MENU_CLOSE, frame_editor::OnMenuClose)
    EVT_MENU (MENU_UNDO, frame_editor::OnUndo)
    EVT_MENU (MENU_REDO, frame_editor::OnRedo)
    EVT_MENU (MENU_CUT, frame_editor::OnCut)
    EVT_MENU (MENU_COPY, frame_editor::OnCopy)
    EVT_MENU (MENU_PASTE, frame_editor::OnPaste)
    EVT_MENU (MENU_LOAD_PDF, frame_editor::OnLoadPdf)
    EVT_MENU_RANGE (MENU_OPEN_RECENT, MENU_OPEN_RECENT_END, frame_editor::OnOpenRecent)
    EVT_MENU_RANGE (MENU_OPEN_PDF_RECENT, MENU_OPEN_PDF_RECENT_END, frame_editor::OnOpenRecentPdf)
    EVT_MENU (MENU_EDITBOX, frame_editor::EditSelectedBox)
    EVT_MENU (MENU_DELETE, frame_editor::OnDelete)
    EVT_MENU (MENU_READDATA, frame_editor::OnReadData)
    EVT_MENU (MENU_EDITCONTROL, frame_editor::OpenControlScript)
    EVT_MENU (MENU_OPEN_LAYOUT_OPTIONS, frame_editor::OnOpenLayoutOptions)
    EVT_TOOL (CTL_AUTO_LAYOUT, frame_editor::OnAutoLayout)
    EVT_TOOL (CTL_ROTATE, frame_editor::OnRotate)
    EVT_TOOL (CTL_LOAD_PDF, frame_editor::OnLoadPdf)
    EVT_COMMAND(CTL_PAGE, EVT_PAGE_SELECTED, frame_editor::OnPageSelect)
    EVT_COMMAND_SCROLL_THUMBTRACK (CTL_SCALE, frame_editor::OnScaleChange)
    EVT_COMMAND_SCROLL_CHANGED (CTL_SCALE, frame_editor::OnScaleChangeFinal)
    EVT_TOOL (TOOL_SELECT, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_NEWBOX, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_DELETEBOX, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_RESIZE, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_TEST, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_MOVEPAGE, frame_editor::OnChangeTool)
    EVT_TOOL (TOOL_MOVEUP, frame_editor::OnMoveUp)
    EVT_TOOL (TOOL_MOVEDOWN, frame_editor::OnMoveDown)
    EVT_LISTBOX (CTL_LIST_BOXES, frame_editor::OnSelectBox)
    EVT_LISTBOX_DCLICK (CTL_LIST_BOXES, frame_editor::EditSelectedBox)
    EVT_CLOSE (frame_editor::OnFrameClose)
END_EVENT_TABLE()

DECLARE_RESOURCE(icon_editor_png)
DECLARE_RESOURCE(tool_select_png)
DECLARE_RESOURCE(tool_newbox_png)
DECLARE_RESOURCE(tool_deletebox_png)
DECLARE_RESOURCE(tool_resize_png)
DECLARE_RESOURCE(tool_test_png)
DECLARE_RESOURCE(tool_move_page_png)
DECLARE_RESOURCE(tool_rotate_png)
DECLARE_RESOURCE(tool_load_pdf_png)
DECLARE_RESOURCE(tool_auto_layout_png)
DECLARE_RESOURCE(tool_settings_png)

constexpr size_t MAX_HISTORY_SIZE = 20;

frame_editor::frame_editor() : wxFrame(nullptr, wxID_ANY, intl::wxformat("PROGRAM_NAME"), wxDefaultPosition, wxSize(900, 700)) {
    m_config = new wxConfig("BillLayoutScript");

    wxMenuBar *menuBar = new wxMenuBar();
    
    m_bls_history = new wxFileHistory(MAX_RECENT_FILES_HISTORY, MENU_OPEN_RECENT);
    m_bls_history->UseMenu(m_bls_history_menu = new wxMenu);

    m_pdf_history = new wxFileHistory(MAX_RECENT_PDFS_HISTORY, MENU_OPEN_PDF_RECENT);
    m_pdf_history->UseMenu(m_pdf_history_menu = new wxMenu);

    m_config->SetPath("/RecentFiles");
    m_bls_history->Load(*m_config);

    m_config->SetPath("/RecentPdfs");
    m_pdf_history->Load(*m_config);

    m_config->SetPath("/");

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(MENU_NEW, intl::wxformat("MENU_NEW"), intl::wxformat("MENU_NEW_HINT"));
    menuFile->Append(MENU_OPEN, intl::wxformat("MENU_OPEN"), intl::wxformat("MENU_OPEN_HINT"));
    menuFile->AppendSubMenu(m_bls_history_menu, intl::wxformat("MENU_RECENT"));
    menuFile->Append(MENU_SAVE, intl::wxformat("MENU_SAVE"), intl::wxformat("MENU_SAVE_HINT"));
    menuFile->Append(MENU_SAVEAS, intl::wxformat("MENU_SAVEAS"), intl::wxformat("MENU_SAVEAS_HINT"));
    menuFile->AppendSeparator();
    menuFile->Append(MENU_LOAD_PDF, intl::wxformat("MENU_LOAD_PDF"), intl::wxformat("MENU_LOAD_PDF_HINT"));
    menuFile->AppendSubMenu(m_pdf_history_menu, intl::wxformat("MENU_RECENT_PDF"));
    menuFile->AppendSeparator();
    menuFile->Append(MENU_CLOSE, intl::wxformat("MENU_CLOSE"), intl::wxformat("MENU_CLOSE_HINT"));
    menuBar->Append(menuFile, intl::wxformat("MENU_FILE"));

    wxMenu *menuEdit = new wxMenu;
    menuEdit->Append(MENU_UNDO, intl::wxformat("MENU_UNDO"), intl::wxformat("MENU_UNDO_HINT"));
    menuEdit->Append(MENU_REDO, intl::wxformat("MENU_REDO"), intl::wxformat("MENU_REDO_HINT"));
    menuEdit->AppendSeparator();
    menuEdit->Append(MENU_CUT, intl::wxformat("MENU_CUT"), intl::wxformat("MENU_CUT_HINT"));
    menuEdit->Append(MENU_COPY, intl::wxformat("MENU_COPY"), intl::wxformat("MENU_COPY_HINT"));
    menuEdit->Append(MENU_PASTE, intl::wxformat("MENU_PASTE"), intl::wxformat("MENU_PASTE_HINT"));
    menuEdit->AppendSeparator();
    menuEdit->Append(MENU_DELETE, intl::wxformat("MENU_DELETE"), intl::wxformat("MENU_DELETE_HINT"));
    menuBar->Append(menuEdit, intl::wxformat("MENU_EDIT"));

    wxMenu *menuEditor = new wxMenu;
    menuEditor->Append(MENU_EDITCONTROL, intl::wxformat("MENU_EDITCONTROL"));

    menuBar->Append(menuEditor, intl::wxformat("MENU_EDITOR"));

    SetMenuBar(menuBar);

    wxToolBar *toolbar_top = CreateToolBar();

    toolbar_top->AddTool(MENU_NEW, intl::wxformat("TOOL_NEW"), wxArtProvider::GetBitmap(wxART_NEW), intl::wxformat("TOOL_NEW"));
    toolbar_top->AddTool(MENU_OPEN, intl::wxformat("TOOL_OPEN"), wxArtProvider::GetBitmap(wxART_FILE_OPEN), intl::wxformat("TOOL_OPEN"));
    toolbar_top->AddTool(MENU_SAVE, intl::wxformat("TOOL_SAVE"), wxArtProvider::GetBitmap(wxART_FILE_SAVE), intl::wxformat("TOOL_SAVE"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(MENU_UNDO, intl::wxformat("TOOL_UNDO"), wxArtProvider::GetBitmap(wxART_UNDO), intl::wxformat("TOOL_UNDO"));
    toolbar_top->AddTool(MENU_REDO, intl::wxformat("TOOL_REDO"), wxArtProvider::GetBitmap(wxART_REDO), intl::wxformat("TOOL_REDO"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(MENU_CUT, intl::wxformat("TOOL_CUT"), wxArtProvider::GetBitmap(wxART_CUT), intl::wxformat("TOOL_CUT"));
    toolbar_top->AddTool(MENU_COPY, intl::wxformat("TOOL_COPY"), wxArtProvider::GetBitmap(wxART_COPY), intl::wxformat("TOOL_COPY"));
    toolbar_top->AddTool(MENU_PASTE, intl::wxformat("TOOL_PASTE"), wxArtProvider::GetBitmap(wxART_PASTE), intl::wxformat("TOOL_PASTE"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(CTL_LOAD_PDF, intl::wxformat("TOOL_LOAD_PDF"), loadPNG(tool_load_pdf_png), intl::wxformat("TOOL_LOAD_PDF"));
    toolbar_top->AddTool(CTL_AUTO_LAYOUT, intl::wxformat("TOOL_AUTO_LAYOUT"), loadPNG(tool_auto_layout_png), intl::wxformat("TOOL_AUTO_LAYOUT"));
    toolbar_top->AddTool(MENU_READDATA, intl::wxformat("TOOL_READDATA"), wxArtProvider::GetBitmap(wxART_REPORT_VIEW), intl::wxformat("TOOL_READDATA"));
    toolbar_top->AddTool(MENU_OPEN_LAYOUT_OPTIONS, intl::wxformat("TOOL_LAYOUT_OPTIONS"), loadPNG(tool_settings_png), intl::wxformat("TOOL_LAYOUT_OPTIONS"));

    toolbar_top->AddStretchableSpace();
    
    toolbar_top->AddTool(CTL_ROTATE, intl::wxformat("TOOL_ROTATE"), loadPNG(tool_rotate_png), intl::wxformat("TOOL_ROTATE"));

    m_page = new PageCtrl(toolbar_top, CTL_PAGE);
    toolbar_top->AddControl(m_page, intl::wxformat("TOOL_PAGE"));

    m_scale = new wxSlider(toolbar_top, CTL_SCALE, 50, 1, 100, wxDefaultPosition, wxSize(150, -1));
    toolbar_top->AddControl(m_scale, intl::wxformat("TOOL_SCALE"));

    toolbar_top->Realize();

    wxSplitterWindow *m_splitter = new wxSplitterWindow(this);

    wxPanel *m_panel_left = new wxPanel(m_splitter);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

    wxToolBar *toolbar_side = new wxToolBar(m_panel_left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_VERTICAL);

    toolbar_side->AddRadioTool(TOOL_SELECT, intl::wxformat("TOOL_SELECT"), loadPNG(tool_select_png), wxNullBitmap, intl::wxformat("TOOL_SELECT"));
    toolbar_side->AddRadioTool(TOOL_NEWBOX, intl::wxformat("TOOL_NEWBOX"), loadPNG(tool_newbox_png), wxNullBitmap, intl::wxformat("TOOL_NEWBOX"));
    toolbar_side->AddRadioTool(TOOL_DELETEBOX, intl::wxformat("TOOL_DELETEBOX"), loadPNG(tool_deletebox_png), wxNullBitmap, intl::wxformat("TOOL_DELETEBOX"));
    toolbar_side->AddRadioTool(TOOL_RESIZE, intl::wxformat("TOOL_RESIZE"), loadPNG(tool_resize_png), wxNullBitmap, intl::wxformat("TOOL_RESIZE"));
    toolbar_side->AddRadioTool(TOOL_TEST, intl::wxformat("TOOL_TEST"), loadPNG(tool_test_png), wxNullBitmap, intl::wxformat("TOOL_TEST"));
    toolbar_side->AddRadioTool(TOOL_MOVEPAGE, intl::wxformat("TOOL_MOVEPAGE"), loadPNG(tool_move_page_png), wxNullBitmap, intl::wxformat("TOOL_MOVEPAGE"));

    toolbar_side->AddSeparator();

    toolbar_side->AddTool(TOOL_MOVEUP, intl::wxformat("TOOL_MOVEUP"), wxArtProvider::GetBitmap(wxART_GO_UP), intl::wxformat("TOOL_MOVEUP"));
    toolbar_side->AddTool(TOOL_MOVEDOWN, intl::wxformat("TOOL_MOVEDOWN"), wxArtProvider::GetBitmap(wxART_GO_DOWN), intl::wxformat("TOOL_MOVEDOWN"));

    toolbar_side->Realize();
    sizer->Add(toolbar_side, wxSizerFlags().Expand());

    m_list_boxes = new wxListBox(m_panel_left, CTL_LIST_BOXES);
    sizer->Add(m_list_boxes, wxSizerFlags(1).Expand());

    m_panel_left->SetSizer(sizer);
    m_image = new box_editor_panel(m_splitter, this);

    m_splitter->SplitVertically(m_panel_left, m_image, 200);
    m_splitter->SetMinimumPaneSize(100);

    auto loadIcon = [&](const auto &resource) {
        wxIcon icon;
        icon.CopyFromBitmap(loadPNG(resource));
        return icon;
    };

    SetIcon(loadIcon(icon_editor_png));
    Show();

    currentHistory = history.begin();
}

void frame_editor::openFile(const wxString &filename) {
    try {
        if (box_dialog::closeAll()) {
            m_filename = filename.ToStdString();
            layout = layout_box_list::from_file(m_filename);

            modified = false;
            history.clear();
            updateLayout();

            m_bls_history->AddFileToHistory(filename);
            m_config->SetPath("/RecentFiles");
            m_bls_history->Save(*m_config);
            m_config->SetPath("/");
        }
    } catch (const std::exception &error) {
        wxMessageBox(intl::wxformat("CANT_OPEN_FILE", filename.ToStdString()), intl::wxformat("PROGRAM_NAME"), wxOK | wxICON_ERROR);
    }
}

bool frame_editor::save(bool saveAs) {
    if (m_filename.empty() || saveAs) {
        wxString lastLayoutDir = m_config->Read("LastLayoutDir");
        wxFileDialog diag(this, intl::wxformat("SAVE_LAYOUT_DIALOG"), lastLayoutDir, m_filename.string(), 
            util::to_wx(std::format("{} (*.bls)|*.bls|{} (*.*)|*.*", intl::format("Layout files"), intl::format("All files"))), wxFD_SAVE);

        if (diag.ShowModal() == wxID_CANCEL)
            return false;

        m_config->Write("LastLayoutDir", wxFileName(diag.GetPath()).GetPath());
        m_filename = diag.GetPath().ToStdString();
    }
    try {
        layout.save_file(m_filename);
    } catch (const std::exception &error) {
        wxMessageBox(error.what(), intl::wxformat("PROGRAM_NAME"), wxICON_ERROR);
        return false;
    }
    modified = false;
    return true;
}

bool frame_editor::saveIfModified() {
    if (modified) {
        wxMessageDialog dialog(this, intl::wxformat("SAVE_CHANGES_DIALOG"), intl::wxformat("PROGRAM_NAME"), wxYES_NO | wxCANCEL | wxICON_WARNING);

        switch (dialog.ShowModal()) {
        case wxID_YES:
            return save();
        case wxID_NO:
            return true;
        case wxID_CANCEL:
            return false;
        }
    }
    return true;
}

void frame_editor::updateLayout(bool addToHistory) {
    m_list_boxes->Clear();
    for (auto &box : layout) {
        if (box.name.empty()) {
            m_list_boxes->Append(intl::wxformat("UNNAMED_BOX"));
        } else {
            m_list_boxes->Append(box.name);
        }
    }
    m_image->Refresh();

    if (addToHistory) {
        if (!history.empty()) {
            modified = true;
        }
        while (!history.empty() && history.end() > currentHistory + 1) {
            history.pop_back();
        }
        history.push_back(layout);
        if (history.size() > MAX_HISTORY_SIZE) {
            history.pop_front();
        }
        currentHistory = history.end() - 1;
    }
}

void frame_editor::loadPdf(const wxString &filename) {
    try {
        m_doc.open(filename.ToStdString());
        m_page->SetMaxPages(m_doc.num_pages());
        setSelectedPage(1, true);

        m_pdf_history->AddFileToHistory(m_doc.filename().string());

        m_config->SetPath("/RecentPdfs");
        m_pdf_history->Save(*m_config);
        m_config->SetPath("/");
    } catch (const file_error &error) {
        wxMessageBox(error.what(), intl::wxformat("PROGRAM_NAME"), wxICON_ERROR);
    }
}

wxString frame_editor::getControlScript(bool open_dialog) {
    wxString filename = m_config->Read("ControlScriptFilename");
    if (filename.empty() || open_dialog) {
        wxFileDialog diag(this, intl::wxformat("OPEN_CONTROL_SCRIPT_DIALOG"), wxFileName(filename).GetPath(), wxEmptyString,
            util::to_wx(std::format("{} (*.bls)|*.bls|{} (*.*)|*.*", intl::format("Layout files"), intl::format("All files"))));

        if (diag.ShowModal() == wxID_OK) {
            filename = diag.GetPath();
            m_config->Write("ControlScriptFilename", filename);
        }
    }
    return filename;
}

void frame_editor::setSelectedPage(int page, bool force) {
    if (!force && page == selected_page) return;
    if (!m_doc.isopen()) return;

    if (page > m_doc.num_pages() || page <= 0) {
        wxBell();
        return;
    }
    
    selected_page = page;

    m_page->SetValue(page);
    m_image->setImage(pdf_to_image(m_doc, page, rotation));
}

void frame_editor::selectBox(layout_box *box) {
    m_image->setSelectedBox(box);
    if (box) {
        m_list_boxes->SetSelection(std::distance(layout.begin(),
            std::ranges::find(layout, box,
            [](const auto &box) { return &box; })));
    } else {
        m_list_boxes->SetSelection(-1);
    }
    m_image->Refresh();
}