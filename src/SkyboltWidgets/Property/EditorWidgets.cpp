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


QVector3PropertyEditor::QVector3PropertyEditor(QtProperty* property, const QStringList& componentLabels, QWidget* parent) :
	mProperty(property),
	DoubleVectorEditor(componentLabels, parent)
{
	setValue(mProperty->value().value<QVector3D>());

	connect(property, &QtProperty::valueChanged, this, [=]() {
		setValue(mProperty->value().value<QVector3D>());
	});
}

void QVector3PropertyEditor::setValue(const QVector3D& value)
{
	for (int i = 0; i < 3; ++i)
	{
		DoubleVectorEditor::setValue(i, value[i]);
	}
}

void QVector3PropertyEditor::componentEdited(int index, double value)
{
	QVector3D vec = mProperty->value().value<QVector3D>();
	vec[index] = value;
	mProperty->setValue(vec);
}


QLineEdit* createDoubleLineEdit(QWidget* parent)
{
	QLineEdit* editor = new QLineEdit(parent);

	QDoubleValidator* validator = new QDoubleValidator();
	validator->setNotation(QDoubleValidator::StandardNotation);
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


QWidget* createComboStringEditor(QtProperty* property, QWidget* parent)
{
	QStringList optionNames = property->property(QtPropertyMetadataKeys::optionNames).toStringList();
	auto widget = new QComboBox(parent);
	widget->addItems(optionNames);
	widget->setCurrentText(property->value().toString());
	widget->setEditable(property->property(QtPropertyMetadataKeys::allowCustomOptions).toBool());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setCurrentText(property->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QComboBox::currentTextChanged, property, [=](const QString& newValue) {
		property->setValue(newValue);
	});

	return widget;
}

QWidget* createSingleLineStringEditor(QtProperty* property, QWidget* parent)
{
	QLineEdit* widget = new QLineEdit(parent);
	widget->setText(property->value().toString());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setText(property->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QLineEdit::editingFinished, property, [=]() {
		property->setValue(widget->text());
	});

	return widget;
}

QWidget* createMultiLineStringEditor(QtProperty* property, QWidget* parent)
{
	QTextEdit* widget = new QTextEdit(parent);
	widget->setText(property->value().toString());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setText(property->value().toString());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QTextEdit::textChanged, property, [=]() {
		property->setValue(widget->toPlainText());
	});

	return widget;
}


QWidget* createStringEditor(QtProperty* property, QWidget* parent)
{
	if (auto value = property->property(QtPropertyMetadataKeys::optionNames); value.isValid())
	{
		return createComboStringEditor(property, parent);
	}

	if (auto value = property->property(QtPropertyMetadataKeys::multiLine); value.isValid() && value.toBool())
	{
		return createMultiLineStringEditor(property, parent);
	}
	else
	{
		return createSingleLineStringEditor(property, parent);
	}
}

QWidget* createIntEditor(QtProperty* property, QWidget* parent)
{
	QSpinBox* widget = new QSpinBox(parent);
	widget->setMaximum(999999);
	widget->setValue(property->value().toInt());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setValue(property->value().toInt());
		widget->blockSignals(false);
	});

	QObject::connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), property, [=](int newValue) {
		property->setValue(newValue);
	});

	return widget;
}

QWidget* createEnumEditor(QtProperty* property, QWidget* parent)
{
	QStringList optionNames = property->property(QtPropertyMetadataKeys::optionNames).toStringList();
	auto widget = new QComboBox(parent);
	widget->addItems(optionNames);
	widget->setCurrentIndex(property->value().toInt());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setCurrentIndex(property->value().toInt());
		widget->blockSignals(false);
	});

	QObject::connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property, [=](int newValue) {
		property->setValue(newValue);
	});

	return widget;
}

bool shouldUseEnumEditor(const QtProperty& property)
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

QWidget* createIntOrEnumEditor(QtProperty* property, QWidget* parent)
{
	if (shouldUseEnumEditor(*property))
	{
		return createEnumEditor(property, parent);
	}
	else
	{
		return createIntEditor(property, parent);
	}
}

QWidget* createDoubleEditor(QtProperty* property, QWidget* parent)
{
	QLineEdit* widget = createDoubleLineEdit(parent);

	auto widgetTextSetter = [widget](double value) {
		widget->setText(QString::number(value, 'f', 4));
	};
	widgetTextSetter(property->value().toDouble());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property, widgetTextSetter]() {
		widget->blockSignals(true);
		widgetTextSetter(property->value().toDouble());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QLineEdit::editingFinished, property, [=]() {
		property->setValue(widget->text().toDouble());
	});

	return widget;
}

QWidget* createBoolEditor(QtProperty* property, QWidget* parent)
{
	QAbstractButton* button;
	if (auto attributeType = property->property(QtPropertyMetadataKeys::representation); attributeType.isValid() && attributeType.toString() == QtPropertyRepresentations::toggleButton)
	{
		button = new QPushButton(property->displayName, parent);
		button->setCheckable(true);
	}
	else
	{
		button = new QCheckBox(parent);
	}
	button->setChecked(property->value().toBool());

	QObject::connect(property, &QtProperty::valueChanged, button, [button, property]() {
		button->blockSignals(true);
		button->setChecked(property->value().toBool());
		button->blockSignals(false);
	});

	QObject::connect(button, &QAbstractButton::toggled, property, [=](int state) {
		property->setValue((bool)state);
	});

	return button;
}

QWidget* createDateTimeEditor(QtProperty* property, QWidget* parent)
{
	QDateTimeEdit* widget = new QDateTimeEdit(parent);
	widget->setDateTime(property->value().toDateTime());

	QObject::connect(property, &QtProperty::valueChanged, widget, [widget, property]() {
		widget->blockSignals(true);
		widget->setDateTime(property->value().toDateTime());
		widget->blockSignals(false);
	});

	QObject::connect(widget, &QDateTimeEdit::dateTimeChanged, property, [=](const QDateTime& dateTime) {
		property->setValue(dateTime);
	});

	return widget;
}

QWidget* createOptionalVariantEditor(const PropertyEditorWidgetFactoryMap& factories, QtProperty* property, QWidget* parent)
{
	auto optionalProperty = property->value().value<OptionalProperty>();
	if (auto i = factories.find(optionalProperty.property->value().userType()); i != factories.end())
	{
		auto widget = new QWidget(parent);
		auto layout = new QVBoxLayout(widget);
		layout->setContentsMargins(0,0,0,0);
		widget->setLayout(layout);

		auto activateCheckbox = new QCheckBox("Enable", widget);
		activateCheckbox->setChecked(optionalProperty.present);
		layout->addWidget(activateCheckbox);

		QWidget* valueEditorWidget = i->second(optionalProperty.property.get(), parent);
		if (valueEditorWidget)
		{
			valueEditorWidget->setEnabled(optionalProperty.present);
			layout->addWidget(valueEditorWidget);
		}

		QObject::connect(property, &QtProperty::valueChanged, activateCheckbox, [activateCheckbox, valueEditorWidget, property]() {
			bool present = property->value().value<OptionalProperty>().present;
			activateCheckbox->blockSignals(true);
			activateCheckbox->setChecked(present);
			activateCheckbox->blockSignals(false);

			if (valueEditorWidget)
			{
				valueEditorWidget->setEnabled(present && property->enabled);
			}
		});

		QObject::connect(activateCheckbox, &QCheckBox::stateChanged, property, [=](bool value) {
			auto optionalProperty = property->value().value<OptionalProperty>();
			optionalProperty.present = value;
			property->setValue(QVariant::fromValue(optionalProperty));

			if (valueEditorWidget)
			{
				valueEditorWidget->setEnabled(value && property->enabled);
			}
		});

		return widget;
	}
	return nullptr;
}

QWidget* createVector3DEditor(QtProperty* property, QWidget* parent)
{
	return new QVector3PropertyEditor(property, { "x", "y", "z" }, parent);
}

class PropertyVectorEditor : public QWidget
{
public:
	PropertyVectorEditor(const PropertyEditorWidgetFactory& factory, QPointer<QtProperty> property, const ListEditorIcons& listEditorIcons, QWidget* parent = nullptr) :
		QWidget(parent),
		mFactory(factory),
		mProperty(property)
	{
		assert(mProperty);

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
		QObject::connect(property, &QtProperty::valueChanged, this, [this]() {
			updateListWidget();
			});

		QObject::connect(mListWidget, &QListWidget::itemSelectionChanged, this, [this] {
			updateControlButtonsState();
			updateEditorItem(getCurrentSelectedItem());
			});

		auto newItemProperty = std::make_shared<QtPropertyPtr>();

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::setCreateItemModeEnabledChanged, this, [this](bool enabled) {
			mListEditorWidget->setEnabled(!enabled);
			});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemAddRequested, [this, newItemProperty]() {
			if (!mProperty)
			{
				return;
			}

			clearLayout(*mItemEditorLayout);

			QVariant defaultValue = mProperty->value().value<PropertyVector>().itemDefaultValue;
			(*newItemProperty) = createQtProperty(/* displayName */"", defaultValue);

			if (newItemProperty)
			{
				updateEditorItem(*newItemProperty);
				mItemEditorWidget->setCreateItemModeEnabled(true);
			}
		});

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::createItemAccepted, [this,newItemProperty]() {
			if (!mProperty)
			{
				return;
			}

			assert(newItemProperty);
			auto propertyVector = mProperty->value().value<PropertyVector>();
			propertyVector.items.push_back(*newItemProperty);
			mProperty->setValue(QVariant::fromValue(propertyVector));
			newItemProperty->reset();
			mItemEditorWidget->setCreateItemModeEnabled(false);

			updateEditorItem(nullptr);
		});

		QObject::connect(mItemEditorWidget, &ItemEditorWidget::createItemCancelled, [this,newItemProperty]() {
				assert(newItemProperty);
				newItemProperty->reset();
				mItemEditorWidget->setCreateItemModeEnabled(false);

				updateEditorItem(nullptr);
			});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemRemoveRequested, [this]() {
			if (!mProperty)
			{
				return;
			}

			auto propertyVector = mProperty->value().value<PropertyVector>();
			int row = mListWidget->currentRow();
			if (row >= 0 && row < propertyVector.items.size())
			{
				propertyVector.items.erase(propertyVector.items.begin() + row);
				mProperty->setValue(QVariant::fromValue(propertyVector));
			}
		});

		QObject::connect(mListEditorWidget, &ListEditorWidget::itemMoveUpRequested, [this]() {
			if (!mProperty)
			{
				return;
			}

			auto propertyVector = mProperty->value().value<PropertyVector>();
			int row = mListWidget->currentRow();
			if (row >= 1 && row < propertyVector.items.size())
			{
				std::swap(propertyVector.items[row - 1], propertyVector.items[row]);
				mProperty->setValue(QVariant::fromValue(propertyVector));
				mListWidget->setCurrentRow(row - 1);
			}
		});


		QObject::connect(mListEditorWidget, &ListEditorWidget::itemMoveDownRequested, [this]() {
			if (!mProperty)
			{
				return;
			}

			auto propertyVector = mProperty->value().value<PropertyVector>();
			int row = mListWidget->currentRow();
			if (row >= 0 && row + 1 < propertyVector.items.size())
			{
				std::swap(propertyVector.items[row], propertyVector.items[row + 1]);
				mProperty->setValue(QVariant::fromValue(propertyVector));
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
		if (!mProperty)
		{
			return;
		}

		QStringList newItems;
		auto propertyVector = mProperty->value().value<PropertyVector>();
		for (const auto& itemProperty : propertyVector.items)
		{
			newItems.push_back(itemProperty->displayName.isEmpty() ? itemProperty->value().toString() : itemProperty->displayName);
		}

		bool asdf = newItems != mPreviousItems;
		bool bar = mPreviousItemProperties != propertyVector.items;
		if (newItems != mPreviousItems || mPreviousItemProperties != propertyVector.items)
		{
			// Update UI state
			int currentSelectedItem = mListWidget->currentRow(); // save selection before updating the list
			mListWidget->clear();
			mListWidget->addItems(newItems);
			mListWidget->setCurrentRow(currentSelectedItem); // restore selection after updating the list
			updateControlButtonsState();

			// Remove old property connections
			for (const auto& oldItemProperty : mPreviousItemProperties)
			{
				QObject::disconnect(oldItemProperty.get(), &QtProperty::valueChanged, mListWidget, nullptr);
			}

			// Connect new properties to trigger list widget updates when they change
			for (const auto& itemProperty : propertyVector.items)
			{
				QObject::connect(itemProperty.get(), &QtProperty::valueChanged, mListWidget, [this]() {
					updateListWidget();
					});
			}

			mPreviousItemProperties = propertyVector.items;
			mPreviousItems = newItems;
		}

		if (!mItemEditorWidget->isCreateItemModeEnabled())
		{
			updateEditorItem(getCurrentSelectedItem());
		}
	}

	void updateEditorItem(const QtPropertyPtr& itemProperty)
	{
		if (itemProperty != mPreviousSelectedItemProperty)
		{
			clearLayout(*mItemEditorLayout);
		}

		if (itemProperty && itemProperty != mPreviousSelectedItemProperty)
		{
			if (QWidget* valueEditorWidget = mFactory(itemProperty.get(), this); valueEditorWidget)
			{
				mItemEditorLayout->addWidget(valueEditorWidget);
			}
		}

		mPreviousSelectedItemProperty = itemProperty;
	}

	QtPropertyPtr getCurrentSelectedItem() const
	{
		if (!mProperty)
		{
			return nullptr;
		}

		QtPropertyPtr itemProperty;
		auto propertyVector = mProperty->value().value<PropertyVector>();
		int row = mListWidget->currentRow();
		if (row >= 0 && row < propertyVector.items.size())
		{
			return propertyVector.items.at(row);
		}
		return nullptr;
	}

private:
	PropertyEditorWidgetFactory mFactory;
	QPointer<QtProperty> mProperty;
	QListWidget* mListWidget;
	ListEditorWidget* mListEditorWidget;
	ItemEditorWidget* mItemEditorWidget;
	QVBoxLayout* mItemEditorLayout;

	QtPropertyPtr mPreviousSelectedItemProperty;
	QStringList mPreviousItems;
	std::vector<QtPropertyPtr> mPreviousItemProperties;
};

QWidget* createPropertyVectorEditor(const PropertyEditorWidgetFactoryMap& factories, QtProperty* property, const ListEditorIcons& listEditorIcons, QWidget* parent)
{
	auto propertyVector = property->value().value<PropertyVector>();
	if (auto i = factories.find(propertyVector.itemDefaultValue.userType()); i != factories.end())
	{
		return new PropertyVectorEditor(i->second, property, listEditorIcons, parent);
	}
	else
	{
		return nullptr;
	}
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

QWidget* createPropertyTupleEditor(const PropertyEditorWidgetFactoryMap& factories, QtProperty* property, QWidget* parent)
{
	auto propertyEditor = new PropertyEditor(std::make_shared<PropertyEditorWidgetFactoryMap>(factories), parent);

	auto propertyTuple = property->value().value<PropertyTuple>();

	auto model = std::make_shared<PropertiesModel>();
	for (const auto& childProperty : propertyTuple.items)
	{
		model->addProperty(childProperty);
	}

	propertyEditor->setModel(model);

	return propertyEditor;
}

} // namespace skybolt