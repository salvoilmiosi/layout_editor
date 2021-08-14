#ifndef __VARIABLE_TABLE_MODEL_H__
#define __VARIABLE_TABLE_MODEL_H__

#include <wx/dataview.h>
#include "reader.h"

struct VariableTableModelNode {
    VariableTableModelNode *parent = nullptr;

    wxString name;
    wxString value;
    std::list<VariableTableModelNode> children;

    VariableTableModelNode(VariableTableModelNode *parent, const wxString &name, const wxString &value = wxEmptyString)
        : parent(parent), name(name), value(value) {}

    VariableTableModelNode(VariableTableModelNode *parent, const wxString &name, const variable &var)
        : parent(parent), name(name), value(util::to_wx(var.as_view()))
    {
        if (var.is_array()) {
            const auto &arr = var.as_array();
            for (size_t i=0; i<arr.size(); ++i) {
                children.emplace_back(this, std::format("[{}]", i), arr[i]);
            }
        }
    }

    VariableTableModelNode(VariableTableModelNode *parent, const wxString &name, const variable_map &table)
        : parent(parent), name(name)
    {
        for (const auto &[key, val] : table) {
            children.emplace_back(this, util::to_wx(key), val);
        }
    }

    bool IsDebug() const {
        return name.empty() ? false : name[0] == '_';
    }
};

class VariableTableModel : public wxDataViewModel {
private:
    std::list<VariableTableModelNode> m_root;
    bool m_show_debug = false;

public:
    void AddTable(const wxString &name, const variable_map &table) {
        const VariableTableModelNode &node = m_root.emplace_back(nullptr, name, table);

        ItemAdded(wxDataViewItem(nullptr), wxDataViewItem((void *) &node));

        wxDataViewItemArray items;
        for (const VariableTableModelNode &child : node.children) {
            if (m_show_debug || ! child.IsDebug()) {
                items.Add(wxDataViewItem((void *) &child));
            }
        }
        ItemsAdded(wxDataViewItem((void *)&node), items);
    }

    void ClearTables() {
        m_root.clear();

        Cleared();
    }

    void SetShowDebug(bool show_debug) {
        if (show_debug == m_show_debug) return;
        m_show_debug = show_debug;
        for (const VariableTableModelNode &node : m_root) {
            wxDataViewItemArray items;
            for (const VariableTableModelNode &child : node.children) {
                if (child.IsDebug()) {
                    items.Add(wxDataViewItem((void *) &child));
                }
            }
            if (m_show_debug) {
                ItemsAdded(wxDataViewItem((void *)&node), items);
            } else {
                ItemsDeleted(wxDataViewItem((void *)&node), items);
            }
        }
    }

    virtual unsigned int GetChildren(const wxDataViewItem &item, wxDataViewItemArray &children) const override {
        VariableTableModelNode *node = (VariableTableModelNode *) item.GetID();

        const std::list<VariableTableModelNode> *list = node ? &node->children : &m_root;
        size_t count = 0;
        for (const VariableTableModelNode &c : *list) {
            if (m_show_debug || ! c.IsDebug()) {
                children.Add(wxDataViewItem((void *) &c));
                ++count;
            }
        }
        return count;
    }

    virtual unsigned int GetColumnCount() const override {
        return 2;
    }

    virtual wxString GetColumnType(unsigned int col) const override {
        return "string";
    }

    virtual wxDataViewItem GetParent(const wxDataViewItem &item) const override {
        VariableTableModelNode *node = (VariableTableModelNode *) item.GetID();
        if (!node) return wxDataViewItem(nullptr);

        return wxDataViewItem((void *) node->parent);
    }

    virtual void GetValue(wxVariant &variant, const wxDataViewItem &item, unsigned int col) const override {
        VariableTableModelNode *node = (VariableTableModelNode *) item.GetID();
        if (!node) return;

        switch (col) {
        case 0: variant = node->name; break;
        case 1: variant = node->value; break;
        }
    }

    virtual bool IsContainer(const wxDataViewItem &item) const override {
        VariableTableModelNode *node = (VariableTableModelNode *) item.GetID();
        if (!node) return true;

        return !node->children.empty();
    }

    virtual bool SetValue(const wxVariant &, const wxDataViewItem &, unsigned int) override {
        return false;
    }
};

#endif