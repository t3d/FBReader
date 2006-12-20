/*
 * Copyright (C) 2004-2006 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../view/GtkViewWidget.h"
#include "../../gtk/util/GtkKeyUtil.h"
#include "../../gtk/util/GtkSignalUtil.h"
#include "../dialogs/GtkDialogManager.h"

#include "GtkApplicationWindow.h"

void GtkDialogManager::createApplicationWindow(ZLApplication *application) const {
	myWindow = GTK_WINDOW((new GtkApplicationWindow(application))->getMainWindow());
	myIsInitialized = true;
}

static bool acceptAction() {
	return
		GtkDialogManager::isInitialized() &&
		!((GtkDialogManager&)GtkDialogManager::instance()).isWaiting();
}

static bool applicationQuit(GtkWidget*, GdkEvent*, gpointer data) {
	if (acceptAction()) {
		((GtkApplicationWindow*)data)->application().closeView();
	}
	return true;
}

static void repaint(GtkWidget*, GdkEvent*, gpointer data) {
	if (acceptAction()) {
		((GtkApplicationWindow*)data)->application().refreshWindow();
	}
}

static void menuActionSlot(GtkWidget*, gpointer data) {
	if (acceptAction()) {
		((ZLApplication::Action*)data)->checkAndRun();
	}
}

static bool handleKey(GtkWidget*, GdkEventKey *key, gpointer data) {
	if (acceptAction()) {
		((GtkApplicationWindow*)data)->handleKeyEventSlot(key);
	}
	return false;
}

static void mousePressed(GtkWidget*, GdkEventButton *event, gpointer data) {
	if (acceptAction()) {
		((GtkViewWidget*)data)->onMousePressed(event);
	}
}

static void mouseReleased(GtkWidget*, GdkEventButton *event, gpointer data) {
	if (acceptAction()) {
		((GtkViewWidget*)data)->onMouseReleased(event);
	}
}

static void mouseMoved(GtkWidget*, GdkEventMotion *event, gpointer data) {
	if (acceptAction()) {
		((GtkViewWidget*)data)->onMouseMoved(event);
	}
}

GtkApplicationWindow::GtkApplicationWindow(ZLApplication *application) : ZLApplicationWindow(application), myFullScreen(false) {
	myProgram = HILDON_PROGRAM(hildon_program_get_instance());
	g_set_application_name("");

	osso_initialize(ZLApplication::ApplicationName().c_str(), "0.0", false, 0);

	myWindow = HILDON_WINDOW(hildon_window_new());

	myToolbar = GTK_TOOLBAR(gtk_toolbar_new());
	gtk_toolbar_set_show_arrow(myToolbar, false);
	gtk_toolbar_set_orientation(myToolbar, GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(myToolbar, GTK_TOOLBAR_ICONS);

	myMenu = GTK_MENU(gtk_menu_new());
	hildon_window_set_menu(myWindow, myMenu);
	gtk_widget_show_all(GTK_WIDGET(myMenu));

	hildon_window_add_toolbar(myWindow, myToolbar);
	hildon_program_add_window(myProgram, myWindow);
	gtk_widget_show_all(GTK_WIDGET(myWindow));

	GtkSignalUtil::connectSignal(GTK_OBJECT(myWindow), "delete_event", GTK_SIGNAL_FUNC(applicationQuit), this);
	GtkSignalUtil::connectSignal(GTK_OBJECT(myWindow), "key_press_event", GTK_SIGNAL_FUNC(handleKey), this);
}

GtkApplicationWindow::~GtkApplicationWindow() {
	((GtkDialogManager&)GtkDialogManager::instance()).setMainWindow(0);
	for (std::map<const ZLApplication::Toolbar::ButtonItem*,ToolbarButton*>::iterator it = myToolbarButtons.begin(); it != myToolbarButtons.end(); ++it) {
		delete it->second;
	}
}

void GtkApplicationWindow::initMenu() {
	MenuBuilder(*this).processMenu(application().menubar());
}

GtkApplicationWindow::MenuBuilder::MenuBuilder(GtkApplicationWindow &window) : myWindow(window) {
	myMenuStack.push(myWindow.myMenu);
}

void GtkApplicationWindow::MenuBuilder::processSubmenuBeforeItems(ZLApplication::Menubar::Submenu &submenu) {
	GtkMenuItem *gtkItem = GTK_MENU_ITEM(gtk_menu_item_new_with_label(submenu.menuName().c_str()));
	GtkMenu *gtkSubmenu = GTK_MENU(gtk_menu_new());
	gtk_menu_item_set_submenu(gtkItem, GTK_WIDGET(gtkSubmenu));
	gtk_menu_shell_append(GTK_MENU_SHELL(myMenuStack.top()), GTK_WIDGET(gtkItem));
	gtk_widget_show_all(GTK_WIDGET(gtkItem));
	myMenuStack.push(gtkSubmenu);
}

void GtkApplicationWindow::MenuBuilder::processSubmenuAfterItems(ZLApplication::Menubar::Submenu&) {
	myMenuStack.pop();
}

void GtkApplicationWindow::MenuBuilder::processItem(ZLApplication::Menubar::PlainItem &item) {
	GtkMenuItem *gtkItem = GTK_MENU_ITEM(gtk_menu_item_new_with_label(item.name().c_str()));
	const int id = item.actionId();
	shared_ptr<ZLApplication::Action> action = myWindow.application().action(id);
	if (!action.isNull()) {
		GtkSignalUtil::connectSignal(GTK_OBJECT(gtkItem), "activate", GTK_SIGNAL_FUNC(menuActionSlot), &*action);
	}
	myWindow.myMenuItems[id] = gtkItem;
	gtk_menu_shell_append(GTK_MENU_SHELL(myMenuStack.top()), GTK_WIDGET(gtkItem));
	gtk_widget_show_all(GTK_WIDGET(gtkItem));
}

void GtkApplicationWindow::MenuBuilder::processSepartor(ZLApplication::Menubar::Separator&) {
	GtkMenuItem *gtkItem = GTK_MENU_ITEM(gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(myMenuStack.top()), GTK_WIDGET(gtkItem));
	gtk_widget_show_all(GTK_WIDGET(gtkItem));
}

void GtkApplicationWindow::handleKeyEventSlot(GdkEventKey *event) {
	application().doActionByKey(GtkKeyUtil::keyName(event));
}

void GtkApplicationWindow::setFullscreen(bool fullscreen) {
	if (fullscreen == myFullScreen) {
		return;
	}
	myFullScreen = fullscreen;

	if (myFullScreen) {
		gtk_window_fullscreen(GTK_WINDOW(myWindow));
		gtk_widget_hide(GTK_WIDGET(myToolbar));
	} else if (!myFullScreen) {
		gtk_window_unfullscreen(GTK_WINDOW(myWindow));
		gtk_widget_show(GTK_WIDGET(myToolbar));
	}
}

bool GtkApplicationWindow::isFullscreen() const {
	return myFullScreen;
}

void GtkApplicationWindow::close() {
	GtkSignalUtil::removeAllSignals();
	gtk_main_quit();
}

void GtkApplicationWindow::addToolbarItem(ZLApplication::Toolbar::ItemPtr item) {
	GtkToolItem *gtkItem;
	if (item->isButton()) {
		ZLApplication::Toolbar::ButtonItem &buttonItem = (ZLApplication::Toolbar::ButtonItem&)*item;

		ToolbarButton *toolbarButton = new ToolbarButton(buttonItem, *this);
		gtkItem = toolbarButton->toolItem();
		myToolbarButtons[&buttonItem] = toolbarButton;
	} else {
		gtkItem = gtk_separator_tool_item_new();
		gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(gtkItem), false);
	}
	gtk_toolbar_insert(myToolbar, gtkItem, -1);
	myToolItems[item] = gtkItem;
	gtk_widget_show_all(GTK_WIDGET(gtkItem));
}

void GtkApplicationWindow::setToolbarItemState(ZLApplication::Toolbar::ItemPtr item, bool visible, bool enabled) {
	GtkToolItem *toolItem = myToolItems[item];
	if (toolItem != 0) {
		gtk_tool_item_set_visible_horizontal(toolItem, visible);
		/*
		 * Not sure, but looks like gtk_widget_set_sensitive(WIDGET, false)
		 * does something strange if WIDGET is already insensitive.
		 */
		bool alreadyEnabled = GTK_WIDGET_STATE(toolItem) != GTK_STATE_INSENSITIVE;
		if (enabled != alreadyEnabled) {
			gtk_widget_set_sensitive(GTK_WIDGET(toolItem), enabled);
		}
	}
}

void GtkApplicationWindow::refresh() {
	ZLApplicationWindow::refresh();

	for (std::map<int,GtkMenuItem*>::iterator it = myMenuItems.begin(); it != myMenuItems.end(); it++) {
		int id = it->first;
		GtkWidget *gtkItem = GTK_WIDGET(it->second);
		if (application().isActionVisible(id)) {
			gtk_widget_show(gtkItem);
		} else {
			gtk_widget_hide(gtkItem);
		}
		bool alreadyEnabled = GTK_WIDGET_STATE(gtkItem) != GTK_STATE_INSENSITIVE;
		if (application().isActionEnabled(id) != alreadyEnabled) {
			gtk_widget_set_sensitive(gtkItem, !alreadyEnabled);
		}
	}
}

ZLViewWidget *GtkApplicationWindow::createViewWidget() {
	GtkViewWidget *viewWidget = new GtkViewWidget(&application(), (ZLViewWidget::Angle)application().AngleStateOption.value());
	gtk_container_add(GTK_CONTAINER(myWindow), viewWidget->area());
	GtkSignalUtil::connectSignal(GTK_OBJECT(viewWidget->area()), "expose_event", GTK_SIGNAL_FUNC(repaint), this);
	GtkSignalUtil::connectSignal(GTK_OBJECT(viewWidget->area()), "button_press_event", GTK_SIGNAL_FUNC(mousePressed), viewWidget);
	GtkSignalUtil::connectSignal(GTK_OBJECT(viewWidget->area()), "button_release_event", GTK_SIGNAL_FUNC(mouseReleased), viewWidget);
	GtkSignalUtil::connectSignal(GTK_OBJECT(viewWidget->area()), "motion_notify_event", GTK_SIGNAL_FUNC(mouseMoved), viewWidget);
	gtk_widget_show_all(GTK_WIDGET(myWindow));
	return viewWidget;
}

bool GtkApplicationWindow::isFullKeyboardControlSupported() const {
	return false;
}

void GtkApplicationWindow::grabAllKeys(bool) {
}

bool GtkApplicationWindow::isFingerTapEventSupported() const {
	return true;
}

bool GtkApplicationWindow::isMousePresented() const {
	return false;
}

bool GtkApplicationWindow::isKeyboardPresented() const {
	return false;
}

static void onGtkButtonPress(GtkWidget*, GdkEventButton*, gpointer data) {
	if (acceptAction()) {
		((GtkApplicationWindow::ToolbarButton*)data)->press(true);
	}
}

static void onGtkButtonRelease(GtkWidget*, GdkEventButton*, gpointer data) {
	if (acceptAction()) {
		((GtkApplicationWindow::ToolbarButton*)data)->press(false);
	}
}

GtkApplicationWindow::ToolbarButton::ToolbarButton(ZLApplication::Toolbar::ButtonItem &buttonItem, GtkApplicationWindow &window) : myButtonItem(buttonItem), myWindow(window) {
	myAction = myWindow.application().action(buttonItem.actionId());

	GdkPixbuf *filePixbuf = gdk_pixbuf_new_from_file((ZLApplication::ImageDirectory() + ZLApplication::PathDelimiter + ZLApplication::ApplicationName() + ZLApplication::PathDelimiter + buttonItem.iconName() + ".png").c_str(), 0);

	const int width = gdk_pixbuf_get_width(filePixbuf);
	const int height = gdk_pixbuf_get_height(filePixbuf);
	const int border = 4;
	const int line = 2;
	const GdkColorspace colorspace = gdk_pixbuf_get_colorspace(filePixbuf);
	const bool hasAlpha = gdk_pixbuf_get_has_alpha(filePixbuf);
	const int bitsPerSample = gdk_pixbuf_get_bits_per_sample(filePixbuf);

	const int w = width + 2 * border;
	const int h = height + 2 * border;
	GdkPixbuf *buttonPixbuf = gdk_pixbuf_new(colorspace, hasAlpha, bitsPerSample, w, h);
	gdk_pixbuf_fill(buttonPixbuf, 0);
	gdk_pixbuf_copy_area(filePixbuf, 0, 0, width, height, buttonPixbuf, border, border);
	myCurrentImage = GTK_IMAGE(gtk_image_new_from_pixbuf(buttonPixbuf));
	myReleasedImage = GTK_IMAGE(gtk_image_new_from_pixbuf(buttonPixbuf));

	GdkPixbuf *pressedButtonPixbuf = gdk_pixbuf_copy(buttonPixbuf);
	GdkPixbuf *top = gdk_pixbuf_new_subpixbuf(pressedButtonPixbuf, 0, 0, w, line);
	GdkPixbuf *bottom = gdk_pixbuf_new_subpixbuf(pressedButtonPixbuf, 0, h - line, w, line);
	GdkPixbuf *left = gdk_pixbuf_new_subpixbuf(pressedButtonPixbuf, 0, 0, line, h);
	GdkPixbuf *right = gdk_pixbuf_new_subpixbuf(pressedButtonPixbuf, w - line, 0, line, h);
	gdk_pixbuf_fill(top, 0x00007FFF);
	gdk_pixbuf_fill(bottom, 0x00007FFF);
	gdk_pixbuf_fill(left, 0x00007FFF);
	gdk_pixbuf_fill(right, 0x00007FFF);
	gdk_pixbuf_copy_area(filePixbuf, 0, 0, width, height, pressedButtonPixbuf, border, border);
	myPressedImage = GTK_IMAGE(gtk_image_new_from_pixbuf(pressedButtonPixbuf));

	gdk_pixbuf_unref(filePixbuf);
	gdk_pixbuf_unref(buttonPixbuf);
	gdk_pixbuf_unref(pressedButtonPixbuf);

	myEventBox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(myEventBox), GTK_WIDGET(myCurrentImage));
	GtkSignalUtil::connectSignal(GTK_OBJECT(myEventBox), "button_press_event", GTK_SIGNAL_FUNC(onGtkButtonPress), this);
	GtkSignalUtil::connectSignal(GTK_OBJECT(myEventBox), "button_release_event", GTK_SIGNAL_FUNC(onGtkButtonRelease), this);

	myToolItem = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(myToolItem), myEventBox);
	gtk_tool_item_set_homogeneous(myToolItem, false);
	gtk_tool_item_set_expand(myToolItem, false);
	GTK_WIDGET_UNSET_FLAGS(myToolItem, GTK_CAN_FOCUS);
}

void GtkApplicationWindow::ToolbarButton::forcePress(bool state) {
	gtk_image_set_from_pixbuf(myCurrentImage, gtk_image_get_pixbuf(state ? myPressedImage : myReleasedImage));
}

void GtkApplicationWindow::setToggleButtonState(const ZLApplication::Toolbar::ButtonItem &button) {
	myToolbarButtons[&button]->forcePress(button.isPressed());
}

void GtkApplicationWindow::ToolbarButton::press(bool state) {
	if (!myButtonItem.isToggleButton()) {
		forcePress(state);
		if (state) {
			return;
		}
	}
	myWindow.onButtonPress(myButtonItem);
}