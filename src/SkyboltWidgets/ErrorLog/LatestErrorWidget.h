// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <QIcon>
#include <QWidget>

class QToolButton;

namespace skybolt {

class ErrorLogModel;

class LatestErrorWidget : public QWidget
{
	Q_OBJECT

public:
	LatestErrorWidget(ErrorLogModel* model, QIcon icon = createDefaultIcon(), QWidget* parent = nullptr);
	
	~LatestErrorWidget() override = default;

	QToolButton* getExpandButton() const { return mExpandButton; }

	static QIcon createDefaultIcon();

private:
	QToolButton* mExpandButton;
};

} // namespace skybolt