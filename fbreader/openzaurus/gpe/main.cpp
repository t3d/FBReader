/*
 * FBReader -- electronic book reader
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
#include <gpe/init.h>

#include <abstract/ZLEncodingConverter.h>
#include <unix/ZLUnixFSManager.h>
#include <unix/ZLUnixTime.h>
#include <abstract/XMLOptions.h>
#include <gtk/GtkDialogManager.h>
#include <gtk/GtkImageManager.h>
#include <gtk/GtkDeviceInfo.h>

#include "GtkFBReader.h"
#include "../../common/Files.h"

int main(int argc, char **argv) {
	gtk_disable_setlocale();
  if (!gpe_application_init (&argc, &argv)) {
		return 1;
	}

	ZLUnixFSManager::createInstance();
	ZLUnixTimeManager::createInstance();
	GtkDialogManager::createInstance();
	GtkImageManager::createInstance();
	((GtkDialogManager&)GtkDialogManager::instance()).setPixmapPath(GtkFBReader::ImageDirectory);
	ZLEncodingConverter::setEncodingDescriptionPath(Files::PathPrefix + "encodings");
	XMLOptions::createInstance("FBReader");
	GtkDeviceInfo::createInstance();

	GtkFBReader *reader = new GtkFBReader(argc == 1 ? std::string() : argv[1]);	// MSS: use the first argument that gtk did not consume

	((GtkDialogManager&)GtkDialogManager::instance()).setMainWindow(reader->getMainWindow());

	gtk_main();

	GtkDeviceInfo::deleteInstance();
	GtkImageManager::deleteInstance();
	GtkDialogManager::deleteInstance();
	XMLOptions::deleteInstance();
	ZLUnixFSManager::deleteInstance();
	ZLUnixTimeManager::deleteInstance();

	return 0;
}

// vim:ts=2:sw=2:noet