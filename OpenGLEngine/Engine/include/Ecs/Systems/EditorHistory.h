#pragma once

#include <deque>
#include <functional>
#include <string>
#include <utility>

class Engine;
class EcsWorld;

struct EditorAction
{
    std::string label;
    std::function<void(Engine&, EcsWorld&)> undo;
    std::function<void(Engine&, EcsWorld&)> redo;
};

class EditorHistory
{
public:
    void Push(EditorAction action)
    {
        undoStack.push_back(std::move(action));
        redoStack.clear();

        if (undoStack.size() > maxEntries)
            undoStack.pop_front();

        changeCounter++;
    }

    bool CanUndo() const
    {
        return !undoStack.empty();
    }
    bool CanRedo() const
    {
        return !redoStack.empty();
    }

    bool Undo(Engine& engine, EcsWorld& world)
    {
        if (undoStack.empty())
            return false;

        EditorAction action = std::move(undoStack.back());
        undoStack.pop_back();
        action.undo(engine, world);
        redoStack.push_back(std::move(action));
        changeCounter++;
        return true;
    }

    bool Redo(Engine& engine, EcsWorld& world)
    {
        if (redoStack.empty())
            return false;

        EditorAction action = std::move(redoStack.back());
        redoStack.pop_back();
        action.redo(engine, world);
        undoStack.push_back(std::move(action));
        changeCounter++;
        return true;
    }

    void MarkChanged()
    {
        changeCounter++;
    }
    void MarkSaved()
    {
        savedCounter = changeCounter;
    }
    bool IsDirty() const
    {
        return changeCounter != savedCounter;
    }

    void Clear()
    {
        undoStack.clear();
        redoStack.clear();
        changeCounter = 0;
        savedCounter = 0;
    }

private:
    static constexpr size_t maxEntries = 64;

    std::deque<EditorAction> undoStack;
    std::deque<EditorAction> redoStack;
    unsigned long long changeCounter = 0;
    unsigned long long savedCounter = 0;
};
