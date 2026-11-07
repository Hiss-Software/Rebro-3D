#pragma once

#include <memory>
#include <vector>

// Base command interface
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    // Attempt to merge an incoming command with this one (same entity, fresh edit)
    virtual bool tryMerge(ICommand*) { return false; }
};

class CommandStack {
public:
    // alreadyApplied: the change has already been applied to the state (push after the fact)
    // allowMerge: allow merging with the command on top of the stack
    void push(std::shared_ptr<ICommand> cmd, bool alreadyApplied, bool allowMerge) {
        if (allowMerge && !m_undo.empty() && m_undo.back()->tryMerge(cmd.get()))
            return;
        if (!alreadyApplied)
            cmd->redo();
        m_undo.push_back(std::move(cmd));
        m_redo.clear();
        if (m_undo.size() > 100)
            m_undo.erase(m_undo.begin());
    }

    void undo() {
        if (m_undo.empty()) return;
        auto cmd = m_undo.back();
        m_undo.pop_back();
        cmd->undo();
        m_redo.push_back(std::move(cmd));
    }

    void redo() {
        if (m_redo.empty()) return;
        auto cmd = m_redo.back();
        m_redo.pop_back();
        cmd->redo();
        m_undo.push_back(std::move(cmd));
    }

    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    void clear() { m_undo.clear(); m_redo.clear(); }

private:
    std::vector<std::shared_ptr<ICommand>> m_undo;
    std::vector<std::shared_ptr<ICommand>> m_redo;
};