// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "EditorWidgets.h"
#include "ContainerProperties.h"
#include "PropertyEditor.h"
#include "QtPropertyMetadata.h"
#include "QtPropertyReflection.h"
#include "Util/QtLayoutUtil.h"
#include "List/ItemEditorWidget.h"
#include "List/ListEditorWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QItemEditorFactory>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVector3D>

namespace skybolt {

DoubleVectorEditor::DoubleVectorEditor(const QStringList& componentLabels, QWidget* parent) :
	QWidget(parent)
{
	QGridLayout* layout = new QGridLayout;
	layout->setContentsMargins(0,0,0,0);
	setLayout(layout);
		
	for (const QString& label : componentLabels)
	{
		int index = (int)mEditors.size();
		mEditors.push_back(addDoubleEditor(*layout, label));
		connect(mEditors.back(), &QLineEdit::editingFinished, this, [this, index]() {
			componentEdited(index, mEditors[index]->text().toDouble());
		});
	}
}

void DoubleVectorEditor::setValue(int index, double value)
{
	mEditors[index]->blockSignals(true);
	mEditors[index]->setText(QString::number(value));
	mEditors[index]->blockSignals(false);
}


QVector3Editor::QVector3Editor(QtValue* value, const QStringList& componentLabels, QWidget* parent) :
	mValue(value),
	DoubleVectorEditor(componentLabels, parent)
{
	setValue(mValue->value().value<QVector3D>());

	connect(mValue, &QtValue::valueChanged, this, [=]() {
		setValue(mValue->value().value<QVector3D>());
	});
}

void QVector3Editor::setValue(const QVector3D& value)
{
	for (int i = 0; i < 3; ++i)
	{
		DoubleVectorEditor::setValue(i, value[i]);
	}
}

void QVector3Editor::componentEdited(int index, double value)
{
	QVector3D vec = mValue->value().value<QVector3D>();
	vec[index] = value;
	mValue->setValue(vec);
}

QLineEdit* createDoubleLineEdit(QWidget* parent, int decimalCount)
{
	QLineEdit* editor = new QLineEdit(parent);

	QDoubleValidator* validator = new QDoubleValidator();
	validator->setNotation(QDoubleValidator::ScientificNotation);
	validator->setDecimals(decimalCount);
	editor->setValidator(validator);

	return editor;
}

QLineEdit* addDoubleEditor(QGridLayout& layout, const QString& name)
{
	int row = layout.rowCount();
	layout.addWidget(new QLabel(name), row, 0);

	QLineEdit* editor = createDoubleLineEdit();
	layout.addWidget(editor, row, 1);

	return editor;
}


QWidget* createComboStringEditor(QtValue* value, QWidget* parent)
{
	QStringList optionNames = value->property(QtPropertyMetadataKeys::optionNames).toStringList();
	auto widget = new QComboBox(parent);
	widget->addItems(optionNames);
	widget->setCurrentText(value->value().toString());
	widget->setEditable(value->property(QtPropertyMetadataKeys::allowCustomOptions).toBool());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setCurrentText(value->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QComboBox::currentTextChanged, value, [=](const QString& newValue) {
		value->setValue(newValue);
	});

	return widget;
}

QWidget* createSingleLineStringEditor(QtValue* value, QWidget* parent)
{
	QLineEdit* widget = new QLineEdit(parent);
	widget->setText(value->value().toString());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setText(value->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QLineEdit::editingFinished, value, [=]() {
		value->setValue(widget->text());
	});

	return widget;
}

QWidget* createMultiLineStringEditor(QtValue* value, QWidget* parent)
{
	QTextEdit* widget = new QTextEdit(parent);
	widget->setText(value->value().toString());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setText(value->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QTextEdit::textChanged, value, [=]() {
		value->setValue(widget->toPlainText());
	});

	return widget;
}


QWidget* createStringEditor(QtValue* value, QWidget* parent)
{
	if (auto optionsNames = value->property(QtPropertyMetadataKeys::optionNames); optionsNames.isValid())
	{
		return createComboStringEditor(value, parent);
	}

	if (auto multiLine = value->property(QtPropertyMetadataKeys::multiLine); multiLine.isValid() && multiLine.toBool())
	{
		return createMultiLineStringEditor(value, parent);
	}
	else
	{
		return createSingleLineStringEditor(value, parent);
	}
}

QWidget* createIntEditor(QtValue* value, QWidget* parent)
{
	QSpinBox* widget = new QSpinBox(parent);
	widget->setMaximum(999999);
	widget->setValue(value->value().toInt());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setValue(value->value().toInt());
		widget->blockSignals(false);
	});

	QObject::connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), value, [=](int newValue) {
		value->setValue(newValue);
	});

	return widget;
}

QWidget* createEnumEditor(QtValue* value, QWidget* parent)
{
	QStringList optionNames = value->property(QtPropertyMetadataKeys::optionNames).toStringList();
	auto widget = new QComboBox(parent);
	widget->addItems(optionNames);
	widget->setCurrentIndex(value->value().toInt());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setCurrentIndex(value->value().toInt());
		widget->blockSignals(false);
	});

	QObject::connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), value, [=](int newValue) {
		value->setValue(newValue);
	});

	return widget;
}

bool shouldUseEnumEditor(const QtValue& property)
{
	if (property.property(QtPropertyMetadataKeys::optionNames).isValid())
	{
		if (auto allowCustomOptions = property.property(QtPropertyMetadataKeys::allowCustomOptions); allowCustomOptions.isValid())
		{
			if (allowCustomOptions.toBool())
			{
				// Can't use enum editor if custom options are allowed.
				return false;
			}
		}
		return true;
	}
	return false;
}

QWidget* createIntOrEnumEditor(QtValue* value, QWidget* parent)
{
	if (shouldUseEnumEditor(*value))
	{
		return createEnumEditor(value, parent);
	}
	else
	{
		return createIntEditor(value, parent);
	}
}

QWidget* createDoubleEditor(QtValue* value, QWidget* parent)
{
	constexpr int decimalCount = 9;
	QLineEdit* widget = createDoubleLineEdit(parent, decimalCount);

	auto widgetTextSetter = [widget](double value) {
		widget->setText(QString::number(value, 'g', decimalCount));
	};
	widgetTextSetter(value->value().toDouble());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value, widgetTextSetter]() {
		widget->blockSignals(true);
		widgetTextSetter(value->value().toDouble());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QLineEdit::editingFinished, value, [=]() {
		value->setValue(widget->text().toDouble());
	});

	return widget;
}

QWidget* createBoolEditor(QtValue* value, QWidget* parent)
{
	QAbstractButton* button;
	if (auto attributeType = value->property(QtPropertyMetadataKeys::representation); attributeType.isValid() && attributeType.toString() == QtPropertyRepresentations::toggleButton)
	{
		QVariant displayName = value->property(QtPropertyMetadataKeys::displayName);
		button = new QPushButton(displayName.isValid() ? displayName.toString() : "Execute", parent);
		button->setCheckable(true);
	}
	else
	{
		button = new QCheckBox(parent);
	}
	button->setChecked(value->value().toBool());

	QObject::connect(value, &QtValue::valueChanged, button, [button, value]() {
		button->blockSignals(true);
		button->setChecked(value->value().toBool());
		button->blockSignals(false);
	});

	QObject::connect(button, &QAbstractButton::toggled, value, [=](bool state) {
		value->setValue(state);
	});

	return button;
}

QWidget* createDateTimeEditor(QtValue* value, QWidget* parent)
{
	QDateTimeEdit* widget = new QDateTimeEdit(parent);
	widget->setDateTime(value->value().toDateTime());

	QObject::connect(value, &QtValue::valueChanged, widget, [widget, value]() {
		widget->blockSignals(true);
		widget->setDateTime(value->value().toDateTime());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QDateTimeEdit::dateTimeChanged, value, [=](const QDateTime& dateTime) {
		value->setValue(dateTime);
	});

	return widget;
}

QWidget* createOptionalVariantEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, QWidget* parent)
{
	auto optionalValue = value->value().value<QtOptionalValue>();
	if (auto i = factories.find(optionalValue.value->value().userType()); i != factories.end())
	{
		auto widget = new QWidget(parent);
		auto layout = new QVBoxLayout(widget);
		layout->setContentsMargins(0,0,0,0);
		widget->setLayout(layout);

		auto activateCheckbox = new QCheckBox("Enable", widget);
		activateCheckbox->setChecked(optionalValue.present);
		layout->addWidget(activateCheckbox);

		QWidget* valueEditorWidget = i->second(optionalValue.value.get(), parent);
		if (valueEditorWidget)
		{
			valueEditorWidget->setEnabled(optionalValue.present);
			layout->addWidget(valueEditorWidget);
		}

		QObject::connect(value, &QtValue::valueChanged, activateCheckbox, [activateCheckbox, valueEditorWidget, value]() {
			bool present = value->value().value<QtOptionalValue>().present;
			activateCheckbox->blockSignals(true);
			activateCheckbox->setChecked(present);
			activateCheckbox->blockSignals(false);

			if (valueEditorWidget)
			{
				valueEditorWidget->setEnabled(present);
			}
		});

		QObject::connect(activateCheckbox, &QCheckBox::stateChanged, value, [=](int state) {
			auto optionalValue = value->value().value<QtOptionalValue>();
			optionalValue.present = (state == Qt::Checked);
			value->setValue(QVariant::fromValue(optionalValue));

			if (valueEditorWidget)
			{
				valueEditorWidget->setEnabled(state == Qt::Checked);
			}
		});

		return widget;
	}
	return nullptr;
}

QWidget* createVector3DEditor(QtValue* value, QWidget* parent)
{
	return new QVector3Editor(value, { "x", "y", "z" }, parent);
}

class QtVectorValueEditor : public QWidget
{
public:
	QtVectorValueEditor(const PropertyEditorWidgetFactoryMap& factories, QPointer<QtValue> value, const ListEditorIcons& listEditorIcons, QWidget* parent = nullptr) :
		QWidget(parent),
		mFactories(factories),
		mValue(value)
	{
		assert(mValue);

		// Create widgets
		auto layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);

		mListWidget = new QListWidget(this);
		layout->addWidget(mListWidget);

		mListEditorWidget = new ListEditorWidget(listEditorIcons, this);
		layout->addWidget(mListEditorWidget);

		auto itemEditorContentWidget = new QWidget(this);
		mItemEditorWidget = new ItemEditorWidget(itemEditorContentWidget, this);
		mItemEditorLayout = new QVBoxLayout(itemEditorContentWidget);
		mItemEditorLayout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(mItemEditorWidget);

		updateListWidget();

		// Connect signals and slots
		QObject::connect(value, &QtValue::valueChanged, this, [this]() {
			updateListWidget();
			});

		QObject::connect(mListWidget, &QListWidget::itemSelectionChanged, this, [this] {
			updateControlButtonsState();
			updateEditorItem(getCurrentSelectedItem());
			});

		auto newItemValue = std::make_shared<QtValuePtr>();

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::setCreateItemModeEnabledChanged, this, [this](bool enabled) {
			mListEditorWidget->setEnabled(!enabled);
			});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemAddRequested, [this, newItemValue]() {
			if (!mValue)
			{
				return;
			}

			clearLayout(*mItemEditorLayout);

			(*newItemValue) = mValue->value().value<QtVectorValue>().itemFactory();
			if (*newItemValue)
			{
				updateEditorItem(*newItemValue);
				mItemEditorWidget->setCreateItemModeEnabled(true);
			}

		});

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::createItemAccepted, [this,newItemValue]() {
			if (!mValue)
			{
				return;
			}

			assert(newItemValue);
			auto propertyVector = mValue->value().value<QtVectorValue>();
			propertyVector.items.push_back(*newItemValue);
			mValue->setValue(QVariant::fromValue(propertyVector));
			newItemValue->reset();
			mItemEditorWidget->setCreateItemModeEnabled(false);

			updateEditorItem(nullptr);
		});

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::createItemCancelled, [this,newItemValue]() {
				assert(newItemValue);
				newItemValue->reset();
				mItemEditorWidget->setCreateItemModeEnabled(false);

				updateEditorItem(nullptr);
			});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemRemoveRequested, [this]() {
			if (!mValue)
			{
				return;
			}

			auto propertyVector = mValue->value().value<QtVectorValue>();
			int row = mListWidget->currentRow();
			if (row >= 0 && row < propertyVector.items.size())
			{
				propertyVector.items.erase(propertyVector.items.begin() + row);
				mValue->setValue(QVariant::fromValue(propertyVector));
			}
		});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemMoveUpRequested, [this]() {
			if (!mValue)
			{
				return;
			}

			auto propertyVector = mValue->value().value<QtVectorValue>();
			int row = mListWidget->currentRow();
			if (row >= 1 && row < propertyVector.items.size())
			{
				std::swap(propertyVector.items[row - 1], propertyVector.items[row]);
				mValue->setValue(QVariant::fromValue(propertyVector));
				mListWidget->setCurrentRow(row - 1);
			}
		});


		QObject::connect(mListEditorWidget, &ListEditorWidget::itemMoveDownRequested, [this]() {
			if (!mValue)
			{
				return;
			}

			auto propertyVector = mValue->value().value<QtVectorValue>();
			int row = mListWidget->currentRow();
			if (row >= 0 && row + 1 < propertyVector.items.size())
			{
				std::swap(propertyVector.items[row], propertyVector.items[row + 1]);
				mValue->setValue(QVariant::fromValue(propertyVector));
				mListWidget->setCurrentRow(row + 1);
			}
		});
	}

	void updateControlButtonsState()
	{
		int index = mListWidget->currentRow();
		mListEditorWidget->setMoveUpEnabled(index > 0);
		mListEditorWidget->setMoveDownEnabled((index >= 0) && (index + 1 < mListWidget->count()));
		mListEditorWidget->setRemoveEnabled(index >= 0);
	}

	void updateListWidget()
	{
		if (!mValue)
		{
			return;
		}

		QStringList newItems;
		auto propertyVector = mValue->value().value<QtVectorValue>();

		for (const QtValuePtr& itemValue : propertyVector.items)
		{
			QVariant displayName = itemValue->property(QtPropertyMetadataKeys::displayName);
			QString itemNumberText = QString::number(newItems.size() + 1);
			QString itemNameText = displayName.isValid() ? displayName.toString() : itemValue->value().toString();
			QString itemText = itemNameText.isEmpty() ? itemNumberText : (itemNumberText + ": " + itemNameText);
			newItems.push_back(itemText);
		}

		if (newItems != mPreviousItems || mPreviousItemValues != propertyVector.items)
		{
			// Update UI state
			int currentSelectedItem = mListWidget->currentRow(); // save selection before updating the list
			mListWidget->clear();
			mListWidget->addItems(newItems);
			mListWidget->setCurrentRow(currentSelectedItem); // restore selection after updating the list
			updateControlButtonsState();

			// Remove old property connections
			for (const auto& oldItemValue : mPreviousItemValues)
			{
				QObject::disconnect(oldItemValue.get(), &QtValue::valueChanged, mListWidget, nullptr);
			}

			// Connect new properties to trigger list widget updates when they change
			for (const auto& itemValue : propertyVector.items)
			{
				QObject::connect(itemValue.get(), &QtValue::valueChanged, mListWidget, [this]() {
					updateListWidget();
					});
			}

			mPreviousItemValues = propertyVector.items;
			mPreviousItems = newItems;
		}

		if (!mItemEditorWidget->isCreateItemModeEnabled())
		{
			updateEditorItem(getCurrentSelectedItem());
		}
	}

	void updateEditorItem(const QtValuePtr& itemValue)
	{
		if (itemValue != mPreviousSelectedItemValue)
		{
			clearLayout(*mItemEditorLayout);
		}

		if (itemValue && itemValue != mPreviousSelectedItemValue)
		{
			auto factoryIt = mFactories.find(itemValue->value().userType());
			if (factoryIt != mFactories.end())
			{
				if (QWidget* valueEditorWidget = factoryIt->second(itemValue.get(), this); valueEditorWidget)
				{
					mItemEditorLayout->addWidget(valueEditorWidget);
				}
			}
		}

		mPreviousSelectedItemValue = itemValue;
	}

	QtValuePtr getCurrentSelectedItem() const
	{
		if (!mValue)
		{
			return nullptr;
		}

		QtValuePtr itemValue;
		auto vectorValue = mValue->value().value<QtVectorValue>();
		int row = mListWidget->currentRow();
		if (row >= 0 && row < vectorValue.items.size())
		{
			return vectorValue.items.at(row);
		}
		return nullptr;
	}

private:
	PropertyEditorWidgetFactoryMap mFactories;
	QPointer<QtValue> mValue;
	QListWidget* mListWidget;
	ListEditorWidget* mListEditorWidget;
	ItemEditorWidget* mItemEditorWidget;
	QVBoxLayout* mItemEditorLayout;

	QtValuePtr mPreviousSelectedItemValue;
	QStringList mPreviousItems;
	std::vector<QtValuePtr> mPreviousItemValues;
};

QWidget* createVectorEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, const ListEditorIcons& listEditorIcons, QWidget* parent)
{
	return new QtVectorValueEditor(factories, value, listEditorIcons, parent);
}

template <typename KeyT, typename ValueT>
std::vector<ValueT> toValuesVector(const std::map<KeyT, ValueT>& m)
{
	std::vector<ValueT> r;
	for (const auto& i : m)
	{
		r.push_back(i.second);
	}
	return r;
}

QWidget* createStructEditor(const PropertyEditorWidgetFactoryMap& factories, QtValue* value, QWidget* parent)
{
	auto propertyEditor = new PropertyEditor(std::make_shared<PropertyEditorWidgetFactoryMap>(factories), parent);

	auto structValue = value->value().value<QtStructValue>();

	auto model = std::make_shared<PropertiesModel>();
	for (const auto& childProperty : structValue.items)
	{
		model->addProperty(childProperty);
	}

	propertyEditor->setModel(model);

	return propertyEditor;
}

} // namespace skybolt