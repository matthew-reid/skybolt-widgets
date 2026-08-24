// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "CollapsiblePanelWidget.h"

#include <QScrollArea>
#include <QStringLiteral>

namespace skybolt {

// We need to set both the background and border here according to https://doc.qt.io/qt-6/stylesheet-reference.html:
// "If you only set a background-color on a QPushButton, the background may not appear unless you set the border property to some value.
// This is because, by default, the QPushButton draws a native border which completely overlaps the background-color."
const QString collapsiblePanelWidgetStyleSheet = "QToolButton:checked { background-color: palette(mid); border: none; }";

CollapsiblePanelWidget::CollapsiblePanelWidget(const QString& title, QWidget* contentWidget, QWidget* parent) :
    QWidget(parent),
    mContentWidget(contentWidget),
    mToggleButton(new QToolButton(this))
{
    mToggleButton->setText(title);
    mToggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mToggleButton->setCheckable(true);
    mToggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mToggleButton->setStyleSheet(collapsiblePanelWidgetStyleSheet);

    mContentLayout = new QVBoxLayout();
    mContentLayout->setContentsMargins(4, 0, 4, 0);
    mContentLayout->addWidget(mContentWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mToggleButton);
    mainLayout->addLayout(mContentLayout);

    connect(mToggleButton, &QAbstractButton::toggled, this, &CollapsiblePanelWidget::setExpanded);
    setExpanded(true);
}

void CollapsiblePanelWidget::setExpanded(bool expanded)
{
    mToggleButton->setChecked(expanded);
    mToggleButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    mContentWidget->setVisible(expanded);

    Q_EMIT expandedStateChanged(expanded);
}

void CollapsiblePanelWidget::setContentMargins(int left, int top, int right, int bottom)
{
    mContentLayout->setContentsMargins(left, top, right, bottom);
}

} // namespace skybolt