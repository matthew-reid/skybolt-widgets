// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <QWidget>

class QDialogButtonBox;

namespace skybolt {

class ItemEditorWidget : public QWidget
{
	Q_OBJECT
public:
	ItemEditorWidget(QWidget* contentWidget, QWidget* parent = nullptr);

	bool isCreateItemModeEnabled() const;

public Q_SLOTS:
	void setCreateItemModeEnabled(bool enabled);

Q_SIGNALS:
	void setCreateItemModeEnabledChanged(bool enabled);
	void createItemAccepted();
	void createItemCancelled();

private:
	QDialogButtonBox* mCreateItemButtonBox;
	bool mCreateItemModeEnabled = false;
};

} // namespace skybolt