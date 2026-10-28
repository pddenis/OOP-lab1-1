#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "geometry.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class UIElement {
private:
    Rect bounds_;
    bool enabled_;

public:
    UIElement(const Point2D& position, const Vector2D& size, bool enabled = true)
        : bounds_(position, size), enabled_(enabled) {
    }

    virtual ~UIElement() {}

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;

    Rect getBoundingBox() const { return bounds_; }
    bool contains(const Point2D& p) const { return bounds_.contains(p); }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

    void handlePress() { if (enabled_) onPress(); }
    void handleTextInput(const std::string& text) { if (enabled_) onTextInput(text); }

    bool canReceiveFocus() const { return enabled_ && supportsFocus(); }

    virtual bool supportsFocus() const = 0;
    virtual std::string toString() const = 0;

protected:
    virtual void onPress() = 0;
    virtual void onTextInput(const std::string&) {}
};

class Button : public UIElement {
private:
    int click_count_ = 0;

public:
    using UIElement::UIElement;

    int getClickCount() const { return click_count_; }
    bool supportsFocus() const override { return false; }
    std::string toString() const override { return "Button:" + std::to_string(click_count_); }

protected:
    void onPress() override { ++click_count_; }
};

class CheckBox : public UIElement {
private:
    bool checked_;

public:
    CheckBox(const Point2D& position, const Vector2D& size, bool checked = false, bool enabled = true)
        : UIElement(position, size, enabled), checked_(checked) {
    }

    bool isChecked() const { return checked_; }
    bool supportsFocus() const override { return false; }
    std::string toString() const override { return checked_ ? "[x] CheckBox" : "[ ] CheckBox"; }

protected:
    void onPress() override { checked_ = !checked_; }
};

class TextField : public UIElement {
private:
    std::string text_;

public:
    TextField(const Point2D& position, const Vector2D& size, const std::string& text = "", bool enabled = true)
        : UIElement(position, size, enabled), text_(text) {
    }

    const std::string& getText() const { return text_; }
    bool supportsFocus() const override { return true; }
    std::string toString() const override { return text_.empty() ? "TextField" : text_; }

protected:
    void onPress() override {}
    void onTextInput(const std::string& text) override { text_ += text; }
};

class Form {
private:
    std::vector<std::unique_ptr<UIElement>> elements_;
    int focused_index_;

    int findTopIndexAt(const Point2D& p) const {
        for (int i = static_cast<int>(elements_.size()) - 1; i >= 0; --i) {
            if (elements_[i]->contains(p)) return i;
        }
        return -1;
    }

    void requireIndex(std::size_t index) const {
        if (index >= elements_.size()) throw std::out_of_range("Form: element index out of range");
    }

public:
    Form() : focused_index_(-1) {}

    template<typename E, typename... Args>
    E* add(Args&&... args) {
        std::unique_ptr<E> element(new E(std::forward<Args>(args)...));
        E* raw = element.get();
        addElement(std::unique_ptr<UIElement>(element.release()));
        return raw;
    }

    void addElement(std::unique_ptr<UIElement> element) {
        if (!element) throw std::invalid_argument("Form: element must not be null");
        elements_.push_back(std::move(element));
    }

    void removeElement(std::size_t index) {
        requireIndex(index);
        const int i = static_cast<int>(index);
        if (focused_index_ == i) focused_index_ = -1;
        else if (focused_index_ > i) --focused_index_;
        elements_.erase(elements_.begin() + i);
    }

    void setElementEnabled(std::size_t index, bool enabled) {
        requireIndex(index);
        elements_[index]->setEnabled(enabled);
        if (!enabled && focused_index_ == static_cast<int>(index)) focused_index_ = -1;
    }

    const UIElement* findTopElementAt(const Point2D& p) const {
        const int i = findTopIndexAt(p);
        return i >= 0 ? elements_[i].get() : nullptr;
    }
    UIElement* findTopElementAt(const Point2D& p) {
        const int i = findTopIndexAt(p);
        return i >= 0 ? elements_[i].get() : nullptr;
    }

    void handlePressAt(const Point2D& p) {
        const int i = findTopIndexAt(p);
        if (i < 0) return;
        UIElement& top = *elements_[i];
        if (!top.isEnabled()) return;
        top.handlePress();
        if (top.canReceiveFocus()) focused_index_ = i;
    }

    void sendTextInput(const std::string& text) {
        const int i = getFocusedIndex();
        if (i >= 0) elements_[i]->handleTextInput(text);
    }

    void nextFocus() {
        const int n = static_cast<int>(elements_.size());
        if (n == 0) { focused_index_ = -1; return; }
        const int start = focused_index_ >= 0 ? focused_index_ : n - 1;
        for (int step = 1; step <= n; ++step) {
            const int idx = (start + step) % n;
            if (elements_[idx]->canReceiveFocus()) { focused_index_ = idx; return; }
        }
        focused_index_ = -1;
    }

    int getFocusedIndex() const {
        if (focused_index_ >= 0 && elements_[focused_index_]->canReceiveFocus()) return focused_index_;
        return -1;
    }

    std::size_t getElementCount() const { return elements_.size(); }
    const UIElement* getElement(std::size_t index) const {
        return index < elements_.size() ? elements_[index].get() : nullptr;
    }
    UIElement* getElement(std::size_t index) {
        return index < elements_.size() ? elements_[index].get() : nullptr;
    }
};

#endif // UI_ELEMENT_H