// Copyright (c) Prograda Pty. Ltd.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#ifdef BUILD_WITH_SKYBOLT_REFLECT

#include "QtPropertyReflection.h"
#include "PropertyEditor.h"
#include "QtMetaTypes.h"

#include <SkyboltReflect/Reflection.h>
#include <QVector3D>

namespace skybolt {

static QtPropertyUpdaterApplier createTupleValueProperty(refl::TypeRegistry& typeRegistry, const ReflInstanceGetter& instanceGetter, const refl::PropertyPtr& property, const ReflTypePropertyFactoryMap& typePropertyFactories)
{
	QString name = QString::fromStdString(property->getName());

	// Create child property for each tuple item
	PropertyTuple propertyTuple;
	std::vector<PropertiesModel::QtPropertyUpdater> updaters;
	std::vector<PropertiesModel::QtPropertyApplier> appliers;

	for (const auto& [childName, childProperty] : property->getType()->getProperties())
	{	
		auto tupleInstanceGetter = [instanceGetter, property, childProperty] () -> std::optional<refl::Instance> {
			auto parentInstance = instanceGetter();
			return parentInstance ? property->getValue(*parentInstance) : std::optional<refl::Instance>{};
		};

		if (std::optional<QtPropertyUpdaterApplier> childUpdatersApplier = reflPropertyToQt(typeRegistry, tupleInstanceGetter, childProperty, typePropertyFactories); childUpdatersApplier)
		{
			propertyTuple.items.push_back(childUpdatersApplier->property);
			updaters.push_back(std::move(childUpdatersApplier->updater));
			appliers.push_back(std::move(childUpdatersApplier->applier));
		}
		else
		{
			// No property factory for this child property, so we skip it and it won't be editable in the UI.
		}
	}

	// Create the property for the tuple itself
	QtPropertyUpdaterApplier r;
	r.property = createQtProperty(name, QVariant::fromValue(propertyTuple));
	r.property->enabled = !property->isReadOnly();

	r.updater = [updaters, children = propertyTuple.items](QtProperty& qtProperty) {
		int i = 0;
		for (const auto& updater : updaters)
		{
			updater(*children[i]);
			++i;
		}
	};
	r.updater(*r.property);

	if (!property->isReadOnly())
	{
		r.applier = [appliers, children = propertyTuple.items](const QtProperty& qtProperty) {
			int i = 0;
			for (const auto& applier : appliers)
			{
				applier(*children[i]);
				++i;
			}
		};
	}

	// Connect child property value change signals to parent property value change signal, so that when a child property value changes, the parent property valueChanged signal is emitted to notify the UI to update.
	for (const auto& itemProperty : propertyTuple.items)
	{
		QObject::connect(itemProperty.get(), &QtProperty::valueChanged, r.property.get(), &QtProperty::valueChanged);
	}

	return r;
}

std::optional<QtPropertyUpdaterApplier> reflPropertyToQt(refl::TypeRegistry& typeRegistry, const ReflInstanceGetter& instanceGetter, const refl::PropertyPtr& property, const ReflTypePropertyFactoryMap& typePropertyFactories)
{
	auto type = property->getType();
	if (auto accessor = type->getContainerValueAccessor(); accessor)
	{
		type = typeRegistry.getTypeByName(accessor->valueTypeName);
		if (!type)
		{
			return std::nullopt;
		}
	}

	// If type has a registered property factory, use it to create a QtProperty for this refl::Property
	if (const auto& i = typePropertyFactories.find(type); i != typePropertyFactories.end())
	{
		return i->second(typeRegistry, instanceGetter, property); 
	}
	
	// No property factory registered for this type, so we fallback to a nested editor if the type has properties
	if (!type->getProperties().empty())
	{
		return createTupleValueProperty(typeRegistry, instanceGetter, property, typePropertyFactories);
	}

	return std::nullopt;
}

void addReflPropertiesToModel(refl::TypeRegistry& typeRegistry, PropertiesModel& model, const std::vector<refl::PropertyPtr>& properties, const ReflInstanceGetter& instanceGetter, const ReflTypePropertyFactoryMap& typePropertyFactories)
{
	for (const refl::PropertyPtr& property : properties)
	{
		std::optional<QtPropertyUpdaterApplier> qtProperty = reflPropertyToQt(typeRegistry, instanceGetter, property, typePropertyFactories);
		if (qtProperty)
		{
			model.addProperty(qtProperty->property, qtProperty->updater, qtProperty->applier, property->getCategory());
		}
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

QWidget* createReflPropertyInstanceEditor(QtProperty* property, QWidget* parent, refl::TypeRegistry* typeRegistry, const ReflTypePropertyFactoryMap& typePropertyFactories, const PropertyEditorWidgetFactoryMapPtr& factoryMap)
{
	auto instanceGetter = [property] () -> std::optional<refl::Instance> {
		auto instanceProperty = property->value().value<ReflPropertyInstanceVariant>();
		return instanceProperty.instance;
	};

	std::optional<refl::Instance> instance = instanceGetter();
	if (!instance) { return nullptr; }

	refl::TypePtr type = instance->getType();

	auto model = std::make_shared<PropertiesModel>();
	addReflPropertiesToModel(*typeRegistry, *model, toValuesVector(type->getProperties()), instanceGetter, typePropertyFactories);

	auto editor = new PropertyEditor(factoryMap, parent);
	editor->setModel(model);
	return editor;
}

void addReflEditorsToFactoryMap(PropertyEditorWidgetFactoryMap& m, refl::TypeRegistry* typeRegistry, const ReflTypePropertyFactoryMapPtr& typePropertyFactories, const PropertyEditorWidgetFactoryMapPtr& factoryMap)
{
	assert(typeRegistry);

	PropertyEditorWidgetFactoryMap result = m;
	result[qMetaTypeId<ReflPropertyInstanceVariant>()] = [typeRegistry, typePropertyFactories, factoryMap] (QtProperty* property, QWidget* parent) {
		return createReflPropertyInstanceEditor(property, parent, typeRegistry, *typePropertyFactories, factoryMap);
	};
}

} // namespace skybolt

#endif // BUILD_WITH_SKYBOLT_REFLECT