/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "MainView.h"
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QDir>
#include <QScreen>

#define MAJOR_VERSION 1
#define MINOR_VERSION 15
#define BUILD_VERSION 168

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QString version = QString::number(MAJOR_VERSION) + "." + QString::number(MINOR_VERSION) + "." + QString::number(BUILD_VERSION);

  // Setup the software details used by the settings system
  a.setOrganizationName("Blueprint Subsea");
  a.setOrganizationDomain("www.blueprintsubsea.com");
  a.setApplicationName("Oculus Demo");
  a.setWindowIcon(QIcon(":/OculusViewer.ico"));
  a.setApplicationVersion(version);

  // Set the settinsg to be in INI format in the directory that the application was run from
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, a.applicationDirPath());

  QSettings settings;
  // Read the stylesheet to be applied to the application
  QString theme = settings.value("Style", "dark").toString();

  // Creat the main view
  MainView w;
  w.SetTheme(theme);
  // Give it a name so we can apply CSS to it
  w.setObjectName("MainWindow");
  w.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

  // Set the minimum size for the window
  w.setMinimumSize(QSize(1280, 768));

  // Try to solve the second screen issue by forcing the application to load
  // on the first screen
  QScreen *s = a.primaryScreen();
  QRect rect = s->geometry();
  rect.setWidth(rect.width() * 0.7);
  rect.setHeight(rect.height() * 0.7);
  w.setGeometry(rect);

  // Show the window maximized (showNormal will use the geometry from above)
  w.showMaximized();

  if (argc > 1) {
	  // Get the file name passed into the application
	  QString fileName = argv[1];
	  w.m_modeCtrls.OpenFileEx(fileName);
  }

  return a.exec();
}
