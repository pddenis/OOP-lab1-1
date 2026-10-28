#include "console_renderer.h"
#include "geometry.h"
#include "ui_element.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

static void check(bool condition, const std::string& name) {
    if (condition) { ++g_passed; std::cout << "[PASS] " << name << "\n"; }
    else { ++g_failed; std::cerr << "[FAIL] " << name << "\n"; }
}

template<typename Ex, typename F>
static void checkThrows(F f, const std::string& name) {
    bool ok = false;
    try { f(); }
    catch (const Ex&) { ok = true; }
    catch (...) {}
    check(ok, name);
}

class FocusableStub : public UIElement {
public:
    using UIElement::UIElement;
    int presses = 0;
    bool supportsFocus() const override { return true; }
    std::string toString() const override { return "Stub"; }
protected:
    void onPress() override { ++presses; }
};

static const Vector2D kSize(10, 10);

static void testGeometry() {
    Point2D p(10, 20);
    Vector2D v(5, 5);
    check(p + v == Point2D(15, 25), "Geometry: Point2D + Vector2D");
    check(p - v == Point2D(5, 15), "Geometry: Point2D - Vector2D");
    check(Point2D(10, 10) - Point2D(5, 5) == Vector2D(5, 5), "Geometry: Point2D - Point2D -> Vector2D");
    check(v + Vector2D(1, 2) == Vector2D(6, 7), "Geometry: Vector2D + Vector2D");
    check(v - Vector2D(1, 2) == Vector2D(4, 3), "Geometry: Vector2D - Vector2D");
    check(v * 2 == Vector2D(10, 10) && 2 * v == Vector2D(10, 10), "Geometry: Vector2D * scalar");
    check(v / 5 == Vector2D(1, 1), "Geometry: Vector2D / scalar");
    check(-v == Vector2D(-5, -5), "Geometry: unary minus");
    checkThrows<std::invalid_argument>([&] { v / 0.0; }, "Geometry: Vector2D / 0 throws");

    Point2D q = p + v;
    check(p == Point2D(10, 20) && v == Vector2D(5, 5) && q == Point2D(15, 25),
        "Geometry: operations do not modify operands");
    check(Vector2D(0.1 + 0.2, 0) == Vector2D(0.3, 0), "Geometry: equality tolerates FP error");
    check(Point2D(1, 1) != Point2D(1, 2) && Vector2D(1, 1) != Vector2D(2, 1), "Geometry: operator!=");

    Rect r(Point2D(0, 0), Vector2D(10, 10));
    check(r.contains(Point2D(5, 5)), "Geometry: Rect contains inner point");
    check(r.contains(Point2D(0, 0)) && r.contains(Point2D(10, 10)), "Geometry: Rect contains border");
    check(!r.contains(Point2D(10.5, 5)), "Geometry: Rect excludes outer point");
    check(r.intersects(Rect(Point2D(5, 5), Vector2D(10, 10))), "Geometry: Rect intersects overlapping");
    check(r.intersects(Rect(Point2D(10, 0), Vector2D(5, 5))), "Geometry: Rect intersects touching");
    check(!r.intersects(Rect(Point2D(11, 0), Vector2D(5, 5))), "Geometry: Rect not intersects disjoint");
    check(r == Rect(Point2D(0, 0), Vector2D(10, 10)) && r != Rect(Point2D(1, 0), Vector2D(10, 10)),
        "Geometry: Rect ==/!=");
    checkThrows<std::invalid_argument>([] { Rect(Point2D(0, 0), Vector2D(-1, 5)); },
        "Geometry: Rect with negative size throws");
    check(Rect(Point2D(3, 3), Vector2D(0, 0)).contains(Point2D(3, 3)), "Geometry: zero-size Rect allowed");
}

static void testElementBehaviour() {
    Form form;
    Button* btn = form.add<Button>(Point2D(0, 0), kSize);
    CheckBox* cb = form.add<CheckBox>(Point2D(20, 0), kSize);
    CheckBox* offCb = form.add<CheckBox>(Point2D(40, 0), kSize, false, false);
    Button* offBtn = form.add<Button>(Point2D(60, 0), kSize, false);

    form.handlePressAt(Point2D(5, 5));
    form.handlePressAt(Point2D(5, 5));
    check(btn->getClickCount() == 2, "Element: Button counts every press");
    check(btn->toString() == "Button:2", "Element: Button text");

    form.handlePressAt(Point2D(25, 5));
    check(cb->isChecked() && cb->toString() == "[x] CheckBox", "Element: CheckBox unchecked -> checked");
    form.handlePressAt(Point2D(25, 5));
    check(!cb->isChecked() && cb->toString() == "[ ] CheckBox", "Element: CheckBox checked -> unchecked");

    form.handlePressAt(Point2D(45, 5));
    check(!offCb->isChecked(), "Element: disabled CheckBox does not change");
    form.handlePressAt(Point2D(65, 5));
    offBtn->handlePress();
    check(offBtn->getClickCount() == 0, "Element: disabled Button does not count");
    check(form.getFocusedIndex() == -1, "Element: Button/CheckBox do not take focus");
}

static void testZOrder() {
    Form form;
    Button* bottom = form.add<Button>(Point2D(0, 0), kSize);
    Button* top = form.add<Button>(Point2D(5, 5), kSize);

    check(form.findTopElementAt(Point2D(7, 7)) == top, "ZOrder: later element is on top");
    check(form.findTopElementAt(Point2D(2, 2)) == bottom, "ZOrder: lower element outside overlap");
    check(form.findTopElementAt(Point2D(100, 100)) == nullptr, "ZOrder: empty point -> nullptr");

    form.handlePressAt(Point2D(7, 7));
    check(top->getClickCount() == 1 && bottom->getClickCount() == 0, "ZOrder: only top element gets press");

    top->setEnabled(false);
    form.handlePressAt(Point2D(7, 7));
    check(top->getClickCount() == 1 && bottom->getClickCount() == 0, "ZOrder: disabled top swallows press");

    form.removeElement(1);
    check(form.findTopElementAt(Point2D(7, 7)) == bottom, "ZOrder: after removal lower becomes top");
}

static void testFocus() {
    Form form;
    form.add<Button>(Point2D(0, 0), kSize);
    TextField* a = form.add<TextField>(Point2D(20, 0), kSize, "A");
    form.add<CheckBox>(Point2D(40, 0), kSize);
    TextField* b = form.add<TextField>(Point2D(60, 0), kSize, "B", false);
    TextField* c = form.add<TextField>(Point2D(80, 0), kSize, "C");

    check(form.getFocusedIndex() == -1, "Focus: initially none");
    form.sendTextInput("x");
    check(a->getText() == "A" && c->getText() == "C", "Focus: text without focus changes nothing");

    form.handlePressAt(Point2D(25, 5));
    check(form.getFocusedIndex() == 1, "Focus: press on TextField focuses it");
    form.sendTextInput("1");
    check(a->getText() == "A1" && c->getText() == "C", "Focus: text goes only to focused element");

    form.handlePressAt(Point2D(85, 5));
    check(form.getFocusedIndex() == 4, "Focus: moves to another field (single focus)");
    form.sendTextInput("2");
    check(a->getText() == "A1" && c->getText() == "C2", "Focus: previous element gets no text");

    form.handlePressAt(Point2D(45, 5));
    form.handlePressAt(Point2D(500, 500));
    check(form.getFocusedIndex() == 4, "Focus: non-focusable press / empty area keeps focus");

    form.handlePressAt(Point2D(65, 5));
    check(form.getFocusedIndex() == 4 && b->getText() == "B", "Focus: disabled element cannot get focus");

    form.nextFocus();
    check(form.getFocusedIndex() == 1, "Focus: nextFocus wraps to first focusable");
    form.nextFocus();
    check(form.getFocusedIndex() == 4, "Focus: nextFocus skips CheckBox and disabled field");

    form.setElementEnabled(4, false);
    check(form.getFocusedIndex() == -1, "Focus: disabling focused element clears focus");
    form.setElementEnabled(4, true);
    check(form.getFocusedIndex() == -1, "Focus: re-enabling does not restore focus");

    form.nextFocus();
    form.getElement(1)->setEnabled(false);
    check(form.getFocusedIndex() == -1, "Focus: element disabled directly is not focused");
    form.sendTextInput("zzz");
    check(a->getText() == "A1", "Focus: such element gets no text");
}

static void testFormEdgeCases() {
    Form empty;
    empty.nextFocus();
    check(empty.getFocusedIndex() == -1, "Form: nextFocus on empty form");

    Form none;
    none.add<Button>(Point2D(0, 0), kSize);
    none.nextFocus();
    check(none.getFocusedIndex() == -1, "Form: nextFocus without focusable elements");

    Form single;
    single.add<TextField>(Point2D(0, 0), kSize);
    single.nextFocus();
    single.nextFocus();
    check(single.getFocusedIndex() == 0, "Form: single focusable element keeps focus on cycle");

    Form shift;
    shift.add<Button>(shift.add<Button>(Point2D(0, 0), kSize) ? Point2D(0, 0) : Point2D(0, 0), kSize); // компактнее ниже:
    Form shift2;
    shift2.add<Button>(Point2D(0, 0), kSize);
    shift2.add<TextField>(Point2D(20, 0), kSize);
    shift2.nextFocus();
    shift2.removeElement(0);
    check(shift2.getFocusedIndex() == 0, "Form: focus index shifts after earlier removal");
    shift2.removeElement(0);
    check(shift2.getFocusedIndex() == -1 && shift2.getElementCount() == 0, "Form: removing focused clears focus");

    Form errors;
    checkThrows<std::out_of_range>([&] { errors.removeElement(0); }, "Form: removeElement out of range");
    checkThrows<std::out_of_range>([&] { errors.setElementEnabled(3, false); }, "Form: setElementEnabled out of range");
    checkThrows<std::invalid_argument>([&] { errors.addElement(nullptr); }, "Form: addElement(nullptr)");
    check(errors.getElement(0) == nullptr, "Form: getElement out of range -> nullptr");
}

static void testExtensibility() {
    Form form;
    FocusableStub* stub = form.add<FocusableStub>(Point2D(0, 0), kSize);
    form.handlePressAt(Point2D(1, 1));
    check(stub->presses == 1 && form.getFocusedIndex() == 0, "Extensibility: new focusable type works, Form unchanged");
    form.sendTextInput("abc");
    check(stub->toString() == "Stub", "Extensibility: element without text input ignores text");
    stub->setEnabled(false);
    stub->handlePress();
    check(stub->presses == 1, "Extensibility: disabled rule applies to new type automatically");
}

static void testRendering() {
    Form form;
    form.add<TextField>(Point2D(11, 0), Vector2D(9, 3), "Alice");
    form.add<Button>(Point2D(0, 0), Vector2D(10, 3));
    form.handlePressAt(Point2D(1, 1));
    form.handlePressAt(Point2D(12, 1));
    std::ostringstream out;
    renderForm(form, out);
    check(out.str() ==
        "+--------+ #########\n"
        "|Button:1| #Alice  #\n"
        "+--------+ #########\n",
        "Render: button + focused text field");

    Form overlap;
    overlap.add<TextField>(Point2D(0, 0), Vector2D(8, 3), "Under");
    overlap.add<Button>(Point2D(4, 0), Vector2D(6, 3), false);
    std::ostringstream out2;
    renderForm(overlap, out2);
    check(out2.str() == "+---......\n|Und:Butt:\n+---......\n", "Render: upper element overdraws lower");
}

int main() {
    std::cout << "=== TESTS ===\n";
    testGeometry();
    testElementBehaviour();
    testZOrder();
    testFocus();
    testFormEdgeCases();
    testExtensibility();
    testRendering();
    std::cout << "=== passed: " << g_passed << ", failed: " << g_failed << " ===\n\n";

    Form form;
    form.add<Button>(Point2D(0, 0), Vector2D(16, 5));
    form.add<CheckBox>(Point2D(19, 0), Vector2D(16, 5), false, true);
    form.add<TextField>(Point2D(0, 6), Vector2D(22, 5), "Alice");
    form.add<TextField>(Point2D(25, 6), Vector2D(20, 5));
    form.handlePressAt(Point2D(1, 1));
    form.handlePressAt(Point2D(2, 8));
    renderForm(form, std::cout);

    return g_failed == 0 ? 0 : 1;
}