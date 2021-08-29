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

    CTL_ROTATE, CTL_LOAD_PDF, CTL_FIND_LAYOUT, CTL_PAGE, CTL_SCALE,

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
    EVT_TOOL (CTL_FIND_LAYOUT, frame_editor::OnFindLayout)
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
DECLARE_RESOURCE(tool_find_layout_png)
DECLARE_RESOURCE(tool_settings_png)

constexpr size_t MAX_HISTORY_SIZE = 20;

frame_editor::frame_editor() : wxFrame(nullptr, wxID_ANY, wxintl::translate("PROGRAM_NAME"), wxDefaultPosition, wxSize(900, 700)) {
    wxMenuBar *menuBar = new wxMenuBar();
    
    m_bls_history = new wxFileHistory(MAX_RECENT_FILES_HISTORY, MENU_OPEN_RECENT);
    m_bls_history->UseMenu(m_bls_history_menu = new wxMenu);

    m_pdf_history = new wxFileHistory(MAX_RECENT_PDFS_HISTORY, MENU_OPEN_PDF_RECENT);
    m_pdf_history->UseMenu(m_pdf_history_menu = new wxMenu);

    wxConfig::Get()->SetPath("/RecentFiles");
    m_bls_history->Load(*wxConfig::Get());

    wxConfig::Get()->SetPath("/RecentPdfs");
    m_pdf_history->Load(*wxConfig::Get());
    
    wxConfig::Get()->SetPath("/");

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(MENU_NEW, wxintl::translate("MENU_NEW"), wxintl::translate("MENU_NEW_HINT"));
    menuFile->Append(MENU_OPEN, wxintl::translate("MENU_OPEN"), wxintl::translate("MENU_OPEN_HINT"));
    menuFile->AppendSubMenu(m_bls_history_menu, wxintl::translate("MENU_RECENT"));
    menuFile->Append(MENU_SAVE, wxintl::translate("MENU_SAVE"), wxintl::translate("MENU_SAVE_HINT"));
    menuFile->Append(MENU_SAVEAS, wxintl::translate("MENU_SAVEAS"), wxintl::translate("MENU_SAVEAS_HINT"));
    menuFile->AppendSeparator();
    menuFile->Append(MENU_LOAD_PDF, wxintl::translate("MENU_LOAD_PDF"), wxintl::translate("MENU_LOAD_PDF_HINT"));
    menuFile->AppendSubMenu(m_pdf_history_menu, wxintl::translate("MENU_RECENT_PDF"));
    menuFile->AppendSeparator();
    menuFile->Append(MENU_CLOSE, wxintl::translate("MENU_CLOSE"), wxintl::translate("MENU_CLOSE_HINT"));
    menuBar->Append(menuFile, wxintl::translate("MENU_FILE"));

    wxMenu *menuEdit = new wxMenu;
    menuEdit->Append(MENU_UNDO, wxintl::translate("MENU_UNDO"), wxintl::translate("MENU_UNDO_HINT"));
    menuEdit->Append(MENU_REDO, wxintl::translate("MENU_REDO"), wxintl::translate("MENU_REDO_HINT"));
    menuEdit->AppendSeparator();
    menuEdit->Append(MENU_CUT, wxintl::translate("MENU_CUT"), wxintl::translate("MENU_CUT_HINT"));
    menuEdit->Append(MENU_COPY, wxintl::translate("MENU_COPY"), wxintl::translate("MENU_COPY_HINT"));
    menuEdit->Append(MENU_PASTE, wxintl::translate("MENU_PASTE"), wxintl::translate("MENU_PASTE_HINT"));
    menuEdit->AppendSeparator();
    menuEdit->Append(MENU_DELETE, wxintl::translate("MENU_DELETE"), wxintl::translate("MENU_DELETE_HINT"));
    menuBar->Append(menuEdit, wxintl::translate("MENU_EDIT"));

    wxMenu *menuEditor = new wxMenu;
    menuEditor->Append(MENU_EDITCONTROL, wxintl::translate("MENU_EDITCONTROL"));

    menuBar->Append(menuEditor, wxintl::translate("MENU_EDITOR"));

    SetMenuBar(menuBar);

    wxToolBar *toolbar_top = CreateToolBar();

    toolbar_top->AddTool(MENU_NEW, wxintl::translate("TOOL_NEW"), wxArtProvider::GetBitmap(wxART_NEW), wxintl::translate("TOOL_NEW"));
    toolbar_top->AddTool(MENU_OPEN, wxintl::translate("TOOL_OPEN"), wxArtProvider::GetBitmap(wxART_FILE_OPEN), wxintl::translate("TOOL_OPEN"));
    toolbar_top->AddTool(MENU_SAVE, wxintl::translate("TOOL_SAVE"), wxArtProvider::GetBitmap(wxART_FILE_SAVE), wxintl::translate("TOOL_SAVE"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(MENU_UNDO, wxintl::translate("TOOL_UNDO"), wxArtProvider::GetBitmap(wxART_UNDO), wxintl::translate("TOOL_UNDO"));
    toolbar_top->AddTool(MENU_REDO, wxintl::translate("TOOL_REDO"), wxArtProvider::GetBitmap(wxART_REDO), wxintl::translate("TOOL_REDO"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(MENU_CUT, wxintl::translate("TOOL_CUT"), wxArtProvider::GetBitmap(wxART_CUT), wxintl::translate("TOOL_CUT"));
    toolbar_top->AddTool(MENU_COPY, wxintl::translate("TOOL_COPY"), wxArtProvider::GetBitmap(wxART_COPY), wxintl::translate("TOOL_COPY"));
    toolbar_top->AddTool(MENU_PASTE, wxintl::translate("TOOL_PASTE"), wxArtProvider::GetBitmap(wxART_PASTE), wxintl::translate("TOOL_PASTE"));

    toolbar_top->AddSeparator();

    toolbar_top->AddTool(CTL_LOAD_PDF, wxintl::translate("TOOL_LOAD_PDF"), loadPNG(tool_load_pdf_png), wxintl::translate("TOOL_LOAD_PDF"));
    toolbar_top->AddTool(CTL_FIND_LAYOUT, wxintl::translate("TOOL_FIND_LAYOUT"), loadPNG(tool_find_layout_png), wxintl::translate("TOOL_FIND_LAYOUT"));
    toolbar_top->AddTool(MENU_READDATA, wxintl::translate("TOOL_READDATA"), wxArtProvider::GetBitmap(wxART_REPORT_VIEW), wxintl::translate("TOOL_READDATA"));
    toolbar_top->AddTool(MENU_OPEN_LAYOUT_OPTIONS, wxintl::translate("TOOL_LAYOUT_OPTIONS"), loadPNG(tool_settings_png), wxintl::translate("TOOL_LAYOUT_OPTIONS"));

    toolbar_top->AddStretchableSpace();
    
    toolbar_top->AddTool(CTL_ROTATE, wxintl::translate("TOOL_ROTATE"), loadPNG(tool_rotate_png), wxintl::translate("TOOL_ROTATE"));

    m_page = new PageCtrl(toolbar_top, CTL_PAGE);
    toolbar_top->AddControl(m_page, wxintl::translate("TOOL_PAGE"));

    m_scale = new wxSlider(toolbar_top, CTL_SCALE, 50, 1, 100, wxDefaultPosition, wxSize(150, -1));
    toolbar_top->AddControl(m_scale, wxintl::translate("TOOL_SCALE"));

    toolbar_top->Realize();

    wxSplitterWindow *m_splitter = new wxSplitterWindow(this);

    wxPanel *m_panel_left = new wxPanel(m_splitter);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

    wxToolBar *toolbar_side = new wxToolBar(m_panel_left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_VERTICAL);

    toolbar_side->AddRadioTool(TOOL_SELECT, wxintl::translate("TOOL_SELECT"), loadPNG(tool_select_png), wxNullBitmap, wxintl::translate("TOOL_SELECT"));
    toolbar_side->AddRadioTool(TOOL_NEWBOX, wxintl::translate("TOOL_NEWBOX"), loadPNG(tool_newbox_png), wxNullBitmap, wxintl::translate("TOOL_NEWBOX"));
    toolbar_side->AddRadioTool(TOOL_DELETEBOX, wxintl::translate("TOOL_DELETEBOX"), loadPNG(tool_deletebox_png), wxNullBitmap, wxintl::translate("TOOL_DELETEBOX"));
    toolbar_side->AddRadioTool(TOOL_RESIZE, wxintl::translate("TOOL_RESIZE"), loadPNG(tool_resize_png), wxNullBitmap, wxintl::translate("TOOL_RESIZE"));
    toolbar_side->AddRadioTool(TOOL_TEST, wxintl::translate("TOOL_TEST"), loadPNG(tool_test_png), wxNullBitmap, wxintl::translate("TOOL_TEST"));
    toolbar_side->AddRadioTool(TOOL_MOVEPAGE, wxintl::translate("TOOL_MOVEPAGE"), loadPNG(tool_move_page_png), wxNullBitmap, wxintl::translate("TOOL_MOVEPAGE"));

    toolbar_side->AddSeparator();

    toolbar_side->AddTool(TOOL_MOVEUP, wxintl::translate("TOOL_MOVEUP"), wxArtProvider::GetBitmap(wxART_GO_UP), wxintl::translate("TOOL_MOVEUP"));
    toolbar_side->AddTool(TOOL_MOVEDOWN, wxintl::translate("TOOL_MOVEDOWN"), wxArtProvider::GetBitmap(wxART_GO_DOWN), wxintl::translate("TOOL_MOVEDOWN"));

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

            wxConfig::Get()->SetPath("/RecentFiles");
            m_bls_history->AddFileToHistory(filename);
            m_bls_history->Save(*wxConfig::Get());
            wxConfig::Get()->SetPath("/");
        }
    } catch (const std::exception &error) {
        wxMessageBox(wxintl::translate("CANT_OPEN_FILE", filename.ToStdString()), wxintl::translate("PROGRAM_NAME"), wxOK | wxICON_ERROR);
    }
}

bool frame_editor::save(bool saveAs) {
    if (m_filename.empty() || saveAs) {
        wxString lastLayoutDir = wxConfig::Get()->Read("LastLayoutDir");
        wxFileDialog diag(this, wxintl::translate("SAVE_LAYOUT_DIALOG"), lastLayoutDir, m_filename.string(), 
            wxintl::to_wx(std::format("{} (*.bls)|*.bls|{} (*.*)|*.*", intl::translate("Layout files"), intl::translate("All files"))), wxFD_SAVE);

        if (diag.ShowModal() == wxID_CANCEL)
            return false;

        wxConfig::Get()->Write("LastLayoutDir", wxFileName(diag.GetPath()).GetPath());
        m_filename = diag.GetPath().ToStdString();
    }
    try {
        layout.save_file(m_filename);
    } catch (const std::exception &error) {
        wxMessageBox(error.what(), wxintl::translate("PROGRAM_NAME"), wxICON_ERROR);
        return false;
    }
    modified = false;
    return true;
}

bool frame_editor::saveIfModified() {
    if (modified) {
        wxMessageDialog dialog(this, wxintl::translate("SAVE_CHANGES_DIALOG"), wxintl::translate("PROGRAM_NAME"), wxYES_NO | wxCANCEL | wxICON_WARNING);

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
            m_list_boxes->Append(wxintl::translate("UNNAMED_BOX"));
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

        wxConfig::Get()->SetPath("/RecentPdfs");
        m_pdf_history->AddFileToHistory(m_doc.filename().string());
        m_pdf_history->Save(*wxConfig::Get());
        wxConfig::Get()->SetPath("/");
    } catch (const file_error &error) {
        wxMessageBox(error.what(), wxintl::translate("PROGRAM_NAME"), wxICON_ERROR);
    }
}

wxString frame_editor::getControlScript(bool open_dialog) {
    wxString filename = wxConfig::Get()->Read("ControlScriptFilename");
    if (filename.empty() || open_dialog) {
        wxFileDialog diag(this, wxintl::translate("OPEN_CONTROL_SCRIPT_DIALOG"), wxFileName(filename).GetPath(), wxEmptyString,
            wxintl::to_wx(std::format("{} (*.bls)|*.bls|{} (*.*)|*.*", intl::translate("Layout files"), intl::translate("All files"))));

        if (diag.ShowModal() == wxID_OK) {
            filename = diag.GetPath();
            wxConfig::Get()->Write("ControlScriptFilename", filename);
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