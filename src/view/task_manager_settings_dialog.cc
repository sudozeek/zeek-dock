/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2022 Viet Dang (dangvd@gmail.com)
 *
 * Crystal Dock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Crystal Dock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Crystal Dock.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "task_manager_settings_dialog.h"
#include "ui_task_manager_settings_dialog.h"

#include <QGuiApplication>

#include <display/window_system.h>

namespace crystaldock {

TaskManagerSettingsDialog::TaskManagerSettingsDialog(QWidget* parent, MultiDockModel* model) :
  QDialog(parent),
  ui(new Ui::TaskManagerSettingsDialog),
  model_(model),
  isSingleScreen_(true) {
  ui->setupUi(this);

  // Adjust the UI for single/multi-screen.
  isSingleScreen_ = (WindowSystem::screens().size() == 1);
  ui->showCurrentScreenOnly->setVisible(!isSingleScreen_);

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));

  loadData();
}

TaskManagerSettingsDialog::~TaskManagerSettingsDialog() {
  delete ui;
}

void TaskManagerSettingsDialog::accept() {
  QDialog::accept();
  saveData();
}

void TaskManagerSettingsDialog::buttonClicked(QAbstractButton* button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  } else if (role == QDialogButtonBox::ResetRole) {
    resetData();
  }
}

void TaskManagerSettingsDialog::loadData() {
  ui->showCurrentDesktopOnly->setChecked(model_->currentDesktopTasksOnly());
  if (!isSingleScreen_) {
    ui->showCurrentScreenOnly->setChecked(model_->currentScreenTasksOnly());
  }
  ui->groupTasksByApplication->setChecked(model_->groupTasksByApplication());
  ui->clickToMinimize->setChecked(model_->clickToMinimize());
  ui->scrollToCycle->setChecked(model_->scrollToCycle());

  // Window preview settings.
  ui->showWindowPreviews->setChecked(model_->showWindowPreviews());
  ui->previewDelayMs->setValue(model_->previewDelayMs());
  ui->previewSizePx->setValue(model_->previewSizePx());
  ui->previewShowCloseButton->setChecked(model_->previewShowCloseButton());
  ui->previewSizeFromDpi->setChecked(model_->previewSizeFromDpi());
}

void TaskManagerSettingsDialog::resetData() {
  ui->showCurrentDesktopOnly->setChecked(kDefaultCurrentDesktopTasksOnly);
  if (!isSingleScreen_) {
    ui->showCurrentScreenOnly->setChecked(kDefaultCurrentScreenTasksOnly);
  }
  ui->groupTasksByApplication->setChecked(kDefaultGroupTasksByApplication);
  ui->clickToMinimize->setChecked(kDefaultClickToMinimize);
  ui->scrollToCycle->setChecked(kDefaultScrollToCycle);

  ui->showWindowPreviews->setChecked(kDefaultShowWindowPreviews);
  ui->previewDelayMs->setValue(kDefaultPreviewDelayMs);
  ui->previewSizePx->setValue(kDefaultPreviewSizePx);
  ui->previewShowCloseButton->setChecked(kDefaultPreviewShowCloseButton);
  ui->previewSizeFromDpi->setChecked(kDefaultPreviewSizeFromDpi);
}

void TaskManagerSettingsDialog::saveData() {
  model_->setCurrentDesktopTasksOnly(ui->showCurrentDesktopOnly->isChecked());
  if (!isSingleScreen_) {
    model_->setCurrentScreenTasksOnly(ui->showCurrentScreenOnly->isChecked());
  }
  model_->setGroupTasksByApplication(ui->groupTasksByApplication->isChecked());
  model_->setClickToMinimize(ui->clickToMinimize->isChecked());
  model_->setScrollToCycle(ui->scrollToCycle->isChecked());

  // Window preview settings.
  model_->setShowWindowPreviews(ui->showWindowPreviews->isChecked());
  model_->setPreviewDelayMs(ui->previewDelayMs->value());
  model_->setPreviewSizePx(ui->previewSizePx->value());
  model_->setPreviewShowCloseButton(ui->previewShowCloseButton->isChecked());
  model_->setPreviewSizeFromDpi(ui->previewSizeFromDpi->isChecked());

  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
