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
        : parent(parent), name(name), value(wxintl::to_wx(var.as_view()))
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
            children.emplace_back(this, wxintl::to_wx(key), val);
        }
    }
};

class VariableTableModel : public wxDataViewModel {
private:
    std::list<VariableTableModelNode> m_root;

    void RecurseUpdateItems(const VariableTableModelNode &node) {
        ItemAdded(wxDataViewItem((void *) node.parent), wxDataViewItem((void *) &node));
        for (const VariableTableModelNode &child : node.children) {
            RecurseUpdateItems(child);
        }
    }

public:
    void AddTable(const wxString &name, const variable_map &table) {
        RecurseUpdateItems(m_root.emplace_back(nullptr, name, table));
    }

    void ClearTables() {
        m_root.clear();

        Cleared();
    }

    virtual unsigned int GetChildren(const wxDataViewItem &item, wxDataViewItemArray &children) const override {
        VariableTableModelNode *node = (VariableTableModelNode *) item.GetID();

        const std::list<VariableTableModelNode> *list = node ? &node->children : &m_root;
        size_t count = 0;
        for (const VariableTableModelNode &c : *list) {
            children.Add(wxDataViewItem((void *) &c));
            ++count;
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