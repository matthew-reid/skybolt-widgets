// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <catch2/catch.hpp>
#include <SkyboltWidgets/Property/ContainerProperties.h>
#include <SkyboltWidgets/Property/EditorWidgets.h>
#include <SkyboltWidgets/Property/QtProperty.h>
#include <SkyboltWidgets/List/ListEditorWidget.h>
#include <SkyboltWidgets/List/ItemEditorWidget.h>

#include <QApplication>
#include <QListWidget>
#include <QSignalSpy>

using namespace skybolt;

namespace {

struct PropertyVectorEditorFixture
{
	PropertyVectorEditorFixture()
	{
		// Create a PropertyVector with int items
		PropertyVector propertyVector;
		propertyVector.itemDefaultValue = QVariant(0);
		property = createQtProperty("test", QVariant::fromValue(propertyVector));

		// Create factory map with int editor
		PropertyEditorWidgetFactoryMap factories;
		factories[qMetaTypeId<int>()] = [](QtProperty* prop, QWidget* parent) -> QWidget* {
			return createIntEditor(prop, parent);
		};

		ListEditorIcons icons = createDefaultListEditorIcons();
		widget.reset(createPropertyVectorEditor(factories, property.get(), icons, nullptr));
		REQUIRE(widget != nullptr);

		listWidget = widget->findChild<QListWidget*>();
		REQUIRE(listWidget != nullptr);

		listEditorWidget = widget->findChild<ListEditorWidget*>();
		REQUIRE(listEditorWidget != nullptr);
	}

	void addItem(int value)
	{
		auto pv = property->value().value<PropertyVector>();
		pv.items.push_back(createQtProperty("", QVariant(value)));
		property->setValue(QVariant::fromValue(pv));
	}

	QtPropertyPtr property;
	std::unique_ptr<QWidget> widget;
	QListWidget* listWidget = nullptr;
	ListEditorWidget* listEditorWidget = nullptr;
};

} // namespace

TEST_CASE_METHOD(PropertyVectorEditorFixture, "PropertyVectorEditor reflects property items to list widget")
{
	CHECK(listWidget->count() == 0);

	addItem(10);
	CHECK(listWidget->count() == 1);
	CHECK(listWidget->item(0)->text() == "10");

	addItem(20);
	CHECK(listWidget->count() == 2);
	CHECK(listWidget->item(1)->text() == "20");
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "PropertyVectorEditor updates list widget when item value changes")
{
	addItem(10);
	CHECK(listWidget->item(0)->text() == "10");

	auto pv = property->value().value<PropertyVector>();
	pv.items[0]->setValue(42);

	// The item property valueChanged signal triggers updateListWidget
	CHECK(listWidget->item(0)->text() == "42");
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "item added to PropertyVector when ListEditorWidget accepts item creation")
{
	// Emit itemAddRequested to trigger add flow
	emit listEditorWidget->itemAddRequested();

	// At this point the item editor widget should be in create mode
	// Accept the item creation
	auto itemEditorWidget = widget->findChild<ItemEditorWidget*>();
	REQUIRE(itemEditorWidget != nullptr);
	REQUIRE(itemEditorWidget->isCreateItemModeEnabled());

	emit itemEditorWidget->createItemAccepted();

	auto pv = property->value().value<PropertyVector>();
	CHECK(pv.items.size() == 1);
	CHECK(pv.items[0]->value().toInt() == 0); // default value
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "Item removed from PropertyVector when ListEditorWidget requests item removal")
{
	addItem(10);
	addItem(20);
	CHECK(listWidget->count() == 2);

	listWidget->setCurrentRow(0);
	emit listEditorWidget->itemRemoveRequested();

	auto pv = property->value().value<PropertyVector>();
	REQUIRE(pv.items.size() == 1);
	CHECK(pv.items[0]->value().toInt() == 20);
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "Item moved up in PropertyVector when ListEditorWidget move up requested")
{
	addItem(10);
	addItem(20);
	addItem(30);

	listWidget->setCurrentRow(2);
	emit listEditorWidget->itemMoveUpRequested();

	auto pv = property->value().value<PropertyVector>();
	REQUIRE(pv.items.size() == 3);
	CHECK(pv.items[0]->value().toInt() == 10);
	CHECK(pv.items[1]->value().toInt() == 30);
	CHECK(pv.items[2]->value().toInt() == 20);
	CHECK(listWidget->currentRow() == 1);
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "Item moved down in PropertyVector when ListEditorWidget move down requested")
{
	addItem(10);
	addItem(20);
	addItem(30);

	listWidget->setCurrentRow(0);
	emit listEditorWidget->itemMoveDownRequested();

	auto pv = property->value().value<PropertyVector>();
	REQUIRE(pv.items.size() == 3);
	CHECK(pv.items[0]->value().toInt() == 20);
	CHECK(pv.items[1]->value().toInt() == 10);
	CHECK(pv.items[2]->value().toInt() == 30);
	CHECK(listWidget->currentRow() == 1);
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "ListEditorWidget move up does nothing for first item")
{
	addItem(10);
	addItem(20);

	listWidget->setCurrentRow(0);
	emit listEditorWidget->itemMoveUpRequested();

	auto pv = property->value().value<PropertyVector>();
	CHECK(pv.items[0]->value().toInt() == 10);
	CHECK(pv.items[1]->value().toInt() == 20);
}

TEST_CASE_METHOD(PropertyVectorEditorFixture, "ListEditorWidget move down does nothing for last item")
{
	addItem(10);
	addItem(20);

	listWidget->setCurrentRow(1);
	emit listEditorWidget->itemMoveDownRequested();

	auto pv = property->value().value<PropertyVector>();
	CHECK(pv.items[0]->value().toInt() == 10);
	CHECK(pv.items[1]->value().toInt() == 20);
}
