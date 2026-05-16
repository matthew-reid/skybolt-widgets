// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include "PropertyEditorWidgetFactory.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QStringList>
#include <QVector3D>
#include <QWidget>

namespace skybolt {

struct ListEditorIcons;
class QtProperty;

class DoubleVectorEditor : public QWidget
{
public:
	DoubleVectorEditor(const QStringList& componentLabels, QWidget* parent = nullptr);

protected:
	void setValue(int index, double value);

	virtual void componentEdited(int index, double value) = 0;

private:
	std::vector<QLineEdit*> mEditors;
};

class QVector3Editor : public DoubleVectorEditor
{
public:
	QVector3Editor(QtValue* value, const QStringList& componentLabels, QWidget* parent = nullptr);

protected:
	void setValue(const QVector3D& value);

	void componentEdited(int index, double value) override;

private:
	QtValue* mValue;
};

QLineEdit* createDoubleLineEdit(QWidget* parent = nullptr);

QLineEdit* addDoubleEditor(QGridLayout& layout, const QString& name);

QWidget* createComboStringEditor(QtValue* value, QWidget* parent);

QWidget* createSingleLineStringEditor(QtValue* value, QWidget* parent);

QWidget* createMultiLineStringEditor(QtValue* value, QWidget* parent);

QWidget* createStringEditor(QtValue* value, QWidget* parent);

QWidget* createIntEditor(QtValue* value, QWidget* parent);

QWidget* createEnumEditor(QtValue* value, QWidget* parent);

bool shouldUseEnumEditor(const QtValue& value);

QWidget* createIntOrEnumEditor(QtValue* value, QWidget* parent);

QWidget* createDoubleEditor(QtValue* value, QWidget* parent);

QWidget* createBoolEditor(QtValue* value, QWidget* parent);

QWidget* createDateTimeEditor(QtValue* value, QWidget* parent);

QWidget* createVector3DEditor(QtValue* value, QWidget* parent);

QWidget* createOptionalVariantEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, QWidget* parent);

QWidget* createVectorEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, const ListEditorIcons& listEditorIcons, QWidget* parent);

QWidget* createStructEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, QWidget* parent);

} // namespace skybolt